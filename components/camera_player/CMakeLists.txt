file(GLOB srcs "*.c")
set(includes ".")
set(public_requires "espressif__esp32-camera" "gui" "io_ext")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
