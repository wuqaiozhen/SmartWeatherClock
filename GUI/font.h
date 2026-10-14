#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>

#define FONT_FIRST_CHAR  0X20
#define FONT_LAST_CHAR   0X7E
#define FONT_W           16
#define FONT_H           32
#define FONT_BYTES_PER_CHAR ((FONT_W /8)*FONT_H) 
#define FONT_CHAR_COUNT  (FONT_LAST_CHAR - FONT_FIRST_CHAR +1)

extern const uint8_t ascii_font [FONT_CHAR_COUNT * FONT_BYTES_PER_CHAR];
const uint8_t * font_find (char ch);

#endif