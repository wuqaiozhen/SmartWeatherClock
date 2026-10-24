#ifndef __CN
#define __CN

#include <stdint.h>

/* CN 16x16, 22 chars (incl. degC) */
#define CN_W  16
#define CN_H  16
#define CN_BPC  ((CN_W+7)/8*CN_H)
#define CN_COUNT  22

extern const uint16_t font_cn_code[CN_COUNT];
extern const uint8_t  font_cn_data[CN_COUNT * CN_BPC];

const uint8_t *cn_font_find(uint16_t code);

#endif
