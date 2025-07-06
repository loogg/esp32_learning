#ifndef CAMERA_PLAYER_H
#define CAMERA_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int camera_player_init(void);
int camera_player_play(void);
int camera_player_stop(void);
int camera_player_send_key_event(uint8_t *key, int len);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_PLAYER_H