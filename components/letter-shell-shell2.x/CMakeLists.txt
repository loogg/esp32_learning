file(GLOB_RECURSE srcs "*.c")
set(includes ".")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ${includes}
                    LDFRAGMENTS "shell.lf")
