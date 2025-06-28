#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG             "rndis_net"
#include "esp_log.h"

#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "dhcpserver/dhcpserver_options.h"
#include "ping/ping_sock.h"
#include "iot_usbh_rndis.h"
#include "iot_eth.h"
#include "iot_eth_netif_glue.h"
#include "iot_usbh_cdc.h"

static void rndis_net_entry(void *param) {
    esp_err_t        ret        = ESP_FAIL;
    iot_eth_handle_t eth_handle = (iot_eth_handle_t)param;

    while (1) {
        if (ret != ESP_OK) {
            ret = iot_eth_start(eth_handle);
            if (ret != ESP_OK) { ESP_LOGW(TAG, "Failed to start USB RNDIS driver, try again..."); }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

int rndis_net_init(void) {
    // install usbh cdc driver
    usbh_cdc_driver_config_t config = {
        .task_stack_size           = 1024 * 4,
        .task_priority             = 5,
        .task_coreid               = 0,
        .skip_init_usb_host_driver = false,
    };
    ESP_ERROR_CHECK(usbh_cdc_driver_install(&config));

    iot_usbh_rndis_config_t rndis_cfg = {
        .auto_detect         = true,
        .auto_detect_timeout = pdMS_TO_TICKS(1000),
    };

    iot_eth_driver_t *rndis_handle = NULL;
    esp_err_t         ret          = iot_eth_new_usb_rndis(&rndis_cfg, &rndis_handle);
    if (ret != ESP_OK || rndis_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create USB RNDIS driver");
        return -1;
    }

    iot_eth_config_t eth_cfg = {
        .driver      = rndis_handle,
        .stack_input = NULL,
        .user_data   = NULL,
    };

    iot_eth_handle_t eth_handle = NULL;
    ret                         = iot_eth_install(&eth_cfg, &eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB RNDIS driver");
        return -1;
    }

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t       *eth_netif = esp_netif_new(&netif_cfg);

    iot_eth_netif_glue_handle_t glue = iot_eth_new_netif_glue(eth_handle);
    if (glue == NULL) {
        ESP_LOGE(TAG, "Failed to create netif glue");
        return -1;
    }
    esp_netif_attach(eth_netif, glue);

    xTaskCreate(rndis_net_entry, "rndis_net", 4096, eth_handle, 5, NULL);

    return 0;
}
