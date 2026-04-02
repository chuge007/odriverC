#!/usr/bin/env python3
"""
诊断CAN ID问题
"""

def decode_can_id(can_id_hex):
    """解码CAN ID"""
    can_id = int(can_id_hex, 16)
    node_id = (can_id >> 5) & 0x3F
    command_id = can_id & 0x1F
    commands = {
        0x00: "Heartbeat",
        0x01: "Estop",
        0x02: "Get_Motor_Error",
        0x03: "Get_Encoder_Error",
        0x04: "Get_Sensorless_Error",
        0x07: "Set_Axis_State",
        0x08: "Set_Axis_Node_ID",
        0x09: "Set_Axis_Requested_State",
        0x0A: "Get_Encoder_Estimates",
        0x0B: "Set_Controller_Mode",
        0x0C: "Set_Input_Pos",
        0x0D: "Set_Input_Vel",
        0x0E: "Set_Input_Torque",
        0x0F: "Set_Limits",
        0x10: "Start_Anticogging",
        0x11: "Set_Traj_VelLimit",
        0x12: "Set_Traj_Acel_Limits",
        0x13: "Set_Traj_Inertia",
        0x14: "Get_Iq",
        0x15: "Get_Sensorless_Estimates",
        0x16: "Reboot",
        0x17: "Get_Vbus_Voltage",
        0x18: "Clear_Errors",
    }
    
    command_name = commands.get(command_id, f"Unknown(0x{command_id:02X})")
    return node_id, command_id, command_name

def calculate_can_id(node_id, command_id):
    """计算CAN ID"""
    return (node_id << 5) | command_id

print("=" * 70)
print("CAN ID 诊断工具")
print("=" * 70)

# 分析日志中的问题帧
print("\n问题分析:")
print("-" * 70)
problem_id = "0x0CF"
node_id, cmd_id, cmd_name = decode_can_id(problem_id)
print(f"发送的CAN ID: {problem_id}")
print(f"  解码结果:")
print(f"    节点ID: {node_id}")
print(f"    命令ID: 0x{cmd_id:02X}")
print(f"    命令名称: {cmd_name}")
print(f"    Payload: 04 00 00 00 (FULL_CALIBRATION_SEQUENCE)")

print("\n问题:")
print(f"  ❌ 你的ODrive节点ID是 16 和 1，但发送的是给节点 {node_id} 的命令！")

print("\n" + "=" * 70)
print("正确的CAN ID计算")
print("=" * 70)

# 计算正确的CAN ID
print("\n对于节点16 (0x10):")
for cmd_name, cmd_id in [
    ("Set_Axis_State", 0x07),
    ("Set_Controller_Mode", 0x0B),
    ("Set_Input_Pos", 0x0C),
    ("Set_Input_Vel", 0x0D),
    ("Set_Input_Torque", 0x0E),
    ("Clear_Errors", 0x18),
]:
    correct_id = calculate_can_id(16, cmd_id)
    print(f"  {cmd_name:25s}: 0x{correct_id:03X} (十进制: {correct_id})")

print("\n对于节点1 (0x01):")
for cmd_name, cmd_id in [
    ("Set_Axis_State", 0x07),
    ("Set_Controller_Mode", 0x0B),
    ("Set_Input_Pos", 0x0C),
    ("Set_Input_Vel", 0x0D),
    ("Set_Input_Torque", 0x0E),
    ("Clear_Errors", 0x18),
]:
    correct_id = calculate_can_id(1, cmd_id)
    print(f"  {cmd_name:25s}: 0x{correct_id:03X} (十进制: {correct_id})")

print("\n对于节点6 (0x06) - 你当前发送的:")
for cmd_name, cmd_id in [
    ("Set_Axis_State", 0x07),
]:
    wrong_id = calculate_can_id(6, cmd_id)
    print(f"  {cmd_name:25s}: 0x{wrong_id:03X} (十进制: {wrong_id}) ❌ 错误!")

print("\n" + "=" * 70)
print("解决方案")
print("=" * 70)

print("\n在Qt程序的自定义发送面板中:")
print("  1. 检查 'Address' 输入框")
print("  2. 确保输入的是正确的CAN ID")
print("\n示例:")
print("  - 要发送给节点16的Set_Axis_State命令:")
print("    Address: 0x207 (或十进制 519)")
print("    Payload: 04 00 00 00 (FULL_CALIBRATION_SEQUENCE)")
print("\n  - 要发送给节点1的Set_Axis_State命令:")
print("    Address: 0x027 (或十进制 39)")
print("    Payload: 04 00 00 00 (FULL_CALIBRATION_SEQUENCE)")

print("\n" + "=" * 70)
print("CAN ID 计算公式")
print("=" * 70)
print("\nCAN_ID = (node_id << 5) | command_id")
print("\n示例:")
print("  节点16, Set_Axis_State(0x07):")
print("    = (16 << 5) | 0x07")
print("    = (16 * 32) | 7")
print("    = 512 | 7")
print("    = 519 (0x207)")

print("\n" + "=" * 70)
print("快速参考表")
print("=" * 70)

print("\n节点16的常用命令:")
print("  Set_Axis_State:       0x207")
print("  Set_Controller_Mode:  0x20B")
print("  Set_Input_Pos:        0x20C")
print("  Set_Input_Vel:        0x20D")
print("  Set_Input_Torque:     0x20E")
print("  Clear_Errors:         0x218")

print("\n节点1的常用命令:")
print("  Set_Axis_State:       0x027")
print("  Set_Controller_Mode:  0x02B")
print("  Set_Input_Pos:        0x02C")
print("  Set_Input_Vel:        0x02D")
print("  Set_Input_Torque:     0x02E")
print("  Clear_Errors:         0x038")

print("\n" + "=" * 70)
