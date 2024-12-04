#include "esp_system.h"

#if CONFIG_BSP_SD_USING_SPI
#include "spi_sdcard.h"
#elif CONFIG_BSP_SD_USING_SDMMC
#include "sdmmc_sdcard.h"
#endif

int sdcard_init(void) {
#if CONFIG_BSP_SD_USING_SPI
    spi_sdcard_init();
#elif CONFIG_BSP_SD_USING_SDMMC
    sdmmc_sdcard_init();
#endif

return 0;
}
