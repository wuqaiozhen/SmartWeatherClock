import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM9"
BAUD = 115200

RETRY_GAP = 0.4        # 每次重连间隔(秒)
RETRY_TIMES = 30       # 单次断开后最多尝试次数


def open_port():
    return serial.Serial(port, BAUD, timeout=0.2)


def banner():
    print("=" * 56)
    print("  读取 %s (%d) ..." % (port, BAUD))
    print("  可以用鼠标选中复制")
    print("  退出: Ctrl + C")
    print("  烧录前请先按 Ctrl+C 退出本窗口 (串口是独占的)")
    print("=" * 56)


banner()

# ---------- 首次连接 ----------
#  打不开就直接退出, 绝不"偷偷重试", 否则会把端口从烧录程序那里抢走
try:
    s = open_port()
except PermissionError:
    print("")
    print("[%s 被占用] 另一个程序正在使用它" % port)
    print("  可能: 烧录窗口 / 另一个看日志窗口 / VSCode 的 ESP-IDF 监视器")
    print("  处理: 关掉它们 (在窗口里按 Ctrl+C), 或直接跑 烧录.cmd 会自动清理")
    sys.exit(1)
except Exception as e:
    print("")
    print("[打不开 %s] %s" % (port, e))
    print("  可能: 端口号不对 / USB 线没插好 / 板子没上电")
    sys.exit(1)

print("[已连接 %s]" % port)
print("[提示] ESP32-S3 的 USB 串口在芯片复位时会短暂断开, 本程序会自动重连")
print("")

try:
    while True:
        try:
            data = s.read(4096)
        except (serial.SerialException, OSError):
            # ---- 串口断了 ----
            try:
                s.close()
            except Exception:
                pass

            print("")
            print("[串口断开, 芯片可能在复位, 正在重连...]")

            ok = False
            for i in range(RETRY_TIMES):
                time.sleep(RETRY_GAP)
                try:
                    s = open_port()
                    ok = True
                    break
                except PermissionError:
                    # 端口被别人拿走了 -> 多半是开始烧录了, 我们主动让位
                    print("[端口已被其它程序占用, 本程序退出以免影响烧录]")
                    sys.exit(0)
                except Exception:
                    continue

            if not ok:
                print("[多次重连失败, 退出]")
                print("  如果芯片在反复重启, 请重新运行本脚本查看崩溃日志")
                sys.exit(1)

            print("[已重连 %s]" % port)
            print("")
            continue

        if data:
            sys.stdout.write(data.decode("utf-8", "replace"))
            sys.stdout.flush()

except KeyboardInterrupt:
    print("")
    print("已退出。")
finally:
    try:
        s.close()
    except Exception:
        pass
