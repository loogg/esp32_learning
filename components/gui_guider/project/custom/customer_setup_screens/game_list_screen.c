#include "game_list_screen.h"
#include <dirent.h>
#include <unistd.h>
#include "nesplayer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nes.h"

#define PREFIX_PATH "/sdcard/Game"

static lv_obj_t *_game_back = NULL;

static void _screen_btn_event_cb(lv_event_t *e) {
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (btn == _game_back) {
        nesplayer_stop();
        ui_load_scr_animation(ui, &ui->Home, ui->Home_del, &ui->GameList_del, setup_scr_Home, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    } else {
        nesplayer_stop();
        char game_path[256] = {0};
        lv_snprintf(game_path, sizeof(game_path), "%s/%s", PREFIX_PATH, lv_label_get_text(lv_obj_get_child(btn, 0)));
        nesplayer_play(game_path);
    }
}

void game_list_screen_custom_setup(lv_ui *ui) {
    lv_obj_set_style_pad_top(ui->GameList, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(ui->GameList, LV_FLEX_FLOW_COLUMN);

    do {
        DIR *dir = opendir(PREFIX_PATH);
        if (dir == NULL) break;

        struct dirent *dirent = NULL;
        do {
            dirent = readdir(dir);
            if (dirent == NULL) break;

            size_t name_len = strlen(dirent->d_name);

            if (memcmp(dirent->d_name + name_len - 4, ".nes", 4) == 0 || memcmp(dirent->d_name + name_len - 4, ".NES", 4) == 0) {
                lv_obj_t *game_btn = lv_btn_create(ui->GameList);
                lv_obj_set_width(game_btn, LV_PCT(100));
                lv_label_set_text(lv_label_create(game_btn), dirent->d_name);
                lv_obj_set_width(lv_obj_get_child(game_btn, 0), LV_PCT(100));
                lv_obj_add_event_cb(game_btn, _screen_btn_event_cb, LV_EVENT_ALL, ui);
            }
        } while (dirent != NULL);
        closedir(dir);
    } while (0);

    _game_back = lv_btn_create(ui->GameList);
    lv_obj_set_width(_game_back, LV_PCT(100));
    lv_label_set_text(lv_label_create(_game_back), "返回");
    lv_obj_add_event_cb(_game_back, _screen_btn_event_cb, LV_EVENT_ALL, ui);
}
