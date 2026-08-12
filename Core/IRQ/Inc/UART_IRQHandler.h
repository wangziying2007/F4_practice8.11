#ifndef UART_IRQHandler_H
#define UART_IRQHandler_H

#include "main.h"
#include "usart.h"

/* ============================================================================
 * 本文件：UART RX 采用 【DMA + 空闲中断(IDLE)】 接收，用于“发消息→响蜂鸣器”。
 *
 * 工作原理概述：
 *  1. UART_Start_Receive() 启动 DMA 接收（数据自动搬入 rx_buffer，CPU 不参与）。
 *  2. 一帧数据发送完成后，总线进入空闲(IDLE)状态，触发 USART1 IDLE 中断。
 *  3. IDLE 中断里读取 DMA 还剩下多少字节没搬(计数器)，从而得到“本次一帧”的长度，
 *     然后把这一帧放进环形接收队列(帧队列)。
 *  4. 主循环调用 UART_ParseFrames() 从队列取出一帧，解析帧内容，
 *     帧头为 0xFF，帧内出现 0x01 则 Beep_Trigger++，从而驱动蜂鸣器。
 *
 * 说明：本模块所有代码均位于用户自定义层(Core/IRQ)，不修改任何 CubeMX 生成代码。
 * ==========================================================================*/

/* ----------------------------------- 参数配置 ---------------------------------- */
#define UART_RX_BUF_SIZE     128U   /* DMA 单次接收缓冲区大小（一帧最大值）       */
#define UART_FRAME_Q_SIZE      4U   /* 待处理帧队列深度（多存几帧防止丢帧）        */

#define UART_FRAME_HEAD      0xFFU  /* 帧起始字节（沿用原有协议语义）              */
#define UART_CMD_BEEP        0x01U  /* 帧内出现该值 → 触发一次蜂鸣                */

/* ----------------------------------- 对外接口 ---------------------------------- */
void UART_Start_Receive(void);      /* 启动 DMA + 空闲中断接收                    */
void UART_ParseFrames(void);        /* 主循环调用：将已收到的帧排队解析为蜂鸣触发  */

/* 蜂鸣器触发的累加计数（中断里累加，主循环(UART_ParseFrames)里清零） */
extern volatile uint8_t Beep_Trigger;

#endif /* UART_IRQHandler_H */
