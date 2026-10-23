#ifndef __CN
#define __CN

#include <stdint.h>

#define CN_W         16
#define CN_H         16
#define CN_BYTES_PER_CHAR ((CN_W/8)*CN_H)
#define CN_CHAR_COUNT     21

extern const uint16_t cn_code[CN_CHAR_COUNT];
extern const uint8_t  cn_data[CN_CHAR_COUNT * CN_BYTES_PER_CHAR];

const uint8_t * cn_font_find(uint16_t code);

#endif
