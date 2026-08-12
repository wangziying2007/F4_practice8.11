#ifndef TIM_IRQHandler_H
#define TIM_IRQHandler_H

#include "main.h"
#include "tim.h"
#include "EXTI_IRQHandler.h"

/*--------------------------------------- Extern Variables ---------------------------------------*/
extern volatile uint16_t g_breath_count;   /* 呼吸灯当前占空比计数(三角波，0~1000) */
extern volatile uint8_t  g_breath_enable;  /* 1=呼吸灯允许变化；0=关闭呼吸(占空比清零) */

/*--------------------------------------- Function ---------------------------------------*/
void TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);   /* TIM2/TIM3 周期中断回调 */
void Breath_Reset(void);                                    /* 复位呼吸灯计数 */

#endif