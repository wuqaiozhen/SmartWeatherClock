#include <uart.h>
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_uart.h"
#include <stdint.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch,1, HAL_MAX_DELAY);
    return ch;
}
void UART_SendString(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}