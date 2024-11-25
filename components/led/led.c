#include "led.h"
#include "driver/gpio.h"
#include <esp_task.h>

#define LED_PIN 48

static void led_task(void *param)
{
    while (1) {
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

int led_init(void)
{
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_config);

    xTaskCreate(led_task, "led_task", 1024, NULL, 5, NULL);

    return 0;
}
