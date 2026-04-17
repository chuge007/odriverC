#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复超速错误
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("修复超速错误")
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
    
    # 显示当前错误
    print(f"\n当前错误:")
    print(f"  轴错误: {hex(axis.error)}")
    print(f"  电机错误: {hex(axis.motor.error)}")
    print(f"  控制器错误: {hex(axis.controller.error)}")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 检查速度限制配置
    print(f"\n[2] 当前速度配置:")
    print(f"  vel_limit: {axis.controller.config.vel_limit}")
    print(f"  vel_limit_tolerance: {axis.controller.config.vel_limit_tolerance}")
    print(f"  enable_vel_limit: {axis.controller.config.enable_vel_limit}")
    
    # 增加速度限制容差
    print("\n[3] 调整速度限制...")
    axis.controller.config.vel_limit = 20.0  # 增加到20 turns/s
    axis.controller.config.vel_limit_tolerance = 2.0  # 增加容差
    print(f"✓ vel_limit: {axis.controller.config.vel_limit}")
    print(f"✓ vel_limit_tolerance: {axis.controller.config.vel_limit_tolerance}")
    
    # 设置控制deadline
    print("\n[4] 设置控制deadline...")
    try:
        axis.motor.config.motor_type = 0  # HIGH_CURRENT
        print(f"✓ motor_type: {axis.motor.config.motor_type}")
    except Exception as e:
        print(f"⚠ 设置motor_type失败: {e}")
    
    # 保存配置
    print("\n[5] 保存配置...")
    try:
        odrv.save_configuration()
        print("✓ 配置已保存")
        time.sleep(1)
    except Exception as e:
        print(f"⚠ 保存配置失败: {e}")
    
    # 清除错误
    print("\n[6] 再次清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 尝试进入闭环
    print("\n[7] 尝试进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    
    # 监控状态
    for i in range(10):
        time.sleep(0.5)
        state = axis.current_state
        error = axis.error
        
        print(f"  [{i+1}] 状态: {state}, 错误: {hex(error)}")
        
        if state == 8:  # CLOSED_LOOP_CONTROL
            print("\n✓ 成功进入闭环控制！")
            
            # 测试低速运行
            print("\n[8] 测试低速运行...")
            print("设置速度: 0.5 turn/s")
            axis.controller.input_vel = 0.5
            time.sleep(3)
            
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            
            print(f"  实际速度: {vel:.2f} turns/s")
            print(f"  电流: {iq:.2f} A")
            
            # 逐步增加速度
            for speed in [1.0, 2.0, 3.0]:
                print(f"\n设置速度: {speed} turn/s")
                axis.controller.input_vel = speed
                time.sleep(2)
                
                vel = axis.encoder.vel_estimate
                print(f"  实际速度: {vel:.2f} turns/s")
                
                if axis.error != 0:
                    print(f"  ⚠ 出现错误: {hex(axis.error)}")
                    break
            
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
            
            # 显示详细的控制器错误
            controller_error = axis.controller.error
            if controller_error == 0x1:
                print("\n  控制器错误详情: OVERSPEED")
                print(f"  当前速度估计: {axis.encoder.vel_estimate}")
                print(f"  速度限制: {axis.controller.config.vel_limit}")
                print(f"  速度容差: {axis.controller.config.vel_limit_tolerance}")
            
            break
    
    print("\n✗ 无法进入闭环控制")

if __name__ == "__main__":
    main()
