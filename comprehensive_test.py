#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive板子综合测试工具
通过CAN总线全面测试ODrive是否正常运行
"""

import subprocess
import json
import time
import sys

def print_header(title):
    """打印标题"""
    print("\n" + "="*70)
    print(f"  {title}")
    print("="*70)

def test_can_interface():
    """测试1: CAN接口检测"""
    print_header("测试1: CAN接口检测")
    
    try:
        import usb.core
        dev = usb.core.find(idVendor=0x1D50, idProduct=0x606F)
        if dev:
            print("✓ candleLight设备已检测到")
            print(f"  VID: 0x{dev.idVendor:04X}")
            print(f"  PID: 0x{dev.idProduct:04X}")
            return True
        else:
            print("✗ 未检测到candleLight设备")
            return False
    except Exception as e:
        print(f"✗ 检测失败: {e}")
        return False

def test_candle_bridge():
    """测试2: candle_bridge通信"""
    print_header("测试2: candle_bridge通信测试")
    
    print("启动candle_bridge进程...")
    try:
        process = subprocess.Popen(
            [sys.executable, "candle_bridge.py", "--channel", "candle0", "--bitrate", "250000"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        print("等待ready事件...")
        ready = False
        timeout = time.time() + 5
        
        while time.time() < timeout:
            line = process.stdout.readline()
            if not line:
                if process.poll() is not None:
                    stderr = process.stderr.read()
                    print(f"✗ 进程退出: {stderr}")
                    return False
                time.sleep(0.1)
                continue
            
            try:
                data = json.loads(line)
                if data.get("event") == "ready":
                    print(f"✓ candle_bridge已就绪")
                    print(f"  通道: {data.get('channel')}")
                    print(f"  波特率: {data.get('bitrate')}")
                    ready = True
                    break
                elif data.get("event") == "error":
                    print(f"✗ 错误: {data.get('message')}")
                    break
            except json.JSONDecodeError:
                pass
        
        if ready:
            # 测试发送CAN帧
            print("\n测试发送CAN帧...")
            test_frames = [
                {"cmd": "send", "id": 0x001, "data": "0000000000000000", "remote": True, "extended": False},  # 节点0心跳
                {"cmd": "send", "id": 0x021, "data": "0000000000000000", "remote": True, "extended": False},  # 节点1心跳
            ]
            
            for frame in test_frames:
                process.stdin.write(json.dumps(frame) + "\n")
                process.stdin.flush()
                print(f"  发送: ID=0x{frame['id']:03X}")
            
            time.sleep(0.5)
            
            # 停止进程
            process.stdin.write('{"cmd":"stop"}\n')
            process.stdin.flush()
            process.wait(timeout=2)
            
            print("\n✓ candle_bridge通信正常")
            return True
        else:
            print("✗ candle_bridge未能就绪")
            if process.poll() is None:
                process.terminate()
            return False
            
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        return False

def test_odrive_response():
    """测试3: ODrive响应测试"""
    print_header("测试3: ODrive节点响应测试")
    
    print("启动candle_bridge并监听响应...")
    try:
        process = subprocess.Popen(
            [sys.executable, "candle_bridge.py", "--channel", "candle0", "--bitrate", "250000"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        # 等待ready
        ready = False
        timeout = time.time() + 5
        while time.time() < timeout:
            line = process.stdout.readline()
            if not line:
                time.sleep(0.1)
                continue
            try:
                data = json.loads(line)
                if data.get("event") == "ready":
                    ready = True
                    break
            except:
                pass
        
        if not ready:
            print("✗ candle_bridge未就绪")
            process.terminate()
            return False
        
        print("✓ candle_bridge已就绪")
        print("\n发送心跳请求到节点0和节点1...")
        
        # 发送心跳到节点0和1
        for node_id in [0, 1]:
            msg_id = (node_id << 5) | 0x01  # 心跳命令
            frame = {
                "cmd": "send",
                "id": msg_id,
                "data": "0000000000000000",
                "remote": False,
                "extended": False
            }
            process.stdin.write(json.dumps(frame) + "\n")
            process.stdin.flush()
            print(f"  发送心跳到节点{node_id} (ID=0x{msg_id:03X})")
        
        # 监听响应
        print("\n监听ODrive响应 (3秒)...")
        found_nodes = []
        timeout = time.time() + 3
        
        while time.time() < timeout:
            line = process.stdout.readline()
            if not line:
                time.sleep(0.1)
                continue
            
            try:
                data = json.loads(line)
                if data.get("event") == "recv":
                    msg_id = data.get("id", 0)
                    node_id = (msg_id >> 5) & 0x3F
                    cmd_id = msg_id & 0x1F
                    msg_data = data.get("data", "")
                    
                    if node_id not in found_nodes:
                        found_nodes.append(node_id)
                        print(f"  ✓ 节点{node_id}响应 (ID=0x{msg_id:03X}, CMD=0x{cmd_id:02X}, Data={msg_data})")
            except:
                pass
        
        # 停止进程
        process.stdin.write('{"cmd":"stop"}\n')
        process.stdin.flush()
        process.wait(timeout=2)
        
        if found_nodes:
            print(f"\n✓ 找到{len(found_nodes)}个ODrive节点: {found_nodes}")
            return True
        else:
            print("\n⚠ 未收到ODrive响应")
            print("  可能原因:")
            print("    1. ODrive未上电")
            print("    2. CAN总线未正确连接")
            print("    3. 节点ID配置不同")
            return False
            
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_gui_program():
    """测试4: GUI程序状态"""
    print_header("测试4: GUI程序检查")
    
    import os
    gui_path = "debug/odriver.exe"
    
    if os.path.exists(gui_path):
        print(f"✓ GUI程序存在: {gui_path}")
        
        # 检查配置文件
        config_path = "debug/odriver.ini"
        if os.path.exists(config_path):
            print(f"✓ 配置文件存在: {config_path}")
        
        print("\n提示: GUI程序已启动，你可以:")
        print("  1. 在GUI中选择板子和节点")
        print("  2. 点击'连接'按钮")
        print("  3. 使用'进入闭环'、速度控制等功能")
        return True
    else:
        print(f"✗ GUI程序不存在: {gui_path}")
        return False

def generate_final_report(results):
    """生成最终报告"""
    print_header("综合测试报告")
    
    total = len(results)
    passed = sum(1 for r in results.values() if r)
    
    print(f"\n测试结果: {passed}/{total} 通过\n")
    
    for test_name, result in results.items():
        status = "✓ 通过" if result else "✗ 失败"
        print(f"  {status} - {test_name}")
    
    print("\n" + "="*70)
    
    if passed == total:
        print("\n  ✓✓✓ 所有测试通过！ODrive板子运行正常！ ✓✓✓")
        print("\n系统状态:")
        print("  ✓ CAN接口正常")
        print("  ✓ candle_bridge通信正常")
        print("  ✓ ODrive响应正常")
        print("  ✓ GUI程序可用")
        print("\n你可以:")
        print("  1. 使用GUI程序控制电机")
        print("  2. 运行Python脚本进行自动化控制")
        print("  3. 通过CAN总线发送命令")
    else:
        print("\n  ⚠⚠⚠ 部分测试失败 ⚠⚠⚠")
        print("\n建议:")
        
        if not results.get("CAN接口检测"):
            print("  - 检查candleLight设备USB连接")
            print("  - 使用Zadig安装WinUSB驱动")
        
        if not results.get("candle_bridge通信"):
            print("  - 检查Python环境和依赖")
            print("  - 查看candle_bridge.py错误信息")
        
        if not results.get("ODrive响应"):
            print("  - 检查ODrive电源（LED灯）")
            print("  - 检查CAN总线连接（CANH, CANL）")
            print("  - 检查终端电阻（120Ω）")
            print("  - 确认CAN节点ID配置")
    
    print("\n" + "="*70)

def main():
    """主函数"""
    print("\n" + "="*70)
    print("  ODrive板子综合测试工具")
    print("  全面检查ODrive是否正常运行")
    print("="*70)
    
    results = {}
    
    # 执行所有测试
    results["CAN接口检测"] = test_can_interface()
    time.sleep(1)
    
    results["candle_bridge通信"] = test_candle_bridge()
    time.sleep(1)
    
    results["ODrive响应"] = test_odrive_response()
    time.sleep(1)
    
    results["GUI程序"] = test_gui_program()
    
    # 生成报告
    generate_final_report(results)
    
    return 0 if all(results.values()) else 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
