#include <stdio.h>
#include "driver/uart.h"
#include <agile_modbus.h>
#include <agile_modbus_slave_util.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mb_slave.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "rtu_slave"
#include "esp_log.h"

#define RTU_UART_COM      UART_NUM_1
#define RTU_UART_BAUD     115200
#define RECV_POLL_TIMEOUT 50

enum { STEP_HEAD = 0, STEP_FUNCTION, STEP_META, STEP_META_DATA, STEP_OK };

typedef struct {
    agile_modbus_rtu_t ctx_rtu;
    uint8_t send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t data_step;
    uint16_t data_len;
    uint16_t len_to_read;
} rtu_slave_t;

static rtu_slave_t _rtu_slave = {0};

static void _recv_step_reset(void) {
    _rtu_slave.data_step = STEP_HEAD;
    _rtu_slave.data_len = 0;
    _rtu_slave.len_to_read = 1;
}

static int _protocol_preprocess(uint8_t *data, uint32_t data_len) {
    int rc = -1;
    agile_modbus_t *ctx = &_rtu_slave.ctx_rtu._ctx;

    if (data_len == 0) return 0;

    switch (_rtu_slave.data_step) {
        case STEP_HEAD: {
            if (data[0] == ctx->slave) {
                _rtu_slave.read_buf[0] = data[0];
                rc = 1;
                _rtu_slave.data_len = 1;
                _rtu_slave.len_to_read = 1;
                _rtu_slave.data_step = STEP_FUNCTION;
            } else {
                rc = -1;
            }
        } break;

        case STEP_FUNCTION: {
            _rtu_slave.read_buf[_rtu_slave.data_len] = data[0];
            rc = _rtu_slave.len_to_read;
            _rtu_slave.data_len += _rtu_slave.len_to_read;
            int after_len = agile_modbus_compute_meta_length_after_function(ctx, _rtu_slave.read_buf[AGILE_MODBUS_RTU_HEADER_LENGTH],
                                                                            AGILE_MODBUS_MSG_INDICATION);
            if (after_len < 0) {
                rc = -1;
                break;
            }
            if ((after_len + _rtu_slave.data_len) > sizeof(_rtu_slave.read_buf)) {
                rc = -1;
                break;
            }
            _rtu_slave.len_to_read = after_len;
            _rtu_slave.data_step = STEP_META;
        } break;

        case STEP_META: {
            if (data_len < _rtu_slave.len_to_read) {
                memcpy(_rtu_slave.read_buf + _rtu_slave.data_len, data, data_len);
                rc = data_len;
                _rtu_slave.data_len += data_len;
                _rtu_slave.len_to_read -= data_len;
            } else {
                memcpy(_rtu_slave.read_buf + _rtu_slave.data_len, data, _rtu_slave.len_to_read);
                rc = _rtu_slave.len_to_read;
                _rtu_slave.data_len += _rtu_slave.len_to_read;
                int after_len =
                    agile_modbus_compute_data_length_after_meta(ctx, _rtu_slave.read_buf, _rtu_slave.data_len, AGILE_MODBUS_MSG_INDICATION);
                if (after_len < 0) {
                    rc = -1;
                    break;
                }
                if ((after_len + _rtu_slave.data_len) > sizeof(_rtu_slave.read_buf)) {
                    rc = -1;
                    break;
                }
                _rtu_slave.len_to_read = after_len;
                _rtu_slave.data_step = STEP_META_DATA;
            }
        } break;

        case STEP_META_DATA: {
            if (data_len < _rtu_slave.len_to_read) {
                memcpy(_rtu_slave.read_buf + _rtu_slave.data_len, data, data_len);
                rc = data_len;
                _rtu_slave.data_len += data_len;
                _rtu_slave.len_to_read -= data_len;
            } else {
                memcpy(_rtu_slave.read_buf + _rtu_slave.data_len, data, _rtu_slave.len_to_read);
                rc = _rtu_slave.len_to_read;
                _rtu_slave.data_len += _rtu_slave.len_to_read;
                _rtu_slave.len_to_read = 0;
                _rtu_slave.data_step = STEP_OK;
            }
        } break;

        case STEP_OK:
            break;

        default:
            break;
    }

    return rc;
}

static int _protocol_process(uint8_t *data, int sz) {
    uint8_t *ptr = data;
    agile_modbus_t *ctx = &_rtu_slave.ctx_rtu._ctx;

    while (sz > 0) {
        int rc = _protocol_preprocess(ptr, sz);
        if (rc >= 0) {
            ptr += rc;
            sz -= rc;
        } else {
            _recv_step_reset();
            ptr++;
            sz--;
        }

        if (_rtu_slave.data_step == STEP_OK) {
            rc = agile_modbus_slave_handle(ctx, _rtu_slave.data_len, 1, agile_modbus_slave_util_callback, &mb_slave_util, NULL);
            if (rc > 0) {
                uart_write_bytes(RTU_UART_COM, (const char *)ctx->send_buf, rc);
                uart_wait_tx_done(RTU_UART_COM, pdMS_TO_TICKS(1000));
            }
            _recv_step_reset();
        }
    }

    return 0;
}

static void rtu_slave_task(void *param) {
    uint8_t tmp_buf[256];

    agile_modbus_rtu_init(&_rtu_slave.ctx_rtu, _rtu_slave.send_buf, AGILE_MODBUS_MAX_ADU_LENGTH, _rtu_slave.read_buf, sizeof(_rtu_slave.read_buf));
    agile_modbus_set_slave(&_rtu_slave.ctx_rtu._ctx, 1);

    _recv_step_reset();

    while (1) {
        int ret = uart_read_bytes(RTU_UART_COM, tmp_buf, sizeof(tmp_buf), pdMS_TO_TICKS(RECV_POLL_TIMEOUT));
        if (ret <= 0) {
            _recv_step_reset();
            continue;
        }

        _protocol_process(tmp_buf, ret);
    }
}

static int uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = RTU_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(RTU_UART_COM, &uart_config);
    uart_set_pin(RTU_UART_COM, 9, 10, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(RTU_UART_COM, 1024, 0, 0, NULL, 0);

    return 0;
}

int rtu_slave_init(void) {
    uart_init();

    xTaskCreate(rtu_slave_task, "rtu_slave", 2048, NULL, 6, NULL);

    ESP_LOGI(LOG_LOCAL_TAG, "init success.");

    return 0;
}
