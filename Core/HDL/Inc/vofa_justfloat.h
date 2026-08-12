#ifndef VOFA_JUSTFLOAT_H
#define VOFA_JUSTFLOAT_H

#include "main.h"
#include "usart.h"      /* 提供 extern UART_HandleTypeDef huart1 */
#include <string.h>     /* memcpy */


/* 发送 N 个 float 组成的 JustFloat 帧（会自动拼上 0x7F800000 + '\n' 帧尾）。
 *  data    : 待发送的 float 数组
 *  n       : float 个数（最大见 #define，超出会被截断）
 */
void JustFloat_Send(const float *data, uint8_t n);

/* 发送一个 float 组成的 JustFloat 帧（等价 JustFloat_Send(&f, 1)） */
void JustFloat_Send1(float f);

/* 便捷封装：单次发送两个 float（两通道） */
void JustFloat_Send2(float a, float b);

#endif /* VOFA_JUSTFLOAT_H */
