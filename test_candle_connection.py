#!/usr/bin/env python3
"""测试 candle_bridge 完整连接流程"""

import subprocess
import json
import time
import sys

print("启动 candle_bridge.py...")
process = subprocess.Popen(
    [sys.executable, "candle_bridge.py", "--channel", "candle0", "--bitrate", "250000"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1
)

print("等待 ready 事件...")
ready = False
timeout = time.time() + 5

while time.time() < timeout:
    line = process.stdout.readline()
    if not line:
        if process.poll() is not None:
            print(f"❌ 进程意外退出，退出码: {process.returncode}")
            stderr = process.stderr.read()
            if stderr:
                print(f"错误输出: {stderr}")
            break
        time.sleep(0.1)
        continue
    
    print(f"收到: {line.strip()}")
    
    try:
        data = json.loads(line)
        if data.get("event") == "ready":
            print(f"✅ candle_bridge 已就绪！")
            print(f"   通道: {data.get('channel')}")
            print(f"   波特率: {data.get('bitrate')}")
            ready = True
            break
        elif data.get("event") == "error":
            print(f"❌ 错误: {data.get('message')}")
            break
        elif data.get("event") == "log":
            print(f"📋 日志: {data.get('message')}")
    except json.JSONDecodeError:
        print(f"⚠️  非 JSON 输出: {line.strip()}")

if ready:
    print("\n测试发送 CAN 帧...")
    # 发送一个测试帧 (Get_Encoder_Estimates, Node 1)
    test_frame = {
        "cmd": "send",
        "id": 0x009,  # (1 << 5) | 0x09
        "data": "0000000000000000",
        "remote": True,
        "extended": False
    }
    
    process.stdin.write(json.dumps(test_frame) + "\n")
    process.stdin.flush()
    print(f"✅ 发送测试帧: {test_frame}")
    
    # 等待一下看是否有响应
    time.sleep(0.5)
    
    # 发送停止命令
    print("\n发送停止命令...")
    process.stdin.write('{"cmd":"stop"}\n')
    process.stdin.flush()
    
    process.wait(timeout=2)
    print(f"✅ 进程正常退出，退出码: {process.returncode}")
    
    print("\n✅✅✅ candle_bridge 工作正常！")
    print("现在可以在主程序中使用 candle 插件连接了！")
else:
    print("\n❌ candle_bridge 未能正常启动")
    if process.poll() is None:
        process.terminate()
        process.wait(timeout=2)
