#ifndef __FONT_BIG_H
#define __FONT_BIG_H

#include <stdint.h>

#define BIG_FONT_W         24
#define BIG_FONT_H         48
#define BIG_BYTES_PER_CHAR ((BIG_FONT_W/8)*BIG_FONT_H)
#define BIG_CHAR_COUNT     11

extern const uint8_t big_font[BIG_CHAR_COUNT * BIG_BYTES_PER_CHAR];

/* returns glyph bitmap for '0'-'9' or ':', NULL if unsupported */
const uint8_t * big_font_find(char ch);

#endif
