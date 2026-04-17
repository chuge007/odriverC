#!/usr/bin/env python3
"""
电机控制失败诊断脚本
检查节点0和节点1的状态、错误和配置
"""

import can
import struct
import time

def send_can_message(bus, node_id, cmd_id, data=b''):
    """发送CAN消息"""
    msg_id = (node_id << 5) | cmd_id
    msg = can.Message(arbitration_id=msg_id, data=data, is_extended_id=False)
    try:
        bus.send(msg)
        return True
    except Exception as e:
        print(f"❌ 发送失败: {e}")
        return False

def request_state(bus, node_id, cmd_id):
    """请求状态信息"""
    send_can_message(bus, node_id, cmd_id)
    time.sleep(0.05)

def decode_heartbeat(data):
    """解码心跳消息"""
    if len(data) >= 8:
        axis_error = struct.unpack('<I', data[0:4])[0]
        axis_state = data[4]
        procedure_result = data[5]
        trajectory_done = data[6]
        return {
            'axis_error': axis_error,
            'axis_state': axis_state,
            'procedure_result': procedure_result,
            'trajectory_done': trajectory_done
        }
    return None

def decode_encoder_estimates(data):
    """解码编码器估计值"""
    if len(data) >= 8:
        pos = struct.unpack('<f', data[0:4])[0]
        vel = struct.unpack('<f', data[4:8])[0]
        return {'pos': pos, 'vel': vel}
    return None

def get_axis_state_name(state):
    """获取轴状态名称"""
    states = {
        0: "UNDEFINED",
        1: "IDLE",
        2: "STARTUP_SEQUENCE",
        3: "FULL_CALIBRATION_SEQUENCE",
        4: "MOTOR_CALIBRATION",
        5: "ENCODER_INDEX_SEARCH",
        6: "ENCODER_OFFSET_CALIBRATION",
        7: "CLOSED_LOOP_CONTROL",
        8: "LOCKIN_SPIN",
        9: "ENCODER_DIR_FIND",
        10: "HOMING",
        11: "ENCODER_HALL_POLARITY_CALIBRATION",
        12: "ENCODER_HALL_PHASE_CALIBRATION"
    }
    return states.get(state, f"UNKNOWN({state})")

def get_error_description(error_code):
    """获取错误描述"""
    errors = []
    error_bits = {
        0x00000001: "INITIALIZING",
        0x00000002: "SYSTEM_LEVEL",
        0x00000004: "TIMING_ERROR",
        0x00000008: "MISSING_ESTIMATE",
        0x00000010: "BAD_CONFIG",
        0x00000020: "DRV_FAULT",
        0x00000040: "MISSING_INPUT",
        0x00000080: "DC_BUS_OVER_VOLTAGE",
        0x00000100: "DC_BUS_UNDER_VOLTAGE",
        0x00000200: "DC_BUS_OVER_CURRENT",
        0x00000400: "DC_BUS_OVER_REGEN_CURRENT",
        0x00000800: "CURRENT_LIMIT_VIOLATION",
        0x00001000: "MOTOR_OVER_TEMP",
        0x00002000: "INVERTER_OVER_TEMP",
        0x00004000: "VELOCITY_LIMIT_VIOLATION",
        0x00008000: "POSITION_LIMIT_VIOLATION",
        0x00010000: "WATCHDOG_TIMER_EXPIRED",
        0x00020000: "ESTOP_REQUESTED",
        0x00040000: "SPINOUT_DETECTED",
        0x00080000: "BRAKE_RESISTOR_DISARMED",
        0x00100000: "THERMISTOR_DISCONNECTED",
        0x00200000: "CALIBRATION_ERROR"
    }
    
    if error_code == 0:
        return ["无错误"]
    
    for bit, desc in error_bits.items():
        if error_code & bit:
            errors.append(desc)
    
    if not errors:
        errors.append(f"未知错误码: 0x{error_code:08X}")
    
    return errors

def diagnose_node(bus, node_id):
    """诊断指定节点"""
    print(f"\n{'='*60}")
    print(f"🔍 诊断节点 {node_id}")
    print(f"{'='*60}")
    
    # 收集消息
    messages = {}
    
    # 请求状态信息
    request_state(bus, node_id, 0x01)  # Heartbeat
    request_state(bus, node_id, 0x09)  # Encoder Estimates
    
    # 收集响应
    start_time = time.time()
    while time.time() - start_time < 0.5:
        msg = bus.recv(timeout=0.1)
        if msg:
            msg_node = msg.arbitration_id >> 5
            msg_cmd = msg.arbitration_id & 0x1F
            if msg_node == node_id:
                messages[msg_cmd] = msg.data
    
    # 分析心跳
    if 0x01 in messages:
        heartbeat = decode_heartbeat(messages[0x01])
        if heartbeat:
            print(f"\n📊 状态信息:")
            print(f"  轴状态: {get_axis_state_name(heartbeat['axis_state'])} ({heartbeat['axis_state']})")
            print(f"  轴错误: 0x{heartbeat['axis_error']:08X}")
            
            if heartbeat['axis_error'] != 0:
                print(f"\n❌ 错误详情:")
                for error in get_error_description(heartbeat['axis_error']):
                    print(f"    • {error}")
            else:
                print(f"  ✅ 无错误")
            
            print(f"  过程结果: {heartbeat['procedure_result']}")
            print(f"  轨迹完成: {heartbeat['trajectory_done']}")
    else:
        print(f"\n❌ 未收到心跳消息 - 节点可能离线")
        return False
    
    # 分析编码器
    if 0x09 in messages:
        encoder = decode_encoder_estimates(messages[0x09])
        if encoder:
            print(f"\n📍 编码器信息:")
            print(f"  位置: {encoder['pos']:.3f} 圈")
            print(f"  速度: {encoder['vel']:.3f} 圈/秒")
    
    return True

def main():
    print("="*60)
    print("🔧 ODrive 电机控制失败诊断")
    print("="*60)
    
    try:
        # 连接CAN总线
        print("\n📡 连接CAN总线...")
        bus = can.interface.Bus(channel='candle0', bustype='socketcan', bitrate=250000)
        print("✅ CAN总线连接成功")
        
        # 诊断节点0
        node0_ok = diagnose_node(bus, 0)
        
        # 诊断节点1
        node1_ok = diagnose_node(bus, 1)
        
        # 总结
        print(f"\n{'='*60}")
        print("📋 诊断总结")
        print(f"{'='*60}")
        print(f"节点0: {'✅ 在线' if node0_ok else '❌ 离线或无响应'}")
        print(f"节点1: {'✅ 在线' if node1_ok else '❌ 离线或无响应'}")
        
        # 检查最近的日志
        print(f"\n📄 检查最近的操作日志...")
        try:
            with open('debug/odriver_runtime.log', 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                # 获取最后50行
                recent_lines = lines[-50:] if len(lines) > 50 else lines
                
                # 查找错误和重要操作
                print("\n最近的重要操作:")
                for line in recent_lines:
                    if any(keyword in line for keyword in ['ERROR', 'WARN', 'Set_Axis_State', 'Estop', 'Clear_Errors']):
                        print(f"  {line.strip()}")
        except Exception as e:
            print(f"❌ 无法读取日志: {e}")
        
        print(f"\n{'='*60}")
        print("💡 建议:")
        print(f"{'='*60}")
        
        if not node0_ok and not node1_ok:
            print("• 两个节点都无响应，检查:")
            print("  1. CAN总线连接是否正常")
            print("  2. ODrive是否上电")
            print("  3. CAN波特率是否正确(250kbps)")
        elif node0_ok or node1_ok:
            print("• 如果有错误码，运行以下命令清除错误:")
            print("  python clear_errors_and_test.py")
            print("\n• 如果节点0有编码器方向问题，参考:")
            print("  节点0问题诊断报告.md")
            print("\n• 重新启动电机:")
            print("  python auto_start_motor.py")
        
        bus.shutdown()
        
    except Exception as e:
        print(f"\n❌ 诊断失败: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
