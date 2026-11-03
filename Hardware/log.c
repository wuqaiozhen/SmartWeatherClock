#include "log.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include <stdarg.h>
#include <stdio.h>

/* uart.c 里实现的"发一个字节" */
extern int __io_putchar(int ch);

static osMutexId_t s_log_mutex = NULL; /* NULL = 调度器还没启动 */
void log_Init(void) { s_log_mutex = osMutexNew(NULL); }

void log_printf(const char *fmt, ...) {
  va_list ap;

  /* ① 有锁就上锁 */
  if (s_log_mutex != NULL) {
    osMutexAcquire(s_log_mutex, osWaitForever);
  }

  /* ② 干活: 不管有没有锁都要打印 */
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  /* ③ 有锁就解锁 */
  if (s_log_mutex != NULL) {
    osMutexRelease(s_log_mutex);
  }
}

int _write(int file, char *ptr, int len) {
  int i;
  (void)file;
  for (i = 0; i < len; i++) {
    __io_putchar(ptr[i]);
  }
  return len;
}