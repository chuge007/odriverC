#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试闭环驱动
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
    print("测试闭环驱动")
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
    
    # 检查当前状态
    print(f"\n当前状态:")
    print(f"  轴状态: {axis.current_state}")
    print(f"  轴错误: {hex(axis.error)}")
    print(f"  电机错误: {hex(axis.motor.error)}")
    print(f"  控制模式: {axis.controller.config.control_mode}")
    print(f"  速度限制: {axis.controller.config.vel_limit} turns/s")
    print(f"  电流限制: {axis.motor.config.current_lim} A")
    
    # 清除错误
    if axis.error != 0:
        print("\n清除错误...")
        axis.clear_errors()
        time.sleep(0.5)
    
    # 进入闭环
    print("\n[1] 进入闭环控制...")
    try:
        axis.requested_state = AxisState.CLOSED_LOOP_CONTROL
        time.sleep(1)
        
        if axis.current_state == AxisState.CLOSED_LOOP_CONTROL:
            print("✓ 成功进入闭环控制")
        else:
            print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
            print(f"  轴错误: {hex(axis.error)}")
            sys.exit(1)
    except Exception as e:
        print(f"✗ 进入闭环失败: {e}")
        sys.exit(1)
    
    # 测试不同速度
    test_speeds = [1.0, 2.0, 3.0, 5.0, -2.0, 0.0]
    
    print("\n[2] 测试不同速度...")
    for speed in test_speeds:
        print(f"\n设置速度: {speed} turns/s")
        axis.controller.input_vel = speed
        time.sleep(2)
        
        # 读取反馈
        vel = axis.encoder.vel_estimate
        pos = axis.encoder.pos_estimate
        iq = axis.motor.current_control.Iq_measured
        voltage = axis.vbus_voltage
        
        print(f"  实际速度: {vel:.2f} turns/s")
        print(f"  位置: {pos:.2f} turns")
        print(f"  电流: {iq:.2f} A")
        print(f"  母线电压: {voltage:.2f} V")
        
        # 检查错误
        if axis.error != 0:
            print(f"  ⚠ 轴错误: {hex(axis.error)}")
            break
    
    # 停止电机
    print("\n[3] 停止电机...")
    axis.controller.input_vel = 0.0
    time.sleep(1)
    
    # 退出闭环
    print("\n[4] 退出闭环...")
    axis.requested_state = AxisState.IDLE
    time.sleep(0.5)
    
    print("\n" + "=" * 60)
    print("✓ 测试完成")
    print("=" * 60)
    
    # 最终状态
    print(f"\n最终状态:")
    print(f"  轴状态: {axis.current_state}")
    print(f"  轴错误: {hex(axis.error)}")
    print(f"  位置: {axis.encoder.pos_estimate:.2f} turns")
    print(f"  速度: {axis.encoder.vel_estimate:.2f} turns/s")

if __name__ == "__main__":
    main()
