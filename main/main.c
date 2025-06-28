#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "led.h"
#include "key.h"
#include "rtu_slave.h"
#include "timer.h"
#if CONFIG_CONSOLE_ADAPTER_LETTER_SHELL
#include "letter_console.h"
#elif CONFIG_CONSOLE_ADAPTER_ESP_CONSOLE
#include "inconsole.h"
#endif
#include "pwm.h"
#include "wifi_sta.h"
#include "nvs_flash.h"
#include "mbtcp_slave.h"
#include "ftp.h"
#include "spisdcard.h"
#if CONFIG_USB_ADAPTER_CHERRYUSB
#include "usbh_core.h"
#elif CONFIG_USB_ADAPTER_IDF_USB
#include "rndis_net.h"
#endif
#include "spi_dev.h"
#include "iic_dev.h"
#include "io_ext.h"
#include "gui.h"
#include "sound_player.h"

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

#if CONFIG_CONSOLE_ADAPTER_LETTER_SHELL
    letter_console_init();
#elif CONFIG_CONSOLE_ADAPTER_ESP_CONSOLE
    inconsole_init();
#endif

    led_init();
    // key_init();
    // rtu_slave_init();
    // timer_init();
    // pwm_init();
    wifi_init_sta();
    spi_dev_init();
    spisdcard_init();
    iic_dev_init();
    io_ext_init();
    mbtcp_slave_start();
    ftp_init(4096, 5);

#if CONFIG_USB_ADAPTER_CHERRYUSB
    usbh_initialize(0, ESP_USBH_BASE);
#elif CONFIG_USB_ADAPTER_IDF_USB
    rndis_net_init();
#endif

    sound_player_init();

    gui_init();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
