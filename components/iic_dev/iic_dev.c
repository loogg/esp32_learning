#include "iic_dev.h"

#define IIC_DEV_PORT_NUMBER I2C_NUM_0

i2c_master_bus_handle_t iic_dev_bus_handle = NULL;

int iic_dev_init(void) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = IIC_DEV_PORT_NUMBER,
        .scl_io_num = CONFIG_IIC_DEV_PIN_SCL,
        .sda_io_num = CONFIG_IIC_DEV_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &iic_dev_bus_handle));

    return 0;
}
