#ifndef __SPISDCARD_H
#define __SPISDCARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int spisdcard_init(void);
int sdcard_check_mount(void);

#ifdef __cplusplus
}
#endif

#endif
