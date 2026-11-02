#include "st7789.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2;

static void ST7789_CS(uint8_t level) {
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ST7789_DC(uint8_t level) {
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ST7789_WriteByte(uint8_t data) {
  HAL_SPI_Transmit(&hspi2, &data, 1, 100);
}

void ST7789_WriteCommand(uint8_t cmd) {
  ST7789_CS(0);
  ST7789_DC(0);
  ST7789_WriteByte(cmd);
}

void ST7789_WriteData(uint8_t data) {
  ST7789_CS(0);
  ST7789_DC(1);
  ST7789_WriteByte(data);
}

void ST7789_Init(void) {
  /* ---- 1) 硬件复位 ----
   * 拉低 20ms 足够让屏幕放电 (原来给 1000ms 只是白等开机时间)。
   * 拉高后按 ST7789 手册必须等 120ms 才能发命令。 */
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120);

  /* ---- 2) 软件复位 (SWRESET) ----
   * 只靠 RST 引脚复位时, "按复位键快速重启"的屏幕可能停在休眠/异常状态,
   * 现象就是"只有背光、没有图像"。这里再发一次 SWRESET, 让芯片内部状态
   * 彻底清零, 不再依赖外部复位电平是否被屏幕正确识别。 */
  ST7789_WriteCommand(0x01); /* SWRESET */
  HAL_Delay(150);            /* 手册: 软复位后需等 120ms 以上 */

  /* ---- 3) 唤醒 (Sleep Out) ---- */
  ST7789_WriteCommand(0x11);
  HAL_Delay(120); // Sleep Out

  ST7789_WriteCommand(0x2A);
  ST7789_WriteData(0x00);
  ST7789_WriteData(0x00);
  ST7789_WriteData(0x00);
  ST7789_WriteData(0xEF);
  ST7789_WriteCommand(0x2B);
  ST7789_WriteData(0x00);
  ST7789_WriteData(0x28);
  ST7789_WriteData(0x01);
  ST7789_WriteData(0x17);

  ST7789_WriteCommand(0xB2);
  ST7789_WriteData(0x0C);
  ST7789_WriteData(0x0C);
  ST7789_WriteData(0x00);
  ST7789_WriteData(0x33);
  ST7789_WriteData(0x33);

  ST7789_WriteCommand(0x20); // 反色关
  ST7789_WriteCommand(0xB7);
  ST7789_WriteData(0x56);
  ST7789_WriteCommand(0xBB);
  ST7789_WriteData(0x18);
  ST7789_WriteCommand(0xC0);
  ST7789_WriteData(0x2C);
  ST7789_WriteCommand(0xC2);
  ST7789_WriteData(0x01);
  ST7789_WriteCommand(0xC3);
  ST7789_WriteData(0x1F);
  ST7789_WriteCommand(0xC4);
  ST7789_WriteData(0x20);
  ST7789_WriteCommand(0xC6);
  ST7789_WriteData(0x0F);
  ST7789_WriteCommand(0xD0);
  ST7789_WriteData(0xA6);
  ST7789_WriteData(0xA1);

  ST7789_WriteCommand(0xE0);
  {
    const uint8_t g1[14] = {0xD0, 0x0D, 0x14, 0x0B, 0x0B, 0x07, 0x3A,
                            0x44, 0x50, 0x08, 0x13, 0x13, 0x2D, 0x32};
    for (int i = 0; i < 14; i++)
      ST7789_WriteData(g1[i]);
  }
  ST7789_WriteCommand(0xE1);
  {
    const uint8_t g2[14] = {0xD0, 0x0D, 0x14, 0x0B, 0x0B, 0x07, 0x3A,
                            0x44, 0x50, 0x08, 0x13, 0x13, 0x2D, 0x32};
    for (int i = 0; i < 14; i++)
      ST7789_WriteData(g2[i]);
  }

  ST7789_WriteCommand(0x36);
  ST7789_WriteData(0x00); // MADCTL
  ST7789_WriteCommand(0x3A);
  ST7789_WriteData(0x55); // 16bit RGB565
  ST7789_WriteCommand(0xE7);
  ST7789_WriteData(0x00); // 厂家有这条

  ST7789_WriteCommand(0x21); // 反色开
  ST7789_WriteCommand(0x29); // 显示开
  ST7789_WriteCommand(0x2C); // 内存写
}
void ST7789_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  /*---列---*/
  ST7789_WriteCommand(0x2A);
  ST7789_WriteData(x0 >> 8);
  ST7789_WriteData(x0 & 0xff);
  ST7789_WriteData(x1 >> 8);
  ST7789_WriteData(x1 & 0xff);

  /*--- 行---*/
  ST7789_WriteCommand(0x2B);
  ST7789_WriteData(y0 >> 8);
  ST7789_WriteData(y0 & 0xff);
  ST7789_WriteData(y1 >> 8);
  ST7789_WriteData(y1 & 0xff);
}

void ST7789_WriteDataBulk(uint8_t *buf, uint16_t len) {
  ST7789_DC(1);
  HAL_SPI_Transmit(&hspi2, buf, len, 1000);
}

uint16_t ST7789_RGB888to565(
    uint32_t color) { /* color 用 RGB888（如 红=0xFF0000）；内部转成 RGB565
                         发送（16bit 模式） */
  return (uint16_t) (((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) |
                           ((color >> 3) & 0x001F));
}

void ST7789_FillColor(uint32_t color) {
  static uint8_t buf[240 * 2];
  uint16_t c565 = ST7789_RGB888to565(color);
  for (int i = 0; i < 240 * 2; i += 2) {
    buf[i] = c565 >> 8;
    buf[i + 1] = c565 & 0xff;
  }
  ST7789_SetWindow(0, 0, 239, 319);
  ST7789_WriteCommand(0x2C);
  for (int row = 0; row < 320; row++)
    ST7789_WriteDataBulk(buf, 240 * 2);
}
