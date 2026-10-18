 #ifndef __WIFI_H
#define __WIFI_H

#include <stdbool.h>

 /* 连接 WiFi。会阻塞直到连上或重试用尽 */
void wifi_init_sta(void);

 /* 现在是否已连上（有 IP） */
bool wifi_is_connend(void);

#endif