#ifndef __SOUND_PLAYER_H
#define __SOUND_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

int sound_player_init(void);
int sound_player_music(const char *file_path);
int sound_player_stop(void);
int sound_player_raw_load(void);
int sound_player_raw_write(char *buffer, int len);
int sound_player_set_i2s_clk(int rate, int bits, int ch);

#ifdef __cplusplus
}
#endif

#endif
