/**
 ******************************************************************************
 * @file    Key.hpp
 * @brief   按键驱动（C++ 面向对象封装）
 *          - 类 Key 封装一个按键：电平读取、软件消抖、按下事件
 *          - 对外通过 extern "C" 接口暴露给 C 代码（main.c）使用
 ******************************************************************************
 */
#ifndef __KEY_HPP__
#define __KEY_HPP__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 给 C（main.c）使用的接口 ---- */

/** @brief 按键 EXTI 中断分发：在 HAL_GPIO_EXTI_Callback() 里调用 */
void KEY_EXTI_Handler(uint16_t GPIO_Pin);

/** @brief 读取按键当前电平，1=按下，0=松开 */
uint8_t KEY_NowPressed(void);

/** @brief 读取并清除“发生了一次有效按下”的事件标志 */
uint8_t KEY_Pressed(void);

#ifdef __cplusplus
}
#endif

/* ---- C++ 类定义（只给 .cpp 看） ---- */
#ifdef __cplusplus

class Key {
public:
    Key(GPIO_TypeDef* port, uint16_t pin, bool activeHigh = true);

    void setDebounceMs(uint32_t ms) { _debounceMs = ms; }

    bool NowPressed() const;   // 读电平
    bool pressed();            // 读并清除事件
    void onExti(uint16_t pin); // 中断入口

private:
    GPIO_TypeDef* _port;
    uint16_t      _pin;
    bool          _activeHigh;
    uint32_t      _debounceMs;
    volatile bool     _eventFlag;  // ISR 置位，主循环读取清除
    volatile uint32_t _lastTick;   // 上次有效按下时刻，用于消抖
};

#endif  /* __cplusplus */

#endif  /* __KEY_HPP__ */
