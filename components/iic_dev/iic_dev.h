#ifndef __IIC_DEV_H
#define __IIC_DEV_H

#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

extern i2c_master_bus_handle_t iic_dev_bus_handle;

int iic_dev_init(void);

#ifdef __cplusplus
}
#endif

#endif
