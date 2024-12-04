file(GLOB srcs "*.c")
set(includes ".")
set(public_requires "fatfs")

if(CONFIG_BSP_SD_USING_SPI)
    file(GLOB sdspi_srcs "sdspi/*.c")
    list(APPEND srcs ${sdspi_srcs})
    list(APPEND includes sdspi)
endif()

if(CONFIG_BSP_SD_USING_SDMMC)
    file(GLOB sdmmc_srcs "sdmmc/*.c")
    list(APPEND srcs ${sdmmc_srcs})
    list(APPEND includes sdmmc)
endif()

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})
