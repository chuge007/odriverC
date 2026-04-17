#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
快速启动已校准的电机
适用于已经校准过的电机，直接进入闭环控制
"""

import odrive
from odrive.enums import *
import sys
import time

def quick_start_motor(odrv, axis_num):
    """快速启动单个电机（跳过校准）"""
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"\n{'='*60}")
    print(f"快速启动节点 {axis_num}")
    print(f"{'='*60}")
    
    try:
        # 1. 清除错误
        print("步骤1: 清除错误...")
        axis.clear_errors()
        time.sleep(0.5)
        
        # 2. 检查校准状态
        print("步骤2: 检查校准状态...")
        print(f"  电机已校准: {axis.motor.is_calibrated}")
        print(f"  编码器已就绪: {axis.encoder.is_ready}")
        
        if not axis.motor.is_calibrated or not axis.encoder.is_ready:
            print(f"  ✗ 电机未校准！请先运行 standard_motor_config.py")
            return False
        
        # 3. 设置保守的控制参数
        print("步骤3: 设置控制参数...")
        axis.controller.config.vel_limit = 10.0
        axis.controller.config.vel_gain = 0.02
        axis.controller.config.vel_integrator_gain = 0.1
        axis.controller.config.pos_gain = 8.0
        axis.controller.config.control_mode = CONTROL_MODE_POSITION_CONTROL
        axis.controller.config.input_mode = INPUT_MODE_PASSTHROUGH
        print(f"  ✓ 控制参数已设置")
        
        # 4. 设置限制
        print("步骤4: 设置安全限制...")
        axis.motor.config.current_lim = 10.0
        axis.controller.config.vel_limit = 10.0
        print(f"  ✓ 电流限制: 10A")
        print(f"  ✓ 速度限制: 10 圈/秒")
        
        # 5. 进入闭环控制
        print("步骤5: 进入闭环控制...")
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print(f"  ✓ 成功进入闭环控制！")
            print(f"     当前状态: {axis.current_state}")
            print(f"     当前位置: {axis.encoder.pos_estimate:.3f} 圈")
            print(f"     当前速度: {axis.encoder.vel_estimate:.3f} 圈/秒")
            return True
        else:
            print(f"  ✗ 未能进入闭环控制")
            print(f"     当前状态: {axis.current_state}")
            if axis.error != 0:
                print(f"     轴错误: 0x{axis.error:08X}")
            if axis.motor.error != 0:
                print(f"     电机错误: 0x{axis.motor.error:08X}")
            if axis.encoder.error != 0:
                print(f"     编码器错误: 0x{axis.encoder.error:08X}")
            if axis.controller.error != 0:
                print(f"     控制器错误: 0x{axis.controller.error:08X}")
            return False
            
    except Exception as e:
        print(f"\n✗ 启动节点 {axis_num} 失败: {e}")
        return False

def main():
    print("="*60)
    print("ODrive 快速启动脚本（已校准电机）")
    print("="*60)
    print("\n此脚本适用于:")
    print("  ✓ 已经完成校准的电机")
    print("  ✓ 需要快速进入闭环控制")
    print("  ✓ 不需要重新校准")
    print("\n如果电机未校准，请运行: python standard_motor_config.py")
    
    input("\n按回车键继续...")
    
    # 连接ODrive
    print("\n正在连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V")
        
        if odrv.vbus_voltage < 11.0:
            print(f"  ⚠ 警告: 母线电压过低！建议 >11V")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    # 快速启动每个轴
    results = {}
    for axis_num in [0, 1]:
        results[axis_num] = quick_start_motor(odrv, axis_num)
    
    # 总结
    print(f"\n{'='*60}")
    print("启动总结")
    print(f"{'='*60}")
    for axis_num, success in results.items():
        status = "✓ 成功" if success else "✗ 失败"
        print(f"节点 {axis_num}: {status}")
    
    if all(results.values()):
        print("\n🎉 所有节点已进入闭环控制！")
        print("现在可以使用Qt程序控制电机了。")
        
        # 可选：保存配置
        if input("\n是否保存当前配置? (y/n): ").lower() == 'y':
            print("保存配置...")
            try:
                odrv.save_configuration()
                print("✓ 配置已保存")
            except Exception as e:
                print(f"✗ 保存失败: {e}")
        
        return 0
    else:
        print("\n⚠ 部分节点启动失败")
        print("建议:")
        print("  1. 检查错误信息")
        print("  2. 如果电机未校准，运行: python standard_motor_config.py")
        print("  3. 在Qt程序中使用'诊断修复'按钮")
        return 1

if __name__ == "__main__":
    sys.exit(main())
