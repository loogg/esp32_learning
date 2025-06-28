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

#define LCD_RST_IO_EXT_PIN 10
#define LCD_RST_LEVEL      0

extern void lv_user_gui_init(void);
extern void lcd_panel_io_spi_set_sync_trans(esp_lcd_panel_io_handle_t io, bool sync);

static lv_disp_t *gui_disp = NULL;
static esp_lcd_panel_io_handle_t lcd_panel_io_handle;

void gui_disp_set_sync_trans(bool sync) {
    lcd_panel_io_spi_set_sync_trans(lcd_panel_io_handle, sync);
}

int gui_init(void) {
    io_ext_set_dir_output(IO_EXT_PIN(LCD_BK_IO_EXT_PIN) | IO_EXT_PIN(LCD_RST_IO_EXT_PIN));

    ESP_LOGI(TAG, "Turn off LCD backlight");
    // Turn off backlight to avoid unpredictable display on the LCD screen while initializing
    // the LCD panel driver. (Different LCD screens may need different levels)
    io_ext_set_level(IO_EXT_PIN(LCD_BK_IO_EXT_PIN), !LCD_BK_LIGHT_ON_LEVEL);

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
    esp_lcd_panel_handle_t     panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(lcd_panel_io_handle, &panel_config, &panel_handle));

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    io_ext_set_level(IO_EXT_PIN(LCD_RST_IO_EXT_PIN), LCD_RST_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(20));
    io_ext_set_level(IO_EXT_PIN(LCD_RST_IO_EXT_PIN), !LCD_RST_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    size_t total = 0, used = 0, max_used = 0;
    printf("DMA-capable memory:\r\n");
    total = heap_caps_get_total_size(MALLOC_CAP_DMA);
    used = total - heap_caps_get_free_size(MALLOC_CAP_DMA);
    max_used = total - heap_caps_get_minimum_free_size(MALLOC_CAP_DMA);
    printf("total   : %d\r\n", total);
    printf("used    : %d\r\n", used);
    printf("maximum : %d\r\n", max_used);

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        const lvgl_port_display_cfg_t disp_cfg = {
            .io_handle     = lcd_panel_io_handle,
            .panel_handle  = panel_handle,
            .buffer_size   = LCD_H_RES * 20,
            .double_buffer = true,
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
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "Turn on LCD backlight");
    io_ext_set_level(IO_EXT_PIN(LCD_BK_IO_EXT_PIN), LCD_BK_LIGHT_ON_LEVEL);

    return 0;
}
