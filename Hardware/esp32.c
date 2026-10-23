#include "esp32.h"
#include "stm32f4xx_hal_uart.h"
#include <string.h>

extern UART_HandleTypeDef huart2;

#define ESP32_RX_BUF_SIZE 512

/* 中断逐字节接收的暂存 */
static uint8_t s_rx_byte;

/* 接收环形缓冲（ISR 写 head，主循环读 tail） */
static uint8_t s_rx_buf[ESP32_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* 按行读取的状态 */
#define ESP32_LINE_MAX 256
static char     s_line[ESP32_LINE_MAX];
static uint16_t s_line_len = 0;
static uint8_t  s_in_esc   = 0;   /* 是否正处于 ANSI 转义序列中 */

void ESP32_Init(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    s_line_len = 0;
    s_in_esc = 0;
    HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
}

void ESP32_SendString(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

void ESP32_SendCommand(const char *cmd)
{
    ESP32_SendString(cmd);
    ESP32_SendString("\r\n");
}

int ESP32_Available(void)
{
    return (int)(uint16_t)((s_rx_head - s_rx_tail + ESP32_RX_BUF_SIZE) % ESP32_RX_BUF_SIZE);
}

uint16_t ESP32_Read(uint8_t *buf, uint16_t len)
{
    uint16_t n = 0;
    while (n < len && s_rx_tail != s_rx_head)
    {
        buf[n++] = s_rx_buf[s_rx_tail];
        s_rx_tail = (uint16_t)((s_rx_tail + 1) % ESP32_RX_BUF_SIZE);
    }
    return n;
}

void ESP32_ClearRxBuffer(void)
{
    s_rx_tail = s_rx_head;
}

/* ---- 按行读取：从环形缓冲拼出完整行，顺带过滤 ANSI 转义码 ---- */
int ESP32_GetLine(char *out, uint16_t max_len)
{
    while (s_rx_tail != s_rx_head)
    {
        uint8_t c = s_rx_buf[s_rx_tail];
        s_rx_tail = (uint16_t)((s_rx_tail + 1) % ESP32_RX_BUF_SIZE);

        /* 过滤 ANSI 转义序列：ESC ... 到 0x40~0x7E 结束 */
        if (s_in_esc)
        {
            if (c >= 0x40 && c <= 0x7E) s_in_esc = 0;
            continue;
        }
        if (c == 0x1B) { s_in_esc = 1; continue; }

        if (c == '\n')
        {
            uint16_t n = s_line_len;
            if (n > 0 && s_line[n - 1] == '\r') n--;
            s_line[n] = '\0';
            s_line_len = 0;

            if (max_len == 0) return 1;
            uint16_t i = 0;
            for (; i < n && i < max_len - 1; i++) out[i] = s_line[i];
            out[i] = '\0';
            return 1;
        }

        if (s_line_len < ESP32_LINE_MAX - 1)
            s_line[s_line_len++] = (char)c;
        /* 超出则丢弃该行多余字符 */
    }
    return 0;
}

/* 每收到 1 字节被 HAL 调用（弱函数覆盖） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uint16_t next = (uint16_t)((s_rx_head + 1) % ESP32_RX_BUF_SIZE);
        if (next != s_rx_tail)   /* 缓冲满则丢弃，避免覆盖未读数据 */
        {
            s_rx_buf[s_rx_head] = s_rx_byte;
            s_rx_head = next;
        }
        HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}

/* 出错（如溢出）后重新挂起接收，否则会永久收不到数据 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        HAL_UART_AbortReceive(&huart2);
        HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}
