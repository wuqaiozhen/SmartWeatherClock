#include "weather.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* 心知天气: 实时天气接口
 * location 用「纬度:经度」而不是城市 ID, 原因:
 *   免费版只开放地级市 ID, 区县级 ID (如吴中区 WTTCCH80FWRR) 会返回 403;
 *   而坐标是开放能力, API 收到坐标后会自己解析成对应区县。
 * 31.25:120.63 = 苏州市吴中区 (区政府一带)
 * language=en  -> 返回英文天气词, 再由 weather_to_code() 转成数字码
 * unit=c       -> 摄氏度 */
#define WEATHER_URL                                                            \
  "https://api.seniverse.com/v3/weather/now.json"                              \
  "?key=Sckfg5jmTv-yWuO1F&location=31.25:120.63&language=en&unit=c"

static const char *TAG = "weather";

/* 天气码表: 英文天气词 -> 数字码
 * 数字码在 STM32 侧对应「中文词 + 彩色图标 + 主题色」, 见 ui.c 的 k_weather[]
 *
 *   0  晴        1  多云      2  阴        3  小雨      4  中雨
 *   5  大雨      6  暴雨      7  雷阵雨    8  小雪      9  中雪
 *  10  大雪     11  雨夹雪   12  雾       13  霾       14  沙尘
 *  15  风       99  未知
 *
 * 为什么要转成数字码:
 *   1) 串口传输短, 解析简单 (STM32 一个 switch 就够)
 *   2) 中文/图标/配色全部由 STM32 决定, 以后想改语言或换图标只改 STM32
 *   3) 英文词可能有多种写法 (sunny / clear), 在这里统一收口 */
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
    {"hazy",          13},
    {"foggy",         12},
    {"blizzard",      10},
    {"drizzle",        3},
    {"shower",         3},
    {"breezy",        15},
    {"smog",          13},
    {"dust",          14},
    {"sand",          14},
    {"sandstorm",     14},
    {"windy",         15},
    {"wind",          15},
};

/* 第二级匹配: 精确查表失败时, 按关键词猜一个最接近的码。
 * 目的: 心知天气的英文词不固定(见过 Foggy / Light Rain / Cloudy...),
 *       穷举表不可能盖全, 所以再加一层"模糊兜底", 避免动不动就掉到"未知"。
 * 做法: 先把整串转小写, 再用 strstr 找关键词。 */
static int code_by_keyword(const char *full)
{
    char low[32];
    unsigned i;

    for (i = 0; i + 1 < sizeof(low) && full[i] != '\0'; i++) {
        char c = full[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        low[i] = c;
    }
    low[i] = '\0';

    /* 顺序有讲究: 先判断更"具体"的词 */
    if (strstr(low, "thunder") != NULL)  return 7;    /* 雷阵雨 */
    if (strstr(low, "sleet") != NULL)    return 11;   /* 雨夹雪 */
    if (strstr(low, "snow") != NULL)     return 8;    /* 雪 */
    if (strstr(low, "hail") != NULL)     return 7;    /* 冰雹, 归到雷阵雨 */
    if (strstr(low, "rain") != NULL)     return 3;    /* 雨 */
    if (strstr(low, "drizzle") != NULL)  return 3;
    if (strstr(low, "fog") != NULL)      return 12;   /* 雾 */
    if (strstr(low, "mist") != NULL)     return 12;
    if (strstr(low, "haz") != NULL)      return 13;   /* 霾 (haze/hazy) */
    if (strstr(low, "sand") != NULL)     return 14;   /* 沙尘 */
    if (strstr(low, "dust") != NULL)     return 14;
    if (strstr(low, "wind") != NULL)     return 15;   /* 风 */
    if (strstr(low, "breez") != NULL)    return 15;
    if (strstr(low, "overcast") != NULL) return 2;    /* 阴 */
    if (strstr(low, "cloud") != NULL)    return 1;    /* 多云 */
    if (strstr(low, "sunny") != NULL)    return 0;    /* 晴 */
    if (strstr(low, "clear") != NULL)    return 0;

    return 99;                                        /* 真的猜不出来 */
}

/* 查表: 英文天气词 -> 数字码
 * 用 strcasecmp 忽略大小写 (API 返回 "Light rain", 表里写 "light rain")
 * 没匹配上返回 99 (未知) */
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
    return code_by_keyword(full);   /* 精确匹配不到, 走关键词兜底 */
}

/* 拉取实时天气
 * 成功: *temp_x10 = 温度x10 (18.0C -> 180), *code = 天气码, 返回 true
 * 失败: 返回 false, 两个输出参数保持原样 */
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
        ESP_LOGI(TAG, "temp_x10=%d, text=[%s] -> code=%d", *temp_x10,
                 text_item->valuestring, *code);

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
