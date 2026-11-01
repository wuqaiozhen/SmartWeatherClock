#include "app.h"
#include "clock.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "esp32.h"
#include "ui.h"
#include "ui_msg.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static osMessageQueueId_t ui_q = NULL;

static const osThreadAttr_t clock_attr = {
    .name = "clock",
    .stack_size = 256 * 4,
    .priority = osPriorityHigh,
};

static const osThreadAttr_t ui_attr = {
    .name = "ui",
    .stack_size = 1024 * 4,
    .priority = osPriorityAboveNormal,
};

static const osThreadAttr_t comm_attr = {
    .name = "comm",
    .stack_size = 512 * 4, /* 2KB: 要放 sscanf + printf */
    .priority = osPriorityNormal,
};

static void TaskClock(void *argument) {
  (void)argument;
  clock_time_t t;
  for (;;) {
    if (Clock_Tick(&t)) {
      ui_msg_t msg;
      msg.type = UI_MSG_TIME;
      msg.u.time.hour = t.hour;
      msg.u.time.min = t.min;
      msg.u.time.sec = t.sec;
      osStatus_t st = osMessageQueuePut(ui_q, &msg, 0, 0);
      if (st != osOK) {
        printf("[CLOCK] put failed, st=%d\r\n", (int)st);
      }
      printf("[CLOCK] put %02d:%02d:%02d\r\n", t.hour, t.min, t.sec);
    }

    osDelay(100);
  }
}

static void TaskUI(void *argument) {
  (void)argument;
  ui_msg_t msg;

  for (;;) {
    osStatus_t st = osMessageQueueGet(ui_q, &msg, NULL, osWaitForever);
    if (st != osOK) {
      printf("[UI] recv failed, st=%d\r\n", (int)st);
      continue; /* 没取到就重新等, 不要用脏数据 */
    }
    switch (msg.type) {
    case UI_MSG_TIME:
      ui_update_time(msg.u.time.hour, msg.u.time.min, msg.u.time.sec);
      printf("[UI] recv %02d:%02d:%02d\r\n", msg.u.time.hour, msg.u.time.min,
             msg.u.time.sec);
      break;

    case UI_MSG_DATE:
      ui_update_date(msg.u.date.year, msg.u.date.mon, msg.u.date.day,
                     msg.u.date.wday);
      printf("[UI] date %04d-%02d-%02d w=%d\r\n", msg.u.date.year,
             msg.u.date.mon, msg.u.date.day, msg.u.date.wday);
      break;

    default:
      break;
    }
  }
}

static bool handle_time_line(const char *line) {
  int year, mon, day, hour, min, sec, wday;
  clock_time_t t;
  ui_msg_t m;

  if (sscanf(line, "$T,%d,%d,%d,%d,%d,%d,%d", &year, &mon, &day, &hour, &min,
             &sec, &wday) != 7) {
    printf("[COMM] $T parse failed\r\n");
    return false;
  }
  if (year >= 2000 && year <= 2100 && mon <= 12 && mon >= 1 && day <= 31 &&
      day >= 1 && hour <= 23 && hour >= 0 && min <= 59 && min >= 0 &&
      sec <= 59 && sec >= 0 && wday <= 6 && wday >= 0) {
    printf("[TIME] sync %04d-%02d-%02d %02d:%02d:%02d w=%d\r\n", year, mon, day,
           hour, min, sec, wday);
    t.year = year;
    t.mon  = mon;
    t.day  = day;
    t.hour = hour;
    t.min  = min;
    t.sec  = sec;
    t.wday = wday;
    Clock_Set(&t);

     m.type        = UI_MSG_TIME;
       m.u.time.hour = t.hour;
       m.u.time.min  = t.min;
       m.u.time.sec  = t.sec;
       osMessageQueuePut(ui_q, &m, 0, 0);

       m.type        = UI_MSG_DATE;      /* 复用同一个 m, 只改 type 和对应分支 */
       m.u.date.year = t.year;
       m.u.date.mon  = t.mon;
       m.u.date.day  = t.day;
       m.u.date.wday = t.wday;
       osMessageQueuePut(ui_q, &m, 0, 0);
    return true;
  } else {
    printf("[TIME] 数据超出范围: year=%04d month=%02d day=%02d hour=%02d "
           "min=%02d sec=%02d week=%d\r\n",
           year, mon, day, hour, min, sec, wday);
    return false;
  }
}

static void TaskComm(void *argument) {
  (void)argument;
  char line[128];
  int n = 0;

  for (;;) {
    if (ESP32_GetLine(line, sizeof(line))) {
      printf("[RX] %s\r\n", line);

      if (strncmp(line, "$T", 2) == 0) 
      {
        handle_time_line(line);
      }
    }
    n++;
    if (n >= 150) {
      n = 0;
      printf("[COMM] TX: GET_TIME\r\n");
      ESP32_SendCommand("GET_TIME");
    }
    osDelay(20);
  }
}

void app_start(void) {
  ui_q = osMessageQueueNew(8, sizeof(ui_msg_t), NULL);
  if (ui_q == NULL) {
    printf("[APP] create queue failed\r\n");
    return;
  }
  printf("[APP] queue ok, item=%u bytes\r\n", (unsigned)sizeof(ui_msg_t));

  osThreadId_t id = osThreadNew(TaskClock, NULL, &clock_attr);
  if (id == NULL) {
    printf("[CLOCK] create task failed\r\n");
  }

  id = osThreadNew(TaskUI, NULL, &ui_attr);
  if (id == NULL) {
    printf("[UI] create task failed\r\n");
  }

  id = osThreadNew(TaskComm, NULL, &comm_attr);
  if (id == NULL) {
    printf("[COMM] create task failed\r\n");
  }
}