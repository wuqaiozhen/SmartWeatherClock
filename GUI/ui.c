#include "ui.h"
#include "gui.h"
#include "icon_wifi.h"
#include "icon_degc.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ================= 配色 (RGB888) ================= */
#define C_BG      0x0A1224   /* 屏幕背景 */
#define C_CARD    0xEBF2FA   /* 卡片背景 */
#define C_TEXT    0x12265A   /* 深色文字 */
#define C_BLUE    0x1955C8   /* 蓝色 */
#define C_CYAN    0x0091C3   /* 青色 */

/* ================= 布局参数 =================
 * 屏幕 240x320
 *   卡片① 时间:  x   6 .. 233   (宽 228)
 *   卡片② 室内:  x   6 .. 116   (宽 111)
 *   卡片③ 室外:  x 123 .. 233   (宽 111)
 *   6 + 111 + 6 + 111 + 6 = 240
 */
#define CARD1_X     6
#define CARD1_Y     6
#define CARD1_W     228
#define CARD1_H     144

#define CARD2_X     6
#define CARD3_X     123
#define CARD_BOT_Y  155
#define CARD_BOT_W  111
#define CARD_BOT_H  159

#define TXT2_X      12       /* 卡片② 文字起点 (6+6)  */
#define TXT3_X      129      /* 卡片③ 文字起点 (123+6) */
#define TXT3_W      98       /* 卡片③ 可用宽度 (129..227) */

/* 卡片③ 竖向排布 */
#define LBL_Y       160      /* card2 title (16x16) */
#define ICON_X      146      /* 图标居中: 129 + (98-64)/2 */
#define ICON_Y      160      /* icon 64x64 */
#define WORD_Y      230      /* 天气中文 (24x24) */
#define OTEMP_Y     270      /* 室外温度 (16x32) */

/* ================= 中文码点 ================= */
static const uint16_t TXT_INDOOR[] = {0x5BA4, 0x5185, 0x73AF, 0x5883, 0};   /* 室内环境 */
static const uint16_t TXT_SUZHOU[] = {0x82CF, 0x5DDE, 0};                   /* 苏州 */
static const uint16_t TXT_SUN[]    = {0x661F, 0x671F, 0x65E5, 0}; 
static const uint16_t TXT_MON[]    = {0x661F, 0x671F, 0x4E00, 0}; 
static const uint16_t TXT_TUE[]    = {0x661F, 0x671F, 0x4E8C, 0}; 
static const uint16_t TXT_WED[]    = {0x661F, 0x671F, 0x4E09, 0};           /* 星期三 */
static const uint16_t TXT_THU[]    = {0x661F, 0x671F, 0x56DB, 0}; 
static const uint16_t TXT_FRI[]    = {0x661F, 0x671F, 0x4E94, 0}; 
static const uint16_t TXT_SAT[]    = {0x661F, 0x671F, 0x516D, 0}; 

/* 天气词 */
static const uint16_t WT_SUNNY[]   = {0x6674, 0};                           /* 晴 */
static const uint16_t WT_CLOUDY[]  = {0x591A, 0x4E91, 0};                   /* 多云 */
static const uint16_t WT_OVERCAST[]= {0x9634, 0};                           /* 阴 */
static const uint16_t WT_RAIN_L[]  = {0x5C0F, 0x96E8, 0};                   /* 小雨 */
static const uint16_t WT_RAIN_M[]  = {0x4E2D, 0x96E8, 0};                   /* 中雨 */
static const uint16_t WT_RAIN_H[]  = {0x5927, 0x96E8, 0};                   /* 大雨 */
static const uint16_t WT_STORM[]   = {0x66B4, 0x96E8, 0};                   /* 暴雨 */
static const uint16_t WT_THUNDER[] = {0x96F7, 0x9635, 0x96E8, 0};           /* 雷阵雨 */
static const uint16_t WT_SNOW_L[]  = {0x5C0F, 0x96EA, 0};                   /* 小雪 */
static const uint16_t WT_SNOW_M[]  = {0x4E2D, 0x96EA, 0};                   /* 中雪 */
static const uint16_t WT_SNOW_H[]  = {0x5927, 0x96EA, 0};                   /* 大雪 */
static const uint16_t WT_SLEET[]   = {0x96E8, 0x5939, 0x96EA, 0};           /* 雨夹雪 */
static const uint16_t WT_FOG[]     = {0x96FE, 0};                           /* 雾 */
static const uint16_t WT_HAZE[]    = {0x57CB, 0};                           /* 霾 */
static const uint16_t WT_DUST[]    = {0x6C99, 0x5C18, 0};                   /* 沙尘 */
static const uint16_t WT_WIND[]    = {0x98CE, 0};                           /* 风 */
static const uint16_t WT_UNKNOWN[] = {0x672A, 0x77E5, 0};                   /* 未知 */

/* ================= 天气码表 ================= */
typedef struct {
    int             code;    /* 协议里的天气码 */
    const uint16_t *cn;      /* 中文词 */
    uint32_t        color;   /* 主题色 */
} weather_info_t;

static const weather_info_t k_weather[] = {
    { 0, WT_SUNNY,    0xF5A623},   /* 晴    橙黄 */
    { 1, WT_CLOUDY,   0x4A7C96},   /* 多云  蓝灰 */
    { 2, WT_OVERCAST, 0x5F7A85},   /* 阴    灰   */
    { 3, WT_RAIN_L,   0x3D8BD9},   /* 小雨  浅蓝 */
    { 4, WT_RAIN_M,   0x1565C0},   /* 中雨  蓝   */
    { 5, WT_RAIN_H,   0x0D47A1},   /* 大雨  深蓝 */
    { 6, WT_STORM,    0x123A75},   /* 暴雨  更蓝 */
    { 7, WT_THUNDER,  0x7B1FA2},   /* 雷阵雨 紫 */
    { 8, WT_SNOW_L,   0x54B7E8},   /* 小雪  冰蓝 */
    { 9, WT_SNOW_M,   0x3D9BD4},   /* 中雪 */
    {10, WT_SNOW_H,   0x1E88C7},   /* 大雪 */
    {11, WT_SLEET,    0x3949AB},   /* 雨夹雪 */
    {12, WT_FOG,      0x78909C},   /* 雾    灰蓝 */
    {13, WT_HAZE,     0x8D6E63},   /* 霾    褐   */
    {14, WT_DUST,     0xA1887F},   /* 沙尘  土黄 */
    {15, WT_WIND,     0x00897B},   /* 风    青绿 */
    {99, WT_UNKNOWN,  0x9E9E9E},   /* 未知  中灰 */
};

#define WEATHER_TABLE_LEN  (sizeof(k_weather) / sizeof(k_weather[0]))

 static const uint16_t *TXT_WEEK[7] = { TXT_SUN, TXT_MON, TXT_TUE, TXT_WED, TXT_THU, TXT_FRI, TXT_SAT};

/* ================= 工具函数 ================= */
/* 把颜色往白色方向提亮: pct = 100 原色, 0 = 纯白 */
#define DEGC_LIGHT_PCT  60      /* ℃ 符号的提亮百分比 (越小越浅) */

static uint32_t lighten(uint32_t c, int pct)
{
    uint32_t r = (c >> 16) & 0xFFu;
    uint32_t g = (c >> 8)  & 0xFFu;
    uint32_t b =  c        & 0xFFu;

    r = 255u - (255u - r) * (uint32_t)pct / 100u;
    g = 255u - (255u - g) * (uint32_t)pct / 100u;
    b = 255u - (255u - b) * (uint32_t)pct / 100u;

    return (r << 16) | (g << 8) | b;
}
/* 用空格把 s 左右补齐到 width 个字符, 内容水平居中。
 * 输出宽度固定 => 新内容自然覆盖旧内容, 不需要 FillRect (不会闪)。
 * 注意: s 的缓冲区必须 >= width + 1。 */
static void center_to(char *s, int width)
{
    int len = (int)strlen(s);
    int lead;
    int i;

    if (len >= width) {
        return;
    }
    lead = (width - len) / 2;

    for (i = len - 1; i >= 0; i--) {   /* 从后往前搬, 避免自我覆盖 */
        s[lead + i] = s[i];
    }
    for (i = 0; i < lead; i++) {
        s[i] = ' ';
    }
    for (i = lead + len; i < width; i++) {
        s[i] = ' ';
    }
    s[width] = '\0';
}
/* 按码查表, 查不到就返回"未知" */
static const weather_info_t *info_find(int code)
{
    unsigned i;

    for (i = 0; i < WEATHER_TABLE_LEN; i++) {
        if (k_weather[i].code == code) {
            return &k_weather[i];
        }
    }
    return &k_weather[WEATHER_TABLE_LEN - 1];   /* 最后一条 = 未知 */
}

/* 码点数组长度 (到 0 结束) */
static int cn_len(const uint16_t *codes)
{
    int n = 0;

    while (codes[n] != 0) {
        n++;
    }
    return n;
}

/* ================= 静态界面 ================= */
/* ---- 画温度: 数字(16x32) + 摄氏度符号(16x24), 整段水平居中 ----
 * x0,width : 可用区间
 * 先用 6 个空格(底色)铺满整行, 保证宽度固定: 不残留旧数字, 也不会闪
 */
static void draw_temp(int x0, int y, int width, const char *num, uint32_t color)
{
    int len   = (int)strlen(num);
    int total = len * 16 + DEGC_W;
    int x     = x0 + (width - total) / 2;

    DrawString((uint16_t)x0, (uint16_t)y, "      ", color, C_CARD);

    DrawString((uint16_t)x, (uint16_t)y, num, color, C_CARD);
    DrawBitmap((uint16_t)(x + len * 16), (uint16_t)(y + (32 - DEGC_H) / 2),
               degc_bits, DEGC_W, DEGC_H, lighten(color, DEGC_LIGHT_PCT), C_CARD);
}

void ui_init(void)
{
    /* 阶段 A: 背景 + 三张卡片 */
    ST7789_FillColor(C_BG);

    FillRect(CARD1_X, CARD1_Y, CARD1_W, CARD1_H, C_CARD);
    FillRect(CARD2_X, CARD_BOT_Y, CARD_BOT_W, CARD_BOT_H, C_CARD);
    FillRect(CARD3_X, CARD_BOT_Y, CARD_BOT_W, CARD_BOT_H, C_CARD);

    /* 阶段 B: 卡片① 时间区 */
    DrawBitmap(14, 12, wifi_icon, 24, 24, C_TEXT, C_CARD);
    DrawTextCN24(48, 12, TXT_SUZHOU, C_TEXT, C_CARD);
    /* 时间和日期先留空 / 占位: 收到 $T 后才会显示真实值
     * 不要画假数据(比如 18:11:35), 以免开机那 1 秒误导人 */
    DrawTextMid(12, 104, "----.--.--", C_TEXT, C_CARD);

    /* 阶段 C: 卡片② 室内区 (占位符, 数据来了覆盖) */
    DrawTextCN(TXT2_X, LBL_Y, TXT_INDOOR, C_TEXT, C_CARD);
    draw_temp(TXT2_X, 186, TXT3_W, "--.-", C_BLUE);      /* 占位: --.-℃ */
    DrawString(TXT2_X, 224, "--%RH",  C_BLUE, C_CARD);
    DrawString(TXT2_X, 262, "----mb", C_BLUE, C_CARD);

    /* 阶段 D: 卡片③ 室外区 */
    DrawBitmapColor(ICON_X, ICON_Y, weather_icon_find(99), WICON_W, WICON_H);
    draw_temp(TXT3_X, OTEMP_Y, TXT3_W, "--.-", C_CYAN);   /* 占位: --.-℃ */
}

/* ================= 更新室外天气 ================= */
/* temp_x10 : 温度 x10 (190 = 19.0C)
 * code     : 天气码, 见 k_weather 表 */
void ui_update_weather(int temp_x10, int code)
{
    static int last_code = -1;      /* 上次画的天气码 (天气没变就不重画图标) */
    const weather_info_t *w = info_find(code);
    char buf[16];
    int  neg;
    int  a;

    /* ---- 温度: 6 字符定宽, 用主题色 ---- */
    neg = (temp_x10 < 0);
    a   = neg ? -temp_x10 : temp_x10;
    snprintf(buf, sizeof(buf), "%s%d.%d", neg ? "-" : "", a / 10, a % 10);
    draw_temp(TXT3_X, OTEMP_Y, TXT3_W, buf, w->color);

    /* ---- 图标 + 中文: 只在天气变化时重画 (面积大, 避免闪烁) ---- */
    if (code != last_code) {
        const uint16_t *icon = weather_icon_find(code);
        int len = cn_len(w->cn);
        int x   = TXT3_X + (TXT3_W - len * 24) / 2;   /* 水平居中 */

        last_code = code;

        FillRect(ICON_X, ICON_Y, WICON_W, WICON_H, C_CARD);
        if (icon != NULL) {
            DrawBitmapColor(ICON_X, ICON_Y, icon, WICON_W, WICON_H);
        }

        FillRect(TXT3_X, WORD_Y, TXT3_W, 24, C_CARD);   /* 字数会变, 整行擦掉 */
        DrawTextCN24(x, WORD_Y, w->cn, w->color, C_CARD);
    }
}

/* ================= 更新室内环境 ================= */
/* temp       : 摄氏度 (20.5f)
 * humi       : 相对湿度 % (47.2f)
 * press_x100 : 气压 x100 (102693 = 1026.93 hPa)
 */
void ui_update_indoor(float temp, float humi, uint32_t press_x100)
   {
       char buf[16];
       int t10;
       int neg;
       int a;
       int hi;
       int pi;

       /* 上次画过的值 (static: 程序全程保留) */
       static int  last_t10   = 0;
       static int  last_humi  = 0;
       static int  last_press = 0;
       static bool has_last   = false;      /* 第一次调用时强制全画 */

       /* 统一量化成"要显示的整数", 便于比较 */
       t10 = (int)(temp * 10.0f + (temp >= 0.0f ? 0.5f : -0.5f));  /* 21.54 → 215 */
       hi  = (int)(humi + 
        0.5f);                                   /* 72.97 → 73  */
       pi  = (int)(press_x100 / 100);                              /* 102109 → 1021 */

       /* ---- 温度: 变了才画 ---- */
       if (!has_last || t10 != last_t10)
       {
           neg = (t10 < 0);
           a   = neg ? -t10 : t10;
           snprintf(buf, sizeof(buf), "%s%d.%d", neg ? "-" : "", a / 10, a % 10);
           draw_temp(TXT2_X, 186, TXT3_W, buf, C_BLUE);
           last_t10 = t10;
       }

       /* ---- 湿度: 变了才画 ---- */
       if (!has_last || hi != last_humi)
       {
           snprintf(buf, sizeof(buf), "%d%%RH", hi);
           center_to(buf, 6);
           DrawString(TXT2_X, 224, buf, C_BLUE, C_CARD);
           last_humi = hi;
       }

       /* ---- 气压: 变了才画 ---- */
       if (!has_last || pi != last_press)
       {
           snprintf(buf, sizeof(buf), "%dmb", pi);
           center_to(buf, 6);
           DrawString(TXT2_X, 262, buf, C_BLUE, C_CARD);
           last_press = pi;
       }

       has_last = true;
   } 
   void ui_update_time(int hour, int min, int sec){
    static char last[9] = "        ";      /* 8 个空格 */
        char buf[9];
       snprintf(buf, sizeof(buf), "%02d:%02d:%02d",hour, min, sec); 
        for (int i = 0; i < 8; i++) {
              if (buf[i] != last[i]) {           /* 只处理变了的位 */
                  DrawCharBig(10 + i * 28, 42, buf[i], C_BLUE, C_CARD);
                  last[i] = buf[i];              /* ← 别忘了更新记忆 */
              }
          }
 }

  void ui_update_date(int year, int mon, int day, int wday){
  static char last_date[16] = "";       /* 上次画过的日期字符串 (static: 程序全程保留) */
  static int  last_wday = -1;           /* 上次画过的星期; -1 = 还没画过的哨兵值 */
  char buf[16];

  snprintf(buf, sizeof(buf), "%04d.%02d.%02d", year, mon, day);

  /* 日期变了才重画 (strcmp 比内容, ==0 表示相同) */
  if (strcmp(buf, last_date) != 0) {
      DrawTextMid(12, 104, buf, C_TEXT, C_CARD);
      strcpy(last_date, buf);           /* 画完再更新记忆 */
  }

  /* 星期变了才重画 */
  if (wday != last_wday) {
      if (wday >= 0 && wday < 7) {      /* 先判越界, 再当下标 */
          DrawTextCN24(155, 104, TXT_WEEK[wday], C_TEXT, C_CARD);
      }
      last_wday = wday;
  }
}