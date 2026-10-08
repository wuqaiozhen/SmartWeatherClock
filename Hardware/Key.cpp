/**
 ******************************************************************************
 * @file    Key.cpp
 * @brief   按键驱动实现（C++ 面向对象）
 ******************************************************************************
 */

#include "Key.hpp"

/* ============================== 一、类方法 ============================== */

Key::Key(GPIO_TypeDef* port, uint16_t pin, bool activeHigh)
    : _port(port)
    , _pin(pin)
    , _activeHigh(activeHigh)
    , _debounceMs(50)
    , _eventFlag(false)
    , _lastTick(0)
{
}

bool Key::NowPressed() const
{
    bool levelHigh = (HAL_GPIO_ReadPin(_port, _pin) == GPIO_PIN_SET);
    return _activeHigh ? levelHigh : !levelHigh;
}

bool Key::pressed()
{
    if (_eventFlag)
    {
        _eventFlag = false;   // 读后即清，一次按下只处理一次
        return true;
    }
    return false;
}

void Key::onExti(uint16_t pin)
{
    if (pin != _pin)          // 不是本按键的引脚，忽略
    {
        return;
    }

    uint32_t now = HAL_GetTick();
    if (now - _lastTick < _debounceMs)  // 软件消抖
    {
        return;
    }

    if (NowPressed())         // 确认是“按下”的电平
    {
        _lastTick  = now;
        _eventFlag = true;
    }
}

/* ==================== 二、静态对象：PA0 上的按键 ==================== */
/* 与 MX_GPIO_Init() 一致：PA0 = IT_RISING + PULLDOWN，按下为高 */
static Key g_key(GPIOA, GPIO_PIN_0, true);

/* ==================== 三、给 C（main.c）的包装函数 ==================== */

extern "C" void KEY_EXTI_Handler(uint16_t GPIO_Pin)
{
    g_key.onExti(GPIO_Pin);
}

extern "C" uint8_t KEY_NowPressed(void)
{
    return g_key.NowPressed() ? 1u : 0u;
}

extern "C" uint8_t KEY_Pressed(void)
{
    return g_key.pressed() ? 1u : 0u;
}
