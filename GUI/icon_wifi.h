#ifndef __ICON_WIFI_H
#define __ICON_WIFI_H

#include <stdint.h>

#define WIFI_ICON_W     24
#define WIFI_ICON_H     24
#define WIFI_ICON_BYTES ((WIFI_ICON_W/8)*WIFI_ICON_H)

extern const uint8_t wifi_icon[WIFI_ICON_BYTES];

#endif
