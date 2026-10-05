/**
 ******************************************************************************
 * @file    Led.cpp
 * @brief   LED 驱动实现（C++ 面向对象）
 *          - 类 Led 负责封装单个 LED 的操作
 *          - 对外通过 extern "C" 包装函数暴露给 C 代码（main.c）使用
 ******************************************************************************
 */

#include "Led.hpp"

/* ==========================================================================
 *                          一、类方法实现
 * ========================================================================== */

/**
 * @brief  构造函数：保存该 LED 所在的端口和引脚
 * @param  port 端口指针，例如 GPIOA
 * @param  pin  引脚号（位掩码），例如 GPIO_PIN_1
 * @note   这里只做“记录”，不进行任何硬件操作。
 *         原因：全局对象 g_led 的构造函数在 main() 之前（C++ 静态初始化阶段）
 *         就会执行，此时 HAL_Init()/MX_GPIO_Init() 还没跑，时钟和 GPIO 都没配好，
 *         若在这里调用 HAL_GPIO_Init 会出问题。
 *         本工程走“路线 A”：GPIO 的具体配置由 CubeMX 生成的 MX_GPIO_Init() 完成，
 *         所以构造函数不需要、也不应该去初始化硬件。
 */
Led::Led(GPIO_TypeDef* port, uint16_t pin)
    : _port(port), _pin(pin)          // 初始化列表：等价于 _port = port; _pin = pin;
{
}

/**
 * @brief  点亮 LED
 * @note   本板 LED 为“低电平点亮”，所以输出 RESET(0)。
 *         若以后接入极性相反的 LED，可增加 _activeHigh 成员再作判断。
 */
void Led::on()
{
    HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_RESET);   // 输出低电平 -> 点亮
}

/**
 * @brief  熄灭 LED
 * @note   低电平点亮，所以熄灭即输出 SET(1)。
 */
void Led::off()
{
    HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_SET);     // 输出高电平 -> 熄灭
}

/**
 * @brief  翻转 LED 状态
 * @note   直接调用 HAL 的翻转函数，无需关心“有效电平”是高还是低。
 */
void Led::toggle()
{
    HAL_GPIO_TogglePin(_port, _pin);                  // 电平取反
}

/* ==========================================================================
 *                     二、静态对象：PA1 上的这颗 LED
 * ========================================================================== */

/*
 * 在文件作用域定义一个静态对象（内部链接，外部不可见）。
 * 参数 (GPIOA, GPIO_PIN_1) 指明这颗 LED 挂在 PA1 上——
 * 与 CubeMX 中 MX_GPIO_Init() 配置的 PA1 保持一致。
 *
 * 之所以放在 .cpp 里且用 static：
 *   - main.c 是 C 代码，无法直接使用 C++ 对象；
 *   - 对外只暴露下面的 C 包装函数，对象本身“藏”在文件内部。
 *
 * 该对象在 main() 之前构造，但因为构造函数只保存成员、不碰硬件，所以安全。
 */
static Led g_led(GPIOA, GPIO_PIN_1);

/* ==========================================================================
 *                   三、给 C（main.c）使用的包装函数
 * ========================================================================== */

/*
 * 说明：
 *   - 这三个函数的名字必须与 Led.hpp 中 extern "C" 里的声明完全一致，
 *     否则 main.c 链接时会报 undefined symbol。
 *   - Led.hpp 中已用 extern "C" 声明过，定义会自动继承 C 链接；
 *     这里再显式写一次 extern "C"，更直观，也不怕以后头文件改动。
 *   - 函数体内只是转发到 g_led 对应的成员方法，桥接 C 与 C++。
 */

/**
 * @brief  点亮 LED（供 C 调用）
 */
extern "C" void LED_On(void)
{
    g_led.on();
}

/**
 * @brief  熄灭 LED（供 C 调用）
 */
extern "C" void LED_Off(void)
{
    g_led.off();
}

/**
 * @brief  翻转 LED（供 C 调用）
 */
extern "C" void LED_Toggle(void)
{
    g_led.toggle();
}
