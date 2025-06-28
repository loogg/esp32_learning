file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "lvgl__lvgl" "espressif__esp_lvgl_port" "io_ext" "cherry-embedded__cherryusb" "espressif__usb_host_hid")

idf_component_get_property(cherryusb_dir cherry-embedded__cherryusb COMPONENT_DIR)

file(GLOB_RECURSE cherryusb_lvgl_srcs "${cherryusb_dir}/platform/lvgl/*.c")

list(APPEND srcs ${cherryusb_lvgl_srcs})
list(APPEND includes ${cherryusb_dir}/platform/lvgl)

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
