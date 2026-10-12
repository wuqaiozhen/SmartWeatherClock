#include "st7789.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2;

static void ST7789_CS(uint8_t level)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,  TFT_CS_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ST7789_DC(uint8_t level)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,  TFT_DC_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ST7789_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi2, &data, 1, 100);
}

void ST7789_WriteCommand(uint8_t cmd)
{
    ST7789_CS(0);
    ST7789_DC(0);
    ST7789_WriteByte(cmd);
}

void ST7789_WriteData(uint8_t data)
{
    ST7789_CS(0);
    ST7789_DC(1);
    ST7789_WriteByte(data);
}
void ST7789_Reset(void)
{
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120);
}

void ST7789_Init(void)
{
    ST7789_Reset();
    ST7789_WriteCommand(0x01);
    HAL_Delay(150);

    ST7789_WriteCommand(0X11);
    HAL_Delay(120);

    ST7789_WriteCommand(0X3A);
    ST7789_WriteData(0X55);

    ST7789_WriteCommand(0X36);
    ST7789_WriteData(0X00);

    ST7789_WriteCommand(0XB2);
    ST7789_WriteData(0X0C);
    ST7789_WriteData(0X0C);
    ST7789_WriteData(0X00);
    ST7789_WriteData(0X33);
    ST7789_WriteData(0X33);

    ST7789_WriteCommand(0XB7);
    ST7789_WriteData(0X35);

    ST7789_WriteCommand(0XBB);
    ST7789_WriteData(0X19);

    ST7789_WriteCommand(0XC0);
    ST7789_WriteData(0X2C);
    
    ST7789_WriteCommand(0XC2);
    ST7789_WriteData(0X01);

    ST7789_WriteCommand(0XC3);
    ST7789_WriteData(0X12);

    ST7789_WriteCommand(0XC4);
    ST7789_WriteData(0X20);

    ST7789_WriteCommand(0XC6);
    ST7789_WriteData(0X0F);

    ST7789_WriteCommand(0XD0);
    ST7789_WriteData(0XA4);
    ST7789_WriteData(0XA1);

    ST7789_WriteCommand(0X21);

    ST7789_WriteCommand(0XE0);
    ST7789_WriteData(0XD0);
    ST7789_WriteData(0X04);
    ST7789_WriteData(0X0D);
    ST7789_WriteData(0X11);
    ST7789_WriteData(0X13);
    ST7789_WriteData(0X2B);
    ST7789_WriteData(0X3F);
    ST7789_WriteData(0X54);
    ST7789_WriteData(0X4C);
    ST7789_WriteData(0X18);
    ST7789_WriteData(0X0D);
    ST7789_WriteData(0X0B);
    ST7789_WriteData(0X1F);
    ST7789_WriteData(0X23);

    ST7789_WriteCommand(0XE1);
    ST7789_WriteData(0XD0);
    ST7789_WriteData(0X04);
    ST7789_WriteData(0X0C);
    ST7789_WriteData(0X11);
    ST7789_WriteData(0X13);
    ST7789_WriteData(0X2C);
    ST7789_WriteData(0X3F);
    ST7789_WriteData(0X44);
    ST7789_WriteData(0X51);
    ST7789_WriteData(0X2F);
    ST7789_WriteData(0X1F);
    ST7789_WriteData(0X1F);
    ST7789_WriteData(0X20);
    ST7789_WriteData(0X23);

    ST7789_WriteCommand(0X29);
}

void ST7789_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /*---列---*/
    ST7789_WriteCommand(0x2A);
    ST7789_WriteData(x0>>8);
    ST7789_WriteData(x0&0xff);
    ST7789_WriteData(x1>>8);
    ST7789_WriteData(x1&0xff);

    /*--- 行---*/
    ST7789_WriteCommand(0x2B);
    ST7789_WriteData(y0>>8);
    ST7789_WriteData(y0 &0xff);
    ST7789_WriteData(y1>>8);
    ST7789_WriteData(y1&0xff);
}   

void ST7789_WriteDataBulk(uint8_t *buf, uint16_t len)
{
    ST7789_DC(1);
    HAL_SPI_Transmit(&hspi2, buf, len, 1000);
}

void ST7789_FillColor(uint16_t color)
{
    uint8_t buf[256];
    uint8_t color_high;
    uint8_t color_low;
    ST7789_SetWindow(0, 0, 239, 319);
    ST7789_WriteCommand(0x2c);
    color_high = color >>8;
    color_low = color & 0xff;
    for(int i=0;i<256;i+=2)
    {
        buf[i] = color_high;
        buf[i+1] = color_low;

    }

    uint32_t total = 240*320*2;
    uint32_t data_send = 0;
    while(data_send < total)
    {
        uint16_t data_now = 256;
        if(total - data_send < 256)
    {
        data_now = total-data_send;
    }
        ST7789_WriteDataBulk(buf, data_now);
        data_send += data_now;
    
    }
}

