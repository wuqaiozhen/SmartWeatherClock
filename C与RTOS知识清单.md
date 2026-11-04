# C 与 RTOS 知识清单（本项目相关）

> 用途：本项目（FreeRTOS 智能天气时钟）涉及的知识点汇总 + 踩坑记录。
> 每条都尽量配"本项目里的真实例子"，方便对照代码回忆。

---

# 第一部分：C 语言

## 1. 变量与类型

| 类型 | 含义 | 项目里的例子 |
|---|---|---|
| `int` | 整数（可正可负） | `int hour = 12;` |
| `float` | 小数 | `float t, h;` |
| `char` | 1 个字符 / 1 个字节 | `char line[128];` |
| `uint8_t` | 无符号 8 位（0~255） | `static uint8_t s_rx_buf[512];` |
| `uint16_t` | 无符号 16 位 | `s_rx_head`（环形缓冲下标） |
| **`uint32_t`** | 无符号 32 位（0~42 亿） | `HAL_GetTick()` 的毫秒数 |
| `bool` | `true` / `false` | `static bool s_weather_ok;` |

**为什么用 `uint32_t`**：`int` 在不同平台宽度不同；`uint32_t` 明确"正好 32 位"。
时间用无符号的额外好处：**两个 `uint32_t` 相减，即使计数器回绕（约 49 天）结果仍然正确**。

---

## 2. 指针（C 语言最大的坎）

```c
int  a = 5;
int *p = &a;      /* p 存的是 a 的地址; & 取地址 */
*p = 7;           /* * 顺着地址找到变量, a 变成 7 */
```

| 写法 | 读法 |
|---|---|
| `&x` | x 的地址 |
| `*p` | p 指向的那个东西 |
| `char *s` | 指向字符的指针 = **一段字符串** |

**项目里的两个高频用法**：

```c
/* ① 数组名本身就是地址 */
char line[128];
ESP32_GetLine(line, sizeof(line));      /* 不用写 &line */

/* ② 想让函数改外部变量, 必须传地址 (C 的参数是值拷贝) */
bool Clock_Tick(clock_time_t *out) {
    *out = s_now;                       /* 通过地址写回去 */
    return true;
}
Clock_Tick(&t);                         /* 调用时取地址 */
```

---

## 3. 数组与字符串

```c
char line[128];        /* 128 个格子, 最多存 127 字符 + 结束符 '\0' */
```
**C 字符串必须以 `'\0'`（数字 0）结尾**，否则 `printf`/`strlen` 会一直读到越界。

| 函数 | 注意点 | 项目里的用法 |
|---|---|---|
| `strlen(s)` | 不含 `\0` | `int explen = (int)strlen(expect);` |
| `strncmp(a,b,n)` | **相等返回 0**，不是 1 | `strncmp(line, "$T", 2) == 0` |
| `strncpy(d,s,n)` | **不保证补 `\0`** | `strncpy(resp, line, len-1); resp[len-1]='\0';` |
| `snprintf(b,n,f,...)` | 安全版 sprintf | `snprintf(buf, sizeof(buf), "%02d:%02d", h, m);` |
| `sscanf(s,f,...)` | 从字符串解析 | `sscanf(line, "$T,%d,%d,...", &year, ...) != 7` |

---

## 4. 结构体 / typedef / union

```c
typedef struct {
    int year, mon, day, hour, min, sec, wday;
} clock_time_t;                    /* typedef: 以后不用写 struct */
```

**访问成员**：变量用 `.`，指针用 `->`

```c
clock_time_t t;   t.hour = 12;
clock_time_t *p = &t;   p->hour = 12;
```

**指定初始化器**（顺序无关，没写的自动为 0）

```c
clock_time_t t = { .year = year, .mon = mon, .sec = sec };
```

**union（联合体）：所有成员共用同一块内存**

```c
typedef struct {
    ui_msg_type_t type;
    union {
        struct { int hour, min, sec; } time;
        struct { int year, mon, day, wday; } date;
        struct { float temp, humi; uint32_t press_hpa; } indoor;
    } u;
} ui_msg_t;
```

| | struct | union |
|---|---|---|
| 内存 | 各成员独立，大小相加 | 共用一份，大小 = 最大成员 |
| 用途 | 同时存在的数据 | **同一时刻只有一种有效** |

`ui_msg_t` 一共只有 **20 字节**，一个队列就能承载 4 类消息。

---

## 5. 枚举 enum

```c
typedef enum { UI_MSG_TIME = 0, UI_MSG_DATE, UI_MSG_WEATHER, UI_MSG_INDOOR } ui_msg_type_t;
```
给整数起名字。**注意：`#if` 里不能用枚举，只能用 `#define`**（这是日志等级用宏的原因）。

---

## 6. 宏与预处理

| 形式 | 例子 | 说明 |
|---|---|---|
| 无参宏 | `#define WEATHER_PERIOD_MS 600000u` | 纯文本替换 |
| 有参宏 | `#define LOGI(tag, ...)` | `...` = 不定参数 |
| `__VA_ARGS__` | `log_print(..., __VA_ARGS__)` | "把收到的参数原样放这里" |
| 条件编译 | `#if (X < 60000u) #warning ... #endif` | 编译期判断 |
| 宏体 `do{}while(0)` | 所有多语句宏 | 让宏成为"一条语句"，`if/else` 里安全 |

**宏 vs 函数**：宏在**预处理阶段**展开，能拿到 `__LINE__`/`__func__`、能由编译器算出常量并删掉死代码；函数做不到。

---

## 7. 函数

### 声明 vs 定义

```c
void f(void);          /* 声明(原型): 分号结尾 */
void f(void) { ... }   /* 定义: 直接 { */
```
**声明和定义的参数顺序、类型必须完全一致**（编译器只按位置匹配，不看名字）。

### `static` 三种用法

| 位置 | 含义 | 例子 |
|---|---|---|
| 文件作用域变量 | **只本文件可见** | `static osMessageQueueId_t ui_q;` |
| 函数前 | 函数只本文件可见 | `static void TaskSensor(void *arg)` |
| 函数内局部变量 | **生存期变成整个程序**，只初始化一次 | `static char last[9];`（差异重绘的记忆） |

### `const`

只读 + 放 Flash（`.rodata`）不占 RAM + 接口契约（我保证不改你的数据）。

### 可变参数

```c
void log_print(int lvl, const char *tag, const char *func, int line,
               const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);     /* 以【最后一个具名参数】为锚点, 算出可变参数起始地址 */
    vprintf(fmt, ap);      /* vprintf 才能接受"打包好的参数" */
    va_end(ap);
}
```

| 函数 | 参数形式 |
|---|---|
| `printf(fmt, a, b, c)` | 参数**散开** |
| `vprintf(fmt, ap)` | 参数**打包**（`va_list`） |

---

## 8. 语句控制流

| 语句 | 注意 |
|---|---|
| `for (;;)` | 死循环（三个位置都空） |
| `while (expr)` | 非 0 就继续 → `while (ESP32_GetLine(...))` = "取空缓冲" |
| `switch/case/break` | 忘了 `break` 会贯穿 |
| **`continue`** | **跳过本轮剩余部分，开始下一轮**（不是"继续往下"） |
| `break` | 跳出整个循环/switch |
| `return` | 结束函数；**任务的 `return` = 任务死亡** |

---

## 9. 运算符

| 运算符 | 注意 |
|---|---|
| `==` vs `=` | 比较 vs 赋值 |
| `&&` `\|\|` `!` | 逻辑与/或/非；`\|\|` 左边真就不算右边（短路） |
| `& \| << >>` | 位运算：RGB888→RGB565 取位、环形缓冲 `% 512` |
| `?:` | 三目：`period = ok ? A : B` |
| `(类型)x` | 强制转换：`(int)st`、`(uint8_t *)ptr` |

**RGB565 压缩**：

```c
(uint16_t)(((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F))
/*           红 5 位                  绿 6 位                  蓝 5 位 */
```

---

## 10. 编译 vs 链接

```
① 编译: 每个 .c 单独编译 → .o, 只检查语法/声明
② 链接: 把所有 .o 和库拼起来, 检查"符号有没有实现"
```

| 报错 | 阶段 | 含义 |
|---|---|---|
| `expected ';' before ...` | 编译 | 语法错 |
| `implicit declaration of 'xxx'` | 编译 | 没声明（忘 include） |
| **`undefined reference to 'xxx'`** | **链接** | 声明有、实现没有 / 名字拼错（大小写） |
| `multiple definition` | 链接 | 同一符号定义两次 |

### 弱符号（weak）

```c
/* syscalls.c: __attribute__((weak)) int _write(...)   ← 备胎 */
/* log.c:      int _write(...)                          ← 正牌 */
```
链接器规则：**同名符号有强符号就用强符号**。所以能在自己的文件里接管 C 库的输出接口，且 CubeMX 重新生成也覆盖不掉。

---

# 第二部分：嵌入式基础

## 1. 中断

- **轮询**：一直问"好了吗" ｜ **中断**：有事才叫你
- `HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1)` 启动一次接收；收满 1 字节触发回调
- 回调里**必须重新 arm**，否则再也收不到（且不报错）

**波特率**：115200 → 1 字节(10 bit) ≈ **87 µs**

---

## 2. 环形缓冲区

```
        head(ISR 写)              tail(任务读)
          ↓                          ↓
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
  │ a │ b │ c │   │   │ x │ y │ z │
  └───┴───┴───┴───┴───┴───┴───┴───┘
                        ↑ 读到头就绕回开头 (% 512)
```
不用搬数据，只移动下标。

## 3. 两层缓冲（行协议必备）

```
HAL 驱动缓冲 → 自己的 static 缓冲 s_rx[64] → 行缓冲 s_line[256] → 一整行
```
**为什么**：一次读操作可能读到**两条命令**；处理完第一条就 return，剩下的会丢在栈上的临时变量里。

## 4. I2C / SPI

| | I2C | SPI |
|---|---|---|
| 线 | SDA + SCL | SCK + MOSI + MISO + CS |
| 速度 | 100~400 k | 几 MHz ~ 几十 MHz |
| 多设备 | 靠**地址** | 靠**CS 片选** |
| 项目里 | AHT20(0x38)、BMP280(0x77) | ST7789 屏幕 |

**ST7789 两根控制线**：`DC`（0=命令 / 1=数据）、`CS`（低电平选中）。

## 5. 时钟

```
HSI 16MHz(内部RC ±1%)  ┐
                       ├→ PLL → SYSCLK 168MHz → APB1 42MHz → SPI2
HSE 8MHz(晶振 ±20ppm)  ┘
```

| 名词 | 含义 |
|---|---|
| HSI | 内部 RC，精度差（±1% → 每小时差 36 秒） |
| HSE | 外部晶振，精度高（±20ppm → 每小时 0.07 秒） |
| PLL | 倍频：8MHz / M × N / P = 168MHz |
| SysTick | 内核自带 1ms 定时器（被 RTOS 用作心跳） |
| TIM6 | HAL 时基（`HAL_GetTick()` 靠它 +1） |
| tick | RTOS 心跳，1ms 一次 |

---

# 第三部分：FreeRTOS

## 1. 任务

```c
static void TaskClock(void *argument)   /* 签名固定, 永不返回 */
{
    (void)argument;
    for (;;) { ...干活...; osDelay(100); }
}

static const osThreadAttr_t clock_attr = {
    .name = "clock", .stack_size = 256 * 4, .priority = osPriorityHigh,
};
osThreadNew(TaskClock, NULL, &clock_attr);
```

**三要素**：函数 + 栈大小（**单位是字节**）+ 优先级。

## 2. 四种状态

| 状态 | CMSIS 值 | 含义 |
|---|---|---|
| Running | 2 | 正在用 CPU |
| **Ready** | **1** | 想跑，排队等 CPU |
| **Blocked** | **3** | 在睡（延时/队列/信号量）→ **健康状态** |
| Terminated | 4 | 已结束（任务函数 return 了） |

## 3. 优先级与抢占

```
clock 40 > ui 32 > comm 24 > sensor 16 > idle 0
```

- **抢占**：高优先级一就绪立刻抢 CPU
- **同优先级**：轮流跑（每 tick 换一次）
- **铁律**：`for(;;)` 里必须有阻塞语句，否则**饿死低优先级任务**

## 4. 栈与堆

```
栈 = 每个任务自己的"草稿纸"（局部变量、函数返回地址）
堆 = FreeRTOS 管理的公共内存（任务栈、队列、信号量都从这里切）
```

**栈溢出的检查**：
```c
osThreadGetStackSpace(handle)   /* 返回还剩多少字节 */
```
判断标准：**剩余 < 40% 就扩容**。

## 5. 队列

```c
ui_q = osMessageQueueNew(8, sizeof(ui_msg_t), NULL);
osMessageQueuePut(ui_q, &msg, 0, 0);                   /* 发送不等待 */
osMessageQueueGet(ui_q, &msg, NULL, osWaitForever);    /* 等到有为止 */
```
- **值拷贝**：放进去的是副本，生产者改自己的变量不影响队列
- 空就睡：`osWaitForever` → 不占 CPU

## 6. 信号量

```c
s_rx_sem = osSemaphoreNew(1, 0, NULL);   /* 最大1, 初始0 = 二值信号量 */
osSemaphoreRelease(s_rx_sem);            /* 中断里按门铃 */
osSemaphoreAcquire(s_rx_sem, timeout);   /* 任务里睡等门铃 */
```

内部机制：

```
Release: 有人在睡 → 当场唤醒(计数保持0);  没人在睡 → 计数+1(不超过最大)
Acquire: 计数>0 → 减1立刻返回;            计数=0 → 挂起自己
```

| 类型 | 用途 |
|---|---|
| 二值 | "有没有数据" 这种**状态** |
| 计数 | "发生了几次" 这种**数量** |

## 7. 互斥量（Mutex）

```c
s_log_mutex = osMutexNew(NULL);      /* 默认属性含【优先级继承】 */
```

| | 信号量 | 互斥量 |
|---|---|---|
| 语义 | 通知事件 | 保护共享资源 |
| 所有者 | 谁都能 release | 只有持有者能 release |
| 优先级继承 | ❌ | ✅ |
| 中断里能用 | ✅ | ❌ |

**优先级翻转**：低优先级持锁 → 高优先级等锁 → 中优先级抢占低优先级 → 高优先级被间接拖住。**互斥量的优先级继承**能自动缓解。

**非递归锁**：同一任务重复 acquire 会**死锁**（所以 `_write` 里不能再加锁）。

## 8. 延时 API

| API | 语义 | 用在哪 |
|---|---|---|
| `osDelay(ms)` | **至少**睡 ms | 不关心周期精度 |
| `osDelayUntil(tick)` | 睡到**指定时刻**（周期严格） | 周期采样 |
| `HAL_Delay(ms)` | ❌ **忙等**，不释放 CPU | 只能在调度器启动前用 |

## 9. 三种经典 bug

| bug | 症状 | 根因 | 修法 |
|---|---|---|---|
| **任务饿死** | 某任务完全不动，栈水位不涨 | 别的任务不睡 | 给霸占者补阻塞语句 |
| **死锁** | 全系统卡住 | 非递归锁重复获取 | 只在一层加锁 |
| **栈溢出** | 随机 HardFault / 静默死机 | 栈给小了（`printf` 是最大杀手） | 实测后扩容 |

---

# 第四部分：项目核心设计（复习卡片）

| # | 设计 | 解决什么问题 |
|---|---|---|
| 1 | 双 MCU 分工 | ESP32 干联网（重、非实时），STM32 干显示（轻、实时） |
| 2 | 行协议 + **两层缓冲** | 串口字节流 → 结构化命令行；防止多条命令一次到达时丢数据 |
| 3 | **struct+union+struct 消息体** | 一个队列承载 4 类 UI 事务，`switch` 单点分发，省内存 |
| 4 | **UI 单线程持有 SPI** | 只有 `TaskUI` 碰屏幕 → 不需要互斥量 → 不可能优先级翻转 |
| 5 | **差异重绘** | 定宽输出 + 只重画变了的字符 → 秒针跳不闪 |
| 6 | **中断驱动 + 信号量** | 唤醒次数从 50 次/秒降到 1~2 次/秒 |
| 7 | **`ESP32_Command` 事务封装** | 发送+等待+前缀匹配+超时（**含 TX 时间**），非期望行不丢 |
| 8 | **日志组件** | 弱符号覆盖 `_write` + 互斥 + 等级 + tag + 时间戳 + 函数/行号 + 编译期过滤 |

---

# 第五部分：踩坑记录

## 坑 1：任务饿死（comm 不睡 → sensor 死）

**现象**：`[UI] indoor` 一条都不出；`osThreadGetState(sensor)` 长期 = 1（Ready）；`sensor` 栈水位几乎没动（只用了 44 字节）。

**原因**：`TaskComm` 循环里漏了 `ESP32_WaitData(1000)` → 优先级 24 的任务永远 Ready → 优先级 16 的 sensor 永远排不上队。

**诊断**：把受害者临时提到最高优先级，立刻正常 → 确认是饿死。

**修法**：给霸占 CPU 的任务补阻塞语句（**治本**），而不是提高受害者优先级（治标）。

---

## 坑 2：多任务 printf 交错

**现象**：
```
[COMM] TX: free = 6888 B
ot)
[MON] heap free = 6888oot)[COMM] TX:  left: clock=840 ...
```
（`(boot)` 变成 `ot)`，内容被覆盖丢失）

**原因一**：`printf` 分两步 —— ①格式化写入**全局 stdout 缓冲区** ②调 `_write` 输出。只给 `_write` 加锁，①仍然裸奔 → 缓冲区被别的任务踩坏。

**原因二**：newlib 一次 `printf` 可能调用 `_write` **多次**（分块冲刷），锁在 `_write` 只能保证"每块"原子，块之间会被插入。

**修法**：把锁提到"整次 printf"这一层（`log_print` 用 `va_list` + `vprintf` 全程持锁），**`_write` 里不再加锁**（非递归锁会死锁）。

---

## 坑 3：日志等级方向与比较符号不一致

```c
#define LOG_LVL_ERROR 0 ... #define LOG_LVL_DEBUG 3    /* 数值越大越详细 */
#define LOGW(tag, ...) if (LOG_LVL_WARN >= LOG_LEVEL_MIN) ...   /* 用了 >= */
```
**现象**：警告和错误全被过滤，调试信息反而刷屏。

**原因**：`1 >= 2` 为假 → WARN(1) 被过滤；`3 >= 2` 为真 → DEBUG(3) 输出。**编译器不报错**（逻辑合法，语义反了）。

**修法**：改成"数值越大越**严重**"（`DEBUG=0 … ERROR=3`），与 `>=` 一致。这也是 syslog / Android logcat 的通用约定。

---

## 坑 4：`log_printf` 把"干活"写进了 `if (锁 != NULL)` 里

```c
if (s_log_mutex != NULL) {        /* ❌ */
    osMutexAcquire(...);
    vprintf(...);                 /* 锁为 NULL 时什么都不打印 */
    osMutexRelease(...);
}
```
**现象**：调度器启动前（锁还没创建）的日志全部静默丢失。

**修法**：**"有锁就加锁 / 无论如何都干活 / 有锁就解锁"** 三段分开写。

---

## 坑 5：CubeMX 吞掉 `main.c` 的右括号（改一次代码报 20 条错）

**现象**：
```
main.c:204: warning: ISO C forbids nested functions
main.c:250: error: invalid storage class for function 'MX_I2C1_Init'
main.c:501: error: expected declaration or statement at end of input
```
**原因**：`USER CODE END 3` 标记位置不对，CubeMX 重新生成时把 `while(1)` 的 `}` 吞掉 → `main()` 没闭合 → 后面所有函数变成"嵌套函数"。

**修法**：把 `USER CODE BEGIN 3 / END 3` 之间的内容清空，`END 3` 后面必须有 `}`（关 while）和 `}`（关 main）。
**工具**：`生成后修复.cmd`（自动修括号 + 检查中文注释编码）。

---

## 坑 6：CubeMX 破坏 UTF-8 中文注释

**现象**：`/* 把 ESP32 的一整行日志 */` 变成 `/* ? ESP32 的一整行日志/数据转�? */`（乱码 + 字节丢失）。

**原因**：CubeMX 按 Windows 系统 ANSI(GBK) 读写文件，UTF-8 中文被误解码。

**修法**：
1. 每次 `GENERATE CODE` 后跑 `生成后修复.cmd`（或 `检查编码.cmd`）
2. 中文注释尽量写在**自己新建的文件**里（CubeMX 不碰）

---

## 坑 7 ⭐：栈溢出 —— 报错的地方不是出事的地方

### 现象

```
[--:--:--][I][APP] queue ok, item=20 bytes
[--:--:--][I][COMM] TX: GET_WEATHER (boot)
[--:--:--][           ← 日志停在半行, 之后再也没有输出
```

另一次崩溃时的 `[MON]` 报告：

```
[MON] stack left: clock=824 ui=3848 comm=2276 sensor=0
                                                      ↑ 一个字节都不剩
```

**矛盾点**：`sensor` 自己的代码实测只用 384/4096 字节（正常时显示 `sensor=3712`），怎么会突然"一个字节都不剩"？

### 真相

**`defaultTask` 的栈只有 512 字节**（CubeMX 默认 `.stack_size = 128 * 4`），而它在执行：

```
StartDefaultTask
  → app_report()             ~30 B
    → log_print()            ~90 B   (5 个参数 + clock_time_t now 28B + va_list)
      → printf / vprintf     ~300~400 B   ← 杀手
        → _write → __io_putchar → HAL_UART_Transmit   ~100 B
        ────────────────────────────────
        合计 ≈ 550~650 B  >  512 B   →  溢出!
```

**栈溢出会写坏"隔壁"的内存**（所有任务栈都从同一块 FreeRTOS 堆里切出来，彼此相邻）：

```
[ defaultTask 栈 512B ][ sensor TCB ][ sensor 栈底 0xA5 标记 ]
            ↓ 溢出往下写
[        被写穿        ][ 被踩坏     ][ 标记被覆盖 → sensor=0 ]
```

**所以 `sensor=0` 是"被撞的车"，`defaultTask` 才是"肇事车"。**

### 为什么最后整个系统静默

栈被踩坏 → 函数的**返回地址**变成垃圾 → 跳到非法地址 → **HardFault** → `HardFault_Handler` 里 `while(1)` → 串口再也不输出一个字。

### 为什么日志停在 `[--:--:--][`

`log_print` 内部是三次 printf：

```c
printf("[%02d:%02d:%02d]", ...);    /* ① 打出来了 → 此时栈已越界 */
printf("[%c][%s] ", ...);           /* ② 一进来彻底用穿 → 返回地址被写坏 → HardFault */
vprintf(fmt, ap);                   /* ③ 没执行到 */
```

**"半行日志"就是崩溃点的指纹。**

### 修法（3 处）

| 改动 | 解决什么 |
|---|---|
| `defaultTask` 栈 `512B → 2KB` | **根治**：不再溢出 |
| 栈溢出钩子：空函数 → 打印任务名 + 停机 | 原来"报警器响了没人管"，现场被破坏 |
| 钩子改用 `__io_putchar`（不用 `printf`） | 钩子运行时栈已经坏了，`printf` 要 400+ 字节栈 → 二次崩溃 |

### 验证闭环

改完后：

```
[MON] heap free = 5152 B          ← 6688 - 1536 = 5152 ✅ 正好等于扩容的 1.5KB
[MON] stack left: ... sensor=4052 ← 从 0 恢复到 4052 ✅ 邻居不再被踩
```

### 规则（重点）

1. **会调 `printf` 的任务，栈 ≥ 1KB，稳妥 2KB**
2. **`osThreadGetStackSpace` 长期监控，剩余 < 40% 就扩容**
3. ⭐ **栈溢出钩子里的代码本身必须"不占栈"**：
   - ❌ 不能调 `printf`（要 400~500 字节栈，会在已损坏的栈上二次崩溃）
   - ⚠️ 可以用 `__io_putchar`（约 20~40 字节栈）
   - ✅ 最安全是**直接写寄存器**（0 栈）：
     ```c
     while ((USART1->SR & USART_SR_TXE) == 0) { }
     USART1->DR = (uint8_t)ch;
     ```
   - 也可以干脆不打印，直接 `__disable_irq(); NVIC_SystemReset();`
4. **CubeMX 默认的 `defaultTask` 栈 512 字节一定要改** —— 它只能跑空循环
5. **看到"某个无辜任务的栈水位异常"，先怀疑它旁边的邻居**

> 一句话：**栈溢出就像车祸 —— 肇事车停在原地，被撞的车飞出去了。**
> 而 `printf` 是嵌入式里最隐蔽的"栈杀手"，一次就要 400~500 字节。

---

## 坑 8：任务函数里用 `return` = 自杀

```c
static void TaskClock(void *argument) {
    sec++;
    if (sec < 60) return;      /* ❌ 任务直接结束 → FreeRTOS 进错误处理, 关中断死循环 */
}
```
**修法**：把"提前退出"的逻辑抽成**普通函数**（普通函数里 `return` 是合法的卫语句）：

```c
static void clock_add_1s(void) { sec++; if (sec < 60) return; ... }   /* ✅ */
static void TaskClock(void *arg) { for (;;) { clock_add_1s(); osDelay(100); } }
```

---

## 坑 9：`HAL_GetTick()` 时基被 SysTick 抢走

FreeRTOS 默认用 SysTick 做调度心跳。若 HAL 时基也设 SysTick：

- RTOS 会把 SysTick 优先级压到最低（15）
- 临界区（`taskENTER_CRITICAL`）会屏蔽它 → `HAL_GetTick()` 不增长 → `HAL_Delay()` / 所有 HAL 超时**永久挂死**

**修法**：CubeMX → `SYS → Timebase Source` 改成 **TIM6**。

---

## 坑 10：弱符号 / 链接期才能发现的错误

```c
log.h 声明: void log_Init(void);
log.c 定义: void Log_Init(void)      ← 大写 L
app.c 调用: log_Init();              ← 小写 l
```
编译通过（声明匹配），**链接报 `undefined reference to 'log_Init'`**。

**记忆**：`undefined reference` = "声明有、实现没 / 名字拼错"。

---

# 第六部分：自检清单

看到这些现象，先想到这些地方：

| 现象 | 大概率原因 | 先查什么 |
|---|---|---|
| `undefined reference to 'xxx'` | 链接期：实现缺失 / 名字大小写 | 定义是否存在、拼写是否一致 |
| `implicit declaration of 'xxx'` | 编译期：没 include 头文件 | `#include` |
| `nested functions` / `invalid storage class` / `expected ... at end of input` | **花括号少了** | 文件末尾数括号（CubeMX 生成后常见） |
| 某个任务完全不动 | **任务饿死** | `osThreadGetState` 长期 = 1？ |
| 全系统静默 | HardFault / 死锁 / 栈溢出 | 有没有半行日志？`heap/stack` 报告异常？ |
| 日志交错 / 内容被吃掉 | printf 不可重入 | 锁的层级是否包住整个 printf |
| 某个任务栈水位 = 0 | **被邻居踩了**（不是它自己） | 查隔壁任务的栈是否太小 |
| 串口收不到数据 | 中断没重新 arm / 对端不在线 | `[RX]` 有没有、ESP32 断电没 |
| 定时任务不触发 | `now` 未初始化 / 周期宏写错 | 编译警告、宏的数值 |
| 时间戳一直 `[--:--:--]` | 时钟从未校准（没收到 `$T`） | ESP32 在线吗 |

---

# 附录：工具脚本

| 脚本 | 用途 |
|---|---|
| `编译.cmd` | 编译，输出写 `build.log` |
| `烧录.cmd` | 烧录（自动处理串口占用） |
| `看日志.cmd` | 看日志（COM9 = ESP32，COM7 = STM32） |
| `检查编码.cmd` | 检查 CubeMX 是否把中文注释搞乱 |
| **`生成后修复.cmd`** | **每次 CubeMX GENERATE CODE 后必跑**：修 `main.c` 括号 + 检查编码 |

---

# 附录：常用调试数字

| 指标 | 正常值 | 说明 |
|---|---|---|
| `heap free` | ~5152 B | 20KB 堆，5 个任务 + 队列 + 信号量 |
| `clock` 栈剩余 | 464 / 1024 | 用了 55%（会 printf） |
| `ui` 栈剩余 | 3380 / 4096 | 富余 |
| `comm` 栈剩余 | 1680 / 3072 | 用了 45%（`sscanf`+`printf`） |
| `sensor` 栈剩余 | 3688 / 4096 | 富余 |
| 任务状态 | `3/3/3/3`（偶尔 1） | 全是 Blocked = 健康 |
| 日志唤醒率（改造后） | 1~2 次/秒 | 改造前 50 次/秒 |
