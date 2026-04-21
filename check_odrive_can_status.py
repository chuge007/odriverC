#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive CAN连接状态检查工具
通过CAN总线检查ODrive板子是否正常运行
"""

import sys
import time
import can

def print_section(title):
    """打印分节标题"""
    print("\n" + "="*60)
    print(f"  {title}")
    print("="*60)

def check_can_interface():
    """检查CAN接口"""
    print_section("1. 检查CAN接口")
    
    # 检查candleLight设备
    try:
        import usb.core
        dev = usb.core.find(idVendor=0x1D50, idProduct=0x606F)
        if dev:
            print("✓ 找到candleLight设备 (VID:0x1D50 PID:0x606F)")
            return True
        else:
            print("✗ 未找到candleLight设备")
            print("\n请检查:")
            print("  1. CAN适配器是否连接到USB")
            print("  2. 是否安装了WinUSB驱动（使用Zadig工具）")
            return False
    except Exception as e:
        print(f"⚠ 无法检查USB设备: {e}")
        return False

def connect_can_bus():
    """连接到CAN总线"""
    print_section("2. 连接CAN总线")
    
    try:
        # 尝试连接到CAN总线 (250kbps)
        print("正在连接CAN总线 (250kbps)...")
        bus = can.interface.Bus(bustype='gs_usb', channel='can0', bitrate=250000)
        print("✓ 成功连接到CAN总线")
        return bus
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        print("\n可能的原因:")
        print("  1. CAN适配器驱动未正确安装")
        print("  2. 波特率不匹配")
        print("  3. CAN总线未连接到ODrive")
        return None

def send_can_message(bus, node_id, cmd_id, data=b''):
    """发送CAN消息"""
    try:
        msg_id = (node_id << 5) | cmd_id
        msg = can.Message(arbitration_id=msg_id, data=data, is_extended_id=False)
        bus.send(msg)
        return True
    except Exception as e:
        print(f"  发送消息失败: {e}")
        return False

def receive_can_message(bus, timeout=1.0):
    """接收CAN消息"""
    try:
        msg = bus.recv(timeout=timeout)
        return msg
    except Exception as e:
        return None

def check_odrive_node(bus, node_id):
    """检查ODrive节点"""
    print_section(f"3. 检查ODrive节点 {node_id}")
    
    print(f"  正在ping节点 {node_id}...")
    
    # 发送心跳请求 (CMD_ID = 0x01)
    if not send_can_message(bus, node_id, 0x01):
        print(f"  ✗ 无法发送消息到节点 {node_id}")
        return False
    
    # 等待响应
    start_time = time.time()
    found = False
    
    while time.time() - start_time < 2.0:
        msg = receive_can_message(bus, timeout=0.5)
        if msg:
            recv_node_id = (msg.arbitration_id >> 5) & 0x3F
            if recv_node_id == node_id:
                print(f"  ✓ 节点 {node_id} 响应正常")
                print(f"    消息ID: 0x{msg.arbitration_id:03X}")
                print(f"    数据: {msg.data.hex()}")
                found = True
                break
    
    if not found:
        print(f"  ✗ 节点 {node_id} 无响应")
        print(f"\n  可能的原因:")
        print(f"    1. 节点ID配置不是 {node_id}")
        print(f"    2. ODrive未上电")
        print(f"    3. CAN总线连接问题")
        print(f"    4. ODrive处于错误状态")
        return False
    
    return True

def get_axis_error(bus, node_id):
    """获取轴错误状态"""
    print(f"\n  读取节点 {node_id} 错误状态...")
    
    # 发送读取错误命令 (CMD_ID = 0x03)
    if not send_can_message(bus, node_id, 0x03):
        return None
    
    # 等待响应
    start_time = time.time()
    while time.time() - start_time < 1.0:
        msg = receive_can_message(bus, timeout=0.5)
        if msg:
            recv_node_id = (msg.arbitration_id >> 5) & 0x3F
            cmd_id = msg.arbitration_id & 0x1F
            
            if recv_node_id == node_id and cmd_id == 0x03:
                if len(msg.data) >= 8:
                    axis_error = int.from_bytes(msg.data[0:4], 'little')
                    motor_error = int.from_bytes(msg.data[4:8], 'little') if len(msg.data) >= 8 else 0
                    
                    print(f"    Axis错误: 0x{axis_error:08X}")
                    if axis_error == 0:
                        print(f"    ✓ 无错误")
                    else:
                        print(f"    ✗ 发现错误")
                    
                    return axis_error
    
    print(f"    ⚠ 无法读取错误状态")
    return None

def get_axis_state(bus, node_id):
    """获取轴状态"""
    print(f"\n  读取节点 {node_id} 当前状态...")
    
    # 发送读取状态命令 (CMD_ID = 0x09)
    if not send_can_message(bus, node_id, 0x09):
        return None
    
    # 等待响应
    start_time = time.time()
    while time.time() - start_time < 1.0:
        msg = receive_can_message(bus, timeout=0.5)
        if msg:
            recv_node_id = (msg.arbitration_id >> 5) & 0x3F
            cmd_id = msg.arbitration_id & 0x1F
            
            if recv_node_id == node_id and cmd_id == 0x09:
                if len(msg.data) >= 4:
                    state = int.from_bytes(msg.data[0:4], 'little')
                    
                    state_names = {
                        0: "UNDEFINED",
                        1: "IDLE",
                        2: "STARTUP_SEQUENCE",
                        3: "FULL_CALIBRATION_SEQUENCE",
                        4: "MOTOR_CALIBRATION",
                        7: "CLOSED_LOOP_CONTROL",
                        8: "LOCKIN_SPIN",
                    }
                    
                    state_name = state_names.get(state, f"UNKNOWN({state})")
                    print(f"    当前状态: {state_name}")
                    
                    return state
    
    print(f"    ⚠ 无法读取状态")
    return None

def test_nodes(bus):
    """测试所有可能的节点"""
    print_section("4. 扫描ODrive节点")
    
    found_nodes = []
    
    print("  扫描节点 0-3...")
    for node_id in range(4):
        print(f"\n  测试节点 {node_id}...")
        
        # 发送心跳
        if send_can_message(bus, node_id, 0x01):
            # 等待响应
            start_time = time.time()
            while time.time() - start_time < 0.5:
                msg = receive_can_message(bus, timeout=0.2)
                if msg:
                    recv_node_id = (msg.arbitration_id >> 5) & 0x3F
                    if recv_node_id == node_id:
                        print(f"    ✓ 找到节点 {node_id}")
                        found_nodes.append(node_id)
                        break
    
    if found_nodes:
        print(f"\n  ✓ 找到 {len(found_nodes)} 个节点: {found_nodes}")
    else:
        print(f"\n  ✗ 未找到任何ODrive节点")
        print(f"\n  请检查:")
        print(f"    1. ODrive是否上电（检查LED灯）")
        print(f"    2. CAN总线连接是否正确（CANH, CANL）")
        print(f"    3. CAN终端电阻是否连接（120Ω）")
        print(f"    4. ODrive的CAN节点ID配置")
    
    return found_nodes

def generate_report(results):
    """生成诊断报告"""
    print_section("诊断报告总结")
    
    if results.get('can_connected') and results.get('nodes_found'):
        print("\n  ✓✓✓ ODrive板子运行正常 ✓✓✓")
        print(f"\n  找到 {len(results['nodes_found'])} 个ODrive节点")
        print(f"  节点ID: {results['nodes_found']}")
        print("\n  系统状态:")
        print("    ✓ CAN接口正常")
        print("    ✓ ODrive响应正常")
        print("    ✓ 可以进行控制操作")
    else:
        print("\n  ⚠⚠⚠ 发现问题 ⚠⚠⚠")
        
        if not results.get('can_interface'):
            print("\n  CAN接口问题:")
            print("    ✗ 未检测到candleLight设备")
            print("    建议: 使用Zadig安装WinUSB驱动")
        
        if not results.get('can_connected'):
            print("\n  CAN总线连接问题:")
            print("    ✗ 无法连接到CAN总线")
            print("    建议: 检查驱动和CAN适配器")
        
        if not results.get('nodes_found'):
            print("\n  ODrive节点问题:")
            print("    ✗ 未找到任何ODrive节点")
            print("    建议:")
            print("      1. 检查ODrive电源（LED灯是否亮）")
            print("      2. 检查CAN总线连接（CANH, CANL）")
            print("      3. 检查终端电阻（120Ω）")
            print("      4. 确认CAN节点ID配置")

def main():
    """主函数"""
    print("\n" + "="*60)
    print("  ODrive CAN连接状态检查工具")
    print("  通过CAN总线检查板子是否正常运行")
    print("="*60)
    
    results = {}
    
    # 1. 检查CAN接口
    results['can_interface'] = check_can_interface()
    
    # 2. 连接CAN总线
    bus = connect_can_bus()
    results['can_connected'] = bus is not None
    
    if not bus:
        generate_report(results)
        return 1
    
    # 3. 扫描节点
    found_nodes = test_nodes(bus)
    results['nodes_found'] = found_nodes
    
    # 4. 检查每个找到的节点
    for node_id in found_nodes:
        print_section(f"5. 详细检查节点 {node_id}")
        get_axis_error(bus, node_id)
        get_axis_state(bus, node_id)
    
    # 5. 生成报告
    generate_report(results)
    
    # 关闭总线
    try:
        bus.shutdown()
    except:
        pass
    
    return 0 if results.get('nodes_found') else 1

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
