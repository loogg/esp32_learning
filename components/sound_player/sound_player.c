#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "raw_stream.h"
#include "i2s_stream.h"
#include "wav_decoder.h"
#include "mp3_decoder.h"
#include "board.h"
#include "io_ext.h"
#include "sound_player.h"

static const char *TAG = "SOUND_PLAYER";

#define SPK_EN_IO_EXT_PIN 2
#define SPK_EN_ON_LEVEL   0
#define SPK_EN_OFF_LEVEL  1

enum {
    AUDIO_EVENT_PLAYER_MUSIC = AUDIO_ELEMENT_TYPE_PLAYER + 1,
};

typedef struct _sound_player_t {
    audio_board_handle_t board_handle;

    audio_pipeline_handle_t pipeline;

    audio_element_handle_t fatfs_stream_reader;
    audio_element_handle_t raw_stream_reader;
    audio_element_handle_t i2s_stream_writer;
    audio_element_handle_t wav_decoder;
    audio_element_handle_t mp3_decoder;

    audio_event_iface_handle_t evt;
} sound_player_t;

static sound_player_t _sound_player = {0};

int sound_player_music(const char *file_path) {
    ESP_LOGI(TAG, "Playing file: %s", file_path);

    sound_player_stop();

    if (strstr(file_path, ".wav") || strstr(file_path, ".WAV")) {
        audio_pipeline_link(_sound_player.pipeline, (const char *[]){"file", "wav", "i2s"}, 3);
    } else if (strstr(file_path, ".mp3") || strstr(file_path, ".MP3")) {
        audio_pipeline_link(_sound_player.pipeline, (const char *[]){"file", "mp3", "i2s"}, 3);
    } else {
        ESP_LOGE(TAG, "Unsupported file format: %s", file_path);
        return ESP_FAIL;
    }

    audio_pipeline_set_listener(_sound_player.pipeline, _sound_player.evt);

    audio_element_set_uri(_sound_player.fatfs_stream_reader, file_path);
    audio_pipeline_reset_ringbuffer(_sound_player.pipeline);
    audio_pipeline_reset_elements(_sound_player.pipeline);
    audio_pipeline_run(_sound_player.pipeline);

    return 0;
}

int sound_player_stop(void) {
    ESP_LOGI(TAG, "Stopping sound player...");

    audio_pipeline_stop(_sound_player.pipeline);
    audio_pipeline_wait_for_stop(_sound_player.pipeline);
    audio_pipeline_terminate(_sound_player.pipeline);
    audio_pipeline_reset_ringbuffer(_sound_player.pipeline);
    audio_pipeline_reset_elements(_sound_player.pipeline);
    audio_pipeline_unlink(_sound_player.pipeline);

    return 0;
}

static void sound_player_entry(void *param) {
    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t               ret = audio_event_iface_listen(_sound_player.evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            // Set music info for a new song to be played
            if (msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO
                && (msg.source == (void *)_sound_player.mp3_decoder || msg.source == (void *)_sound_player.wav_decoder)) {
                audio_element_info_t music_info = {0};
                if (msg.source == (void *)_sound_player.mp3_decoder) {
                    audio_element_getinfo(_sound_player.mp3_decoder, &music_info);
                    ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d", music_info.sample_rates,
                             music_info.bits, music_info.channels);
                } else if (msg.source == (void *)_sound_player.wav_decoder) {
                    audio_element_getinfo(_sound_player.wav_decoder, &music_info);
                    ESP_LOGI(TAG, "[ * ] Received music info from wav decoder, sample_rates=%d, bits=%d, ch=%d", music_info.sample_rates,
                             music_info.bits, music_info.channels);
                }
                audio_element_setinfo(_sound_player.i2s_stream_writer, &music_info);
                i2s_stream_set_clk(_sound_player.i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }

            // Advance to the next song when previous finishes
            if (msg.source == (void *)_sound_player.i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                audio_element_state_t el_state = audio_element_get_state(_sound_player.i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED) { ESP_LOGI(TAG, "[ * ] Finished, advancing to the next song"); }
                continue;
            }
        }
    }
}

int sound_player_init(void) {
    ESP_LOGI(TAG, "Initializing sound player...");

    ESP_LOGI(TAG, "Start codec chip");
    _sound_player.board_handle = audio_board_init();
    audio_hal_ctrl_codec(_sound_player.board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    _sound_player.pipeline        = audio_pipeline_init(&pipeline_cfg);
    AUDIO_NULL_CHECK(TAG, _sound_player.pipeline, return ESP_FAIL);

    ESP_LOGI(TAG, "Create fatfs stream to read data from sdcard");
    fatfs_stream_cfg_t fatfs_cfg      = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type                    = AUDIO_STREAM_READER;
    _sound_player.fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

    ESP_LOGI(TAG, "Create raw stream to read data");
    raw_stream_cfg_t raw_cfg        = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type                    = AUDIO_STREAM_READER;
    _sound_player.raw_stream_reader = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg        = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type                    = AUDIO_STREAM_WRITER;
    _sound_player.i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    wav_decoder_cfg_t wav_dec_cfg = DEFAULT_WAV_DECODER_CONFIG();
    _sound_player.wav_decoder     = wav_decoder_init(&wav_dec_cfg);

    ESP_LOGI(TAG, "Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    _sound_player.mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "Register all elements to pipeline");
    audio_pipeline_register(_sound_player.pipeline, _sound_player.fatfs_stream_reader, "file");
    audio_pipeline_register(_sound_player.pipeline, _sound_player.raw_stream_reader, "raw");
    audio_pipeline_register(_sound_player.pipeline, _sound_player.i2s_stream_writer, "i2s");
    audio_pipeline_register(_sound_player.pipeline, _sound_player.wav_decoder, "wav");
    audio_pipeline_register(_sound_player.pipeline, _sound_player.mp3_decoder, "mp3");

    ESP_LOGI(TAG, "Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    _sound_player.evt               = audio_event_iface_init(&evt_cfg);

    io_ext_set_dir_output(IO_EXT_PIN(SPK_EN_IO_EXT_PIN));
    io_ext_set_level(IO_EXT_PIN(SPK_EN_IO_EXT_PIN), SPK_EN_ON_LEVEL);

    audio_hal_set_volume(_sound_player.board_handle->audio_hal, 100);

    xTaskCreatePinnedToCore(sound_player_entry, "sound_player", 8192, NULL, 5, NULL, 0);

    return 0;
}
