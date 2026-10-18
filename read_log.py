import serial
import sys
import time

port = sys.argv[1] if len(sys.argv) > 1 else "COM9"

print("=" * 56)
print("  正在读取 %s (115200) ..." % port)
print("  没有 raw 模式：可以用鼠标选中复制！")
print("  串口掉线(例如按 RST)会自动重连")
print("  退出: Ctrl + C")
print("=" * 56)

s = None
try:
    while True:
        # 没连上就尝试连接
        if s is None:
            try:
                s = serial.Serial(port, 115200, timeout=0.2)
                print("\n[已连接 %s]" % port)
            except Exception:
                time.sleep(0.3)
                continue

        try:
            data = s.read(4096)
            if data:
                sys.stdout.write(data.decode("utf-8", "replace"))
                sys.stdout.flush()
        except (serial.SerialException, OSError):
            print("\n[串口断开，等待重连...]")
            try:
                s.close()
            except Exception:
                pass
            s = None
            time.sleep(0.3)

except KeyboardInterrupt:
    print("\n已退出。")
finally:
    if s is not None:
        try:
            s.close()
        except Exception:
            pass
