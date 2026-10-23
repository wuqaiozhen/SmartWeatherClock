#include "gui.h"
#include "font.h"
#include "st7789.h"
#include <stdint.h>
#include "font_big.h"
#include "font_small.h"
#include "font_cn.h"

#define LCD_W 240
#define LCD_H 320

void DrawChar(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg)
{
  const uint8_t *g = font_find(ch);
  uint16_t c_fg = ST7789_RGB888to565(fg);
  uint16_t c_bg = ST7789_RGB888to565(bg);
  static uint8_t buf[FONT_W * FONT_H * 2];
  int idx = 0;
  for (int row = 0; row < FONT_H; row++)
  {
    for (int col = 0; col < FONT_W; col++)
    {
      int byteidx = (row * (FONT_W / 8) + col / 8);
      int bit = (7 - (col % 8));
      uint16_t color;
      color = (g[byteidx] & (1 << bit)) ? c_fg : c_bg;
      buf[idx] = color >> 8;
      buf[idx + 1] = color & 0xff;
      idx += 2;
    }
  }
  ST7789_SetWindow(x, y, x + FONT_W - 1, y + FONT_H - 1);
  ST7789_WriteCommand(0x2C);
  ST7789_WriteDataBulk(buf, FONT_W * FONT_H * 2);
}

void DrawString(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg)
{
  while (*s != '\0')
  {
    DrawChar(x, y, *s, fg, bg);
    x += FONT_W;
    s++;
  }
}

void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
  if (w == 0 || h == 0)
    return;
  if (x >= LCD_W || y >= LCD_H)
    return;
  if ((x + w) > LCD_W)
  {
    w = LCD_W - x;
  }
  if ((y + h) > LCD_H)
  {
    h = LCD_H - y;
  }

  uint16_t c565 = ST7789_RGB888to565(color);
  ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
  ST7789_WriteCommand(0x2C);

  static uint8_t line[LCD_W * 2];
  int idx = 0;
  for (int i = 0; i < w; i++)
  {
    line[idx] = c565 >> 8;
    line[idx + 1] = c565 & 0xff;
    idx += 2;
  }

  for (int r = 0; r < h; r++)
  {
    ST7789_WriteDataBulk(line, w * 2);
  }
}

void DrawBitmap(uint16_t x, uint16_t y, const uint8_t *data,
                uint16_t w, uint16_t h, uint32_t fg, uint32_t bg)
{
  if (w == 0 || h == 0 || data == 0)
    return;

  uint16_t c_fg = ST7789_RGB888to565(fg);
  uint16_t c_bg = ST7789_RGB888to565(bg);

  static uint8_t buf[48 * 48 * 2];
  int idx = 0;
  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {

      int byteidx = row * (w / 8) + col / 8;
      int bit = 7 - (col % 8);

      uint16_t color = (data[byteidx] & (1 << bit)) ? c_fg : c_bg;

      buf[idx] = color >> 8;
      buf[idx + 1] = color & 0xFF;
      idx += 2;
    }
  }
   ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
   ST7789_WriteCommand(0x2C);
    ST7789_WriteDataBulk(buf, w * h * 2);
}

 void DrawTextBig(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg){
  while(*s != 0){
     const uint8_t *  glyph = big_font_find(*s);
     if( glyph == NULL){
       x += 28;      
       s++;
       continue;
     }
     DrawBitmap(x, y, glyph, 24, 48, fg, bg);
     x += 24+4;
     s++;
  }
 }

 void DrawTextSmall(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg){
  while(*s != 0){
     const uint8_t *  glyph = small_font_find(*s);
     if( glyph == NULL){
       x += 8;      
       s++;
       continue;
     }
     DrawBitmap(x, y, glyph, 8, 16, fg, bg);
     x += 8;
     s++;
  }
 }

 void DrawTextCN(uint16_t x, uint16_t y, const uint16_t*codes, uint32_t fg, uint32_t bg){
  while(*codes != 0){
     const uint8_t *  glyph = cn_font_find(*codes);
     if( glyph == NULL){
       x += 16;      
       codes++;
       continue;
     }
     DrawBitmap(x, y, glyph, 16, 16, fg, bg);
     x += 16;
     codes++;
  }
 }