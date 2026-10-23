#ifndef __FONT_CN_H
#define __FONT_CN_H

#include <stdint.h>

#define CN_FONT_W         16
#define CN_FONT_H         16
#define CN_BYTES_PER_CHAR ((CN_FONT_W/8)*CN_FONT_H)
#define CN_CHAR_COUNT     20

extern const uint16_t cn_font_code[CN_CHAR_COUNT];
extern const uint8_t  cn_font_data[CN_CHAR_COUNT * CN_BYTES_PER_CHAR];

/* lookup by unicode code point, NULL if not found */
const uint8_t * cn_font_find(uint16_t code);

#endif
