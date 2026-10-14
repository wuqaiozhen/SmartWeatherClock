#include "gui.h"
#include "font.h"
#include "st7789.h"
#include <stdint.h>

void DrawChar(uint16_t x, uint16_t y, char ch, uint32_t fg, uint32_t bg) {
  const uint8_t *g = font_find(ch);
  uint16_t c_fg = ST7789_RGB888to565(fg);
  uint16_t c_bg = ST7789_RGB888to565(bg);
  static uint8_t buf[FONT_W * FONT_H * 2];
  int idx = 0;
  for (int row = 0; row < FONT_H; row++) {
    for (int col = 0; col < FONT_W; col++) {
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
    while(*s != '\0')
    {
        DrawChar(x, y, *s, fg, bg);
        x += FONT_W;
        s++;
    }
}