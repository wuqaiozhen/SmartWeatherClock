#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
        uart_send_line("$BUSY");
        ESP_LOGI(TAG, "TODO: time");
      } else {
        uart_send_line("$E,9");
        ESP_LOGI(TAG, "unknow cmd:[%s]", line);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
