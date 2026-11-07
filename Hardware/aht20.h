#ifndef __AHT20_H
#define __AHT20_H

#include "main.h"
#include <stdint.h>
#define AHT20_ADDR (0x38 << 1)

void AHT20_Init(void);
uint8_t AHT20_Read (float *temp, float *humi);
#endif