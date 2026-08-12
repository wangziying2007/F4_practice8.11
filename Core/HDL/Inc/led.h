#ifndef LED_H
#define LED_H

/*------------------------include------------------------*/
#include "main.h"
#include "gpio.h"
#include "tim.h"    /* 使用 TIM3(htim3) 驱动 LED3/LED4 呼吸灯(PWM) */

/*-----------------------Define-------------------------*/
/* 普通 GPIO 灯(LED1/LED2)的开关控制 */
#define LED_ON(x)  HAL_GPIO_WritePin(led_##x##_GPIO_Port, led_##x##_Pin, GPIO_PIN_SET)
#define LED_OFF(x) HAL_GPIO_WritePin(led_##x##_GPIO_Port, led_##x##_Pin, GPIO_PIN_RESET)
#define LED_TOGGLE(x) HAL_GPIO_TogglePin(led_##x##_GPIO_Port, led_##x##_Pin)

/*
 * 灯光状态机需要的外部操作状态
 */
typedef enum
{
    LED_STATE_OFF = 0,      /* 四灯全灭                 */
    LED_STATE_FLOW,         /* LED1/LED2 流水灯         */
    LED_STATE_BREATH        /* LED3/LED4 呼吸灯(PWM)    */
} Led_State_t;

/* LED3/LED4 由 TIM3 的 PWM 通道输出 (PA6->CH1, PA7->CH2) */
#define LED3_PWM_CH        TIM_CHANNEL_1   /* LED3 : TIM3_CH1 (PA6) */
#define LED4_PWM_CH        TIM_CHANNEL_2   /* LED4 : TIM3_CH2 (PA7) */
/* 呼吸灯亮度峰值调试：由 TIM_IRQHandler.c 中 BREATH_PEAK 调整 */

/*-----------------------Function-----------------------*/

void LED_Init(void);            /* 初始化所有 LED：关闭 LED1/2，清空呼吸 PWM */
void LED_AllOff(void);          /* 四灯全灭          */
void LED_Flow(void);            /* LED1/LED2 流水灯（阻塞） */
void LED_BreathStart(void);     /* 开启 LED3/LED4 呼吸 */
void LED_BreathStop(void);      /* 关闭呼吸并熄灭 LED3/LED4 */

#endif/*LED_H*/