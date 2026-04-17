#!/usr/bin/env python3
"""
检查CAN配置
"""

import odrive
import sys

def check_can_config():
    print("=" * 60)
    print("检查CAN配置")
    print("=" * 60)
    
    try:
        print("\n正在连接 ODrive...")
        odrv = odrive.find_any(timeout=10)
        if odrv is None:
            print("❌ 未找到 ODrive 设备")
            return False
        
        print("✓ 已连接到 ODrive")
        
        print("\n" + "=" * 60)
        print("节点0 (axis0) CAN配置")
        print("=" * 60)
        try:
            node0_id = odrv.axis0.config.can_node_id
            print(f"  CAN Node ID: {node0_id}")
        except:
            try:
                node0_id = odrv.axis0.config.can.node_id
                print(f"  CAN Node ID: {node0_id}")
            except:
                print("  ⚠️  无法读取CAN Node ID (可能未配置CAN)")
                node0_id = 0
        
        print("\n" + "=" * 60)
        print("节点1 (axis1) CAN配置")
        print("=" * 60)
        try:
            node1_id = odrv.axis1.config.can_node_id
            print(f"  CAN Node ID: {node1_id}")
        except:
            try:
                node1_id = odrv.axis1.config.can.node_id
                print(f"  CAN Node ID: {node1_id}")
            except:
                print("  ⚠️  无法读取CAN Node ID (可能未配置CAN)")
                node1_id = 1
        
        print("\n" + "=" * 60)
        print("CAN命令ID计算")
        print("=" * 60)
        
        print(f"\n节点0 (实际CAN ID={node0_id}):")
        print(f"  Set_Axis_State:     0x{(node0_id << 5) | 0x07:03X}")
        print(f"  Clear_Errors:       0x{(node0_id << 5) | 0x18:03X}")
        print(f"  Set_Controller_Mode: 0x{(node0_id << 5) | 0x0B:03X}")
        print(f"  Set_Input_Pos:      0x{(node0_id << 5) | 0x0C:03X}")
        print(f"  Set_Input_Vel:      0x{(node0_id << 5) | 0x0D:03X}")
        print(f"  Set_Limits:         0x{(node0_id << 5) | 0x0F:03X}")
        
        print(f"\n节点1 (实际CAN ID={node1_id}):")
        print(f"  Set_Axis_State:     0x{(node1_id << 5) | 0x07:03X}")
        print(f"  Clear_Errors:       0x{(node1_id << 5) | 0x18:03X}")
        print(f"  Set_Controller_Mode: 0x{(node1_id << 5) | 0x0B:03X}")
        print(f"  Set_Input_Pos:      0x{(node1_id << 5) | 0x0C:03X}")
        print(f"  Set_Input_Vel:      0x{(node1_id << 5) | 0x0D:03X}")
        print(f"  Set_Limits:         0x{(node1_id << 5) | 0x0F:03X}")
        
        print("\n" + "=" * 60)
        print("Qt程序使用的CAN ID (从日志)")
        print("=" * 60)
        print("  节点0:")
        print("    Set_Axis_State:  0x007")
        print("    Clear_Errors:    0x018")
        print("    Set_Limits:      0x00F")
        print("  节点1:")
        print("    Set_Axis_State:  0x027")
        print("    Clear_Errors:    0x038")
        
        print("\n" + "=" * 60)
        print("诊断结果")
        print("=" * 60)
        
        if node0_id == 0 and node1_id == 1:
            print("✓ CAN ID配置正确！")
            print("  节点0 CAN ID = 0 (命令ID: 0x007, 0x018等)")
            print("  节点1 CAN ID = 1 (命令ID: 0x027, 0x038等)")
        else:
            print("❌ CAN ID配置不正确！")
            print(f"  当前节点0 CAN ID = {node0_id} (应该是 0)")
            print(f"  当前节点1 CAN ID = {node1_id} (应该是 1)")
            print("\n修复方法:")
            print("  odrv.axis0.config.can.node_id = 0")
            print("  odrv.axis1.config.can.node_id = 1")
            print("  odrv.save_configuration()")
            print("  odrv.reboot()")
        
        return True
        
    except Exception as e:
        print(f"\n❌ 检查过程中出错: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = check_can_config()
    sys.exit(0 if success else 1)
