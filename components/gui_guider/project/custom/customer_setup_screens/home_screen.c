#include "home_screen.h"
#include <math.h>


#define LV_CUSTOM_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

enum {
    LV_OBJ_TYPE_LABEL = 0,
    LV_OBJ_TYPE_IMG
};

#define SCREEN_BODY_MAX 50
#define MUSIC_BTN_ID 18
#define GAME_BTN_ID 20

#define PAD_SPACING 10          // 内边距
#define GRID_SPACING 10          // 网格间距
#define BODY_LABEL_SIZE 30      // 图标默认尺寸
#define BODY_IMG_SIZE  40

// 缩放参数
#define MAX_ZOOM 160  // 中心区域最大缩放比例 (150%)
#define MIN_ZOOM 30   // 边缘区域最小缩放比例 (80%)
#define MAX_DISTANCE 150  // 计算缩放的最大距离

typedef struct _lv_custom_body {
    uint8_t type;
    lv_obj_t *cont;
    lv_obj_t *obj;

    union {
        struct {
            const char *text;
        } lbl;

        struct {
            const lv_img_dsc_t *src;
        } img;
    } info;
} lv_custom_body_t;

static lv_obj_t *main_cont = NULL;

static lv_custom_body_t bodys[SCREEN_BODY_MAX] = {0};
static const char *text_arr[] = {
    "我",
    "是",
    "马",
    "龙",
    "伟",
};

LV_IMG_DECLARE(img_game);
LV_IMG_DECLARE(img_music);

static void custom_bodys_init(lv_ui *ui, lv_custom_body_t *body_arr, int arr_size) {
    for (int i = 0; i < arr_size; i++) {
        body_arr[i].cont = lv_obj_create(main_cont);

        lv_obj_set_style_pad_all(body_arr[i].cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(body_arr[i].cont, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(body_arr[i].cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(body_arr[i].cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_width(body_arr[i].cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(body_arr[i].cont, false, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(body_arr[i].cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(body_arr[i].cont, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_pad(body_arr[i].cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(body_arr[i].cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        // 创建实际的按钮
        if (body_arr[i].type == LV_OBJ_TYPE_LABEL) {
            lv_obj_set_size(body_arr[i].cont, BODY_LABEL_SIZE, BODY_LABEL_SIZE);
            body_arr[i].obj = lv_label_create(body_arr[i].cont);
            lv_obj_set_width(body_arr[i].obj, LV_PCT(100));
            lv_obj_set_style_text_align(body_arr[i].obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(body_arr[i].obj, body_arr[i].info.lbl.text);
        } else if (body_arr[i].type == LV_OBJ_TYPE_IMG) {
            lv_obj_set_size(body_arr[i].cont, BODY_IMG_SIZE, BODY_IMG_SIZE);
            body_arr[i].obj = lv_img_create(body_arr[i].cont);
            lv_img_set_src(body_arr[i].obj, body_arr[i].info.img.src);
            lv_obj_set_style_bg_opa(body_arr[i].obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(body_arr[i].obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(body_arr[i].obj, LV_OBJ_FLAG_EVENT_BUBBLE);
        }

        lv_obj_center(body_arr[i].obj);
        lv_obj_add_flag(body_arr[i].cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
}

lv_point_t point_final = {0,0};

static void _press_cb(lv_event_t * e) {
    lv_ui *ui = lv_event_get_user_data(e);
    lv_obj_t * obj = lv_event_get_target(e);

    // 获取拖动事件的数据
    lv_indev_t * indev = lv_indev_get_act();
    if (indev == NULL) return;

    // 获取拖动的开始和当前位置
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    lv_obj_scroll_by(ui->Home, vect.x, vect.y, LV_ANIM_OFF);

    // point_final.x += vect.x;
    // point_final.y += vect.y;

    // lv_obj_scroll_to(ui->Home, -point_final.x, -point_final.y, LV_ANIM_OFF);
}


// 获取对象到屏幕中心的距离
static float get_distance_to_center(lv_obj_t *obj, lv_obj_t *parent) {
    // 获取对象中心点坐标
    lv_coord_t obj_x = obj->coords.x1 + (obj->coords.x2 - obj->coords.x1) / 2;
    lv_coord_t obj_y = obj->coords.y1 + (obj->coords.y2 - obj->coords.y1) / 2;

    // 获取父对象中心点坐标
    lv_coord_t parent_x = parent->coords.x1 + (parent->coords.x2 - parent->coords.x1) / 2;
    lv_coord_t parent_y = parent->coords.y1 + (parent->coords.y2 - parent->coords.y1) / 2;

    // 计算距离
    float dx = obj_x - parent_x;
    float dy = obj_y - parent_y;
    return sqrtf(dx * dx + dy * dy);
}

// 滚动事件处理函数 - 处理动态缩放效果
static void _scroll_cb(lv_event_t * e) {
    lv_ui *ui = lv_event_get_user_data(e);

    // 遍历所有元素计算它们到中心的距离，并相应地调整大小
    for (int i = 0; i < SCREEN_BODY_MAX; i++) {
        // 跳过无效对象
        if (bodys[i].cont == NULL) continue;

        // 计算对象到中心的距离
        float distance = get_distance_to_center(bodys[i].cont, ui->Home);

        // 基于距离计算缩放比例
        float scale_factor;
        if (distance >= MAX_DISTANCE) {
            // 距离太远，使用最小缩放比例
            scale_factor = MIN_ZOOM / 100.0f;
        } else {
            // 线性插值计算中间值: MIN_ZOOM到MAX_ZOOM
            scale_factor = MIN_ZOOM / 100.0f +
                           (1.0f - distance / MAX_DISTANCE) *
                           ((MAX_ZOOM - MIN_ZOOM) / 100.0f);
        }

        // 设置对象大小和字体大小
        if (bodys[i].type == LV_OBJ_TYPE_IMG) {
            lv_img_set_zoom(bodys[i].obj, (lv_coord_t)(scale_factor * LV_IMG_ZOOM_NONE));
        } else if (bodys[i].type == LV_OBJ_TYPE_LABEL) {
            lv_coord_t font_size = (lv_coord_t)(20 * scale_factor); // 假设基础字体大小为32

            // 根据计算出的大小选择最接近的字体
            const lv_font_t *font = NULL;
           if (font_size >= 32) {
                font = lv_ttf_font_32;
            } else if (font_size >= 30) {
                font = lv_ttf_font_30;
            } else if (font_size >= 28) {
                font = lv_ttf_font_28;
            } else if (font_size >= 26) {
                font = lv_ttf_font_26;
            } else if (font_size >= 24) {
                font = lv_ttf_font_24;
            } else if (font_size >= 22) {
                font = lv_ttf_font_22;
            } else if (font_size >= 20) {
                font = lv_ttf_font_20;
            } else if (font_size >= 18) {
                font = lv_ttf_font_18;
            } else if (font_size >= 16) {
                font = lv_ttf_font_16;
            } else if (font_size >= 14) {
                font = lv_ttf_font_14;
            } else if (font_size >= 12) {
                font = lv_ttf_font_12;
            } else if (font_size >= 10) {
                font = lv_ttf_font_10;
            } else {
                font = lv_ttf_font_8;
            }

            // 应用字体
            lv_obj_set_style_text_font(bodys[i].obj, font, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void _screen_img_click_event_cb(lv_event_t *e) {
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (btn == bodys[MUSIC_BTN_ID].obj) {
        ui_load_scr_animation(ui, &ui->AudioList, ui->AudioList_del, &ui->Home_del, setup_scr_AudioList, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    } else if (btn == bodys[GAME_BTN_ID].obj) {
        ui_load_scr_animation(ui, &ui->GameList, ui->GameList_del, &ui->Home_del, setup_scr_GameList, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

void home_screen_custom_setup(lv_ui *ui) {
    lv_obj_set_style_bg_color(ui->Home, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->Home, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui->Home, LV_OBJ_FLAG_SCROLLABLE);

    main_cont = lv_obj_create(ui->Home);
    lv_obj_set_size(main_cont, LV_PCT(150), LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(main_cont, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(main_cont, PAD_SPACING, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(main_cont, GRID_SPACING, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(main_cont, GRID_SPACING, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_flex_main_place(main_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_cross_place(main_cont, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_track_place(main_cont, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 添加默认样式到每个图标
    for (int i = 0; i < SCREEN_BODY_MAX; i++) {
        bodys[i].type = LV_OBJ_TYPE_LABEL;
        bodys[i].info.lbl.text = text_arr[i % LV_CUSTOM_ARRAY_SIZE(text_arr)];
    }

    // 特殊图标样式
    bodys[MUSIC_BTN_ID].type = LV_OBJ_TYPE_IMG;
    bodys[MUSIC_BTN_ID].info.img.src = &img_music;

    bodys[GAME_BTN_ID].type = LV_OBJ_TYPE_IMG;
    bodys[GAME_BTN_ID].info.img.src = &img_game;


    // 初始化所有图标
    custom_bodys_init(ui, bodys, SCREEN_BODY_MAX);
    lv_memset(&point_final, 0, sizeof(point_final));

    lv_obj_add_event_cb(main_cont, _press_cb, LV_EVENT_PRESSING, ui);

    // 添加滚动监听器
    lv_obj_add_event_cb(ui->Home, _scroll_cb, LV_EVENT_SCROLL, ui);

    lv_obj_update_layout(main_cont);
    // 初始调用一次滚动回调以设置初始大小
    lv_event_send(ui->Home, LV_EVENT_SCROLL, ui);

    lv_obj_add_event_cb(bodys[MUSIC_BTN_ID].obj, _screen_img_click_event_cb, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(bodys[GAME_BTN_ID].obj, _screen_img_click_event_cb, LV_EVENT_ALL, ui);
}
