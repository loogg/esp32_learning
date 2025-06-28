file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "audio_pipeline" "audio_stream" "esp-adf-libs" "esp_peripherals" "io_ext")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
