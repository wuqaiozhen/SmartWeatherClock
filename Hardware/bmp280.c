#include "bmp280.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t t_fine;

uint8_t BMP280_ReadReg(uint8_t reg) {
  uint8_t id = 0;
  HAL_I2C_Master_Transmit(&hi2c1, BMP280_ADDR, &reg, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, BMP280_ADDR, &id, 1, 100);
  return id;
}
void BMP280_WriteReg(uint8_t reg, uint8_t val) {
  uint8_t buf[2];
  buf[0] = reg;
  buf[1] = val;
  HAL_I2C_Master_Transmit(&hi2c1, BMP280_ADDR, buf, 2, 100);
}

void BMP280_ReadCalib(void) {
  dig_T1 = (uint16_t)((BMP280_ReadReg(0x89) << 8) | BMP280_ReadReg(0x88));
  dig_T2 = (int16_t)((BMP280_ReadReg(0x8B) << 8) | BMP280_ReadReg(0x8A));
  dig_T3 = (int16_t)((BMP280_ReadReg(0x8D) << 8) | BMP280_ReadReg(0x8C));
  dig_P1 = (uint16_t)((BMP280_ReadReg(0x8F) << 8) | BMP280_ReadReg(0x8E));
  dig_P2 = (int16_t)((BMP280_ReadReg(0x91) << 8) | BMP280_ReadReg(0x90));
  dig_P3 = (int16_t)((BMP280_ReadReg(0x93) << 8) | BMP280_ReadReg(0x92));
  dig_P4 = (int16_t)((BMP280_ReadReg(0x95) << 8) | BMP280_ReadReg(0x94));
  dig_P5 = (int16_t)((BMP280_ReadReg(0x97) << 8) | BMP280_ReadReg(0x96));
  dig_P6 = (int16_t)((BMP280_ReadReg(0x99) << 8) | BMP280_ReadReg(0x98));
  dig_P7 = (int16_t)((BMP280_ReadReg(0x9B) << 8) | BMP280_ReadReg(0x9A));
  dig_P8 = (int16_t)((BMP280_ReadReg(0x9D) << 8) | BMP280_ReadReg(0x9C));
  dig_P9 = (int16_t)((BMP280_ReadReg(0x9F) << 8) | BMP280_ReadReg(0x9E));

   
}
void BMP280_ReadData(uint32_t *adc_P, uint32_t *adc_T)
{
    uint8_t P_msb = BMP280_ReadReg(0xF7);
    uint8_t P_lsb = BMP280_ReadReg(0xF8);
    uint8_t P_xlsb = BMP280_ReadReg(0xF9);

    uint8_t T_msb = BMP280_ReadReg(0xFA);
    uint8_t T_lsb = BMP280_ReadReg(0xFB);
    uint8_t T_xlsb = BMP280_ReadReg(0xFC);

    *adc_P = ( (uint32_t)(P_msb<<12|P_lsb<<4|P_xlsb>>4));
    *adc_T = ( (uint32_t)(T_msb<<12|T_lsb<<4|T_xlsb>>4));
}

 int32_t BMP280_CompensateTemp(int32_t adc_T)
   {
       int32_t var1, var2, T;

       var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
       var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)
               * ((int32_t)dig_T3)) >> 14;

       t_fine = var1 + var2;

       T = (t_fine * 5 + 128) >> 8;
       return T;   /* 单位 0.01℃，例如 2512 = 25.12℃ */
   }

    uint32_t BMP280_CompensatePress(int32_t adc_P)
   {
       int32_t var1, var2;
       uint32_t p;

       var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
       var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
       var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
       var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
       var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
               + ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
       var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);

       if (var1 == 0)
           return 0;   /* 防止除零 */

       p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;

       if (p < 0x80000000)
           p = (p << 1) / ((uint32_t)var1);
       else
           p = (p / (uint32_t)var1) * 2;

       var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
       var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;

       p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));

       return p;   /* 单位 Pa */
   }