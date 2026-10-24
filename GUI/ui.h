#ifndef __UI_H
#define __UI_H

#include <stdint.h>

void ui_init(void);
void ui_update_weather(int temp_x10, int code);
void ui_update_indoor(float temp, float humi, uint32_t press_hpa);
void ui_update_time(int hour, int min, int sec);
void ui_update_date(int year, int mon, int day, int wday);


#endif