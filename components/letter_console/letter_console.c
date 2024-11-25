#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "shell.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "ltconsole"
#include "esp_log.h"

#define SHELL_UART UART_NUM_0

static SHELL_TypeDef _shell = {0};

static int cmd_free(int argc, char *agrv[]) {
    size_t total = 0, used = 0, max_used = 0;


    total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    used = total - esp_get_free_heap_size();
    max_used = total - esp_get_minimum_free_heap_size();

    printf("total   : %d\r\n", total);
    printf("used    : %d\r\n", used);
    printf("maximum : %d\r\n", max_used);

    return 0;
}
SHELL_EXPORT_CMD(free, cmd_free, Show the memory usage in the system.);

static int cmd_reboot(void) {
    esp_restart();

    return 0;
}
SHELL_EXPORT_CMD(reboot, cmd_reboot, Reboot the system.);


static int cmd_ps(int argc, char *agrv[]) {
    char * task_list_buffer = malloc(uxTaskGetNumberOfTasks() * 40);
    if (task_list_buffer == NULL) {
        ESP_LOGE(LOG_LOCAL_TAG, "malloc failed");
        return -1;
    }

    vTaskList(task_list_buffer);

    printf("---------------------------------------------\r\n");
    printf("thread       status     pri   lfstack index\r\n");
    printf("%s", task_list_buffer);
    printf("---------------------------------------------\r\n");

    free(task_list_buffer);

    return 0;
}
SHELL_EXPORT_CMD(ps, cmd_ps, List threads in the system.);

static void _shell_write(const char ch) { uart_write_bytes(SHELL_UART, &ch, 1); }

static signed char _shell_read(char *ch) {
    uart_read_bytes(SHELL_UART, (uint8_t *)ch, 1, portMAX_DELAY);

    return 0;
}

int letter_console_init(void) {
    uart_config_t uartConfig = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(SHELL_UART, &uartConfig);
    uart_driver_install(SHELL_UART, 256 * 2, 0, 0, NULL, 0);

    _shell.read = _shell_read;
    _shell.write = _shell_write;
    shellInit(&_shell);

    xTaskCreate(shellTask, "shell", 2048, &_shell, 10, NULL);

    return 0;
}
