set(srcs)

file(GLOB_RECURSE insrcs "*.c")

if(CONFIG_CONSOLE_ADAPTER_LETTER_SHELL)
    list(APPEND srcs ${insrcs})
endif()

set(includes ".")
set(public_requires "driver" "letter-shell-shell2.x" "esp_netif")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes}
                    WHOLE_ARCHIVE)
