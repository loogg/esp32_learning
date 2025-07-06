#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "io_ext.h"
#include "lvgl.h"
#if CONFIG_USB_ADAPTER_CHERRYUSB
#include "usbh_hid_lvgl.h"
#elif CONFIG_USB_ADAPTER_IDF_USB
#include "usb/hid_host.h"
#endif
#include "esp_lvgl_port.h"
#include "lv_examples.h"
#include "nes.h"
#include "nesplayer.h"
#include "camera_player.h"

#define TAG "gui"

#define LCD_HOST           SPI2_HOST
#define LCD_DC_PIN         40
#define LCD_CS_PIN         21
#define LCD_PIXEL_CLOCK_HZ (60 * 1000 * 1000)
#define LCD_CMD_BITS       8
#define LCD_PARAM_BITS     8
#define LCD_H_RES          240
#define LCD_V_RES          320

#define LCD_BK_IO_EXT_PIN     11
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF() \
    do { io_ext_set_level(IO_EXT_PIN(LCD_BK_IO_EXT_PIN), !LCD_BK_LIGHT_ON_LEVEL); } while (0)
#define LCD_BK_LIGHT_ON() \
    do { io_ext_set_level(IO_EXT_PIN(LCD_BK_IO_EXT_PIN), LCD_BK_LIGHT_ON_LEVEL); } while (0)

#define LCD_RST_IO_EXT_PIN 10
#define LCD_RST_LEVEL      0
#define LCD_RST_EN() \
    do { io_ext_set_level(IO_EXT_PIN(LCD_RST_IO_EXT_PIN), LCD_RST_LEVEL); } while (0)
#define LCD_RST_DIS() \
    do { io_ext_set_level(IO_EXT_PIN(LCD_RST_IO_EXT_PIN), !LCD_RST_LEVEL); } while (0)

extern void lv_user_gui_init(void);
extern void lcd_panel_io_spi_set_sync_trans(esp_lcd_panel_io_handle_t io, bool sync);

lv_disp_t *gui_disp = NULL;
esp_lcd_panel_io_handle_t lcd_panel_io_handle;
esp_lcd_panel_handle_t lcd_panel_handle;

static uint8_t _gui_io_ext_inited = 0;

void gui_disp_set_sync_trans(bool sync) {
    lcd_panel_io_spi_set_sync_trans(lcd_panel_io_handle, sync);
}

int gui_key_send(uint8_t *key, int len) {
    nesplayer_send_key_event(key, len);
    camera_player_send_key_event(key, len);

    return 0;
}

static void gui_init_io_ext(void) {
    if (_gui_io_ext_inited) {
        return;
    }

    ESP_LOGI(TAG, "Initialize IO_EXT for LCD");
    io_ext_set_dir_output(IO_EXT_PIN(LCD_BK_IO_EXT_PIN) | IO_EXT_PIN(LCD_RST_IO_EXT_PIN));
    _gui_io_ext_inited = 1;
}

void gui_close_lcd_blk(void) {
    gui_init_io_ext();

    ESP_LOGI(TAG, "Turn off LCD backlight");
    LCD_BK_LIGHT_OFF();
}

int gui_init(void) {
    gui_init_io_ext();
    gui_close_lcd_blk();

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num       = LCD_DC_PIN,
        .cs_gpio_num       = LCD_CS_PIN,
        .pclk_hz           = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits      = LCD_CMD_BITS,
        .lcd_param_bits    = LCD_PARAM_BITS,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &lcd_panel_io_handle));

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(lcd_panel_io_handle, &panel_config, &lcd_panel_handle));

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
    LCD_RST_EN();
    vTaskDelay(pdMS_TO_TICKS(20));
    LCD_RST_DIS();
    vTaskDelay(pdMS_TO_TICKS(20));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack = 10240;
    lvgl_port_init(&lvgl_cfg);

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        const lvgl_port_display_cfg_t disp_cfg = {
            .io_handle     = lcd_panel_io_handle,
            .panel_handle  = lcd_panel_handle,
            .buffer_size   = LCD_V_RES * LCD_H_RES / 2,
            .double_buffer = false,
            .hres          = LCD_H_RES,
            .vres          = LCD_V_RES,
            .monochrome    = false,
#if LVGL_VERSION_MAJOR >= 9
            .color_format = LV_COLOR_FORMAT_RGB565,
#endif
            .rotation =
                {
                           .swap_xy  = false,
                           .mirror_x = false,
                           .mirror_y = false,
                           },
            .flags =
                {
                           .buff_dma     = true,
#if LVGL_VERSION_MAJOR >= 9
                           .swap_bytes = false,
#endif
                           .sw_rotate = false,
                           }
        };
        gui_disp = lvgl_port_add_disp(&disp_cfg);
        lv_disp_set_rotation(gui_disp, LV_DISP_ROT_270);

        lv_indev_t *kb_indev = NULL;
#if CONFIG_USB_ADAPTER_CHERRYUSB
        usbh_hid_lvgl_add_mouse(1);
        kb_indev = usbh_hid_lvgl_add_keyboard();
#elif CONFIG_USB_ADAPTER_IDF_USB
        const lvgl_port_hid_mouse_cfg_t mouse_cfg = {
            .disp        = gui_disp,
            .sensitivity = 1, /* Sensitivity of the mouse moving */
        };
        lvgl_port_add_usb_hid_mouse_input(&mouse_cfg);

        const lvgl_port_hid_keyboard_cfg_t kb_cfg = {
            .disp = gui_disp,
        };
        kb_indev = lvgl_port_add_usb_hid_keyboard_input(&kb_cfg);
#endif

        lv_group_t *g = lv_group_create();
        lv_group_set_default(g);
        lv_indev_set_group(kb_indev, g);

        lv_user_gui_init();

        // Release the mutex
        lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "Turn on LCD backlight");
    LCD_BK_LIGHT_ON();

    return 0;
}
