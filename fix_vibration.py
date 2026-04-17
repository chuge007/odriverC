#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复电机震动问题
震动通常由以下原因引起：
1. PID增益过高（特别是vel_gain和vel_integrator_gain）
2. 速度反馈不稳定
3. 电流环增益不匹配
4. 编码器噪声
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("修复电机震动问题")
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
    
    # 显示当前配置
    print(f"\n[诊断] 当前PID配置:")
    print(f"  pos_gain: {axis.controller.config.pos_gain}")
    print(f"  vel_gain: {axis.controller.config.vel_gain}")
    print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    print(f"\n[诊断] 电流环配置:")
    try:
        print(f"  current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    except:
        print(f"  current_control_bandwidth: N/A")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 降低PID增益以减少震动
    print("\n[2] 调整PID增益（降低以减少震动）...")
    
    # 大幅降低速度增益
    axis.controller.config.vel_gain = 0.02  # 从0.16降到0.02
    print(f"✓ vel_gain: {axis.controller.config.vel_gain}")
    
    # 大幅降低速度积分增益
    axis.controller.config.vel_integrator_gain = 0.04  # 从0.32降到0.04
    print(f"✓ vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    # 降低位置增益
    axis.controller.config.pos_gain = 10.0  # 从20降到10
    print(f"✓ pos_gain: {axis.controller.config.pos_gain}")
    
    # 调整电流控制带宽
    print("\n[3] 调整电流控制带宽...")
    try:
        axis.motor.config.current_control_bandwidth = 100.0  # 降低带宽
        print(f"✓ current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    except Exception as e:
        print(f"⚠ 无法设置current_control_bandwidth: {e}")
    
    # 设置合理的速度限制
    print("\n[4] 设置速度限制...")
    axis.controller.config.vel_limit = 15.0
    axis.controller.config.vel_limit_tolerance = 2.0
    print(f"✓ vel_limit: {axis.controller.config.vel_limit}")
    
    # 保存配置
    print("\n[5] 保存配置...")
    try:
        odrv.save_configuration()
        print("✓ 配置已保存")
        time.sleep(1)
    except Exception as e:
        print(f"⚠ 保存配置失败: {e}")
    
    # 清除错误
    print("\n[6] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 进入闭环
    print("\n[7] 进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(1)
    
    if axis.current_state == 8:
        print("✓ 成功进入闭环控制")
        
        # 测试低速运行
        print("\n[8] 测试低速运行（观察震动情况）...")
        
        test_speeds = [0.2, 0.5, 1.0]
        
        for speed in test_speeds:
            print(f"\n设置速度: {speed} turn/s")
            axis.controller.input_vel = speed
            time.sleep(3)
            
            # 读取反馈
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            voltage = axis.vbus_voltage
            
            print(f"  实际速度: {vel:.3f} turns/s")
            print(f"  电流: {iq:.2f} A")
            print(f"  母线电压: {voltage:.2f} V")
            
            # 检查速度稳定性
            velocities = []
            for _ in range(10):
                velocities.append(axis.encoder.vel_estimate)
                time.sleep(0.05)
            
            vel_std = sum([(v - vel)**2 for v in velocities])**0.5 / len(velocities)
            print(f"  速度波动: {vel_std:.3f} turns/s")
            
            if vel_std > 0.5:
                print(f"  ⚠ 速度波动较大，可能仍有震动")
            else:
                print(f"  ✓ 速度稳定")
            
            if axis.error != 0:
                print(f"  ⚠ 出现错误: {hex(axis.error)}")
                break
        
        # 停止
        print("\n[9] 停止电机...")
        axis.controller.input_vel = 0.0
        time.sleep(1)
        
        print("\n" + "=" * 60)
        print("✓ 测试完成")
        print("=" * 60)
        
        print("\n最终配置:")
        print(f"  vel_gain: {axis.controller.config.vel_gain}")
        print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
        print(f"  pos_gain: {axis.controller.config.pos_gain}")
        
        print("\n如果仍有震动，可以尝试:")
        print("1. 进一步降低vel_gain（例如0.01）")
        print("2. 进一步降低vel_integrator_gain（例如0.02）")
        print("3. 检查机械连接是否松动")
        print("4. 检查编码器安装是否牢固")
        print("5. 提高供电电压到12V以上")
        
    else:
        print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
        print(f"  轴错误: {hex(axis.error)}")

if __name__ == "__main__":
    main()
