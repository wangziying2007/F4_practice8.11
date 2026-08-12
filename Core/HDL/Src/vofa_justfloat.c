#include "vofa_justfloat.h"

/* 拼接缓冲：可容纳 JUST_FLOAT_MAX_N 个“4字节 float + 1字节'\n'”块 */
#define JUST_FLOAT_MAX_N        16U                                /* 一次最多发送的 float 个数  */
#define JUST_FLOAT_SEP          '\n'                                /* 每个数据块后的分隔/帧尾符 */
#define JUST_FLOAT_BUF_LEN      (JUST_FLOAT_MAX_N * 5U)            /* 16*(4+1)=80 字节，够用     */

static uint8_t s_tx_buf[JUST_FLOAT_BUF_LEN];

void JustFloat_Send(const float *data, uint8_t n)
{
    uint16_t len = 0U;

    if (data == NULL || n == 0U)
    {
        return;
    }
    if (n > JUST_FLOAT_MAX_N)
    {
        n = JUST_FLOAT_MAX_N;           /* 超限截断，避免越界 */
    }

    for (uint8_t i = 0U; i < n; i++)
    {
        /* 用 memcpy 把第 i 个 float 的 4 字节按内存小端顺序拷贝到发送缓冲 */
        memcpy(&s_tx_buf[len], &data[i], sizeof(float));
        len += sizeof(float);

        /* 每块数据之后紧跟一个 '\n'(0x0A)：VOFA+ JustFloat 依此切分通道/帧 */
        s_tx_buf[len++] = (uint8_t)JUST_FLOAT_SEP;
    }

    /* 一帧内容拼接完成：形如 [f1]\n[f2]\n...[fn]\n，最后一块的 '\n' 即帧尾。
     * 直接阻塞发送到 USART1（115200,8N1） */
    HAL_UART_Transmit(&huart1, s_tx_buf, len, 100);
}

void JustFloat_Send1(float f)
{
    JustFloat_Send(&f, 1U);
}

void JustFloat_Send2(float a, float b)
{
    float d[] = {a, b};
    JustFloat_Send(d, 2U);
}
