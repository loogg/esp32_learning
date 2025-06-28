file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "espressif__iot_eth" "espressif__iot_usbh_cdc" "espressif__iot_usbh_rndis")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
