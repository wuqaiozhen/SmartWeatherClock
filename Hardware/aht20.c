#include "aht20.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define AHT20_OK 0
#define AHT20_E_TX 1
#define AHT20_E_RX 2
#define AHT20_E_BUSY 3

extern I2C_HandleTypeDef hi2c1;

static osSemaphoreId_t s_i2c_sem = NULL; /* I2C 传输完成信号量 */
static volatile bool s_i2c_ok = false;   /* 本次传输是否成功 */

void AHT20_Init(void) {
  if (s_i2c_sem == NULL) { /* 幂等: 重复调用不会重复创建 */
    s_i2c_sem = osSemaphoreNew(1, 0, NULL);
  }
}

static bool i2c_tx(const uint8_t *buf, uint16_t len, uint32_t timeout_ms) {
  s_i2c_ok = false;
  if (HAL_I2C_Master_Transmit_IT(&hi2c1, AHT20_ADDR, (uint8_t *)buf, len) !=
      HAL_OK)
    return false;
  if (osSemaphoreAcquire(s_i2c_sem, timeout_ms) != osOK)
    return false;
  return s_i2c_ok;
}

static bool i2c_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms) {
  s_i2c_ok = false;
  if (HAL_I2C_Master_Receive_IT(&hi2c1, AHT20_ADDR, (uint8_t *)buf, len) !=
      HAL_OK)
    return false;
  if (osSemaphoreAcquire(s_i2c_sem, timeout_ms) != osOK)
    return false;
  return s_i2c_ok;
}

uint8_t AHT20_Read(float *temp, float *humi) {
  uint8_t cmd[3] = {0xAC, 0x33, 0x00};
  uint8_t data[7];
  /*  发测量命令(中断式) */
  if (!i2c_tx(cmd, 3, 100))
    return AHT20_E_TX;
  osDelay(80);
  /*  等忙标志清除(中断式读状态) */
  uint32_t start = HAL_GetTick();
  uint8_t st;
  for (;;) {
    if (!i2c_rx(&st, 1, 100))
      return AHT20_E_RX;
    if ((st & 0x80) == 0)
      break;
    if (HAL_GetTick() - start > 200)
      return AHT20_E_BUSY;
    osDelay(5);
  }
  /*  读 7 字节(中断式) */
  if (!i2c_rx(data, 7, 100))
    return AHT20_E_RX;
  if (data[0] & 0x80)
    return AHT20_E_BUSY;
  uint32_t hum_raw = (data[1] << 12 | data[2] << 4 | data[3] >> 4);
  uint32_t temp_raw = ((data[3] & 0x0F) << 16 | data[4] << 8 | data[5]);

  *humi = (float)hum_raw / 1048576.0f * 100.0f;          /* %RH */
  *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f; /* ℃  */

  return 0;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    s_i2c_ok = true;
    osSemaphoreRelease(s_i2c_sem);
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    s_i2c_ok = true;
    osSemaphoreRelease(s_i2c_sem);
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    s_i2c_ok = false;
    osSemaphoreRelease(s_i2c_sem);
  }
}
