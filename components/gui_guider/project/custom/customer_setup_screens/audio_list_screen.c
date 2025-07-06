#include "audio_list_screen.h"
#include <dirent.h>
#include <unistd.h>
#include "sound_player.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gui.h"

#define PREFIX_PATH "/sdcard/Music"

static lv_obj_t *_audio_back = NULL;

static void _screen_btn_event_cb(lv_event_t *e) {
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (btn == _audio_back) {
        sound_player_stop();
        ui_load_scr_animation(ui, &ui->Home, ui->Home_del, &ui->AudioList_del, setup_scr_Home, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        gui_disp_set_sync_trans(false);
    } else {
        gui_disp_set_sync_trans(true);
        sound_player_stop();
        char audio_path[256] = {0};
        lv_snprintf(audio_path, sizeof(audio_path), "%s/%s", PREFIX_PATH, lv_label_get_text(lv_obj_get_child(btn, 0)));
        sound_player_music(audio_path);
    }
}

void audio_list_screen_custom_setup(lv_ui *ui) {
    lv_obj_set_style_pad_top(ui->AudioList, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(ui->AudioList, LV_FLEX_FLOW_COLUMN);

    do {
        DIR *dir = opendir(PREFIX_PATH);
        if (dir == NULL) break;

        struct dirent *dirent = NULL;
        do {
            dirent = readdir(dir);
            if (dirent == NULL) break;

            size_t name_len = strlen(dirent->d_name);

            if (memcmp(dirent->d_name + name_len - 4, ".wav", 4) == 0 || memcmp(dirent->d_name + name_len - 4, ".WAV", 4) == 0
                || memcmp(dirent->d_name + name_len - 4, ".mp3", 4) == 0 || memcmp(dirent->d_name + name_len - 4, ".MP3", 4) == 0) {
                lv_obj_t *wav_btn = lv_btn_create(ui->AudioList);
                lv_obj_set_width(wav_btn, LV_PCT(100));
                lv_label_set_text(lv_label_create(wav_btn), dirent->d_name);
                lv_obj_set_width(lv_obj_get_child(wav_btn, 0), LV_PCT(100));
                lv_obj_add_event_cb(wav_btn, _screen_btn_event_cb, LV_EVENT_ALL, ui);
            }
        } while (dirent != NULL);
        closedir(dir);
    } while (0);

    _audio_back = lv_btn_create(ui->AudioList);
    lv_obj_set_width(_audio_back, LV_PCT(100));
    lv_label_set_text(lv_label_create(_audio_back), "返回");
    lv_obj_add_event_cb(_audio_back, _screen_btn_event_cb, LV_EVENT_ALL, ui);
}
