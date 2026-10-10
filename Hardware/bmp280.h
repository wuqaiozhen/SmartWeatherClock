#ifndef __BMP280_H
#define __BMP280_H

#include "main.h"
#include <stdint.h>

#define BMP280_ADDR (0x77 << 1)
#define BMP280_REG_ID 0xD0

uint8_t BMP280_ReadReg(uint8_t reg);
void BMP280_WriteReg(uint8_t reg, uint8_t val);
void BMP280_ReadCalib(void);
void BMP280_ReadData(uint32_t *adc_P, uint32_t *adc_T);
int32_t BMP280_CompensateTemp(int32_t adc_T);
uint32_t BMP280_CompensatePress(int32_t adc_P);

#endif