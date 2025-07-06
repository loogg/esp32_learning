#include "esp_log.h"
#include "nes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sound_player.h"
#include "nesplayer.h"
#include "gui.h"

static const char *TAG = "NES_PLAYER";

enum {
    NESPLAYER_MSG_NONE = 0,
    NESPLAYER_MSG_START,
    NESPLAYER_MSG_STOP,
};

enum {
    NESPLAYER_STATE_STOPPED = 0,
    NESPLAYER_STATE_PLAYING = 1,
};

const uint8_t key_id[] = {
    79, //R2
    80, //L2
    81, //D2
    82, //U2
    90, //ST2
    89, //SE2
    94, //B2
    93, //A2

    7, //R1
    4, //L1
    22, //D1
    26, //U1
    5, //ST1
    25, //SE1
    14, //B1
    13, //A1

    41, //ESC
};

typedef struct _nesplayer {
    QueueHandle_t play_queue;
    uint32_t key_data;
    char *uri;
    uint8_t state;
} nesplayer_t;

static nesplayer_t _player = {0};

/* memory */
void *nes_malloc(int num){
    return heap_caps_malloc(num, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void nes_free(void *address){
    heap_caps_free(address);
}

void *nes_memcpy(void *str1, const void *str2, size_t n){
    return memcpy(str1, str2, n);
}

void *nes_memset(void *str, int c, size_t n){
    return memset(str,c,n);
}

int nes_memcmp(const void *str1, const void *str2, size_t n){
    return memcmp(str1,str2,n);
}

#if (NES_USE_FS == 1)
/* io */
FILE *nes_fopen(const char * filename, const char * mode ){
    return fopen(filename,mode);
}

size_t nes_fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    return fread(ptr, size, nmemb,stream);
}

size_t nes_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
    return fwrite(ptr, size, nmemb,stream);
}

int nes_fseek(FILE *stream, long int offset, int whence){
    return fseek(stream,offset,whence);
}

int nes_fclose(FILE *stream ){
    return fclose(stream);
}
#endif

static void update_joypad(nes_t *nes)
{
    nes->nes_cpu.joypad.joypad = _player.key_data & 0xffff;

    if (_player.key_data & 0x10000) {
        nes->nes_quit = 1;
    }
}

#if (NES_ENABLE_SOUND == 1)

int nes_sound_output(uint8_t *buffer, size_t len){
    sound_player_raw_write((char *)buffer, len);

    return 0;
}
#endif

int nes_initex(nes_t *nes){
    sound_player_raw_load();
    sound_player_set_i2s_clk(NES_APU_SAMPLE_RATE, 8, 1);

    nes->nes_draw_data = gui_disp->driver->draw_buf->buf1;

    gui_disp_set_sync_trans(true);
    return 0;
}

int nes_deinitex(nes_t *nes){
    gui_disp_set_sync_trans(false);
    sound_player_stop();

    return 0;
}

int nes_draw(int x1, int y1, int x2, int y2, nes_color_t* color_data){
    esp_lcd_panel_draw_bitmap(lcd_panel_handle, x1, y1, x2 + 1, y2 + 1, color_data);

    return 0;
}

#define FRAMES_PER_SECOND   1000/60

void nes_frame(nes_t* nes){
    uint32_t mb_data = 0;
    if (xQueueReceive(_player.play_queue, &mb_data, 0) == pdTRUE) {
        switch (mb_data) {
            case NESPLAYER_MSG_STOP: {
                nes->nes_quit = 1;
            } break;

            default:
                break;
        }
    }

    update_joypad(nes);
    vTaskDelay(pdMS_TO_TICKS(5));
}

int nesplayer_play(char *uri) {
    if (_player.state != NESPLAYER_STATE_STOPPED) {
        nesplayer_stop();
    }

    if (_player.uri)
    {
        free(_player.uri);
    }
    _player.uri = strdup(uri);

    uint32_t play_data = NESPLAYER_MSG_START;
    xQueueSend(_player.play_queue, &play_data, 0);

    return 0;
}

int nesplayer_stop(void) {
    if (_player.state != NESPLAYER_STATE_STOPPED) {
        uint32_t play_data = NESPLAYER_MSG_STOP;
        xQueueSend(_player.play_queue, &play_data, 0);
    }

    return 0;
}

int nesplayer_send_key_event(uint8_t *key, int len) {
    uint32_t key_data = 0;

    for (int i = 0; i < len; i++) {
        for (int j = 0; j < sizeof(key_id); j++) {
            if (key[i] == key_id[j]) {
                key_data |= (1 << j);
                break;
            }
        }
    }

    _player.key_data = key_data;

    return 0;
}

static void nesplayer_entry(void *param) {
    uint32_t play_data = 0;

    while (1) {
        if (xQueueReceive(_player.play_queue, &play_data, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (play_data != NESPLAYER_MSG_START) continue;

        _player.state = NESPLAYER_STATE_PLAYING;
        _player.key_data = 0;

        vTaskDelay(pdMS_TO_TICKS(200));

        lvgl_port_lock(0);

        nes_t *nes = nes_init();
        do {
            int ret = nes_load_file(nes, _player.uri);
            if (ret) {
                ESP_LOGE(TAG, "nes load file fail");
                break;
            }

            nes_run(nes);
            nes_unload_file(nes);
        } while (0);
        nes_deinit(nes);

        lv_obj_invalidate(lv_scr_act());
        lvgl_port_unlock();

        _player.state = NESPLAYER_STATE_STOPPED;
    }
}

int nesplayer_init(void) {
    _player.play_queue = xQueueCreate(32, sizeof(uint32_t));

    if (_player.play_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }

    xTaskCreateWithCaps(nesplayer_entry, "nesplayer", 4096, NULL, 20, NULL, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    return 0;
}
