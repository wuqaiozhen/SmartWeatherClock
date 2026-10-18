#include "uart_proto.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define UART_PORT UART_NUM_0
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define RX_BUF_SIZE 1024
#define LINE_BUF_SIZE 128

static const char *TAG = "uart_proto";
static char s_line[LINE_BUF_SIZE];
static int s_line_len = 0;


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

int uart_read_line(char *out, int max_len)
{
    uint8_t buf[64];
    int n = uart_read_bytes(UART_PORT, buf, sizeof(buf), 0);
    if(n<=0){
        return 0;
    }
    for(int i = 0;i < n; i++){
        char c = buf[i];
        if(c=='\n'){
            int len = s_line_len;
            if(len>0 && s_line[len-1] == '\r'){
                len--;
            }
            s_line[len] = '\0';
            strncpy(out, s_line, max_len-1);
            out[max_len-1]='\0';
            s_line_len = 0;
            return 1;
        }
        if (s_line_len < LINE_BUF_SIZE-1){
            s_line[s_line_len] = c;
            s_line_len++;
        }
    }
    return 0;
}

void uart_send_line(const char *s)
{
  uart_write_bytes(UART_PORT, s, strlen(s));
  uart_write_bytes(UART_PORT,"\r\n", 2);
}
            