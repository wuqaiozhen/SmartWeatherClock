#include "aht20.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;

uint8_t AHT20_ReadStatus(void) {
  uint8_t status = 0;
  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, &status, 1, 100);
  return status;
}

uint8_t AHT20_Read(float *temp, float *humi) {
  uint8_t cmd[3] = {0xAC, 0x33, 0x00};
  uint8_t data[7];
  if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, cmd, 3, 100) != HAL_OK)
    return 1;
  HAL_Delay(80);

  uint32_t start = HAL_GetTick();
  while (AHT20_ReadStatus() & 0x80) {
    if (HAL_GetTick() - start > 200)
      return 1;
    HAL_Delay(1);
  }

  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, data, 7, 100) != HAL_OK)
    return 1;
  if (data[0] & 0x80)
    return 1;

  uint32_t hum_raw = (data[1] << 12 | data[2] << 4 | data[3] >> 4);
  uint32_t temp_raw = ((data[3] & 0x0F) << 16 | data[4] << 8 | data[5]);

  *humi = (float)hum_raw / 1048576.0f * 100.0f;          /* %RH */
  *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f; /* ℃  */

  return 0;
}