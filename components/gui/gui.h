#ifndef __GUI_H
#define __GUI_H

#include <stdint.h>
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_disp_t *gui_disp;
extern esp_lcd_panel_handle_t lcd_panel_handle;

int gui_init(void);
void gui_disp_set_sync_trans(bool sync);
void gui_close_lcd_blk(void);

#ifdef __cplusplus
}
#endif


#endif
