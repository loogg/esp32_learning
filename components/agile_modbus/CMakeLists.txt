file(GLOB_RECURSE SOURCES "src/*.c" "util/*.c")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "inc" "util")