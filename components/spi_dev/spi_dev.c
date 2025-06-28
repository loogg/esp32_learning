#include "driver/gpio.h"
#include "driver/spi_master.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG             "spi_dev"
#include "esp_log.h"

#define SPI_DEV_HOST     SPI2_HOST

// 由于复用，所以将两个 CS 都先拉高
static void spi_dev_cs_init(void) {

    gpio_config_t spi_dev_cs_config = {
        .pin_bit_mask = BIT64(CONFIG_SPI_DEV_PIN_SD_CS) | BIT64(CONFIG_SPI_DEV_PIN_LCD_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&spi_dev_cs_config);
    gpio_set_level(CONFIG_SPI_DEV_PIN_SD_CS, 1);
    gpio_set_level(CONFIG_SPI_DEV_PIN_LCD_CS, 1);
}

int spi_dev_init(void) {
    spi_dev_cs_init();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = CONFIG_SPI_DEV_PIN_MOSI,
        .miso_io_num     = CONFIG_SPI_DEV_PIN_MISO,
        .sclk_io_num     = CONFIG_SPI_DEV_PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 320 * 240 * sizeof(uint16_t), // 320x240 RGB565
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_DEV_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    return 0;
}
