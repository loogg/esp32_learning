file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires espressif__esp_io_expander_tca95xx_16bit iic_dev)

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
