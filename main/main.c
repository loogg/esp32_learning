#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "led.h"
#include "key.h"
#include "rtu_slave.h"
#include "timer.h"
#include "letter_console.h"
#include "pwm.h"
#include "wifi_sta.h"
#include "nvs_flash.h"
#include "mbtcp_slave.h"
#include "ftp.h"
#include "sdcard.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    letter_console_init();
    // led_init();
    // key_init();
    // rtu_slave_init();
    // timer_init();
    // pwm_init();
    wifi_init_sta();
    sdcard_init();
    mbtcp_slave_start();
    ftp_init(4096, 5);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
