#ifndef __CN32
#define __CN32

#include <stdint.h>

#define CN32_W         32
#define CN32_H         32
#define CN32_BYTES_PER_CHAR ((CN32_W/8)*CN32_H)
#define CN32_CHAR_COUNT     21

extern const uint16_t cn32_code[CN32_CHAR_COUNT];
extern const uint8_t  cn32_data[CN32_CHAR_COUNT * CN32_BYTES_PER_CHAR];

const uint8_t * cn32_font_find(uint16_t code);

#endif
