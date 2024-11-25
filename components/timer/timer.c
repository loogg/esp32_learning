#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "timer"
#include "esp_log.h"

static TaskHandle_t _tid = NULL;

static bool IRAM_ATTR TimerCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;

    vTaskNotifyGiveFromISR(_tid, &high_task_awoken);

    return (high_task_awoken == pdTRUE);
}


static void timer_task(void *param) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(LOG_LOCAL_TAG, "Timer callback");
    }
}

int timer_init(void) {
    if (xTaskCreate(timer_task, "timer_task", 4096, NULL, 10, &_tid) != pdPASS) {
        ESP_LOGE(LOG_LOCAL_TAG, "Create timer task failed");
        return -1;
    }

    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    gptimer_new_timer(&timer_config, &gptimer);

    //绑定一个回调函数
    gptimer_event_callbacks_t cbs = {
        .on_alarm = TimerCallback,
    };
    //设置定时器gptimer的 回调函数为cbs  传入的参数为NULL
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);

    //使能定时器
    gptimer_enable(gptimer);

    //通用定时器的报警值设置
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,  //重载计数值为0
        .alarm_count = 1000000, // 报警目标计数值 1000000 = 1s
        .flags.auto_reload_on_alarm = true, //开启重加载
    };
    //设置触发报警动作
    gptimer_set_alarm_action(gptimer, &alarm_config);
    //开始定时器开始工作
    gptimer_start(gptimer);

    return 0;
}
