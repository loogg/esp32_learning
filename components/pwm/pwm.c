#include "driver/ledc.h"
#include "shell.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "pwm"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0        //定时器0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE //低速模式
#define LEDC_OUTPUT_IO          (48)                // 定义输出GPIO为GPIO48
#define LEDC_CHANNEL            LEDC_CHANNEL_0      // 使用LEDC的通道0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT   // LEDC分辨率设置为13位
#define LEDC_DUTY               (819)              // 设置占空比为50%。 ((2的13次方) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (100)              // 频率单位是Hz。设置频率为5000 Hz

int pwm_init(void) {
    // 准备并应用led PWM定时器配置
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,          //LED模式 低速模式
        .timer_num        = LEDC_TIMER,         //通道的定时器源    定时器0
        .duty_resolution  = LEDC_DUTY_RES,      //将占空比分辨率设置为13位
        .freq_hz          = LEDC_FREQUENCY,     // 设置输出频率为5 kHz
        .clk_cfg          = LEDC_AUTO_CLK       //设置LEDPWM的时钟来源 为自动
        //LEDC_AUTO_CLK = 启动定时器时，将根据给定的分辨率和占空率参数自动选择led源时钟
    };
    ledc_timer_config(&ledc_timer);

    // 准备并应用LEDC PWM通道配置
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,            //LED模式 低速模式
        .channel        = LEDC_CHANNEL,         //通道0
        .timer_sel      = LEDC_TIMER,           //定时器源 定时器0
        .intr_type      = LEDC_INTR_DISABLE,    //关闭中断
        .gpio_num       = LEDC_OUTPUT_IO,       //输出引脚  GPIO5
        .duty           = 0,                    // 设置占空比为0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    return 0;
}
