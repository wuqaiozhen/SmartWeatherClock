#include "uart_proto.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

#define UART_PORT UART_NUM_0
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define RX_BUF_SIZE 1024
#define LINE_BUF_SIZE 128

static const char *TAG = "uart_proto";
static char s_line[LINE_BUF_SIZE];
static int s_line_len = 0;

/* ---- 两级缓冲 ----
 * 第 1 级: ESP-IDF 驱动内部的环形缓冲 (RX_BUF_SIZE = 1024 字节)
 * 第 2 级: 下面这两个变量 —— "已经读出来、但还没处理完"的字节区
 *
 * 为什么必须有第 2 级:
 *   一次 uart_read_bytes 可能读到**好几条完整命令行**
 *   (比如 STM32 开机时背靠背发 "GET_WEATHER\r\nGET_TIME\r\n")。
 *   如果处理完第一条就 return, 剩下的字节存在栈上的临时 buf 里,
 *   函数一返回就没了 -> 第二条命令永久丢失。
 *   所以必须把它们留在 static 区, 下次调用继续消费。 */
static uint8_t s_rx[64];        /* 本批从驱动读到的字节 (必须 static, 不能放栈上!) */
static int     s_rx_len = 0;    /* 本批一共有多少字节 */
static int     s_rx_pos = 0;    /* 已经消费到本批的哪个位置 */

void uart_proto_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_0, &cfg);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, RX_BUF_SIZE, 0, 0, NULL, 0);
    ESP_LOGI(TAG, "uart ready");
}

/* 从串口拼出一整行（以 '\n' 结尾，自动去掉 '\r'）
 * 返回 1 = 拿到一行(已写进 out)；0 = 暂时没有完整的一行
 *
 * 外层 for(;;) 的意义:
 *   内层 while 先把"上一批剩下的字节"消费完;
 *   用完了才向驱动要新的一批 —— 这样一批里的多条命令都能处理到 */
int uart_read_line(char *out, int max_len)
{
    int n;

    if (out == NULL || max_len <= 0) {
        return 0;
    }

    for (;;) {
        /* ---- 1) 先消费上次没处理完的字节 ---- */
        while (s_rx_pos < s_rx_len) {
            char c = (char)s_rx[s_rx_pos++];

            if (c == '\n') {
                int len = s_line_len;
                if (len > 0 && s_line[len - 1] == '\r') {
                    len--;                  /* 去掉 '\r' */
                }
                s_line[len] = '\0';
                s_line_len = 0;             /* 行缓冲清零 (为下一行做准备) */

                strncpy(out, s_line, max_len - 1);
                out[max_len - 1] = '\0';
                return 1;                   /* 本批剩下的字节还在 s_rx 里, 下次接着用 */
            }

            if (s_line_len < LINE_BUF_SIZE - 1) {
                s_line[s_line_len++] = c;
            }
            /* 超长则丢弃该行多余字符 */
        }

        /* ---- 2) 本批用完了, 再向驱动要一批 ---- */
        n = uart_read_bytes(UART_PORT, s_rx, sizeof(s_rx), pdMS_TO_TICKS(20));
        if (n <= 0) {
            return 0;                       /* 没有新数据, 让调用者先去干别的 */
        }
        s_rx_len = n;
        s_rx_pos = 0;
    }
}

void uart_send_line(const char *s)
{
  char buf[LINE_BUF_SIZE + 3];

  int n = snprintf(buf, sizeof(buf),"%s\r\n", s);
  if(n <= 0){
    return ;
  }
  if(n > (int)sizeof(buf)-1){
    n = (int)sizeof(buf)-1 ;
  }

  uart_write_bytes(UART_PORT, buf, n);
}
