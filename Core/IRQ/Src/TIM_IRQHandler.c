#include "TIM_IRQHandler.h"
#include "led.h"    /* 使用 LED3_PWM_CH / LED4_PWM_CH 通道宏 */

/*
 * 呼吸灯 PWM 驱动：
 *  - TIM2 周期中断回调里把计数做成 0 <-> 1000 的三角波，
 *    再写入 TIM3 的两路 PWM 占空比，从而让 LED3/LED4 呈现"呼吸"效果。
 *  - 通过 g_breath_enable 门控：仅在呼吸状态时才更新占空比；
 *    其它状态把占空比写 0(熄灭)。
 */

/* 呼吸灯亮度峰值/谷值 */
#define BREATH_PEAK     1000    /* 呼吸峰值(与 TIM3 的 ARR 相同=满亮) */
#define BREATH_VALLEY   0       /* 呼吸谷值(熄灭)                     */

volatile uint16_t g_breath_count = 0;   /* 当前三角波计数               */
volatile uint8_t  g_breath_enable = 0;  /* 1=呼吸变化允许，0=熄灭       */
static uint8_t    s_dir_up = 1;         /* 三角波方向：1 增 / 0 减       */

/**
 * @brief  复位呼吸灯计数（供状态机进入呼吸态时调用）
 */
void Breath_Reset(void)
{
    g_breath_count = 0;
    s_dir_up = 1;
}

/**
 * @brief  TIM2/TIM3 周期中断回调（由 main.c 的 HAL_TIM_PeriodElapsedCallback 转发）
 * @param  htim 发生周期中断的定时器句柄
 * @note   仅 TIM2 用于推进呼吸三角波；TIM3 无中断需求
 */
void TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        if (g_breath_enable != 0)
        {
            /* 三角波：增到峰值转减，减到谷值转增 */
            if (s_dir_up)
            {
                g_breath_count++;
                if (g_breath_count >= BREATH_PEAK)
                {
                    s_dir_up = 0;
                }
            }
            else
            {
                g_breath_count--;
                if (g_breath_count <= BREATH_VALLEY)
                {
                    s_dir_up = 1;
                }
            }

            /* 写入两路 PWM 占空比实现呼吸 */
            __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, g_breath_count);
            __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, g_breath_count);
        }
        else
        {
            /* 非呼吸状态：占空比清零(熄灭 LED3/LED4) */
            __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, 0);
            __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, 0);
        }
    }
}
