file(GLOB_RECURSE SOURCES "*.c")

idf_component_register(SRCS ${SOURCES}
                    REQUIRES driver agile_modbus
                    INCLUDE_DIRS ".")