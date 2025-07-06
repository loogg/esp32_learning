#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "iic_dev.h"
#include "esp_io_expander_tca95xx_16bit.h"
#include "esp_log.h"
#include "gui.h"

#define TAG             "io_ext"

static esp_io_expander_handle_t io_expander = NULL;

esp_err_t io_ext_set_dir_output(uint32_t pin_mask) {
    return esp_io_expander_set_dir(io_expander, pin_mask, IO_EXPANDER_OUTPUT);
}

esp_err_t io_ext_set_dir_input(uint32_t pin_mask) {
    return esp_io_expander_set_dir(io_expander, pin_mask, IO_EXPANDER_INPUT);
}

esp_err_t io_ext_set_level(uint32_t pin_mask, uint8_t level) {
    return esp_io_expander_set_level(io_expander, pin_mask, level);
}

esp_err_t io_ext_get_level(uint32_t pin_mask, uint32_t *level) {
    return esp_io_expander_get_level(io_expander, pin_mask, level);
}

int io_ext_init(void) {
    esp_err_t ret;

    ret = esp_io_expander_new_i2c_tca95xx_16bit(iic_dev_bus_handle, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, &io_expander);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create IO expander: %s", esp_err_to_name(ret));
        return -1;
    }

    gui_close_lcd_blk();
    return 0;
}
