#ifndef __IO_EXT_H
#define __IO_EXT_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IO_EXT_PIN(pin) (1U << (pin))

int io_ext_init(void);
esp_err_t io_ext_set_dir_output(uint32_t pin_mask);
esp_err_t io_ext_set_dir_input(uint32_t pin_mask);
esp_err_t io_ext_set_level(uint32_t pin_mask, uint8_t level);
esp_err_t io_ext_get_level(uint32_t pin_mask, uint32_t *level);

#ifdef __cplusplus
}
#endif

#endif
