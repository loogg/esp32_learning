# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# 添加编译选项来禁用特定的格式警告
add_compile_options(-Wno-format -Wno-error=format)

add_compile_definitions(
    LV_LVGL_H_INCLUDE_SIMPLE
    LV_ATTRIBUTE_LARGE_RAM_ARRAY=EXT_RAM_BSS_ATTR
    CONFIG_USBHOST_PSC_PRIO=15
    CONFIG_USBHOST_PSC_STACKSIZE=4096
)

add_compile_definitions(
    "LV_FONT_CUSTOM_DECLARE=extern lv_font_t *lv_ttf_font_32\;extern lv_font_t *lv_ttf_font_30\;extern lv_font_t *lv_ttf_font_28\;extern lv_font_t *lv_ttf_font_26\;extern lv_font_t *lv_ttf_font_24\;extern lv_font_t *lv_ttf_font_22\;extern lv_font_t *lv_ttf_font_20\;extern lv_font_t *lv_ttf_font_18\;extern lv_font_t *lv_ttf_font_16\;extern lv_font_t *lv_ttf_font_14\;extern lv_font_t *lv_ttf_font_12\;extern lv_font_t *lv_ttf_font_10\;extern lv_font_t *lv_ttf_font_8\;"
    LV_FONT_DEFAULT=lv_ttf_font_24
)

include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(01_led)