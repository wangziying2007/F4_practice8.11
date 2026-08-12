#include "EXTI_IRQHandler.h"

/*
 * 按键输入处理：
 *  - 按键接在 INPUT_1 (PC11，CubeMX 配置为 Pull-down + EXTI 上升沿)。
 *  - 空闲时默认被下拉为低电平，按下 -> 高电平(上升沿)触发 EXTI；
 *    主循环轮询释放(回到低)完成长短按判定。
 *  - 长按阈值在本文件统一定义，调整 KEY_LONG_PRESS_MS 即可，无需改动其它位置。
 */

/*--------------------------------------- 参数定义 ---------------------------------------*/
/* 长按阈值(单位:ms)：按压时间 >= 此值 → 长按；否则 → 短按 */
#define KEY_LONG_PRESS_MS     500

/*--------------------------------------- 内部变量 ---------------------------------------*/
static volatile uint8_t       g_key_press_flag;      /* 1=检测到按下，等待处理 */
static volatile uint32_t      g_press_tick;          /* 本次按下的时刻(HAL_GetTick) */

/**
 * @brief  外部中断回调：INPUT_1 按下(上升沿)时触发
 * @note   仅记录按下起点与标志，具体长短按判定交给主循环的 Key_Scan()
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == INPUT_1_Pin)
    {
        g_press_tick     = HAL_GetTick();   /* 记录按下起点 */
        g_key_press_flag = 1;               /* 置"有待处理"标志 */
    }
}

/**
 * @brief  扫描按键并返回一次完整按下-释放的事件类型
 * @return KEY_EVENT_LONG  长按(达到/超过阈值)
 *         KEY_EVENT_SHORT 短按(未达到阈值)
 *         KEY_EVENT_NONE  无按键事件
 * @note   阻塞等待按键释放(释放后返回)，用于计算按压时长。
 *         需在主循环中周期调用。
 */
Key_Event_t Key_Scan(void)
{
    /* 无按键按下则直接返回 */
    if (!g_key_press_flag)
    {
        return KEY_EVENT_NONE;
    }
    g_key_press_flag = 0;

    /* 去抖：按下时引脚为高；若此刻读到的不是高电平，视为抖动，忽略本次 */
    if (HAL_GPIO_ReadPin(INPUT_1_GPIO_Port, INPUT_1_Pin) == GPIO_PIN_RESET)
    {
        return KEY_EVENT_NONE;
    }

    /* 等待按键释放：Pull-down，释放后引脚回到低电平 */
    while (HAL_GPIO_ReadPin(INPUT_1_GPIO_Port, INPUT_1_Pin) == GPIO_PIN_SET)
    {
        /* 等待释放(空转)，一旦释放即跳出 */
    }

    /* 计算按压时长并判定短按/长按 */
    if ((uint32_t)(HAL_GetTick() - g_press_tick) >= KEY_LONG_PRESS_MS)
    {
        return KEY_EVENT_LONG;
    }
    return KEY_EVENT_SHORT;
}
