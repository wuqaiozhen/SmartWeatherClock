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

  static uint8_t buf[64 * 64 * 2];
  int idx = 0;
  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {

      int byteidx = row * ((w + 7) / 8) + col / 8;  /* ??? 8 ??(?12)?????? */
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
      DrawCharBig(x, y, *s, fg, bg);  
           x += 24 + 4;                      
           s++;
  }
 }

  void DrawCharBig(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg){
    const uint8_t *glyph = big_font_find(ch);
        if (glyph == NULL) return;      
         DrawBitmap(x, y, glyph, 24, 48, fg, bg);
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
 /* 32x32 ???: ?????? */
 void DrawTextCN32(uint16_t x, uint16_t y, const uint16_t *codes, uint32_t fg, uint32_t bg){
  while(*codes != 0){
     const uint8_t *glyph = cn32_font_find(*codes);
     if( glyph == NULL){
       x += 32;
       codes++;
       continue;
     }
     DrawBitmap(x, y, glyph, 32, 32, fg, bg);
     x += 32;
     codes++;
  }
 }
 /* 12x24 ASCII: ??????? */
 void DrawTextMid(uint16_t x, uint16_t y, const char *s, uint32_t fg, uint32_t bg){
  while(*s != 0){
     const uint8_t *glyph = font_mid_find((uint8_t)*s);
     if( glyph == NULL){
       x += 12;
       s++;
       continue;
     }
     DrawBitmap(x, y, glyph, 12, 24, fg, bg);
     x += 12;
     s++;
  }
 }

 /* 24x24 ??? */
 void DrawTextCN24(uint16_t x, uint16_t y, const uint16_t *codes, uint32_t fg, uint32_t bg){
  while(*codes != 0){
     const uint8_t *glyph = font_cn24_find(*codes);
     if( glyph == NULL){
       x += 24;
       codes++;
       continue;
     }
     DrawBitmap(x, y, glyph, 24, 24, fg, bg);
     x += 24;
     codes++;
  }
 }
 /* ????: ??? 2 ?? RGB565 (?????, ??? SPI) */
 void DrawBitmapColor(uint16_t x, uint16_t y, const uint16_t *data, uint16_t w, uint16_t h){
  if(data == 0 || w == 0 || h == 0) return;
  if((uint32_t)w * h > 64u * 64u) return;          /* ??????? */

  static uint8_t buf[64 * 64 * 2];
  uint32_t n = (uint32_t)w * h;
  for(uint32_t i = 0; i < n; i++){
     buf[i * 2]     = (uint8_t)(data[i] >> 8);
     buf[i * 2 + 1] = (uint8_t)(data[i] & 0xFF);
  }
  ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
  ST7789_WriteCommand(0x2C);
  ST7789_WriteDataBulk(buf, n * 2);
 }