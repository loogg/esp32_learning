#include <stdio.h>
#include "key.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_rom_sys.h"

#define KEY_PIN 0

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    esp_rom_printf("key pressed\n");
}

int key_init(void) {
    gpio_config_t key_config = {
        .pin_bit_mask = BIT64(KEY_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE};
    gpio_config(&key_config);

    //注册中断服务
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    //设置GPIO的中断服务函数
    gpio_isr_handler_add(KEY_PIN, gpio_isr_handler, (void*)NULL);

    //使能GPIO模块中断信号
    gpio_intr_enable(KEY_PIN);

    return 0;
}
