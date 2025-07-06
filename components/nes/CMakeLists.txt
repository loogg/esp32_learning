file(GLOB srcs "src/*.c" "src/nes_mapper/*.c")
set(includes "inc")
set(public_requires "nesplayer")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
