#include "esp32.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;

#define ESP32_RX_BUF_SIZE 512
#define TX_BUF_SIZE 64

/* DMA 用的发送缓冲区 */
static char s_tx_buf[TX_BUF_SIZE];

/* 中断逐字节接收的暂存 */
static uint8_t s_rx_byte;

/* 接收环形缓冲（ISR 写 head，主循环读 tail） */
static uint8_t s_rx_buf[ESP32_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* 按行读取的状态 */
#define ESP32_LINE_MAX 256
static char s_line[ESP32_LINE_MAX];
static uint16_t s_line_len = 0;

/* ANSI 转义序列解析状态
 *   0 = 普通字符
 *   1 = 刚收到 ESC (0x1B)
 *   2 = 已收到 "ESC [" (CSI), 正在跳过参数直到终止字节 */
static uint8_t s_esc_state = 0;

static osSemaphoreId_t s_rx_sem = NULL;

void ESP32_Init(void) {
  s_rx_sem = osSemaphoreNew(1, 0, NULL);
  s_rx_head = 0;
  s_rx_tail = 0;
  s_line_len = 0;
  s_esc_state = 0;
  HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
}

static void tx_dma(const char *data, uint16_t n) {
  while (huart2.gState != HAL_UART_STATE_READY) {
    osDelay(1);
  }
  if (n > TX_BUF_SIZE) {
    n = TX_BUF_SIZE;
  }
  memcpy(s_tx_buf, data, n);
  HAL_UART_Transmit_DMA(&huart2, (uint8_t *)s_tx_buf, n);
}

void ESP32_SendCommand(const char *cmd) {
  char line[TX_BUF_SIZE];
  int n = snprintf(line, sizeof(line), "%s\r\n", cmd);
  if (n > 0) {
    tx_dma(line, (uint16_t)n);
  }
}

int ESP32_Available(void) {
  return (int)(uint16_t)((s_rx_head - s_rx_tail + ESP32_RX_BUF_SIZE) %
                         ESP32_RX_BUF_SIZE);
}

uint16_t ESP32_Read(uint8_t *buf, uint16_t len) {
  uint16_t n = 0;
  while (n < len && s_rx_tail != s_rx_head) {
    buf[n++] = s_rx_buf[s_rx_tail];
    s_rx_tail = (uint16_t)((s_rx_tail + 1) % ESP32_RX_BUF_SIZE);
  }
  return n;
}

void ESP32_ClearRxBuffer(void) { s_rx_tail = s_rx_head; }

/* ---- 按行读取：从环形缓冲拼出完整行，顺带过滤 ANSI 转义码 ---- */
int ESP32_GetLine(char *out, uint16_t max_len) {
  while (s_rx_tail != s_rx_head) {
    uint8_t c = s_rx_buf[s_rx_tail];
    s_rx_tail = (uint16_t)((s_rx_tail + 1) % ESP32_RX_BUF_SIZE);

    /* ---------- 过滤 ANSI 转义序列 ----------
     * 完整格式: ESC [ 参数 中间字节 终止字节
     *   ESC(0x1B)  '['(0x5B)  0x30~0x3F  0x20~0x2F  0x40~0x7E
     * 关键: '[' 本身也是 0x40~0x7E, 不能把它当成终止符,
     *       否则后面的 "32m" 会被当成正常数据拼进行里。
     */
    if (s_esc_state == 1) /* 刚收到 ESC */
    {
      s_esc_state =
          (c == '[') ? 2 : 0; /* ESC[ = CSI; 否则是双字符转义, 直接丢弃 */
      continue;
    }
    if (s_esc_state == 2) /* CSI 参数中, 一直跳到终止字节 */
    {
      if (c >= 0x40 && c <= 0x7E)
        s_esc_state = 0;
      continue;
    }
    if (c == 0x1B) {
      s_esc_state = 1;
      continue;
    }

    if (c == '\n') {
      uint16_t n = s_line_len;
      uint16_t start = 0;
      uint16_t i;

      if (n > 0 && s_line[n - 1] == '\r')
        n--;
      s_line[n] = '\0';
      s_line_len = 0;

      /* 跳过行首空白(空格/Tab)——ESP32 打印时可能带缩进或颜色码残留,
       * 不跳过会使 strncmp(line, "$W", 2) 永远不成立, 整行数据被丢掉。 */
      while (start < n && (s_line[start] == ' ' || s_line[start] == '\t'))
        start++;

      if (max_len == 0)
        return 1;
      i = 0;
      for (; start < n && i < max_len - 1; i++, start++)
        out[i] = s_line[start];
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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    uint16_t next = (uint16_t)((s_rx_head + 1) % ESP32_RX_BUF_SIZE);
    if (next != s_rx_tail) /* 缓冲满则丢弃，避免覆盖未读数据 */
    {
      s_rx_buf[s_rx_head] = s_rx_byte;
      s_rx_head = next;
    }
    HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);

    if (s_rx_sem != NULL) {
      osSemaphoreRelease(s_rx_sem);
    }
  }
}

/* 出错（如溢出）后重新挂起接收，否则会永久收不到数据 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    HAL_UART_AbortReceive(&huart2);
    HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
  }
}

bool ESP32_WaitData(uint32_t timeout_ms) {
  if (s_rx_sem == NULL) {
    return false;
  }
  return (osSemaphoreAcquire(s_rx_sem, timeout_ms) == osOK);
}
