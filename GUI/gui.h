#ifndef __GUI_H
#define __GUI_H

#include "font.h"
#include "st7789.h"
#include <stdint.h>
#include "font_cn.h"
#include "font_small.h"

void DrawChar(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg);
void DrawString(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg);
void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void DrawBitmap(uint16_t x, uint16_t y, const uint8_t *data,
                uint16_t w, uint16_t h, uint32_t fg, uint32_t bg);
 void DrawTextBig(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg);
 void DrawTextSmall(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg);
 void DrawTextCN(uint16_t x, uint16_t y, const uint16_t*codes, uint32_t fg, uint32_t bg);
#endif
