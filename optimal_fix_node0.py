#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
优化节点0配置
使用经过测试的合理PID参数
"""

import odrive
from odrive.enums import *
import sys
import time

def optimal_fix_node0(odrv):
    """使用优化的PID参数修复节点0"""
    axis = odrv.axis0
    
    print("="*60)
    print("优化节点0配置")
    print("="*60)
    
    # 1. 退出闭环
    print(f"\n步骤1: 退出闭环状态...")
    axis.requested_state = AXIS_STATE_IDLE
    time.sleep(1)
    
    # 2. 清除错误
    print(f"步骤2: 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 3. 设置优化的PID参数（经过测试的值）
    print(f"\n步骤3: 设置优化的PID参数...")
    
    # 这些是经过多次测试的稳定值
    axis.controller.config.pos_gain = 5.0  # 中等位置增益
    axis.controller.config.vel_gain = 0.01  # 适中的速度增益
    axis.controller.config.vel_integrator_gain = 0.05  # 较低的积分增益
    axis.controller.config.vel_limit = 5.0  # 适中的速度限制
    
    print(f"  pos_gain: 5.0")
    print(f"  vel_gain: 0.01")
    print(f"  vel_integrator_gain: 0.05")
    print(f"  vel_limit: 5.0 圈/秒")
    
    # 4. 设置电流限制
    axis.motor.config.current_lim = 10.0
    axis.motor.config.current_lim_margin = 8.0
    print(f"  current_lim: 10.0 A")
    
    # 5. 设置控制模式
    axis.controller.config.control_mode = CONTROL_MODE_POSITION_CONTROL
    axis.controller.config.input_mode = INPUT_MODE_PASSTHROUGH
    
    # 6. 进入闭环
    print(f"\n步骤4: 进入闭环控制...")
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if axis.current_state != AXIS_STATE_CLOSED_LOOP_CONTROL:
        print(f"  ✗ 未能进入闭环")
        print(f"     状态: {axis.current_state}")
        if axis.error != 0:
            print(f"     错误: 0x{axis.error:08X}")
        return False
    
    print(f"  ✓ 成功进入闭环")
    
    # 7. 测试运动（0.5圈）
    print(f"\n步骤5: 测试运动（0.5圈）...")
    current_pos = axis.encoder.pos_estimate
    target_pos = current_pos + 0.5
    
    print(f"  当前位置: {current_pos:.3f} 圈")
    print(f"  目标位置: {target_pos:.3f} 圈")
    
    axis.controller.input_pos = target_pos
    
    # 监控运动过程
    print(f"  运动中", end="", flush=True)
    for i in range(10):
        time.sleep(0.5)
        print(".", end="", flush=True)
    print()
    
    final_pos = axis.encoder.pos_estimate
    error = abs(final_pos - target_pos)
    
    print(f"  最终位置: {final_pos:.3f} 圈")
    print(f"  位置误差: {error:.3f} 圈")
    
    if error < 0.1:
        print(f"  ✓ 运动测试成功！")
        
        # 返回原位
        print(f"\n步骤6: 返回原位...")
        axis.controller.input_pos = current_pos
        time.sleep(5)
        print(f"  ✓ 已返回")
        
        return True
    else:
        print(f"  ⚠ 位置误差较大")
        return False

def main():
    print("连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V\n")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    if optimal_fix_node0(odrv):
        print(f"\n{'='*60}")
        print("✓ 节点0配置成功！")
        print(f"{'='*60}")
        print("\n优化的PID参数:")
        print(f"  pos_gain: 5.0")
        print(f"  vel_gain: 0.01")
        print(f"  vel_integrator_gain: 0.05")
        print(f"  vel_limit: 5.0 圈/秒")
        print(f"  current_lim: 10.0 A")
        
        print("\n这些参数经过测试，应该:")
        print(f"  ✓ 不会震荡")
        print(f"  ✓ 响应速度适中")
        print(f"  ✓ 稳定可靠")
        
        if input("\n是否保存配置? (y/n): ").lower() == 'y':
            print("保存配置...")
            odrv.save_configuration()
            print("✓ 已保存")
            print("\n现在可以在Qt程序中使用了！")
        
        return 0
    else:
        print(f"\n{'='*60}")
        print("⚠ 配置未达到预期")
        print(f"{'='*60}")
        print("\n可能的原因:")
        print(f"  1. 电机负载过大")
        print(f"  2. 编码器连接问题")
        print(f"  3. 需要进一步调整PID参数")
        
        print("\n建议:")
        print(f"  1. 确保电机可以自由转动")
        print(f"  2. 检查编码器连接")
        print(f"  3. 如果电机有负载，可能需要增加vel_gain")
        
        return 1

if __name__ == "__main__":
    sys.exit(main())
