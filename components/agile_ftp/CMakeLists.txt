file(GLOB_RECURSE srcs "src/*.c")
set(includes "inc")
set(public_requires "fatfs")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
