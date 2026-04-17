#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
通过CAN直接诊断节点0的错误
"""

import can
import struct
import time

# CAN命令ID定义
CMD_ID_HEARTBEAT = 0x001
CMD_ID_GET_ENCODER_ERROR = 0x003
CMD_ID_GET_MOTOR_ERROR = 0x005
CMD_ID_GET_CONTROLLER_ERROR = 0x006
CMD_ID_SET_AXIS_STATE = 0x007
CMD_ID_GET_ENCODER_ESTIMATES = 0x009
CMD_ID_CLEAR_ERRORS = 0x018

def make_can_id(node_id, cmd_id):
    """构造CAN ID"""
    return (node_id << 5) | cmd_id

def diagnose_node0():
    """诊断节点0"""
    print("正在连接 CAN 总线...")
    
    try:
        # 连接到CAN总线
        bus = can.interface.Bus(channel='candle0', bustype='candle', bitrate=250000)
        print("✅ 已连接到 CAN 总线")
        
        node_id = 0
        
        # 1. 清除错误
        print("\n1. 清除所有错误...")
        msg = can.Message(
            arbitration_id=make_can_id(node_id, CMD_ID_CLEAR_ERRORS),
            data=[0],
            is_extended_id=False
        )
        bus.send(msg)
        time.sleep(0.5)
        print("   ✅ 已发送清除错误命令")
        
        # 2. 尝试进入闭环
        print("\n2. 尝试进入闭环状态...")
        msg = can.Message(
            arbitration_id=make_can_id(node_id, CMD_ID_SET_AXIS_STATE),
            data=struct.pack('<I', 8),  # AXIS_STATE_CLOSED_LOOP_CONTROL = 8
            is_extended_id=False
        )
        bus.send(msg)
        time.sleep(2)
        print("   ✅ 已发送闭环命令")
        
        # 3. 监听心跳消息获取状态
        print("\n3. 监听心跳消息...")
        timeout = time.time() + 3
        heartbeat_received = False
        
        while time.time() < timeout:
            msg = bus.recv(timeout=0.5)
            if msg is None:
                continue
            
            # 检查是否是节点0的心跳消息
            cmd_id = msg.arbitration_id & 0x1F
            msg_node_id = (msg.arbitration_id >> 5) & 0x3F
            
            if msg_node_id == node_id and cmd_id == CMD_ID_HEARTBEAT:
                heartbeat_received = True
                # 解析心跳数据
                if len(msg.data) >= 8:
                    axis_error = struct.unpack('<I', msg.data[0:4])[0]
                    axis_state = msg.data[4]
                    procedure_result = msg.data[5]
                    trajectory_done = msg.data[6] & 0x01
                    
                    print(f"\n   ✅ 收到节点{node_id}心跳:")
                    print(f"      Axis Error: 0x{axis_error:08X}")
                    print(f"      Axis State: {axis_state}")
                    print(f"      Procedure Result: {procedure_result}")
                    print(f"      Trajectory Done: {trajectory_done}")
                    
                    if axis_error != 0:
                        print(f"\n   ⚠️ 检测到轴错误: 0x{axis_error:08X}")
                        # 解析常见错误
                        if axis_error & 0x00000001:
                            print("      - INVALID_STATE: 状态转换无效")
                        if axis_error & 0x00000040:
                            print("      - MOTOR_FAILED: 电机故障")
                        if axis_error & 0x00000100:
                            print("      - ENCODER_FAILED: 编码器故障")
                        if axis_error & 0x00000200:
                            print("      - CONTROLLER_FAILED: 控制器故障")
                        if axis_error & 0x00080000:
                            print("      - UNKNOWN_POSITION: 位置未知（可能需要校准）")
                    
                    if axis_state == 8:
                        print("\n   ✅ 节点0已成功进入闭环状态！")
                        return True
                    elif axis_state == 1:
                        print("\n   ⚠️ 节点0处于IDLE状态")
                    else:
                        print(f"\n   ⚠️ 节点0处于状态{axis_state}")
                    
                    break
        
        if not heartbeat_received:
            print("\n   ❌ 未收到节点0的心跳消息")
            return False
        
        print("\n建议操作:")
        print("1. 如果有INVALID_STATE错误，可能需要先进行校准")
        print("2. 如果有ENCODER_FAILED，检查编码器连接")
        print("3. 如果有UNKNOWN_POSITION，需要运行编码器校准")
        print("\n可以尝试运行: python fix_encoder.py")
        
        return False
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        try:
            bus.shutdown()
        except:
            pass

if __name__ == "__main__":
    import sys
    success = diagnose_node0()
    sys.exit(0 if success else 1)
