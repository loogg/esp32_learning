#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "camera_player.h"
#include "esp_camera.h"
#include "gui.h"
#include "io_ext.h"

static const char *TAG = "CAM_PLAYER";

#define CAM_PIN_PWDN                GPIO_NUM_NC
#define CAM_PIN_RESET               GPIO_NUM_NC
#define CAM_PIN_XCLK                GPIO_NUM_NC
#define CAM_PIN_SIOD                GPIO_NUM_39
#define CAM_PIN_SIOC                GPIO_NUM_38

#define CAM_PIN_D7                  GPIO_NUM_18
#define CAM_PIN_D6                  GPIO_NUM_17
#define CAM_PIN_D5                  GPIO_NUM_16
#define CAM_PIN_D4                  GPIO_NUM_15
#define CAM_PIN_D3                  GPIO_NUM_7
#define CAM_PIN_D2                  GPIO_NUM_6
#define CAM_PIN_D1                  GPIO_NUM_5
#define CAM_PIN_D0                  GPIO_NUM_4
#define CAM_PIN_VSYNC               GPIO_NUM_47
#define CAM_PIN_HREF                GPIO_NUM_48
#define CAM_PIN_PCLK                GPIO_NUM_45

#define CAM_PWDN_IO_EXT_PIN 4
#define CAM_PWDN(x) \
    do { io_ext_set_level(IO_EXT_PIN(CAM_PWDN_IO_EXT_PIN), x); } while (0)

#define CAM_RESET_IO_EXT_PIN 5
#define CAM_RESET(x) \
    do { io_ext_set_level(IO_EXT_PIN(CAM_RESET_IO_EXT_PIN), x); } while (0)

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    /* pin_xclk 有效时 */
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_QVGA,

    .jpeg_quality = 12,     //0-63 lower number means higher quality
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

enum {
    CAMERA_PLAYER_MSG_NONE = 0,
    CAMERA_PLAYER_MSG_START,
    CAMERA_PLAYER_MSG_STOP,
};

enum {
    CAMERA_PLAYER_STATE_STOPPED = 0,
    CAMERA_PLAYER_STATE_PLAYING = 1,
};

typedef struct _camera_player {
    QueueHandle_t play_queue;
    uint32_t key_data;
    uint8_t state;
} camera_player_t;

static camera_player_t _player = {0};

int camera_player_play(void) {
    if (_player.state != CAMERA_PLAYER_STATE_STOPPED) { camera_player_stop(); }

    uint32_t play_data = CAMERA_PLAYER_MSG_START;
    xQueueSend(_player.play_queue, &play_data, 0);

    return 0;
}

int camera_player_stop(void) {
    if (_player.state != CAMERA_PLAYER_STATE_STOPPED) {
        uint32_t play_data = CAMERA_PLAYER_MSG_STOP;
        xQueueSend(_player.play_queue, &play_data, 0);
    }

    return 0;
}

int camera_player_send_key_event(uint8_t *key, int len) {
    for (int i = 0; i < len; i++) {
        // ESC
        if (key[i] == 41) {
            camera_player_stop();
            break;
        }
    }

    return 0;
}

static void camera_player_run(void) {
    uint32_t mb_data = 0;
    void *draw_ptr = gui_disp->driver->draw_buf->buf1;

    while (1) {
        if (xQueueReceive(_player.play_queue, &mb_data, 0) == pdTRUE) {
            if (mb_data == CAMERA_PLAYER_MSG_STOP) {
                ESP_LOGI(TAG, "Camera player stopped");
                return;
            }
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == NULL) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        memcpy(draw_ptr, fb->buf, fb->width * fb->height);
        esp_lcd_panel_draw_bitmap(lcd_panel_handle, 0, 0, fb->width, fb->height / 2, draw_ptr);

        memcpy(draw_ptr, fb->buf + fb->width * fb->height, fb->width * fb->height);
        esp_lcd_panel_draw_bitmap(lcd_panel_handle, 0, fb->height / 2, fb->width, fb->height, draw_ptr);

        esp_camera_fb_return(fb);
    }
}

static void camera_player_init_ex(void) {
    gui_disp_set_sync_trans(true);

    CAM_PWDN(1);
    vTaskDelay(pdMS_TO_TICKS(10));
    CAM_PWDN(0);

    CAM_RESET(0);
    vTaskDelay(pdMS_TO_TICKS(20));
    CAM_RESET(1);
    vTaskDelay(pdMS_TO_TICKS(20));

    esp_camera_init(&camera_config);
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
}

static void camera_player_deinit_ex(void) {
    gui_disp_set_sync_trans(false);

    esp_camera_deinit();
    CAM_PWDN(1);
}

static void camera_player_entry(void *param) {
    uint32_t play_data = 0;

    while (1) {
        if (xQueueReceive(_player.play_queue, &play_data, portMAX_DELAY) != pdTRUE) { continue; }

        if (play_data != CAMERA_PLAYER_MSG_START) continue;

        ESP_LOGI(TAG, "Camera player started");
        _player.state = CAMERA_PLAYER_STATE_PLAYING;
        _player.key_data = 0;

        vTaskDelay(pdMS_TO_TICKS(200));

        lvgl_port_lock(0);

        camera_player_init_ex();
        camera_player_run();
        camera_player_deinit_ex();

        lv_obj_invalidate(lv_scr_act());
        lvgl_port_unlock();

        _player.state = CAMERA_PLAYER_STATE_STOPPED;
    }
}

int camera_player_init(void) {
    _player.play_queue = xQueueCreate(32, sizeof(uint32_t));
    if (_player.play_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }

    io_ext_set_dir_output(IO_EXT_PIN(CAM_PWDN_IO_EXT_PIN) | IO_EXT_PIN(CAM_RESET_IO_EXT_PIN));

    CAM_PWDN(1);
    CAM_RESET(1);

    xTaskCreateWithCaps(camera_player_entry, "camera_player", 4096, NULL, 15, NULL, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    return 0;
}

