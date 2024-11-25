file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "driver" "letter-shell-shell2.x")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
