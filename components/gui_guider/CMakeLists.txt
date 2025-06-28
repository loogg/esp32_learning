file(GLOB srcs
    "*.c"
    "project/generated/*.c"
    "project/generated/guider_fonts/*.c"
    "project/generated/guider_customer_fonts/*.c"
    "project/custom/*.c"
    "project/custom/customer_setup_screens/*.c")
set(includes
    "."
    "project/generated"
    "project/generated/guider_fonts"
    "project/generated/guider_customer_fonts"
    "project/custom"
    "project/custom/customer_setup_screens")
set(public_requires "lvgl__lvgl" "sound_player")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
