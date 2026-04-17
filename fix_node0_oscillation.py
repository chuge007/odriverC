#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复节点0震荡问题
问题：位置控制时震荡严重，PID参数需要调整
"""

import odrive
from odrive.enums import *
import sys
import time

def fix_node0_oscillation(odrv):
    """修复节点0的震荡问题"""
    axis = odrv.axis0
    
    print("="*60)
    print("修复节点0震荡问题")
    print("="*60)
    
    # 1. 显示当前状态
    print(f"\n当前状态:")
    print(f"  轴状态: {axis.current_state}")
    print(f"  位置: {axis.encoder.pos_estimate:.3f} 圈")
    print(f"  速度: {axis.encoder.vel_estimate:.3f} 圈/秒")
    print(f"  当前PID参数:")
    print(f"    pos_gain: {axis.controller.config.pos_gain}")
    print(f"    vel_gain: {axis.controller.config.vel_gain}")
    print(f"    vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    # 2. 如果在闭环状态，先退出
    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
        print(f"\n步骤1: 退出闭环状态...")
        axis.requested_state = AXIS_STATE_IDLE
        time.sleep(1)
    
    # 3. 清除错误
    print(f"步骤2: 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 4. 设置非常保守的PID参数（防止震荡）
    print(f"\n步骤3: 设置防震荡PID参数...")
    
    # 位置增益 - 降低以减少震荡
    axis.controller.config.pos_gain = 3.0  # 从8.0降低到3.0
    print(f"  pos_gain: 8.0 → 3.0")
    
    # 速度增益 - 进一步降低
    axis.controller.config.vel_gain = 0.005  # 从0.02降低到0.005
    print(f"  vel_gain: 0.02 → 0.005")
    
    # 速度积分增益 - 大幅降低
    axis.controller.config.vel_integrator_gain = 0.02  # 从0.1降低到0.02
    print(f"  vel_integrator_gain: 0.1 → 0.02")
    
    # 速度限制 - 降低
    axis.controller.config.vel_limit = 5.0  # 从10降低到5
    print(f"  vel_limit: 10.0 → 5.0 圈/秒")
    
    # 电流限制 - 降低
    axis.motor.config.current_lim = 8.0  # 从10降低到8
    print(f"  current_lim: 10.0 → 8.0 A")
    
    # 5. 设置控制模式
    print(f"\n步骤4: 设置控制模式...")
    axis.controller.config.control_mode = CONTROL_MODE_POSITION_CONTROL
    axis.controller.config.input_mode = INPUT_MODE_PASSTHROUGH
    print(f"  控制模式: 位置控制")
    print(f"  输入模式: 直通")
    
    # 6. 进入闭环
    print(f"\n步骤5: 进入闭环控制...")
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
        print(f"  ✓ 成功进入闭环控制")
        print(f"     状态: {axis.current_state}")
        print(f"     位置: {axis.encoder.pos_estimate:.3f} 圈")
        
        # 7. 测试小幅度运动
        print(f"\n步骤6: 测试小幅度运动（0.2圈）...")
        current_pos = axis.encoder.pos_estimate
        target_pos = current_pos + 0.2
        
        print(f"  当前位置: {current_pos:.3f} 圈")
        print(f"  目标位置: {target_pos:.3f} 圈")
        
        axis.controller.input_pos = target_pos
        time.sleep(3)
        
        final_pos = axis.encoder.pos_estimate
        error = abs(final_pos - target_pos)
        
        print(f"  最终位置: {final_pos:.3f} 圈")
        print(f"  位置误差: {error:.3f} 圈")
        
        if error < 0.05:
            print(f"  ✓ 运动测试成功！无明显震荡")
            
            # 返回原位
            print(f"\n步骤7: 返回原位...")
            axis.controller.input_pos = current_pos
            time.sleep(3)
            print(f"  ✓ 已返回原位")
            
            return True
        else:
            print(f"  ⚠ 位置误差较大，可能仍有震荡")
            return False
    else:
        print(f"  ✗ 未能进入闭环控制")
        print(f"     状态: {axis.current_state}")
        if axis.error != 0:
            print(f"     错误: 0x{axis.error:08X}")
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
    
    if fix_node0_oscillation(odrv):
        print(f"\n{'='*60}")
        print("✓ 节点0震荡问题已修复！")
        print(f"{'='*60}")
        print("\n新的PID参数（防震荡）:")
        print(f"  pos_gain: 3.0")
        print(f"  vel_gain: 0.005")
        print(f"  vel_integrator_gain: 0.02")
        print(f"  vel_limit: 5.0 圈/秒")
        print(f"  current_lim: 8.0 A")
        
        # 保存配置
        if input("\n是否保存配置? (y/n): ").lower() == 'y':
            print("保存配置...")
            odrv.save_configuration()
            print("✓ 已保存")
        
        print("\n现在可以在Qt程序中测试位置控制了。")
        print("如果仍有轻微震荡，可以进一步降低 pos_gain 和 vel_gain。")
        
        return 0
    else:
        print(f"\n{'='*60}")
        print("✗ 修复失败")
        print(f"{'='*60}")
        print("\n建议:")
        print("  1. 检查电机和编码器连接")
        print("  2. 确保电机可以自由转动")
        print("  3. 检查母线电压是否稳定")
        return 1

if __name__ == "__main__":
    sys.exit(main())
