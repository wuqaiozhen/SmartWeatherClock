#ifndef __LED_HPP__
#define __LED_HPP__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 给 C（main.c）使用的接口 ---- */
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);

#ifdef __cplusplus
}
#endif

/* ---- C++ 类定义（只给 .cpp 看） ---- */
#ifdef __cplusplus

class Led {
public:
    Led(GPIO_TypeDef* port, uint16_t pin);

    void on();
    void off();
    void toggle();

private:
    GPIO_TypeDef* _port;
    uint16_t      _pin;
};

#endif  /* __cplusplus */

#endif  /* __LED_HPP__ */
