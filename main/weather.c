#include "weather.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdbool.h>
#include <string.h>
#include <strings.h>


#define WEATHER_URL                                                            \
  "https://api.seniverse.com/v3/weather/now.json"                              \
  "?key=Sckfg5jmTv-yWuO1F&location=WTTDPCGXTWUS&language=en&unit=c"

static const char *TAG = "weather";

/* 把 seniverse 返回的长天气词，映射成 6 个字符以内的短词。
 * STM32 端用 16x32 字体（最多 6 字符）显示天气，长词会被截断，
 * 所以在这里先缩写。没匹配上的原样返回，由 STM32 截断。 */
/* weather code -> ? STM32 ? k_weather ?????
 *   0 ?      1 ??     2 ?       3 ??     4 ??
 *   5 ??    6 ??     7 ???   8 ??     9 ??
 *  10 ??   11 ???  12 ?      13 ?     14 ??
 *  15 ?     99 ??
 * ?????"??? -> ???", ??/??/????? STM32 ??,
 * ??????????, ?? STM32 ??? */
typedef struct {
    const char *full;
    int code;
} weather_code_t;

static const weather_code_t k_weather_code[] = {
    {"sunny",          0},
    {"clear",          0},
    {"partly cloudy",  1},
    {"mostly cloudy",  1},
    {"cloudy",         1},
    {"overcast",       2},
    {"light rain",     3},
    {"moderate rain",  4},
    {"heavy rain",     5},
    {"rainstorm",      6},
    {"storm",          6},
    {"thunderstorm",   7},
    {"thundershower",  7},
    {"light snow",     8},
    {"moderate snow",  9},
    {"heavy snow",    10},
    {"snowstorm",     10},
    {"sleet",         11},
    {"rain and snow", 11},
    {"freezing rain", 11},
    {"fog",           12},
    {"mist",          12},
    {"haze",          13},
    {"smog",          13},
    {"dust",          14},
    {"sand",          14},
    {"sandstorm",     14},
    {"windy",         15},
    {"wind",          15},
};

/* ????? -> ???; ?????? 99 */
static int weather_to_code(const char *full)
{
    unsigned i;

    if (full == NULL) {
        return 99;
    }
    for (i = 0; i < sizeof(k_weather_code) / sizeof(k_weather_code[0]); i++) {
        if (strcasecmp(full, k_weather_code[i].full) == 0) {
            return k_weather_code[i].code;
        }
    }
    return 99;
}
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
        cJSON *text_item = cJSON_GetObjectItemCaseSensitive(now, "text");
        if (t_item == NULL || text_item == NULL) {
          ESP_LOGE(TAG, "missing temperature or text");
          goto cleanup;
        }

        *temp_x10 = (int)(atof(t_item->valuestring) * 10.0f);
        *code = weather_to_code(text_item->valuestring);
        ok = true;
        ESP_LOGI(TAG, "temp_x10=%d, text=[%s] -> code=%d", *temp_x10, text_item->valuestring, *code);

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