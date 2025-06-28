set(srcs)

file(GLOB_RECURSE insrcs "*.c")

if(CONFIG_CONSOLE_ADAPTER_ESP_CONSOLE)
    list(APPEND srcs ${insrcs})
endif()

set(includes ".")
set(public_requires "espressif__console_simple_init" "console" "cmd_nvs" "cmd_system" "cmd_wifi")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})


