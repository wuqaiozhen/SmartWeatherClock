#include "log.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include <stdarg.h>
#include <stdio.h>

/* uart.c 里实现的"发一个字节" */
extern int __io_putchar(int ch);

static osMutexId_t s_log_mutex = NULL; /* NULL = 调度器还没启动 */

void log_Init(void) { s_log_mutex = osMutexNew(NULL); }

/* ---------- 等级 -> 单字符 (日志前缀用) ---------- */
static char lvl_to_char(int lvl) {
  switch (lvl) {
  case LOG_LVL_ERROR:
    return 'E';
  case LOG_LVL_WARN:
    return 'W';
  case LOG_LVL_INFO:
    return 'I';
  default:
    return 'D';
  }
}

/* ---------- 带等级和模块 tag 的日志 ----------
 * 输出: [I][COMM] TX: GET_TIME
 * "前缀 + 正文"在同一次持锁里完成, 所以不会被别的任务切开 */
void log_print(int lvl, const char *tag, const char *fmt, ...) {
  va_list ap;

  if (s_log_mutex != NULL) {
    osMutexAcquire(s_log_mutex, osWaitForever);
  }

  printf("[%c][%s] ", lvl_to_char(lvl), tag); /* 前缀 (printf 最终调 _write, 不加锁) */

  va_start(ap, fmt);
  vprintf(fmt, ap); /* 正文 */
  va_end(ap);

  if (s_log_mutex != NULL) {
    osMutexRelease(s_log_mutex);
  }
}

/* ---------- 老接口: 无等级/tag, 保留给已有代码 ---------- */
void log_printf(const char *fmt, ...) {
  va_list ap;

  if (s_log_mutex != NULL) {
    osMutexAcquire(s_log_mutex, osWaitForever);
  }

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  if (s_log_mutex != NULL) {
    osMutexRelease(s_log_mutex);
  }
}

/* ---------- 覆盖 syscalls.c 的弱符号: 只管把字节送出去, 不加锁 ---------- */
int _write(int file, char *ptr, int len) {
  int i;
  (void)file;
  for (i = 0; i < len; i++) {
    __io_putchar(ptr[i]);
  }
  return len;
}
