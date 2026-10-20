#ifndef __WEATHER_H_
#define __WEATHER_H_

#include <stdbool.h>

 /* 拉取天气。成功返回 true，并通过指针交回结果 */
bool weather_fetch(int *temp_x10, int *code);

#endif