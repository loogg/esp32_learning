#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "esp_log.h"
#include <lvgl.h>
#include "lv_examples.h"
#include "gui_guider.h"
#include "custom.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG             "lvgl.demo"

#define SD_TTF_PATH "/sdcard/Cubic_11.ttf"
#define FREETYPE_TTF_PATH "S:" SD_TTF_PATH

extern int sdcard_check_mount(void);

lv_ui guider_ui;
lv_font_t *lv_ttf_font_32 = NULL;
lv_font_t *lv_ttf_font_30 = NULL;
lv_font_t *lv_ttf_font_28 = NULL;
lv_font_t *lv_ttf_font_26 = NULL;
lv_font_t *lv_ttf_font_24 = NULL;
lv_font_t *lv_ttf_font_22 = NULL;
lv_font_t *lv_ttf_font_20 = NULL;
lv_font_t *lv_ttf_font_18 = NULL;
lv_font_t *lv_ttf_font_16 = NULL;
lv_font_t *lv_ttf_font_14 = NULL;
lv_font_t *lv_ttf_font_12 = NULL;
lv_font_t *lv_ttf_font_10 = NULL;
lv_font_t *lv_ttf_font_8 = NULL;

#ifdef CONFIG_FONT_TTF_USING_FREETYPE
lv_ft_info_t ft_info;
#endif

static uint8_t *ttf_ram_buf = NULL;
static size_t ttf_ram_buf_size = 0;

static int read_ttf_file_to_ram(void) {
    struct stat s;
    memset(&s, 0, sizeof(struct stat));
    if (stat(SD_TTF_PATH, &s) != 0) return -1;

    ttf_ram_buf = malloc(s.st_size);
    if (ttf_ram_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for TTF file.");
        return -1;
    }

    int fd = open(SD_TTF_PATH, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open TTF file: %s", SD_TTF_PATH);
        free(ttf_ram_buf);
        ttf_ram_buf = NULL;
        return -1;
    }
    int bytes_read = read(fd, ttf_ram_buf, s.st_size);
    close(fd);

    if (bytes_read != s.st_size) {
        ESP_LOGE(TAG, "Failed to read the entire TTF file: %s", SD_TTF_PATH);
        free(ttf_ram_buf);
        ttf_ram_buf = NULL;
        return -1;
    }

    ESP_LOGI(TAG, "TTF file read into RAM successfully, size: %d bytes", s.st_size);
    ttf_ram_buf_size = s.st_size;
    return 0;
}

void lv_user_gui_init(void)
{
    while(sdcard_check_mount() != 1)
    {
        ESP_LOGW(TAG, "waiting for SD card mount...");
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    ESP_LOGI(TAG, "now create TTF fonts...");

    if (read_ttf_file_to_ram() != 0) {
        ESP_LOGE(TAG, "Failed to read TTF file into RAM.");
        return;
    }

#if CONFIG_FONT_TTF_USING_TINYTTF
    lv_ttf_font_32 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 32, 32 * 1024);
    lv_ttf_font_30 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 30, 32 * 1024);
    lv_ttf_font_28 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 28, 32 * 1024);
    lv_ttf_font_26 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 26, 32 * 1024);
    lv_ttf_font_24 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 24, 32 * 1024);
    lv_ttf_font_22 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 22, 32 * 1024);
    lv_ttf_font_20 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 20, 32 * 1024);
    lv_ttf_font_18 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 18, 32 * 1024);
    lv_ttf_font_16 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 16, 32 * 1024);
    lv_ttf_font_14 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 14, 32 * 1024);
    lv_ttf_font_12 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 12, 32 * 1024);
    lv_ttf_font_10 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 10, 32 * 1024);
    lv_ttf_font_8 = lv_tiny_ttf_create_data_ex(ttf_ram_buf, ttf_ram_buf_size, 8, 32 * 1024);
#endif

#ifdef CONFIG_FONT_TTF_USING_FREETYPE
    /*FreeType uses C standard file system, so no driver letter is required.*/
    ft_info.name = FREETYPE_TTF_PATH;
    ft_info.weight = 32;
    ft_info.style = FT_FONT_STYLE_NORMAL;
    ft_info.mem = NULL;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_32 = ft_info.font;

    ft_info.weight = 30;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_30 = ft_info.font;

    ft_info.weight = 28;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_28 = ft_info.font;

    ft_info.weight = 26;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_26 = ft_info.font;

    ft_info.weight = 24;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_24 = ft_info.font;

    ft_info.weight = 22;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_22 = ft_info.font;

    ft_info.weight = 20;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_20 = ft_info.font;

    ft_info.weight = 18;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_18 = ft_info.font;

    ft_info.weight = 16;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_16 = ft_info.font;

    ft_info.weight = 14;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_14 = ft_info.font;

    ft_info.weight = 12;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_12 = ft_info.font;

    ft_info.weight = 10;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_10 = ft_info.font;

    ft_info.weight = 8;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_8 = ft_info.font;
#endif

    setup_ui(&guider_ui);
    custom_init(&guider_ui);
}
