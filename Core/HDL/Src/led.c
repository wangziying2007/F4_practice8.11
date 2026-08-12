#include "led.h"

/*
 * 本文件负责所有 LED 的驱动：
 *   - LED1/LED2 : 普通 GPIO 输出 (PA4, PA5)，用于"流水灯"效果
 *   - LED3/LED4 : 由 TIM3 的 PWM 输出 (PA6->CH1, PA7->CH2)，用于"呼吸灯"效果
 */


/**
 * @brief  初始化所有 LED
 *         关闭 LED1/LED2、清空呼吸 PWM，并启动 TIM3 的两路 PWM 输出
 * @note   呼吸灯依赖 TIM3 的 PWM；PWM 在此启动一次，此后常驻
 */
void LED_Init(void)
{
    /* 关闭普通 GPIO 灯 */
    LED_OFF(1);
    LED_OFF(2);

    /* 启动 TIM3 两路 PWM 输出 (PA6->CH1/LED3, PA7->CH2/LED4) */
    HAL_TIM_PWM_Start(&htim3, LED3_PWM_CH);
    HAL_TIM_PWM_Start(&htim3, LED4_PWM_CH);

    /* 呼吸灯占空比清零（熄灭） */
    __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, 0);
    __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, 0);
}


/**
 * @brief  关闭所有 LED（四灯全灭）
 *         - 普通 GPIO 灯直接拉低
 *         - 呼吸灯把 PWM 占空比清零
 * @note   只清亮度，不关闭 PWM 输出本身（PWM 保持输出 0 占空比即不亮）
 */
void LED_AllOff(void)
{
    LED_OFF(1);
    LED_OFF(2);
    __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, 0);
    __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, 0);
}


/**
 * @brief  LED1/LED2 流水灯（阻塞实现）
 *         依次：亮1 -> 灭1 -> 亮2 -> 灭2，循环
 * @note   主循环通过状态机调用；内部使用 HAL_Delay 产生节奏
 */
void LED_Flow(void)
{
    LED_ON(1);
    HAL_Delay(200);
    LED_OFF(1);
    HAL_Delay(200);
    LED_ON(2);
    HAL_Delay(200);
    LED_OFF(2);
    HAL_Delay(200);
}


/**
 * @brief  开启 LED3/LED4 呼吸灯
 *         把 TIM3 两路 PWM 占空比复位到 0，使呼吸三角波从 0 开始
 * @note   TIM3 PWM 需已被 HAL_TIM_PWM_Start 启动（见 tim.c）
 */
void LED_BreathStart(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, 0);
    __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, 0);
}


/**
 * @brief  关闭呼吸灯并熄灭 LED3/LED4
 *         将占空比清零，同时对内部呼吸计数复位（见 TIM_IRQHandler.c）
 */
void LED_BreathStop(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, LED3_PWM_CH, 0);
    __HAL_TIM_SET_COMPARE(&htim3, LED4_PWM_CH, 0);
    /* 呼吸三角波方向/计数由 TIM_IRQHandler.c 复位，这里仅清亮度 */
}
