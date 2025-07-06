file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "espressif__iot_eth" "espressif__iot_usbh_cdc" "espressif__iot_usbh_rndis")

idf_build_get_property(sdkconfig_dir SDKCONFIG)
file(STRINGS "${sdkconfig_dir}" idfusbconfig REGEX "^CONFIG_USB_ADAPTER_IDF_USB=y$")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
