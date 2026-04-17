#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试电机长时间转动（5秒以上）
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("测试电机长时间转动")
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
    print(f"\n[当前配置]")
    print(f"  vel_gain: {axis.controller.config.vel_gain}")
    print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    print(f"  pos_gain: {axis.controller.config.pos_gain}")
    print(f"  current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    print(f"  vel_limit: {axis.controller.config.vel_limit}")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 配置极低增益以消除震动
    print("\n[2] 配置极低增益...")
    axis.controller.config.vel_gain = 0.005
    axis.controller.config.vel_integrator_gain = 0.01
    axis.controller.config.pos_gain = 5.0
    axis.motor.config.current_control_bandwidth = 100.0
    axis.controller.config.vel_limit = 20.0
    axis.controller.config.vel_limit_tolerance = 5.0
    print("✓ 配置完成")
    
    # 禁用速度限制检查
    print("\n[3] 暂时禁用速度限制...")
    axis.controller.config.enable_vel_limit = False
    
    # 清除错误
    axis.clear_errors()
    time.sleep(0.5)
    
    # 进入闭环
    print("\n[4] 进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(1)
    
    if axis.current_state != 8:
        print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
        print(f"  轴错误: {hex(axis.error)}")
        print(f"  电机错误: {hex(axis.motor.error)}")
        print(f"  控制器错误: {hex(axis.controller.error)}")
        sys.exit(1)
    
    print("✓ 成功进入闭环控制")
    
    # 重新启用速度限制
    print("\n[5] 重新启用速度限制...")
    axis.controller.config.enable_vel_limit = True
    
    # 测试不同速度，每个速度持续10秒
    test_speeds = [0.5, 1.0, 1.5]
    
    print("\n[6] 开始长时间测试...")
    print("=" * 60)
    
    for speed in test_speeds:
        print(f"\n>>> 测试速度: {speed} turn/s (持续10秒)")
        print("-" * 60)
        
        axis.controller.input_vel = speed
        
        # 持续监控10秒
        velocities = []
        currents = []
        
        for i in range(20):  # 10秒，每0.5秒采样一次
            time.sleep(0.5)
            
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            voltage = axis.vbus_voltage
            
            velocities.append(vel)
            currents.append(iq)
            
            # 每2秒显示一次状态
            if (i + 1) % 4 == 0:
                elapsed = (i + 1) * 0.5
                avg_vel = sum(velocities[-8:]) / min(8, len(velocities))
                avg_iq = sum(currents[-8:]) / min(8, len(currents))
                
                print(f"  [{elapsed:.1f}s] 速度: {vel:.3f} turns/s, 电流: {iq:.2f} A, 电压: {voltage:.2f} V")
            
            # 检查错误
            if axis.error != 0:
                print(f"\n  ✗ 出现错误: {hex(axis.error)}")
                print(f"    电机错误: {hex(axis.motor.error)}")
                print(f"    控制器错误: {hex(axis.controller.error)}")
                break
        
        # 分析速度稳定性
        if len(velocities) > 0:
            avg_vel = sum(velocities) / len(velocities)
            vel_std = (sum([(v - avg_vel)**2 for v in velocities]) / len(velocities))**0.5
            vel_min = min(velocities)
            vel_max = max(velocities)
            
            print(f"\n  统计数据:")
            print(f"    平均速度: {avg_vel:.3f} turns/s")
            print(f"    速度范围: {vel_min:.3f} ~ {vel_max:.3f} turns/s")
            print(f"    速度波动(标准差): {vel_std:.3f} turns/s")
            print(f"    设定误差: {abs(avg_vel - speed):.3f} turns/s")
            
            # 判断震动程度
            if vel_std > 0.5:
                print(f"    评价: ⚠⚠⚠ 震动严重")
            elif vel_std > 0.3:
                print(f"    评价: ⚠⚠ 震动明显")
            elif vel_std > 0.15:
                print(f"    评价: ⚠ 有轻微震动")
            else:
                print(f"    评价: ✓ 运行平稳")
        
        if axis.error != 0:
            break
    
    # 停止电机
    print("\n[7] 停止电机...")
    axis.controller.input_vel = 0.0
    time.sleep(2)
    
    print("\n" + "=" * 60)
    print("✓ 测试完成")
    print("=" * 60)
    
    print("\n最终配置:")
    print(f"  vel_gain: {axis.controller.config.vel_gain}")
    print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    print(f"  pos_gain: {axis.controller.config.pos_gain}")
    print(f"  current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    
    print("\n震动分析:")
    print("如果速度波动(标准差):")
    print("  > 0.5 turns/s → 震动严重，需要进一步降低增益")
    print("  0.3-0.5 → 震动明显，建议降低增益")
    print("  0.15-0.3 → 轻微震动，可接受或微调")
    print("  < 0.15 → 运行平稳")
    
    print("\n如果震动严重，尝试:")
    print("  1. vel_gain降到0.002")
    print("  2. vel_integrator_gain降到0.005")
    print("  3. 检查机械连接")
    print("  4. 提高供电电压")

if __name__ == "__main__":
    main()
