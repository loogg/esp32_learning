#ifndef __NESPLAYER_H
#define __NESPLAYER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int nesplayer_play(char *uri);
int nesplayer_stop(void);
int nesplayer_send_key_event(uint8_t *key, int len);
int nesplayer_init(void);

#ifdef __cplusplus
}
#endif
#endif

