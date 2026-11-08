#include "FreeRTOSConfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h" /* ← 新增 */
#include "freertos/task.h"
#include "ntp.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "uart_proto.h"
#include "weather.h"
#include "wifi.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "main";

/* ---- 命令枚举 ---- */
typedef enum {
  CMD_PING = 0,
  CMD_WEATHER,
  CMD_TIME,
  CMD_UNKNOWN,
} cmd_t;
static QueueHandle_t s_cmd_q;

static cmd_t parse_cmd(const char *line) {
  if (strcmp(line, "PING") == 0)
    return CMD_PING;
  if (strcmp(line, "GET_WEATHER") == 0)
    return CMD_WEATHER;
  if (strcmp(line, "GET_TIME") == 0)
    return CMD_TIME;
  return CMD_UNKNOWN;
}

static void handle_cmd(cmd_t cmd) {
  switch (cmd) {
  case CMD_PING: {
    uart_send_line("$PONG");
    ESP_LOGI(TAG, "TX: $PONG");
    break;
  }
  case CMD_WEATHER: {
    int t = 0, code = 99; 
    if (weather_fetch(&t, &code)) {
      char buf[48];
      snprintf(buf, sizeof(buf), "$W,%d,%d", t, code);
      uart_send_line(buf);
      ESP_LOGI(TAG, "TX: %s", buf);
    } else {
      uart_send_line("$E,2");
      ESP_LOGE(TAG, "weather failed");
    }
    break;
  }
  case CMD_TIME: {
    struct tm t; 
    char buf[48];
    if (ntp_get_local(&t, 15000)) {
      snprintf(buf, sizeof(buf), "$T,%d,%d,%d,%d,%d,%d,%d", t.tm_year + 1900,
               t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
               t.tm_wday);
      uart_send_line(buf);
      ESP_LOGI(TAG, "TX: %s", buf);
    } else {
      uart_send_line("$E,3");
      ESP_LOGI(TAG, "time not ready");
    }
    break;
  }
  case CMD_UNKNOWN:
  default: {
    uart_send_line("$E,9");
    ESP_LOGI(TAG, "TX: $E,9 (unknown cmd)");
    break;
  }
  }
}

/* ================= 任务 1：串口 IO 边界 ================= */
static void uart_task(void *arg)
{
  (void)arg;
  char line[128];

  for(;;){
if(!uart_read_line(line, sizeof(line))){
  continue;
}
ESP_LOGI(TAG,"RX: %s",line);

cmd_t c = parse_cmd(line);
  if (xQueueSend(s_cmd_q, &c, 0) != pdTRUE) {
    ESP_LOGE(TAG, "queue full, drop cmd %d", (int)c);
  }
}
}

/* ================= 任务 2：网络事务 ================= */
static void net_task(void *arg)
{
  (void)arg;
  cmd_t got;

  for(;;){
  if (xQueueReceive(s_cmd_q, &got, portMAX_DELAY) != pdTRUE) {
    continue;
  }
  handle_cmd(got);
}
}

void app_main(void) {
  uart_proto_init();
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "boot");
  wifi_init_sta();

  s_cmd_q = xQueueCreate(1, sizeof(cmd_t));
  configASSERT(s_cmd_q != NULL);

  BaseType_t ok;
  ok = xTaskCreate(uart_task, "uart", 4096, NULL, 10, NULL);
  configASSERT(ok == pdPASS);

  ok = xTaskCreate(net_task, "net", 8192, NULL, 5, NULL);
  configASSERT(ok == pdPASS);

  ESP_LOGI(TAG,"tasks started");

  return;
}
