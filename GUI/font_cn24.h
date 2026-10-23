#ifndef __CN24
#define __CN24

#include <stdint.h>

/* CN 24x24, 41 chars (incl. weather words) */
#define CN24_W  24
#define CN24_H  24
#define CN24_BPC  ((CN24_W+7)/8*CN24_H)
#define CN24_COUNT  41

extern const uint16_t font_cn24_code[CN24_COUNT];
extern const uint8_t  font_cn24_data[CN24_COUNT * CN24_BPC];

const uint8_t *font_cn24_find(uint16_t code);

#endif
