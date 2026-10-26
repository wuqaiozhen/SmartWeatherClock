#ifndef __NTP_H
#define __NTP_H

#include <stdbool.h>
#include <time.h>

void ntp_start(void);
bool ntp_get_local(struct tm *out, int wait_ms);

#endif