#ifndef __SOUND_PLAYER_H
#define __SOUND_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

int sound_player_init(void);
int sound_player_music(const char *file_path);
int sound_player_stop(void);

#ifdef __cplusplus
}
#endif

#endif
