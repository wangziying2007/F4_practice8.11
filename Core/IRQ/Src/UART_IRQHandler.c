/**
  ******************************************************************************
  * @author: FTB01_106731476@qq.com
  * @date: 2026-08-13 09:23:42
  * @lastEditior: FTB01_106731476@qq.com
  * @lastEditTime: 2026-08-13 09:26:34
  * @filePath: f:\show\IRQ\src\UART_IRQHandler.c
  * @Description: 这里是UART处理，请配置"custommode"，并"FlowOfLifeMaster"寄存器配置 进行配置。https://github.com/okero/KororFilleader/wiki/1%E7%BB%9F%E8%AE%A1%E5%AE%9A%E5%88%86
  */
/**
  * @file  UART_IRQHandler.c
  * @brief USART1 通过 【DMA + 空闲中断(IDLE)】 接收，实现“收到消息 → 响蜂鸣器”。
  *
  * 数据流：
  *   USART1 外设  --DMA-->  rx_buffer  --空闲中断判定一帧-->  帧队列
  *   帧队列  --主循环 UART_ParseFrames()-->  解析帧  --Beep_Trigger++-->  蜂鸣器
  *
  * 说明（遵守工程约束）：
  *   - 本文件属于用户自定义隔离层(Core/IRQ)，不修改任何 CubeMX 生成的
  *     usart.c / usart.h / dma.c / dma.h / stm32f4xx_it.c 等文件。
  *   - USART1_RX 对应 DMA：DMA2_Stream2 通道4（这是 STM32F4 的硬件映射）。
  *   - USART1 中断服务程序、DMA 中断服务程序都定义在本文件，以此强符号
  *     覆盖启动文件里的 [WEAK] 默认实现 —— 无需改动 stm32f4xx_it.c。
  */
#include "UART_IRQHandler.h"

/* ============================================================================
 * 模块内部私有数据
 * ==========================================================================*/

/* DMA 接收缓冲区：DMA 收到一帧数据后存放在这里 */
static uint8_t        s_rx_buf[UART_RX_BUF_SIZE];

/* 待处理帧队列（环形）：中断里把完整“一帧”拷贝进来，主循环解析取走 */
static uint8_t        s_frame_q[UART_FRAME_Q_SIZE][UART_RX_BUF_SIZE]; /* 帧缓冲池   */
static uint8_t        s_frame_q_len[UART_FRAME_Q_SIZE];               /* 每一帧长度 */
static volatile uint8_t s_q_head = 0U;   /* 写指针（中断里推进） */
static volatile uint8_t s_q_tail = 0U;   /* 读指针（主循环推进） */

/* 蜂鸣器触发累计计数：由 UART_ParseFrames() 在解析帧时累加，主循环清零 */
volatile uint8_t Beep_Trigger = 0U;

/* USART1 的 RX DMA 句柄（手动初始化，因 CubeMX 未生成） */
DMA_HandleTypeDef hdma_usart1_rx;

/* ============================================================================
 * 私有函数声明
 * ==========================================================================*/
static void UART_DMA_RxInit(void);            /* 初始化并绑定 DMA                */
static void UART_PushFrame(const uint8_t *p, uint16_t len); /* 存入帧队列      */

/**
 * @brief  初始化 USART1 的 DMA 接收通道并绑定到 huart1.
 * @note   不修改 usart.c / dma.c，DMA 时钟、句柄、链路都在这里完成。
 */
static void UART_DMA_RxInit(void)
{
    /* 1. 使能 DMA2 控制器时钟（DMA2 挂载在 AHB1 总线） */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* 2. 配置 DMA2_Stream2 / 通道4 用于 USART1_RX */
    hdma_usart1_rx.Instance                 = DMA2_Stream2;   /* USART1_RX 标准流    */
    hdma_usart1_rx.Init.Channel             = DMA_CHANNEL_4;  /* USART1_RX 标准通道  */
    hdma_usart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY; /* 外设→内存(接收)  */
    hdma_usart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;      /* 外设地址不增    */
    hdma_usart1_rx.Init.MemInc              = DMA_MINC_ENABLE;       /* 内存地址递增    */
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;   /* 8 位数据         */
    hdma_usart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;   /* 8 位数据         */
    hdma_usart1_rx.Init.Mode                = DMA_NORMAL;            /* 非循环模式       */
    hdma_usart1_rx.Init.Priority            = DMA_PRIORITY_HIGH;     /* 优先级           */
    hdma_usart1_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;  /* 直接模式         */

    /* 3. 初始化 DMA 流 */
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
        Error_Handler();
    }

    /* 4. 把 DMA 句柄绑定到 USART1 的 hdmarx 成员（HAL_UART_Receive_DMA 依赖它） */
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    /* 5. 使能 DMA2_Stream2 的传输完成中断并配置 NVIC。
     *    这里自行配置，不依赖 dma.c。 */
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

/**
 * @brief  启动 USART1 的 DMA + 空闲中断接收。
 * @note   main() 初始化串口后调用一次。这是主入口(对外保留原函数名)。
 */
void UART_Start_Receive(void)
{
    /* 首次调用时完成 DMA 初始化与绑定（后续重复调用不会重复初始化） */
    if (huart1.hdmarx == NULL)
    {
        UART_DMA_RxInit();
    }

    /* 使能 USART1 全局中断（IDLE、错误、接收等都需在中断里处理） */
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* 开始 DMA 接收：数据自动写入 s_rx_buf，收满则触发完成回调 */
    HAL_UART_Receive_DMA(&huart1, s_rx_buf, UART_RX_BUF_SIZE);

    /* 打开“空闲中断”(IDLE)：一帧发送结束后总线空闲 → 判定一帧结束 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

/**
 * @brief  USART1 全局中断服务程序（覆盖启动文件的 [WEAK] 弱定义）。
 * @note   HAL_UART_IRQHandler 处理普通接收(含 DMA完成)，IDLE 标志自行处理。
 */
void USART1_IRQHandler(void)
{
    /* NOTE: 这里不调用 HAL_UART_IRQHandler()。
     * 在 DMA 接收模式下，HAL_UART_IRQHandler() 一旦检测到 ORE/FE/NE 错误就会
     * 中止 DMA 接收(UART_EndRxTransfer + Abort)，导致接收被反复打断，蜂鸣器不响。
     * 本模块只依赖 IDLE 中断判定帧结束，因此直接手动处理 IDLE，不走 HAL 的错误中止逻辑。 */

    /* ---- 空闲中断(IDLE)处理：说明“这一帧数据已发完” ---- */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        /* 清除 IDLE 标志，允许下次空闲再次触发 */
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);

        /* 实际长度 = 配置的 DMA 长度 - DMA 计数器剩余值 */
        uint16_t rx_len = (uint16_t)(UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));

        if (rx_len > 0U)
        {
            /* 把这一帧放入待处理队列 */
            UART_PushFrame(s_rx_buf, rx_len);
        }

        /* 重启 DMA 接收以接收下一帧。
         * 需先停止(复位DMA流)再重新启动，避免残留计数导致丢帧。 */
        HAL_UART_DMAStop(&huart1);
        HAL_UART_Receive_DMA(&huart1, s_rx_buf, UART_RX_BUF_SIZE);
        /* DMAStop 会连同把 IDLE 使能清掉，这里重新打开空闲中断 */
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    }
    else if ((READ_REG(huart1.Instance->SR) & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0U)
    {
        /* 非 IDLE 的错误中断(ORE/FE/NE)：读 SR 再读 DR 清除错误标志，避免错误标志滞留造成中断风暴 */
        (void)READ_REG(huart1.Instance->SR);
        (void)huart1.Instance->DR;
    }
}

/**
 * @brief  DMA2_Stream2 全局中断服务程序（覆盖启动文件的 [WEAK] 弱定义）。
 * @note   收满一整缓冲时会进入这里。
 */
void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

/**
 * @brief  USART1 接收完成回调（收满一整缓冲时 HAL 调用）。
 * @note   连续收到整整 UART_RX_BUF_SIZE 字节时触发；一帧也算完整，直接入队处理。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 把一整缓冲作为一帧放入队列 */
        UART_PushFrame(s_rx_buf, UART_RX_BUF_SIZE);

        /* 重新启动 DMA 接收（继续监听后续数据） */
        HAL_UART_Receive_DMA(&huart1, s_rx_buf, UART_RX_BUF_SIZE);
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    }
}

/**
 * @brief  把一帧数据拷贝进环形待处理队列（供 UART_ParseFrames 消费）。
 * @param  p    待拷入的数据
 * @param  len  数据长度（不超过 UART_RX_BUF_SIZE）
 * @note   在中断上下文调用。队满时采用“覆盖最旧一帧”策略，保证总是处理最新数据。
 */
static void UART_PushFrame(const uint8_t *p, uint16_t len)
{
    if (len > UART_RX_BUF_SIZE) len = UART_RX_BUF_SIZE;

    /* 若队列已满，则覆盖最旧的一帧（读指针+1），保证不丢失最新数据 */
    if (((s_q_head + 1U) & (UART_FRAME_Q_SIZE - 1U)) == s_q_tail)
    {
        s_q_tail = (uint8_t)((s_q_tail + 1U) & (UART_FRAME_Q_SIZE - 1U));
    }

    /* 拷贝数据 */
    uint16_t i;
    for (i = 0U; i < len; i++)
    {
        s_frame_q[s_q_head][i] = p[i];
    }
    s_frame_q_len[s_q_head] = (uint8_t)len;

    /* 推进写指针 */
    s_q_head = (uint8_t)((s_q_head + 1U) & (UART_FRAME_Q_SIZE - 1U));
}

/**
 * @brief  主循环中调用：逐帧取出队列里的消息帧，解析并累加蜂鸣触发。
 * @note   帧协议沿用原有语义：
 *           - 帧头必须为 UART_FRAME_HEAD(0xFF)
 *           - 帧头之后的字节里，每出现一个 UART_CMD_BEEP(0x01) 就计一次蜂鸣
 *         解析后 Beep_Trigger 由 main 主循环读取并触发 Beep_Alarm()。
 */
void UART_ParseFrames(void)
{
    /* 队列空则退出 */
    if (s_q_head == s_q_tail)
    {
        return;
    }

    uint8_t idx = s_q_tail;   /* 取最旧的一帧 */
    uint8_t len = s_frame_q_len[idx];

    /* 校验帧头：必须是 0xFF 才按协议解析 */
    if (len > 0U && s_frame_q[idx][0] == UART_FRAME_HEAD)
    {
        /* 遍历帧头之后的每个字节，出现 0x01 即触发一次蜂鸣 */
        uint8_t i;
        for (i = 1U; i < len; i++)
        {
            if (s_frame_q[idx][i] == UART_CMD_BEEP)
            {
                Beep_Trigger++;          /* 累加蜂鸣触发次数 */
            }
        }
    }
    /* 非 0xFF 开头的帧：按协议视为无效，忽略不处理 */

    /* 推进读指针，消费掉这一帧 */
    s_q_tail = (uint8_t)((s_q_tail + 1U) & (UART_FRAME_Q_SIZE - 1U));
}
