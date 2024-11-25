file(GLOB_RECURSE srcs "*.c")
set(includes ".")
set(public_requires "agile_modbus" "mb_slave")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
