file(GLOB_RECURSE SOURCES "*.c")

idf_component_register(SRCS ${SOURCES}
                    REQUIRES driver
                    INCLUDE_DIRS ".")