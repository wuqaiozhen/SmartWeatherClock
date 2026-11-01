#ifndef __UI_MSG_H
#define __UI_MSG_H

#include <stdint.h>

/* ---- 消息类型 ---- */
typedef enum {
    UI_MSG_TIME = 0, /* 时间变了 (秒针走)     用 u.time    */
    UI_MSG_DATE,     /* 日期/星期变了          用 u.date    */
    UI_MSG_WEATHER,  /* 室外天气来了          用 u.weather */
    UI_MSG_INDOOR,   /* 室内温湿度/气压来了    用 u.indoor  */
} ui_msg_type_t;

/* ---- 消息本体 ---- */
typedef struct {
    ui_msg_type_t type; /* 先看它是谁, 再看 union */

    union {
        /* 时间: 12:34:56 */
        struct {
            int hour;
            int min;
            int sec;
        } time;

        /* 日期: 2026-10-30 星期五(wday=5, 0=周日) */
        struct {
            int year;
            int mon;
            int day;
            int wday;
        } date;

        /* 室外天气: 190 = 19.0C, code 见 GUI/ui.c 的天气码表 */
        struct {
            int temp_x10;
            int code;
        } weather;

        /* 室内环境 */
        struct {
            float    temp;      /* 摄氏度 */
            float    humi;      /* %RH */
            uint32_t press_hpa; /* 百帕 */
        } indoor;
    } u;
} ui_msg_t;

#endif /* __UI_MSG_H */
