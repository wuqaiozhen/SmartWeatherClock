#ifndef __LOG_H
#define __LOG_H

/* ============================================================
 *  日志组件 (线程安全 + 等级 + 模块 tag)
 *
 *  用法:
 *      LOGI("COMM", "TX: %s\r\n", cmd);      普通信息
 *      LOGW("SENSOR", "AHT20 failed\r\n");   警告
 *      LOGE("APP", "queue create failed\r\n"); 错误
 *      LOGD("UI", "recv %d\r\n", n);         调试(默认不编进固件)
 *
 *  输出形如:
 *      [I][COMM] TX: GET_TIME
 *      [E][SENSOR] AHT20 failed
 * ============================================================ */

/* ---------- 日志等级 (数值越大越严重) ----------
 *   平时看日志: 数字大的先看
 *   过滤规则:   只输出 等级 >= LOG_LEVEL_MIN 的日志 */
#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO  1
#define LOG_LVL_WARN  2
#define LOG_LVL_ERROR 3

/* ---------- 编译期总开关 ----------
 * 只有 等级 >= LOG_LEVEL_MIN 的日志才会被编进固件 (其余在编译时直接丢弃)
 *   全部打开:  LOG_LVL_DEBUG
 *   只看错误:  LOG_LVL_ERROR
 *   发布版本:  LOG_LVL_WARN 或 LOG_LVL_INFO */
#define LOG_LEVEL_MIN LOG_LVL_INFO

void log_Init(void);

/* 新接口: 等级 + 模块 tag */
void log_print(int lvl, const char *tag, const char *fmt, ...);

/* ---------- 四个便捷宏 ----------
 * do { ... } while (0) 是为了让宏在任何 if/else 里都当成"一条语句",
 * 而且里面的 if 条件在编译期就能算出真假, 被关掉的日志不会产生任何代码 */
#define LOGE(tag, ...) do { if (LOG_LVL_ERROR >= LOG_LEVEL_MIN) log_print(LOG_LVL_ERROR, tag, __VA_ARGS__); } while (0)
#define LOGW(tag, ...) do { if (LOG_LVL_WARN  >= LOG_LEVEL_MIN) log_print(LOG_LVL_WARN,  tag, __VA_ARGS__); } while (0)
#define LOGI(tag, ...) do { if (LOG_LVL_INFO  >= LOG_LEVEL_MIN) log_print(LOG_LVL_INFO,  tag, __VA_ARGS__); } while (0)
#define LOGD(tag, ...) do { if (LOG_LVL_DEBUG >= LOG_LEVEL_MIN) log_print(LOG_LVL_DEBUG, tag, __VA_ARGS__); } while (0)

#endif /* __LOG_H */
