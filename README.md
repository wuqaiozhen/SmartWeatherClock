# 智能天气时钟（STM32F407 + ESP32-S3 双芯）

> FreeRTOS 任务化固件 · 自研串口行协议 · 硬件 I2C 中断驱动 · 自研日志组件

---

## 1. 项目简介

| 项 | 内容 |
|---|---|
| 主控 | **STM32F407VGT6**（168MHz / 1MB Flash / 128KB RAM）+ **ESP32-S3**（WiFi） |
| RTOS | FreeRTOS（CMSIS-RTOS2 封装），4 个应用任务 + 1 个消息队列 |
| 显示 | ST7789 240×320（SPI2 @21MHz），5 套自建点阵字库 + 16 张彩色天气图标 |
| 传感器 | AHT20（温湿度）+ BMP280（气压），硬件 I2C + 中断方式 |
| 时间 | ESP32 SNTP 对时 → STM32 软时钟（差值走秒、闰年进位） |
| 联网 | ESP32 通过 HTTPS 拉取心知天气，行协议转发给 STM32 |
| 时钟源 | HSE 8MHz 晶振（±20ppm，每小时漂移约 0.07 秒） |

**分工原则**：把"联网/换算"这类重活放资源多的一侧（ESP32），把"显示/控制"放实时性好的一侧（STM32）。

---

## 2. 系统架构

```
                 UART2 (PA2/PA3, 115200 8N1)
┌──────────────┐  自研行协议 $W/$T/$E   ┌──────────────────────────────┐
│  ESP32-S3    │ ◄───────────────────► │  STM32F407                   │
│  WiFi / HTTPS│                       │                              │
│  SNTP 对时    │                       │  TaskComm → 队列 → TaskUI    │
└──────────────┘                       │       ↓              ↓       │
                                       │  TaskClock       SPI2→ST7789 │
       ┌──────────┐                    │  TaskSensor      I2C1→传感器  │
       │ AHT20    │◄── I2C1 ──────────►│                              │
       │ BMP280   │                    └──────────────────────────────┘
       └──────────┘
```

### STM32 侧软件分层

```
① 驱动层   esp32.c（串口字节↔行）  aht20.c/bmp280.c（I2C）
           st7789.c（SPI 屏）      uart.c（printf 重定向）
              ↓
② 业务层   app.c（4 个任务 + 命令事务 + 协议解析 + 消息投递）
              ↓ ui_msg_t
③ UI 层    ui.c（布局/差异重绘）  gui.c（绘制原语）  font*.c  icon*.c
              ↑
④ 时钟层   clock.c（软时钟）
⑤ 基础设施 log.c（日志组件）
```

---

## 3. 硬件接线

### 屏幕 ST7789 ↔ STM32

| 模块 | STM32 | 说明 |
|---|---|---|
| VDC | 3.3V（或 5V，取决于模块有无稳压芯片） | **电流要够（≥150mA）** |
| GND | GND | 必须共地 |
| SCL | **PB13** | SPI2_SCK |
| SDA | **PB15** | SPI2_MOSI |
| RES | **PB14** | 复位（初始化后拉高） |
| DC | **PC5** | 0=命令 / 1=数据 |
| CS | **PB12** | 片选（一直拉低） |
| BL | **PB1** | 背光 |

### ESP32 ↔ STM32

| ESP32 | STM32 |
|---|---|
| U0TXD (GPIO43) | **PA3**（USART2_RX） |
| U0RXD (GPIO44) | **PA2**（USART2_TX） |
| GND | GND（必须共地） |

### 传感器（I2C1）

| 信号 | STM32 |
|---|---|
| SCL | **PB6** |
| SDA | **PB7** |

AHT20 地址 `0x38`，BMP280 地址 `0x77`（同一对线上靠地址区分）。

### 调试串口

USART1（PA9/PA10）→ 板载 CH340 → PC 端 **COM7**，115200。

> ⚠️ **供电注意**：不要用 ST-Link 的 3V3 引脚给整块板供电（它只有几十 mA，会把电压拉到 2V 左右，MCU 和屏幕都会异常）。请用板子的 USB 口供电；烧录时 ST-Link 只接 `GND / SWDIO / SWCLK` 三根。

---

## 4. 目录结构与文件职责

### `Hardware/`（板级驱动 + 应用层）

| 文件 | 职责 |
|---|---|
| `app.c` / `app.h` | **4 个任务的实现** + `ESP32_Command()` 命令事务 + 协议解析 + 消息投递 + 资源监控 |
| `ui_msg.h` | UI 消息体（`struct + union + struct`，20 字节） |
| `esp32.c` / `.h` | USART2 驱动：中断接收（环形缓冲 + 两级缓冲 + ANSI 过滤）、**DMA 发送**、信号量等待 |
| `aht20.c` / `.h` | AHT20 驱动：**硬件 I2C + 中断（IT）+ 信号量**，错误码 `AHT20_E_TX/RX/BUSY` |
| `bmp280.c` / `.h` | BMP280 驱动（阻塞式 I2C + 补偿算法） |
| `clock.c` / `.h` | 软时钟：`Clock_Set / Clock_IsSet / Clock_Tick / Clock_Get`（差值走秒、闰年、`+=` 保余数） |
| `st7789.c` / `.h` | ST7789 驱动：SPI 命令/数据、窗口、填充、RGB888→RGB565 |
| `uart.c` / `.h` | `__io_putchar()`：printf 的字节出口 |
| `log.c` / `.h` | **日志组件**：覆盖 `_write` 弱符号 + 互斥量 + 等级/tag/时间戳/函数名行号 |

### `GUI/`（显示层）

| 文件 | 职责 |
|---|---|
| `ui.c` / `.h` | 布局 + `ui_init()` + `ui_update_time/date/weather/indoor`（差异重绘） |
| `gui.c` / `.h` | 绘制原语：`DrawChar / DrawString / DrawCharBig / DrawTextMid / DrawTextCN / DrawTextCN24 / DrawBitmap / DrawBitmapColor / FillRect` |
| `font*.c` | 字库：16×32 ASCII / 24×48 数字 / 12×24 ASCII / 16×16 中文 / 24×24 中文 |
| `icon_*.c` | 图标：WiFi(24×24) / ℃(16×32) / 天气(64×64 ×16 张 RGB565) |

### `Core/`（CubeMX 生成 + 用户代码）

| 文件 | 说明 |
|---|---|
| `main.c` | 初始化 + `app_start()`（只在 `USER CODE` 区改） |
| `freertos.c` | CubeMX 任务注册 + **栈溢出钩子**（已改为打印任务名 + 停机） |
| `stm32f4xx_it.c` | 中断向量：USART1/2、TIM6、I2C1_EV/ER、DMA1_Stream6、EXTI0 |
| `stm32f4xx_hal_timebase_tim.c` | HAL 1ms 时基用 TIM6（与 RTOS 的 SysTick 分离） |

### 根目录工具与文档

| 文件 | 用途 |
|---|---|
| `编译.cmd` / `烧录.cmd` / `看日志.cmd` | 常用的编译/烧录/串口工具（见第 5 节） |
| `生成后修复.cmd` | **每次 CubeMX GENERATE CODE 后必跑**：修 `main.c` 括号、检查 defaultTask 栈、把自有注释 ASCII 化、检查编码 |
| `检查编码.cmd` | 只检查中文注释有没有被 CubeMX 破坏 |
| `日志系统说明.md` | 日志组件设计、16 条 C 知识点、4 个踩坑、自测题 |
| `项目笔记.md` | 双芯项目总笔记：协议 v2.0、显示布局、关键设计、32 条踩坑清单 |
| `C与RTOS知识清单.md` | C 语言 / 嵌入式 / FreeRTOS / 项目设计 / 11 个踩坑 / 自检清单 |

---

## 5. 构建 / 烧录 / 看日志

### 编译

```powershell
cd D:\Code\SmartWeatherClock
$env:PATH = "C:\Users\DELL\AppData\Local\stm32cube\bundles\cmake\4.3.1+st.1\bin;C:\Users\DELL\AppData\Local\stm32cube\bundles\ninja\1.13.2+st.1\bin;D:\APP\14.3 rel1\bin;$env:PATH"
cmake --build build\Debug
```

或使用 VS Code 的 STM32 扩展 Build 按钮。

### 修改 CubeMX 配置后

```
① 改 SmartWeatherClock.ioc（不要手改 main.c 里的"配置性代码"，会被覆盖）
② GENERATE CODE
③ 双击 生成后修复.cmd     ← 必做
④ 编译
```

### 端口

| 端口 | 设备 |
|---|---|
| **COM7** | STM32 调试串口（板载 CH340） |
| **COM9** | ESP32 原生 USB（日志/烧录） |

---

## 6. 通信协议（STM32 ↔ ESP32，v2.0）

数据行以 `$` 开头、`\r\n` 结尾；命令全大写；逗号分隔。

### 命令（STM32 → ESP32）

| 命令 | 含义 |
|---|---|
| `PING` | 心跳 |
| `GET_WEATHER` | 要天气 |
| `GET_TIME` | 要对时 |

### 应答（ESP32 → STM32）

| 消息 | 含义 | 示例 |
|---|---|---|
| `$PONG` | 心跳应答 | |
| `$W,<温度x10>,<天气码>` | 天气 | `$W,180,1` = 18.0℃ 多云 |
| `$T,<年>,<月>,<日>,<时>,<分>,<秒>,<星期>` | 时间（北京时间，星期 0=周日） | `$T,2026,11,7,18,13,39,6` |
| `$E,<码>` | 错误 | `$E,2` 天气失败 / `$E,3` 时间未就绪 / `$E,9` 未知命令 |

### 请求策略

| 数据 | 未就绪时 | 已就绪后 |
|---|---|---|
| 时间 | 3 秒重试 | 1 小时 |
| 天气 | 30 秒重试 | 10 分钟 |

---

## 7. 日志系统

```c
LOGE("SENSOR", "AHT20 err=%d\r\n", err);      /* 错误 + 函数名/行号 */
LOGW("COMM",   "queue full\r\n");             /* 警告 + 函数名/行号 */
LOGI("CLOCK",  "put %02d:%02d:%02d\r\n", h,m,s);  /* 信息 */
LOGD("RX",     "raw: %s\r\n", line);          /* 调试（默认不编进固件） */
```

输出格式：

```
[18:13:40][I][CLOCK] put 18:13:40
[16:56:01][E][SENSOR] TaskSensor:315 AHT20 failed
```

等级开关（`Hardware/log.h`）：

```c
#define LOG_LEVEL_MIN LOG_LVL_INFO   /* DEBUG=0 < INFO=1 < WARN=2 < ERROR=3 */
```

细节见 `日志系统说明.md`。

---

## 8. FreeRTOS 任务与资源

| 任务 | 优先级 | 栈 | 周期/触发 | 职责 |
|---|---|---|---|---|
| `TaskClock` | High(40) | 1KB | 100ms 轮询 | `Clock_Tick` → 投 `UI_MSG_TIME` |
| `TaskUI` | AboveNormal(32) | 4KB | 队列阻塞 | **唯一**操作 ST7789 的任务 |
| `TaskComm` | Normal(24) | 3KB | 信号量 + 1s 超时 | 收行/解析/投消息、周期发请求 |
| `TaskSensor` | BelowNormal(16) | 4KB | 2s (`osDelayUntil`) | AHT20 + BMP280 → 投 `UI_MSG_INDOOR` |
| `defaultTask` | Normal(24) | **2KB** | 10s | 资源监控（`[MON]` 三行） |

**消息队列**：`ui_q`，深度 8，每条 20 字节。

### 关键设计

| 设计 | 解决的问题 |
|---|---|
| **UI 单线程持有 SPI** | 不需要互斥量，不可能优先级翻转 |
| **struct+union+struct 消息体** | 一个队列承载 4 类 UI 事务 |
| **差异重绘** | 定宽输出 + 逐位比较 → 秒跳不闪 |
| **中断收 + DMA 发** | 唤醒从 50 次/秒降到 1~2 次/秒；发送不阻塞 CPU |
| **硬件 I2C + IT** | 80ms 测量期间任务 Blocked（原 `HAL_Delay` 空转） |

### 当前资源占用（实测）

```
FLASH 225 KB / 1 MB  (21.5%)
RAM    50 KB / 128 KB (38.7%)
heap free  ≈ 5064 B
栈剩余: clock 464/1024, ui 3480/4096, comm 1680/3072, sensor 3656/4096
```

---

## 9. 当前状态

### ✅ 已完成并验证

- ESP32：WiFi + HTTPS 天气 + SNTP 对时 + 行协议
- STM32：4 任务 + 队列 + 消息驱动 UI
- 串口：中断接收（信号量）+ **DMA 发送** + `ESP32_Command` 事务（超时含 TX 时间）
- 时间：软时钟走秒/闰年/对时策略，每秒上屏（日志可见）
- 天气/室内数据：解析 → 上屏
- 传感器：AHT20 硬件 I2C + 中断方式
- 日志组件：互斥 + 等级 + tag + 时间戳 + 函数/行号 + 编译期过滤
- 时钟源：HSE 8MHz
- 调试基建：栈水位/任务状态监控、栈溢出钩子、CubeMX 生成修复脚本

### ❌ 未完成

| 项 | 原因 / 说明 |
|---|---|
| **屏幕显示** | 软件侧已完全排除（`spi_err=0`、全屏填充无失败、速率/时序/软复位都试过）；硬件侧发现过供电不足（2.2V）、疑似模块损坏，待换屏验证 |
| SPI DMA 刷屏 | 依赖屏幕；且当前"差异重绘"每帧仅 2KB，收益有限 |
| 多页面 UI（欢迎/WiFi/错误页） | 设计已成型（页面状态机 + 描述表 + 全量重绘/差异更新），未实现 |
| STM32 IAP/Bootloader（OTA） | 未做；分区方案已规划（Boot 64KB / APP 448KB / 下载区 512KB） |

---

## 10. 已知坑（详见 `C与RTOS知识清单.md`）

| # | 坑 |
|---|---|
| 1 | 任务循环里没有阻塞点 → 低优先级任务被饿死 |
| 2 | `printf` 不可重入 → 只锁 `_write` 挡不住交错，要锁住整个 printf |
| 3 | 日志等级数值方向与比较符号不一致 → 错误全被过滤 |
| 4 | CubeMX 吞掉 `main.c` 的右括号 / 破坏 UTF-8 中文注释 |
| 5 | 栈溢出 → 报错处不是出事处（`printf` 一次要 400~500 字节栈） |
| 6 | 任务函数里 `return` = 任务自杀 |
| 7 | HAL 时基被 SysTick 抢走（已改 TIM6） |
| 8 | 弱符号覆盖（`_write`、HAL 回调） |
| 9 | **调度器启动前不能调用阻塞 API**（`osSemaphoreAcquire` 等） |
| 10 | **ST-Link 的 3V3 不是电源**（几十 mA，带不动 MCU+屏幕） |
| 11 | 背光亮 ≠ 模块有电（背光 LED 只需 2V，逻辑要 ≥2.3V） |

---

## 11. 仓库分支（两颗芯片分开存放）

| 分支 | 固件 | 构建方式 |
|---|---|---|
| `main` | STM32F407 时钟（CubeMX + CMake + FreeRTOS） | 本分支根目录 `CMakeLists.txt` / `编译.cmd` |
| `esp32` | ESP32-S3 联网（ESP-IDF v5.4.4） | 切到 `esp32` 分支后 `idf.py build` |

ESP32 侧源码位于 `main/`：`station_example_main.c`（uart/net 双任务）、`uart_proto.c`（行协议）、`wifi.c`、`weather.c`、`ntp.c`，配置见 `sdkconfig`。

```bash
git fetch origin
git switch -c esp32 origin/esp32
```

两分支历史独立，互不影响；协议与引脚定义见 `项目笔记.md`。
