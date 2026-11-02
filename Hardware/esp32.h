#ifndef __ESP32_H
#define __ESP32_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* ESP32 通信串口：USART2 (PA2=TX, PA3=RX), 115200 8N1 */

/* 初始化：清空缓冲并启动接收中断 */
void ESP32_Init(void);

/* 发送字符串（不自动换行） */
void ESP32_SendString(const char *s);

/* 发送一条命令，自动补 "\r\n" */
void ESP32_SendCommand(const char *cmd);

/* 环形缓冲里还有多少字节可读 */
int ESP32_Available(void);

/* 从环形缓冲读取最多 len 字节，返回实际读出字节数 */
uint16_t ESP32_Read(uint8_t *buf, uint16_t len);

/* 取出一条完整行（不含 \r\n，已过滤 ANSI 颜色转义码）。
 * 返回 1：out 里是一条完整行；返回 0：还没凑齐一整行。 */
int ESP32_GetLine(char *out, uint16_t max_len);

/* 丢弃环形缓冲中未读的数据 */
void ESP32_ClearRxBuffer(void);

/* 阻塞等待串口数据 (中断里会释放信号量)
 * 返回 true = 有数据到了; false = 超时 */
bool ESP32_WaitData(uint32_t timeout_ms);

#endif /* __ESP32_H */
