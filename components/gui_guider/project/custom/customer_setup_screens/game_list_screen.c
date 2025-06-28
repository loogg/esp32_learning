#include "game_list_screen.h"
#include <dirent.h>
#include <unistd.h>
// #include "nesplayer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PREFIX_PATH "/sdcard/Game"

static lv_obj_t *_game_back = NULL;

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
static lv_obj_t *_game_img = NULL;
static lv_img_dsc_t *_game_img_dsc = NULL;
static lv_timer_t *_game_img_timer = NULL;
static uint8_t _game_fresh_flag = 0;
static uint8_t _game_exit_flag = 0;
#endif

static void _screen_btn_event_cb(lv_event_t *e) {
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (btn == _game_back) {
        // nesplayer_stop();
        vTaskDelay(pdMS_TO_TICKS(300));
#ifdef BSP_NES_FRESH_USING_LVGL_IMG
        lv_timer_del(_game_img_timer);
        lv_obj_del(_game_img);
        lv_img_buf_free(_game_img_dsc);
#endif
        ui_load_scr_animation(ui, &ui->Home, ui->Home_del, &ui->GameList_del, setup_scr_Home, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    } else {
        // nesplayer_stop();
        vTaskDelay(pdMS_TO_TICKS(300));
        char game_path[256] = {0};
        lv_snprintf(game_path, sizeof(game_path), "%s/%s", PREFIX_PATH, lv_label_get_text(lv_obj_get_child(btn, 0)));
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
        disp_disable_update();
#endif
#ifdef BSP_NES_FRESH_USING_LVGL_IMG
        _game_fresh_flag = 0;
        _game_exit_flag = 0;
        lv_memset((uint8_t *)_game_img_dsc->data, 0, 240 * 240 * sizeof(lv_color_t));
        lv_img_set_src(_game_img, _game_img_dsc);
        lv_obj_clear_flag(_game_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
#endif
        // nesplayer_play(game_path);
    }
}

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
void lv_game_img_draw(void *pcolor) {
    lv_memcpy((uint8_t *)_game_img_dsc->data, pcolor, 240 * 240 * sizeof(lv_color_t));
    _game_fresh_flag = 1;
}

void lv_game_img_exit(void) {
    _game_exit_flag = 1;
}

static void _game_img_timer_cb(struct _lv_timer_t *t) {
    if (_game_fresh_flag) {
        _game_fresh_flag = 0;
        lv_img_set_src(_game_img, _game_img_dsc);
    }

    if (_game_exit_flag) {
        _game_exit_flag = 0;
        lv_obj_add_flag(_game_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    }
}
#endif

void game_list_screen_custom_setup(lv_ui *ui) {
    lv_obj_set_style_pad_top(ui->GameList, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(ui->GameList, LV_FLEX_FLOW_COLUMN);

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
    _game_img_dsc = lv_img_buf_alloc(240, 240, LV_IMG_CF_TRUE_COLOR);
    _game_img = lv_img_create(lv_layer_top());
    lv_obj_set_size(_game_img, 240, 240);
    lv_obj_center(_game_img);
    lv_img_set_src(_game_img, _game_img_dsc);
    lv_obj_add_flag(_game_img, LV_OBJ_FLAG_HIDDEN);
    _game_img_timer = lv_timer_create(_game_img_timer_cb, 1, NULL);
#endif

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
