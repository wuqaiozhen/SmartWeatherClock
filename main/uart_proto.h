#ifndef __UART_PROTO_H
#define __UART_PROTO_H

#include <stdint.h>

void uart_proto_init(void);
int uart_read_line(char *out, int max_len);
void uart_send_line(const char *s);

#endif