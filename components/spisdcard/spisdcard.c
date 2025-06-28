/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG             "sdcard"
#include "esp_log.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT   "/sdcard"

typedef struct _sd_mgr {
    int           sd_mount;
    sdmmc_card_t *card;
} sd_mgr_t;

EXT_RAM_BSS_ATTR sd_mgr_t sd_mgr;

static int _mount_sd(void) {
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {.format_if_mount_failed = false, .max_files = 15, .allocation_unit_size = 16 * 1024};
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");
    sdmmc_host_t          host        = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = CONFIG_SPI_DEV_PIN_SD_CS;
    slot_config.host_id               = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &sd_mgr.card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG,
                     "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }

        sd_mgr.sd_mount = -1;
        return -1;
    }

    sd_mgr.sd_mount = 1;

    ESP_LOGI(TAG, "Filesystem mounted at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, sd_mgr.card);

    return 0;
}

static int _unmount_sd(void) {
    sd_mgr.sd_mount = 0;

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sd_mgr.card);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Failed to unmount filesystem (%s)", esp_err_to_name(ret)); }

    ESP_LOGI(TAG, "Card unmounted");

    return 0;
}

#if CONFIG_SDCARD_DETECT
static void sdcard_detect_entry(void *param) {
    while (1) {
        if (gpio_get_level(CONFIG_SDCARD_DETECT_GPIO) == 1) {
            if (sd_mgr.sd_mount == 1) {
                ESP_LOGW(TAG, "SD card removed");
                _unmount_sd();
            }
        } else {
            if (sd_mgr.sd_mount != 1) {
                ESP_LOGI(TAG, "SD card inserted");
                _mount_sd();
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
#endif

int sdcard_check_mount(void) {
    return sd_mgr.sd_mount;
}

int spisdcard_init(void) {
    memset(&sd_mgr, 0, sizeof(sd_mgr_t));

#if CONFIG_SDCARD_DETECT
    gpio_set_direction(CONFIG_SDCARD_DETECT_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_SDCARD_DETECT_GPIO, GPIO_PULLUP_ONLY);
    xTaskCreate(sdcard_detect_entry, "sdcard_detect", 4096, NULL, 5, NULL);
#else
    _mount_sd();
#endif

    return 0;
}
