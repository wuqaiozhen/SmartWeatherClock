#ifndef __GUI_H
#define __GUI_H

#include "font.h"
#include "st7789.h"
#include <stdint.h>
#include "font_cn.h"
#include "font_cn24.h"
#include "font_mid.h"
#include "icon_weather.h"

/* 基础原语 */
void DrawChar(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg);
void DrawString(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg);
void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void DrawBitmap(uint16_t x, uint16_t y, const uint8_t *data,
                uint16_t w, uint16_t h, uint32_t fg, uint32_t bg);
void DrawBitmapColor(uint16_t x, uint16_t y, const uint16_t *data, uint16_t w, uint16_t h);

/* 各档字号 */
void DrawCharBig(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg);  /* 24x48 ASCII, 单字 */
void DrawTextMid(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg); /* 12x24 ASCII */
void DrawTextCN(uint16_t x, uint16_t y, const uint16_t *codes, uint32_t fg, uint32_t bg); /* 16x16 中文 */
void DrawTextCN24(uint16_t x, uint16_t y, const uint16_t *codes, uint32_t fg, uint32_t bg); /* 24x24 中文 */

#endif
