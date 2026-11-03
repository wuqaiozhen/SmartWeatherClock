#include "app.h"
#include "FreeRTOS.h"
#include "aht20.h"
#include "bmp280.h"
#include "clock.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "esp32.h"
#include "log.h"
#include "stm32f4xx_hal.h"
#include "ui.h"
#include "ui_msg.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/* ---- 请求周期 (ms) ---- */
#define WEATHER_RETRY_MS 30000u   /* 还没拿到过天气: 30 秒重试 */
#define WEATHER_PERIOD_MS 600000u /* 天气: 10 分钟 */
#define TIME_RETRY_MS 3000u       /* 未校准: 3 秒重试 (正式版改 30000u) */
#define TIME_PERIOD_MS 3600000u   /* 已校准: 1 小时 */

#define STACK_OF(x) (unsigned)osThreadGetStackSpace(x)

static osMessageQueueId_t ui_q = NULL;
static osThreadId_t s_clock_id = NULL;
static osThreadId_t s_ui_id = NULL;
static osThreadId_t s_comm_id = NULL;
static osThreadId_t s_sensor_id = NULL;

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
    .stack_size = 768 * 4, /* 2KB: 要放 sscanf + printf */
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
        LOGI("CLOCK", "put failed, st=%d\r\n", (int)st);
      }
      LOGI("CLOCK", "put %02d:%02d:%02d\r\n", t.hour, t.min, t.sec);
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
      LOGI("UI", "recv failed, st=%d\r\n", (int)st);
      continue; /* 没取到就重新等, 不要用脏数据 */
    }
    switch (msg.type) {
    case UI_MSG_TIME:
      ui_update_time(msg.u.time.hour, msg.u.time.min, msg.u.time.sec);
      LOGI("UI", "recv %02d:%02d:%02d\r\n", msg.u.time.hour, msg.u.time.min,
             msg.u.time.sec);
      break;

    case UI_MSG_DATE:
      ui_update_date(msg.u.date.year, msg.u.date.mon, msg.u.date.day,
                     msg.u.date.wday);
      LOGI("UI", "date %04d-%02d-%02d w=%d\r\n", msg.u.date.year,
             msg.u.date.mon, msg.u.date.day, msg.u.date.wday);
      break;

    case UI_MSG_WEATHER:
      ui_update_weather(msg.u.weather.temp_x10, msg.u.weather.code);
      LOGI("UI", "weather x10=%d code=%d\r\n", msg.u.weather.temp_x10,
             msg.u.weather.code);
      break;

    case UI_MSG_INDOOR:
      ui_update_indoor(msg.u.indoor.temp, msg.u.indoor.humi,
                       msg.u.indoor.press_hpa);
      LOGI("UI", "indoor %d.%d C %d%%RH %lu hPa\r\n",
             (int)(msg.u.indoor.temp * 10) /
                 10, /* 用整数打印, 避免浮点 printf 吃栈 */
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
    LOGI("COMM", "$T parse failed\r\n");
    return false;
  }
  if (year >= 2000 && year <= 2100 && mon <= 12 && mon >= 1 && day <= 31 &&
      day >= 1 && hour <= 23 && hour >= 0 && min <= 59 && min >= 0 &&
      sec <= 59 && sec >= 0 && wday <= 6 && wday >= 0) {
    clock_time_t t = {.year = year,
                      .mon = mon,
                      .day = day,
                      .hour = hour,
                      .min = min,
                      .sec = sec,
                      .wday = wday};
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

    LOGI("TIME", "sync %04d-%02d-%02d %02d:%02d:%02d w=%d\r\n", year, mon, day,
           hour, min, sec, wday);
    return true;
  } else {
    LOGI("TIME", "数据超出范围: year=%04d month=%02d day=%02d hour=%02d "
           "min=%02d sec=%02d week=%d\r\n",
           year, mon, day, hour, min, sec, wday);
    return false;
  }
}

static bool s_weather_ok = false;
static bool handle_weather_line(const char *line) {
  int temp_x10, code;
  ui_msg_t m;

  if (sscanf(line, "$W,%d,%d", &temp_x10, &code) != 2) {
    LOGI("COMM", "$W parse failed\r\n");
    return false;
  }
  if (temp_x10 <= 1250 && temp_x10 >= -550 && code >= 0 && code <= 99) {
    m.type = UI_MSG_WEATHER;
    m.u.weather.temp_x10 = temp_x10;
    m.u.weather.code = code;
    osMessageQueuePut(ui_q, &m, 0, 0);
    LOGI("WEATHER", "temp_x10=%d code=%d\r\n", temp_x10, code);
    s_weather_ok = true;
    return true;
  } else {
    LOGI("WEATHER", "数据超出范围: t=%d code=%d\r\n", temp_x10, code);
    return false;
  }
}

static void handle_line(const char *line) {
  LOGI("RX", "%s\r\n", line);

  if (strncmp(line, "$T", 2) == 0) {
    handle_time_line(line);
  } else if (strncmp(line, "$E", 2) == 0) {
    LOGI("COMM", "ESP32 error reply: %s\r\n", line);
  } else if (strncmp(line, "$W", 2) == 0) {
    handle_weather_line(line);
  }
}

static bool ESP32_Command(const char *cmd, const char *expect, char *resp,
                          int resp_len, uint32_t timeout_ms) {
  char line[128];                   /* 临时存放取出来的一整行        */
  uint32_t start = HAL_GetTick();   /* ① 出发时间: 必须在发送【之前】取 */
  int explen = (int)strlen(expect); /* ② 要比较几个字符 ("$T" → 2)   */

  ESP32_SendCommand(cmd); /* ③ 把命令发出去               */

  for (;;) /* ④ 等 → 看 → 没到再等          */
  {
    uint32_t elapsed = HAL_GetTick() - start; /* ⑤ 到现在花了多久(无符号减法) */
    uint32_t remain; /* 还能等多久                    */

    if (elapsed >= timeout_ms) /* ⑥ 整体超时了吗?               */
    {
      return false; /*    超时 → 放弃                */
    }

    remain = timeout_ms - elapsed; /* ⑦ 剩下还能等多久              */
    if (!ESP32_WaitData(remain))   /* ⑧ 睡到有数据(最多 remain)     */
    {
      continue; /*    没数据 → 回循环头重新算时间 */
    }

    while (ESP32_GetLine(line, sizeof(line))) /* ⑨ 把缓冲里的行全取出来 */
    {
      if (strncmp(line, expect, (size_t)explen) == 0) /* ⑩ 是我要的那行吗?  */
      {
        if (resp != NULL && resp_len > 0) /* ⑪ 调用者要内容吗?            */
        {
          strncpy(resp, line, (size_t)(resp_len - 1)); /* ⑫ 安全复制      */
          resp[resp_len - 1] = '\0'; /* ⑬ strncpy 不保证结尾, 手动补 0 */
        }
        return true; /* ⑭ 成功!                      */
      }

      handle_line(line); /* ⑮ 不是我要的 → 交给业务处理   */
    }
    /* 把缓冲取空了还没匹配 → 回到 for 开头, 继续等                    */
  }
}

static uint32_t s_last_weather_ms = 0;
static uint32_t s_last_time_ms = 0;
static void TaskComm(void *argument) {
  (void)argument;
  char line[128];
  uint32_t now;
  uint32_t period;

  /* ---- 开机首帧: 立刻请求一次, 不等周期 ---- */
  LOGI("COMM", "TX: GET_WEATHER (boot)\r\n");
  ESP32_SendCommand("GET_WEATHER");
  s_last_weather_ms = HAL_GetTick();

  LOGI("COMM", "TX: GET_TIME (boot)\r\n");
  ESP32_SendCommand("GET_TIME");
  s_last_time_ms = HAL_GetTick();

  for (;;) {
    /* ---- 1) 收数据 ---- */
    if (ESP32_GetLine(line, sizeof(line))) {
      handle_line(line);
      continue;
    }
    /* ---- 2) 发请求 ---- */
    now = HAL_GetTick();
    period = s_weather_ok ? WEATHER_PERIOD_MS : WEATHER_RETRY_MS;
    if (now - s_last_weather_ms >= period) {
      s_last_weather_ms = now;
      char resp[128];
      LOGI("COMM", "TX: GET_WEATHER\r\n");
      if (ESP32_Command("GET_WEATHER", "$W", resp, sizeof(resp), 3000)) {
        handle_weather_line(resp);
      } else {
        LOGI("COMM", "weather timeout\r\n");
      }
    }

    period = Clock_IsSet() ? TIME_PERIOD_MS : TIME_RETRY_MS;
    if (now - s_last_time_ms >= period) {
      s_last_time_ms = now;
      char resp[128];
      LOGI("COMM", "TX: GET_TIME\r\n");
      if (ESP32_Command("GET_TIME", "$T", resp, sizeof(resp), 8000)) {
        handle_time_line(resp);
      } else {
        LOGI("COMM", "time timeout\r\n");
      }
    }
    ESP32_WaitData(1000);
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
      LOGE("SENSOR", "AHT20 failed\r\n");
    }
    /* ---- 读BMP280 ---- */
    BMP280_WriteReg(0xF4, 0x27);
    osDelay(20);
    BMP280_ReadData(&adc_p, &adc_t);
    press = BMP280_CompensatePress((int32_t)adc_p);
    /* ---- 数据有效才投消息---- */
    if (ok) {
      ui_msg_t m;
      m.type = UI_MSG_INDOOR;
      m.u.indoor.temp = t;
      m.u.indoor.humi = h;
      m.u.indoor.press_hpa = press;
      osMessageQueuePut(ui_q, &m, 0, 0);
    }
    wake += 2000 * osKernelGetTickFreq() / 1000;
    osDelayUntil(wake);
  }
}

void app_start(void) {
  log_Init();

  ui_q = osMessageQueueNew(8, sizeof(ui_msg_t), NULL);
  if (ui_q == NULL) {
    LOGE("APP", "create queue failed\r\n");
    return;
  }
  LOGI("APP", "queue ok, item=%u bytes\r\n", (unsigned)sizeof(ui_msg_t));

  s_clock_id = osThreadNew(TaskClock, NULL, &clock_attr);
  if (s_clock_id == NULL) {
    LOGI("CLOCK", "create task failed\r\n");
  }

  s_ui_id = osThreadNew(TaskUI, NULL, &ui_attr);
  if (s_ui_id == NULL) {
    LOGI("UI", "create task failed\r\n");
  }

  s_comm_id = osThreadNew(TaskComm, NULL, &comm_attr);
  if (s_comm_id == NULL) {
    LOGI("COMM", "create task failed\r\n");
  }

  s_sensor_id = osThreadNew(TaskSensor, NULL, &sensor_attr);
  if (s_sensor_id == NULL) {
    LOGE("SENSOR", "create task failed\r\n");
  }
}

void app_report(void) {
  LOGI("MON", "heap free = %u B\r\n", (unsigned)xPortGetFreeHeapSize());
  LOGI("MON", "stack left: clock=%u ui=%u comm=%u sensor=%u  (bytes)\r\n",
         STACK_OF(s_clock_id), STACK_OF(s_ui_id), STACK_OF(s_comm_id),
         STACK_OF(s_sensor_id));
  /* ↓ 新增这一行 */
  LOGI("MON", "state: clock=%d ui=%d comm=%d sensor=%d\r\n",
         (int)osThreadGetState(s_clock_id), (int)osThreadGetState(s_ui_id),
         (int)osThreadGetState(s_comm_id), (int)osThreadGetState(s_sensor_id));
}