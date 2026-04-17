#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复节点0无法进入闭环状态的问题
"""

import odrive
from odrive.enums import *
import time
import sys

def fix_node0():
    """诊断并修复节点0"""
    print("正在连接 ODrive...")
    
    try:
        # 连接到ODrive
        odrv = odrive.find_any(channel="can0", node_id=0)
        print(f"✅ 已连接到节点 0")
        
        # 读取当前状态
        print(f"\n当前状态:")
        print(f"  axis.current_state = {odrv.axis0.current_state}")
        print(f"  axis.error = 0x{odrv.axis0.error:08X}")
        print(f"  motor.error = 0x{odrv.axis0.motor.error:08X}")
        print(f"  encoder.error = 0x{odrv.axis0.encoder.error:08X}")
        print(f"  controller.error = 0x{odrv.axis0.controller.error:08X}")
        
        # 检查是否有错误
        if odrv.axis0.error != 0:
            print(f"\n⚠️ 检测到轴错误: 0x{odrv.axis0.error:08X}")
            print("  正在清除错误...")
            odrv.clear_errors()
            time.sleep(0.5)
            print("  ✅ 错误已清除")
        
        # 检查编码器是否已校准
        print(f"\n编码器状态:")
        print(f"  encoder.is_ready = {odrv.axis0.encoder.is_ready}")
        print(f"  encoder.config.pre_calibrated = {odrv.axis0.encoder.config.pre_calibrated}")
        
        if not odrv.axis0.encoder.is_ready:
            print("\n⚠️ 编码器未就绪，需要校准")
            print("  建议：在 odrivetool 中运行完整校准")
            print("  odrv0.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE")
            return False
        
        # 尝试进入闭环
        print("\n正在尝试进入闭环状态...")
        odrv.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        # 检查状态
        current_state = odrv.axis0.current_state
        print(f"  当前状态: {current_state}")
        
        if current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print("  ✅ 成功进入闭环状态！")
            
            # 测试小幅度运动
            print("\n测试小幅度运动...")
            odrv.axis0.controller.input_pos = 0.1
            time.sleep(1)
            pos = odrv.axis0.encoder.pos_estimate
            print(f"  位置: {pos:.3f} 圈")
            
            odrv.axis0.controller.input_pos = 0
            time.sleep(1)
            pos = odrv.axis0.encoder.pos_estimate
            print(f"  返回位置: {pos:.3f} 圈")
            
            print("\n✅ 节点0工作正常！")
            return True
        else:
            print(f"  ❌ 未能进入闭环状态，当前状态: {current_state}")
            print(f"  axis.error = 0x{odrv.axis0.error:08X}")
            
            # 显示可能的原因
            if odrv.axis0.error & 0x00000001:  # INVALID_STATE
                print("\n可能原因: INVALID_STATE错误")
                print("  - 检查电机是否已校准")
                print("  - 检查编码器是否已校准")
                print("  - 尝试运行完整校准序列")
            
            return False
            
    except Exception as e:
        print(f"❌ 错误: {e}")
        print("\n提示:")
        print("  1. 确保 ODrive 已通过 CAN 连接")
        print("  2. 确保节点ID设置为0")
        print("  3. 尝试使用 odrivetool 手动连接测试")
        return False

if __name__ == "__main__":
    success = fix_node0()
    sys.exit(0 if success else 1)
