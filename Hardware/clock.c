#include "clock.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

static clock_time_t s_now;        /*当前时间*/
static bool         s_valid;      /*有没有校准过*/
static uint32_t     s_last_ms;    /*上次"走秒"时的 HAL_GetTick()*/

/* 每个月的天数(平年)。下标 0~11 对应 1~12 月, 查表要写 s_month_days[mon - 1] */
static const uint8_t s_month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

/* 闰年: 能被4整除且不能被100整除, 或者能被400整除
 * 2024 闰 / 1900 平 / 2000 闰 / 2100 平 */
static bool is_leap(int y)
{
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

/* 时间 +1 秒, 逐级进位: 秒->分->时->日(星期也+1)->月->年
 * 写法: 每级处理完自己的事就 return, 不满足才往下一级走 (卫语句),
 *       这样代码保持一维平铺, 不会越嵌套越深 */
static void add_one_second(clock_time_t *t)
{
    int days_in_month;

    /* ---- 1) 秒 ---- */
    t->sec++;
    if (t->sec < 60) return;
    t->sec = 0;

    /* ---- 2) 分 ---- */
    t->min++;
    if (t->min < 60) return;
    t->min = 0;

    /* ---- 3) 时 ---- */
    t->hour++;
    if (t->hour < 24) return;
    t->hour = 0;

    /* ---- 4) 日: 跨天了, 星期同时 +1 ---- */
    t->wday = (t->wday + 1) % 7;
    t->day++;

    days_in_month = s_month_days[t->mon - 1];   /* 注意 -1 */
    if (t->mon == 2 && is_leap(t->year)) {
        days_in_month = 29;                     /* 闰年 2 月 */
    }
    if (t->day <= days_in_month) return;
    t->day = 1;

    /* ---- 5) 月 ---- */
    t->mon++;
    if (t->mon <= 12) return;
    t->mon = 1;

    /* ---- 6) 年 ---- */
    t->year++;
}

void Clock_Set(const clock_time_t *t)
{
        if (t == 0) return;         /* 防御: 空指针 */
        s_now = *t;
        s_last_ms = HAL_GetTick();
        s_valid = true;

}

bool Clock_IsSet(void)
{
    return s_valid;
}

bool Clock_Tick(clock_time_t *out)
{
    uint32_t elapsed_ms = 0;
    uint32_t seconds = 0;
     if (!s_valid) return false;
    /* 无符号减法: 定时器回绕(约49天)也正确 */
    elapsed_ms = HAL_GetTick() - s_last_ms;
     if(elapsed_ms < 1000) return false;
      seconds = elapsed_ms / 1000;
      s_last_ms += seconds * 1000;   /* += 不是 = : 余数留着, 不累积漂移 */
      /* 用 while: 主循环若卡了几秒, 要把欠的秒都补上 */
      while (seconds > 0) {
         add_one_second(&s_now);
         seconds--;
      }
    if (out != 0) {
             *out = s_now;      /* 把当前时间交给调用者 */
         }
         return true;
}

bool Clock_Get(clock_time_t *out)
{
    if(out == NULL || s_valid == false) return false;
    *out = s_now;
    return true;

}
