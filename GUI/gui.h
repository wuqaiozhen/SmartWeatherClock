#ifndef __GUI_H
#define __GUI_H

#include "font.h"
#include "st7789.h"
#include <stdint.h>

void DrawChar(uint16_t x,uint16_t y,char ch, uint32_t fg, uint32_t bg);
void DrawString(uint16_t x,uint16_t y,const char *s, uint32_t fg, uint32_t bg);
#endif
