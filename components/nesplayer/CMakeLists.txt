file(GLOB srcs "*.c")
set(includes ".")
set(public_requires "nes" "sound_player" "gui")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
