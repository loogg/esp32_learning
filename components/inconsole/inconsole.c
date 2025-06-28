#include "console_simple_init.h"
#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"

int inconsole_init(void) {
    ESP_ERROR_CHECK(console_cmd_init()); // Initialize console

    /* Register commands */
    esp_console_register_help_command();
    register_system_common();
#if SOC_LIGHT_SLEEP_SUPPORTED
    register_system_light_sleep();
#endif
#if SOC_DEEP_SLEEP_SUPPORTED
    register_system_deep_sleep();
#endif
#if SOC_WIFI_SUPPORTED
    register_wifi();
#endif
    register_nvs();

    // Register any other plugin command added to your project
    ESP_ERROR_CHECK(console_cmd_all_register());

    ESP_ERROR_CHECK(console_cmd_start()); // Start console

    return 0;
}
