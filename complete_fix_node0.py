#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
完整修复节点0
包括：检查编码器方向、重新校准、设置正确的PID参数
"""

import odrive
from odrive.enums import *
import sys
import time

def complete_fix_node0(odrv):
    """完整修复节点0"""
    axis = odrv.axis0
    
    print("="*60)
    print("完整修复节点0")
    print("="*60)
    
    # 1. 退出闭环
    print(f"\n步骤1: 退出闭环状态...")
    axis.requested_state = AXIS_STATE_IDLE
    time.sleep(1)
    
    # 2. 清除错误
    print(f"步骤2: 清除所有错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 3. 重新配置基本参数
    print(f"\n步骤3: 重新配置基本参数...")
    
    # 电机参数
    axis.motor.config.pole_pairs = 7
    axis.motor.config.calibration_current = 10.0
    axis.motor.config.current_lim = 8.0
    axis.motor.config.current_lim_margin = 6.0
    print(f"  ✓ 电机参数已设置")
    
    # 编码器参数
    axis.encoder.config.cpr = 8192
    axis.encoder.config.mode = ENCODER_MODE_INCREMENTAL
    axis.encoder.config.bandwidth = 1000
    print(f"  ✓ 编码器参数已设置")
    
    # 4. 设置极低的PID参数（从零开始）
    print(f"\n步骤4: 设置极低的PID参数...")
    axis.controller.config.pos_gain = 1.0  # 极低的位置增益
    axis.controller.config.vel_gain = 0.001  # 极低的速度增益
    axis.controller.config.vel_integrator_gain = 0.001  # 极低的积分增益
    axis.controller.config.vel_limit = 2.0  # 极低的速度限制
    print(f"  pos_gain: 1.0")
    print(f"  vel_gain: 0.001")
    print(f"  vel_integrator_gain: 0.001")
    print(f"  vel_limit: 2.0 圈/秒")
    
    # 5. 设置控制模式
    axis.controller.config.control_mode = CONTROL_MODE_POSITION_CONTROL
    axis.controller.config.input_mode = INPUT_MODE_PASSTHROUGH
    
    # 6. 重新校准编码器偏移
    print(f"\n步骤5: 重新校准编码器偏移...")
    print(f"  ⚠ 请确保电机可以自由转动！")
    
    axis.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION
    print(f"  校准中", end="", flush=True)
    
    start_time = time.time()
    while axis.current_state != AXIS_STATE_IDLE:
        if time.time() - start_time > 30:
            print(f"\n  ✗ 校准超时")
            return False
        
        if axis.error != 0:
            print(f"\n  ✗ 校准失败: 0x{axis.error:08X}")
            return False
        
        time.sleep(0.5)
        print(".", end="", flush=True)
    
    print()
    print(f"  ✓ 编码器校准完成")
    print(f"     编码器偏移: {axis.encoder.config.offset}")
    
    # 7. 进入闭环
    print(f"\n步骤6: 进入闭环控制...")
    axis.clear_errors()
    time.sleep(0.5)
    
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if axis.current_state != AXIS_STATE_CLOSED_LOOP_CONTROL:
        print(f"  ✗ 未能进入闭环")
        print(f"     状态: {axis.current_state}")
        if axis.error != 0:
            print(f"     错误: 0x{axis.error:08X}")
        return False
    
    print(f"  ✓ 成功进入闭环")
    
    # 8. 测试方向（非常小的移动）
    print(f"\n步骤7: 测试编码器方向（移动0.05圈）...")
    current_pos = axis.encoder.pos_estimate
    target_pos = current_pos + 0.05
    
    print(f"  当前位置: {current_pos:.4f} 圈")
    print(f"  目标位置: {target_pos:.4f} 圈")
    
    axis.controller.input_pos = target_pos
    time.sleep(4)  # 给足够时间
    
    final_pos = axis.encoder.pos_estimate
    actual_move = final_pos - current_pos
    
    print(f"  最终位置: {final_pos:.4f} 圈")
    print(f"  实际移动: {actual_move:.4f} 圈")
    
    if actual_move > 0.02:
        print(f"  ✓ 方向正确！")
        direction_ok = True
    elif actual_move < -0.02:
        print(f"  ✗ 方向相反！需要反转编码器")
        direction_ok = False
    else:
        print(f"  ⚠ 移动太小，无法判断方向")
        direction_ok = None
    
    # 返回原位
    axis.controller.input_pos = current_pos
    time.sleep(3)
    
    return direction_ok

def main():
    print("连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V\n")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    result = complete_fix_node0(odrv)
    
    if result == True:
        print(f"\n{'='*60}")
        print("✓ 节点0修复成功！方向正确")
        print(f"{'='*60}")
        print("\n当前配置（极低参数，用于测试）:")
        print(f"  pos_gain: 1.0")
        print(f"  vel_gain: 0.001")
        print(f"  vel_integrator_gain: 0.001")
        print(f"  vel_limit: 2.0 圈/秒")
        
        print("\n建议:")
        print("  1. 保存当前配置")
        print("  2. 在Qt程序中测试位置控制")
        print("  3. 如果运动正常但太慢，可以逐步增加PID参数")
        
        if input("\n是否保存配置? (y/n): ").lower() == 'y':
            print("保存配置...")
            odrv.save_configuration()
            print("✓ 已保存")
        
        return 0
        
    elif result == False:
        print(f"\n{'='*60}")
        print("⚠ 编码器方向相反！")
        print(f"{'='*60}")
        print("\n需要反转编码器方向。有两种方法:")
        print("  方法1: 交换编码器的A/B信号线")
        print("  方法2: 在软件中反转（不推荐，可能不稳定）")
        
        if input("\n是否尝试软件反转? (y/n): ").lower() == 'y':
            print("注意：软件反转可能不稳定，建议硬件交换线缆")
            # ODrive可能没有direction参数，需要通过其他方式
            print("请手动交换编码器A/B线，然后重新运行此脚本")
        
        return 1
        
    else:
        print(f"\n{'='*60}")
        print("✗ 修复失败或无法判断")
        print(f"{'='*60}")
        print("\n建议:")
        print("  1. 检查电机和编码器连接")
        print("  2. 确保电机可以自由转动")
        print("  3. 检查母线电压")
        return 1

if __name__ == "__main__":
    sys.exit(main())
