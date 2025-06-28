set(srcs)

file(GLOB_RECURSE insrcs "*.c")

if(CONFIG_CONSOLE_ADAPTER_LETTER_SHELL)
    list(APPEND srcs ${insrcs})
endif()

set(includes ".")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ${includes}
                    LDFRAGMENTS "shell.lf")
