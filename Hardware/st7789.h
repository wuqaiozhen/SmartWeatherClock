#ifndef __ST7789_H
#define __ST7789_H

#include "main.h"
#include "st7789.h"
#include <stdint.h>


void ST7789_WriteCommand(uint8_t cmd);
void ST7789_WriteData(uint8_t data);
void ST7789_Reset(void);
void ST7789_Init(void);
void ST7789_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7789_WriteDataBulk(uint8_t *buf, uint16_t len);
void ST7789_FillColor(uint32_t color);

#endif