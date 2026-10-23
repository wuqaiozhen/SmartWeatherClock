#ifndef __MID
#define __MID

#include <stdint.h>

/* ASCII 0x20-0x7E, 12x24, Arial Narrow */
#define MID_W  12
#define MID_H  24
#define MID_BPC  ((MID_W+7)/8*MID_H)
#define MID_COUNT  95

extern const uint16_t font_mid_code[MID_COUNT];
extern const uint8_t  font_mid_data[MID_COUNT * MID_BPC];

const uint8_t *font_mid_find(uint16_t code);

#endif
