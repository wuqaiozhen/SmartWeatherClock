#ifndef __FONT_SMALL_H
#define __FONT_SMALL_H

#include <stdint.h>

#define SMALL_FONT_W         8
#define SMALL_FONT_H         16
#define SMALL_BYTES_PER_CHAR ((SMALL_FONT_W/8)*SMALL_FONT_H)
#define SMALL_FIRST_CHAR     0x20
#define SMALL_LAST_CHAR      0x7E
#define SMALL_CHAR_COUNT     (SMALL_LAST_CHAR - SMALL_FIRST_CHAR + 1)

extern const uint8_t small_font[SMALL_CHAR_COUNT * SMALL_BYTES_PER_CHAR];

const uint8_t * small_font_find(char ch);

#endif
