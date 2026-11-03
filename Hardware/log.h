#ifndef __LOG_H
#define __LOG_H

void log_Init(void);
void log_printf(const char *fmt, ...);   /* 线程安全的 printf: 整次输出(格式化+发送)全程持锁 */

#endif