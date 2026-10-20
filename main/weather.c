#include "weather.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdbool.h>
#include <string.h>


#define WEATHER_URL                                                            \
  "https://api.seniverse.com/v3/weather/now.json"                              \
  "?key=Sckfg5jmTv-yWuO1F&location=WTTDPCGXTWUS&language=zh-Hans&unit=c"

static const char *TAG = "weather";

bool weather_fetch(int *temp_x10, int *code) {

  bool ok = false;
  cJSON *root = NULL;

  /* --- HTTP 部分 --- */
  esp_http_client_config_t cfg = {.url = WEATHER_URL,
                                  .method = HTTP_METHOD_GET,
                                  .timeout_ms = 10000,
                                  .crt_bundle_attach = esp_crt_bundle_attach};

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  if (client == NULL) {
    ESP_LOGE(TAG, "client init failed");
    return false;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "open failed: %s", esp_err_to_name(err));
    goto cleanup;
  } else {
    esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "status: %d", status);

    if (status == 200) {
      static char buf[1024];
      int total = 0;
      int r;
      while ((r = esp_http_client_read(client, buf + total,
                                       sizeof(buf) - total - 1)) > 0) {
        total = total + r;
        if (total >= (int)sizeof(buf) - 1) {
          break;
        }
      }

      buf[total] = '\0';
      ESP_LOGI(TAG, "body(%d): %s", total, buf);

      root = cJSON_Parse(buf);
      if (root != NULL) {
        ESP_LOGI(TAG, "JSON parse is ok");

        cJSON *results = cJSON_GetObjectItemCaseSensitive(root, "results");
        if (results == NULL) {
          ESP_LOGE(TAG, "no 'results' field");
          goto cleanup;
        }

        cJSON *first = cJSON_GetArrayItem(results, 0);
        if (first == NULL) {
          ESP_LOGE(TAG, "no 'first' field");
          goto cleanup;
        }

        cJSON *now = cJSON_GetObjectItemCaseSensitive(first, "now");
        if (now == NULL) {
          ESP_LOGE(TAG, "no 'now' field");
          goto cleanup;
        }

        cJSON *t_item = cJSON_GetObjectItemCaseSensitive(now, "temperature");
        cJSON *c_item = cJSON_GetObjectItemCaseSensitive(now, "code");
        if (t_item == NULL || c_item == NULL) {
          ESP_LOGE(TAG, "missing temperature or code");
          goto cleanup;
        }

        *temp_x10 = (int)(atof(t_item->valuestring) * 10.0f);
        *code = atoi(c_item->valuestring);
        ok = true;
         ESP_LOGI(TAG, "temp=%d.%d C, code=%d", *temp_x10/10, *temp_x10%10, *code);

      cleanup:
        if (root != NULL) {
          cJSON_Delete(root);
          root = NULL;
        }
      } else {
        ESP_LOGI(TAG, "JSON parse failed");
      }
    }
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return ok;
}