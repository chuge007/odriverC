#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复电机震动问题（不保存配置，立即测试）
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
    print(f"  current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 降低PID增益以减少震动
    print("\n[2] 调整PID增益（大幅降低以减少震动）...")
    
    # 极低的速度增益
    axis.controller.config.vel_gain = 0.01  # 非常低
    print(f"✓ vel_gain: {axis.controller.config.vel_gain}")
    
    # 极低的速度积分增益
    axis.controller.config.vel_integrator_gain = 0.02  # 非常低
    print(f"✓ vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    # 降低位置增益
    axis.controller.config.pos_gain = 5.0
    print(f"✓ pos_gain: {axis.controller.config.pos_gain}")
    
    # 降低电流控制带宽
    axis.motor.config.current_control_bandwidth = 100.0
    print(f"✓ current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    
    # 设置速度限制
    print("\n[3] 设置速度限制...")
    axis.controller.config.vel_limit = 20.0
    axis.controller.config.vel_limit_tolerance = 2.0
    print(f"✓ vel_limit: {axis.controller.config.vel_limit}")
    
    # 清除错误
    print("\n[4] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 进入闭环
    print("\n[5] 进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(1)
    
    if axis.current_state == 8:
        print("✓ 成功进入闭环控制")
        
        # 测试低速运行
        print("\n[6] 测试低速运行（观察震动情况）...")
        print("=" * 60)
        
        test_speeds = [0.2, 0.5, 1.0, 1.5]
        
        for speed in test_speeds:
            print(f"\n>>> 设置速度: {speed} turn/s")
            axis.controller.input_vel = speed
            time.sleep(3)
            
            # 读取反馈
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            voltage = axis.vbus_voltage
            
            print(f"    实际速度: {vel:.3f} turns/s (误差: {abs(vel-speed):.3f})")
            print(f"    电流: {iq:.2f} A")
            print(f"    母线电压: {voltage:.2f} V")
            
            # 检查速度稳定性（采样10次）
            velocities = []
            for _ in range(10):
                velocities.append(axis.encoder.vel_estimate)
                time.sleep(0.05)
            
            avg_vel = sum(velocities) / len(velocities)
            vel_std = (sum([(v - avg_vel)**2 for v in velocities]) / len(velocities))**0.5
            
            print(f"    速度波动(标准差): {vel_std:.3f} turns/s")
            
            if vel_std > 0.3:
                print(f"    ⚠ 速度波动较大，震动明显")
            elif vel_std > 0.1:
                print(f"    ⚠ 有轻微震动")
            else:
                print(f"    ✓ 运行平稳")
            
            if axis.error != 0:
                print(f"    ✗ 出现错误: {hex(axis.error)}")
                break
        
        # 停止
        print("\n[7] 停止电机...")
        axis.controller.input_vel = 0.0
        time.sleep(1)
        
        print("\n" + "=" * 60)
        print("✓ 测试完成")
        print("=" * 60)
        
        print("\n最终配置:")
        print(f"  vel_gain: {axis.controller.config.vel_gain}")
        print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
        print(f"  pos_gain: {axis.controller.config.pos_gain}")
        print(f"  current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
        
        print("\n震动原因分析:")
        print("1. PID增益过高 - 已降低vel_gain和vel_integrator_gain")
        print("2. 电流环带宽过高 - 已从1000降到100")
        print("3. 速度反馈噪声 - 低增益可以减少噪声影响")
        
        print("\n如果仍有震动:")
        print("1. 进一步降低vel_gain到0.005")
        print("2. 进一步降低vel_integrator_gain到0.01")
        print("3. 检查编码器连接和安装")
        print("4. 检查电机相线连接")
        print("5. 提高供电电压（当前可能偏低）")
        print("6. 检查机械负载是否平衡")
        
        print("\n如果效果好，可以保存配置:")
        print("  python -c \"import odrive; odrv=odrive.find_any(); odrv.save_configuration()\"")
        
    else:
        print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
        print(f"  轴错误: {hex(axis.error)}")
        print(f"  电机错误: {hex(axis.motor.error)}")
        print(f"  控制器错误: {hex(axis.controller.error)}")

if __name__ == "__main__":
    main()
