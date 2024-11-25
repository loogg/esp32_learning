file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "letter-shell-shell2.x" "esp_event" "esp_wifi" "nvs_flash")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
