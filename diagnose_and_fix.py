#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
诊断并修复闭环问题
"""

import odrive
import time
import sys

# 导入枚举类型
try:
    from odrive.enums import AxisState, ControlMode, InputMode
except ImportError:
    class AxisState:
        IDLE = 1
        CLOSED_LOOP_CONTROL = 8
    
    class ControlMode:
        VELOCITY_CONTROL = 2
    
    class InputMode:
        PASSTHROUGH = 1

def main():
    print("=" * 60)
    print("诊断并修复闭环问题")
    print("=" * 60)
    
    # 查找设备
    print("\n正在查找ODrive设备...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 找到ODrive设备，序列号: {odrv.serial_number}")
    except Exception as e:
        print(f"✗ 未找到ODrive设备: {e}")
        sys.exit(1)
    
    axis = odrv.axis0
    
    # 详细诊断
    print("\n[诊断] 当前配置:")
    print(f"  控制模式: {axis.controller.config.control_mode}")
    print(f"  输入模式: {axis.controller.config.input_mode}")
    print(f"  速度限制: {axis.controller.config.vel_limit}")
    print(f"  电流限制: {axis.motor.config.current_lim}")
    print(f"  校准电流: {axis.motor.config.calibration_current}")
    print(f"  pos_gain: {axis.controller.config.pos_gain}")
    print(f"  vel_gain: {axis.controller.config.vel_gain}")
    print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    # 检查编码器
    print(f"\n[诊断] 编码器状态:")
    print(f"  is_ready: {axis.encoder.is_ready}")
    print(f"  shadow_count: {axis.encoder.shadow_count}")
    print(f"  count_in_cpr: {axis.encoder.count_in_cpr}")
    print(f"  pos_estimate: {axis.encoder.pos_estimate}")
    
    # 检查电机
    print(f"\n[诊断] 电机状态:")
    print(f"  is_calibrated: {axis.motor.is_calibrated}")
    print(f"  phase_resistance: {axis.motor.config.phase_resistance}")
    print(f"  phase_inductance: {axis.motor.config.phase_inductance}")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 检查控制器错误
    print(f"\n[诊断] 控制器详细状态:")
    try:
        print(f"  控制器错误: {hex(axis.controller.error)}")
        print(f"  config.enable_vel_limit: {axis.controller.config.enable_vel_limit}")
        print(f"  config.enable_current_control_vel_limit: {axis.controller.config.enable_current_control_vel_limit if hasattr(axis.controller.config, 'enable_current_control_vel_limit') else 'N/A'}")
    except Exception as e:
        print(f"  无法读取某些控制器属性: {e}")
    
    # 重新配置
    print("\n[2] 重新配置参数...")
    
    # 确保控制模式正确
    axis.controller.config.control_mode = 2  # VELOCITY_CONTROL
    axis.controller.config.input_mode = 1    # PASSTHROUGH
    
    # 设置限制
    axis.controller.config.vel_limit = 10.0
    axis.motor.config.current_lim = 10.0
    
    # 设置增益
    axis.controller.config.pos_gain = 20.0
    axis.controller.config.vel_gain = 0.16
    axis.controller.config.vel_integrator_gain = 0.32
    
    # 设置速度限制使能
    axis.controller.config.enable_vel_limit = True
    
    print("✓ 参数配置完成")
    
    # 保存配置
    print("\n[3] 保存配置...")
    try:
        odrv.save_configuration()
        print("✓ 配置已保存")
        time.sleep(1)
    except Exception as e:
        print(f"⚠ 保存配置失败: {e}")
    
    # 再次清除错误
    print("\n[4] 再次清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 尝试进入闭环
    print("\n[5] 尝试进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    
    # 等待并监控状态变化
    for i in range(20):
        time.sleep(0.5)
        state = axis.current_state
        error = axis.error
        
        print(f"  [{i+1}] 状态: {state}, 错误: {hex(error)}")
        
        if state == 8:  # CLOSED_LOOP_CONTROL
            print("\n✓ 成功进入闭环控制！")
            
            # 测试运行
            print("\n[6] 测试运行...")
            print("设置速度: 1 turn/s")
            axis.controller.input_vel = 1.0
            time.sleep(3)
            
            print(f"  实际速度: {axis.encoder.vel_estimate:.2f} turns/s")
            print(f"  电流: {axis.motor.current_control.Iq_measured:.2f} A")
            print(f"  母线电压: {axis.vbus_voltage:.2f} V")
            
            # 停止
            print("\n停止电机...")
            axis.controller.input_vel = 0.0
            time.sleep(1)
            
            print("\n✓ 测试成功！")
            return
        
        if error != 0:
            print(f"\n✗ 出现错误: {hex(error)}")
            print(f"  电机错误: {hex(axis.motor.error)}")
            print(f"  编码器错误: {hex(axis.encoder.error)}")
            print(f"  控制器错误: {hex(axis.controller.error)}")
            break
    
    print("\n✗ 无法进入闭环控制")
    print("\n可能的原因:")
    print("1. 电机或编码器未校准")
    print("2. 控制器配置不正确")
    print("3. 硬件连接问题")
    print("\n建议:")
    print("1. 运行完整校准: odrv0.axis0.requested_state = 3")
    print("2. 检查编码器连接")
    print("3. 检查电机相线连接")

if __name__ == "__main__":
    main()
