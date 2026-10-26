#include "ntp.h"

#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <time.h>

static const char *TAG = "ntp";

static bool s_started = false;          /* 防止重复 init */

/* 判据阈值: 约 2023-11。
 * 系统时间大于它 => SNTP 已经同步成功; 上电初值远小于它 */
#define NTP_VALID_EPOCH  1700000000L

void ntp_start(void)
{
    if (s_started) {
        return;                     /* 已经启动过了, 什么也不做 */
    }

    /* ---- 1) 设置时区: 东八区 ----
     * POSIX 的 TZ 符号是反的: "CST-8" 才表示 UTC+8 (北京时间)
     * 不要写 "Asia/Shanghai", 那个需要 TZ 数据库, 默认没编译进固件 */
    setenv("TZ", "CST-8", 1);
    tzset();

    /* ---- 2) 配置 SNTP ----
     * 这两类设置必须在 esp_sntp_init() 之前完成, 否则不生效 */
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);   /* 周期轮询: 后台自动重对 */
    esp_sntp_setservername(0, "ntp.aliyun.com");       /* 主服务器 */
    esp_sntp_setservername(1, "cn.pool.ntp.org");      /* 备用服务器 */

    /* ---- 3) 点火: 创建后台任务, 本函数立刻返回 ----
     * 返回时系统时间还是错的, 要等后台收到 NTP 回包 */
    esp_sntp_init();

    s_started = true;
    ESP_LOGI(TAG, "sntp started");
}

bool ntp_get_local(struct tm *out, int wait_ms)
{
    time_t ts;          /* Unix 时间戳: 从 1970-01-01 00:00:00 UTC 起走了多少秒 */
    int    waited = 0;  /* 已经等了多少毫秒 */

    if (out == 0) {
        return false;   /* 调用者给了空指针, 没地方填 */
    }

    ntp_start();        /* 兜底: 保证后台任务已启动 (重复调用会被 s_started 挡住) */

    /* ---- 等后台把时间对上 ----
     * 判据看 time(NULL) 的数值大小, 而不是 SNTP 的同步状态标志:
     * 那个标志在同步完成后会被立刻复位, 200ms 轮询很容易错过那一瞬间 */
    for (;;) {
        ts = time(NULL);            /* 取当前系统时间 */

        if (ts > NTP_VALID_EPOCH) {
            break;                  /* 已经跳到 2023 年以后 -> 说明对上了 */
        }
        if (waited >= wait_ms) {
            ESP_LOGE(TAG, "sntp not ready (waited %d ms)", waited);
            return false;           /* 超时; 调用者应稍后再试 */
        }
        vTaskDelay(pdMS_TO_TICKS(200));  /* 让出 CPU 200ms; 空转会触发看门狗重启 */
        waited += 200;
    }

    /* ---- 时间戳 -> 年月日时分秒 (自动做时区换算和闰年计算) ----
     * 用 _r 版本: 结果写进调用者给的 out, 多任务下安全 */
    localtime_r(&ts, out);

    ESP_LOGI(TAG, "sync ok: %04d-%02d-%02d %02d:%02d:%02d wday=%d",
             out->tm_year + 1900,   /* tm_year 存的是"距 1900 多少年" */
             out->tm_mon + 1,       /* tm_mon 是 0~11 */
             out->tm_mday,
             out->tm_hour, out->tm_min, out->tm_sec,
             out->tm_wday);         /* 0 = 周日 */

             
    return true;
}
