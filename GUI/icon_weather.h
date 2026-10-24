#ifndef __ICON_WEATHER_H
#define __ICON_WEATHER_H

#include <stdint.h>

#define WICON_W  64
#define WICON_H  64

const uint16_t *weather_icon_find(int code);

#endif
