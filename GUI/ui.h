#ifndef __UI_H
#define __UI_H

#include <stdint.h>

void ui_init(void);
void ui_update_weather(int temp_x10, int code);
void ui_update_indoor(float temp, float humi, uint32_t press_hpa);


#endif