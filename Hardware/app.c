#include "app.h"
#include "aht20.h"
#include "bmp280.h"
#include "clock.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "esp32.h"
#include "stm32f4xx_hal.h"
#include "ui.h"
#include "ui_msg.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/* ---- 请求周期 (ms) ---- */
#define WEATHER_PERIOD_MS 600000u /* 天气: 10 分钟 */
#define TIME_RETRY_MS 3000u       /* 未校准: 3 秒重试 (正式版改 30000u) */
#define TIME_PERIOD_MS 3600000u   /* 已校准: 1 小时 */

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

static const osThreadAttr_t sensor_attr = {
    .name = "sensor",
    .stack_size = 1024 * 4,
    .priority = osPriorityBelowNormal,
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

    case UI_MSG_WEATHER:
      ui_update_weather(msg.u.weather.temp_x10, msg.u.weather.code);
      printf("[UI] weather x10=%d code=%d\r\n", msg.u.weather.temp_x10,
             msg.u.weather.code);
      break;

      case UI_MSG_INDOOR:
       ui_update_indoor(msg.u.indoor.temp, msg.u.indoor.humi,
                                msg.u.indoor.press_hpa);
       printf("[UI] indoor %d.%d C %d%%RH %lu hPa\r\n",
                      (int)(msg.u.indoor.temp * 10) / 10,   /* 用整数打印, 避免浮点 printf 吃栈 */
                      (int)(msg.u.indoor.temp * 10) % 10,
                      (int)(msg.u.indoor.humi + 0.5f),
                      (unsigned long)(msg.u.indoor.press_hpa / 100));
               break;
      break;

    default:
      break;
    }
  }
}

static bool handle_time_line(const char *line) {
  int year, mon, day, hour, min, sec, wday;
  ui_msg_t m;

  if (sscanf(line, "$T,%d,%d,%d,%d,%d,%d,%d", &year, &mon, &day, &hour, &min,
             &sec, &wday) != 7) {
    printf("[COMM] $T parse failed\r\n");
    return false;
  }
  if (year >= 2000 && year <= 2100 && mon <= 12 && mon >= 1 && day <= 31 &&
      day >= 1 && hour <= 23 && hour >= 0 && min <= 59 && min >= 0 &&
      sec <= 59 && sec >= 0 && wday <= 6 && wday >= 0) {
    clock_time_t t = {t.year = year, t.mon = mon, t.day = day,  t.hour = hour,
                      t.min = min,   t.sec = sec, t.wday = wday};
    Clock_Set(&t);

    m.type = UI_MSG_TIME;
    m.u.time.hour = t.hour;
    m.u.time.min = t.min;
    m.u.time.sec = t.sec;
    osMessageQueuePut(ui_q, &m, 0, 0);

    m.type = UI_MSG_DATE; /* 复用同一个 m, 只改 type 和对应分支 */
    m.u.date.year = t.year;
    m.u.date.mon = t.mon;
    m.u.date.day = t.day;
    m.u.date.wday = t.wday;
    osMessageQueuePut(ui_q, &m, 0, 0);

    printf("[TIME] sync %04d-%02d-%02d %02d:%02d:%02d w=%d\r\n", year, mon, day,
           hour, min, sec, wday);
    return true;
  } else {
    printf("[TIME] 数据超出范围: year=%04d month=%02d day=%02d hour=%02d "
           "min=%02d sec=%02d week=%d\r\n",
           year, mon, day, hour, min, sec, wday);
    return false;
  }
}

static bool handle_weather_line(const char *line) {
  int temp_x10, code;
  ui_msg_t m;

  if (sscanf(line, "$W,%d,%d", &temp_x10, &code) != 2) {
    printf("[COMM] $W parse failed\r\n");
    return false;
  }
  if (temp_x10 <= 1250 && temp_x10 >= -550 && code >= 0 && code <= 99) {
    m.type = UI_MSG_WEATHER;
    m.u.weather.temp_x10 = temp_x10;
    m.u.weather.code = code;
    osMessageQueuePut(ui_q, &m, 0, 0);
    printf("[WEATHER]  temp_x10=%d code=%d\r\n", temp_x10, code);
    return true;
  } else {
    printf("[WEATHER] 数据超出范围: t=%d code=%d\r\n", temp_x10, code);
    return false;
  }
}

static uint32_t s_last_weather_ms = 0;
static uint32_t s_last_time_ms = 0;
static void TaskComm(void *argument) {
  (void)argument;
  char line[128];
  uint32_t now;
  uint32_t period;
  for (;;) {
    /* ---- 1) 收数据 ---- */
    if (ESP32_GetLine(line, sizeof(line))) {
      printf("[RX] %s\r\n", line);
      if (strncmp(line, "$T", 2) == 0) {
        handle_time_line(line);
      } else if (strncmp(line, "$E", 2) == 0) {
        printf("[COMM] ESP32 error reply: %s\r\n", line);
      } else if (strncmp(line, "$W", 2) == 0) {
        handle_weather_line(line);
      }
    }
    /* ---- 2) 发请求 ---- */
    now = HAL_GetTick();
    if (now - s_last_weather_ms >= WEATHER_PERIOD_MS) {
      s_last_weather_ms = now;
      printf("[COMM] TX: GET_WEATHER\r\n");
      ESP32_SendCommand("GET_WEATHER");
    }

    period = Clock_IsSet() ? TIME_PERIOD_MS : TIME_RETRY_MS;
    if (now - s_last_time_ms >= period) {
      s_last_time_ms = now;
      printf("[COMM] TX: GET_TIME\r\n");
      ESP32_SendCommand("GET_TIME");
    }
    osDelay(20);
  }
}

static void TaskSensor(void *argument) {
  (void)argument;
  uint32_t wake = osKernelGetTickCount();
  float t = 0.0f, h = 0.0f;
  uint32_t adc_p, adc_t, press;

  BMP280_ReadCalib();

  for (;;) {
    bool ok = false;

    /* ---- 读AHT20 ---- */
    if (AHT20_Read(&t, &h) == 0) {
      ok = true;
    } else {
      printf("[SENSOR] AHT20 failed\r\n");
    }
    /* ---- 读BMP280 ---- */
    BMP280_WriteReg(0xF4, 0x27);
    osDelay(20);
    BMP280_ReadData(&adc_p, &adc_t);
    press = BMP280_CompensatePress((int32_t)adc_p);
    /* ---- 数据有效才投消息---- */
    if (ok) {ui_msg_t m; m.type =  UI_MSG_INDOOR;
        m.u.indoor.temp = t; m.u.indoor.humi = h;
        m.u.indoor.press_hpa = press;
        osMessageQueuePut(ui_q, &m, 0, 0);
    }
    wake += 2000 * osKernelGetTickFreq() / 1000;
     osDelayUntil(wake);
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

   id = osThreadNew(TaskSensor, NULL, &sensor_attr);
       if (id == NULL)
       {
           printf("[SENSOR] create task failed\r\n");
       }
}