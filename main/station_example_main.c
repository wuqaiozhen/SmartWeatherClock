#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ntp.h"
#include "nvs_flash.h"
#include "uart_proto.h"
#include "weather.h"
#include "wifi.h"
#include <stdio.h>
#include <string.h>


static const char *TAG = "main";

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
  char line[128];
/*测试*/
  struct tm tm_now; 
  if(ntp_get_local(&tm_now, 15000) == 1){
    char buf[48];
          snprintf(buf, sizeof(buf), "$T,%d,%d,%d,%d,%d,%d,%d",
                   tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour,
                   tm_now.tm_min, tm_now.tm_sec, tm_now.tm_wday); 
          ESP_LOGI(TAG, "self-test: %s", buf);    
    } else {
          ESP_LOGE(TAG, "self-test: time failed");
      }

  while (1) {
    if (uart_read_line(line, sizeof(line))) {
      if (strcmp(line, "PING") == 0) {
        uart_send_line("$PONG");
        ESP_LOGI(TAG, "TX: $PONG");
      } else if (strcmp(line, "GET_WEATHER") == 0) {
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
      } else if (strcmp(line, "GET_TIME") == 0) {
        struct tm t;
        char buf[48];
        if (ntp_get_local(&t, 15000)) {
          snprintf(buf, sizeof(buf), "$T,%d,%d,%d,%d,%d,%d,%d",
                   t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, t.tm_wday);
          uart_send_line(buf);
          ESP_LOGI(TAG, "TX: %s", buf);
        } else {
          uart_send_line("$E,3");
          ESP_LOGI(TAG, "time not ready");
        }
      } else {
        uart_send_line("$E,9");
        ESP_LOGI(TAG, "unknow cmd:[%s]", line);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
