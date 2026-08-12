/**
  ******************************************************************************
  * @author: FTB01_106731476@qq.com
  * @date: 2026-08-13 09:23:42
  * @lastEditior: FTB01_106731476@qq.com
  * @lastEditTime: 2026-08-13 09:26:34
  * @filePath: f:\show\IRQ\src\UART_IRQHandler.c
  * @Description: 这里是UART处理，请配置"custommode"，并"FlowOfLifeMaster"寄存器配置 进行配置。https://github.com/okero/KororFilleader/wiki/1%E7%BB%9F%E8%AE%A1%E5%AE%9A%E5%88%86
  */
#include "UART_IRQHandler.h"
#include "EXTI_IRQHandler.h"

uint8_t rx_buffer[5] = {0};

void UART_Start_Receive(void)
{
    HAL_UART_Receive_IT(&huart1, rx_buffer, 5);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // todo
        if (rx_buffer[0] == 0xFF)
        {
            for (uint8_t i = 1; i < 5; i++)
            {
                if (rx_buffer[i] == 1)
                {
                    
                }
            }
        }
    }
    HAL_UART_Receive_IT(&huart1, rx_buffer, 5);
}