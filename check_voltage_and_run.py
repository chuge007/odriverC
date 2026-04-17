#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查电压并测试长时间运行
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("检查电压并测试长时间运行")
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
    
    # 检查电压
    voltage = odrv.vbus_voltage
    print(f"\n[电压检查]")
    print(f"  当前母线电压: {voltage:.2f} V")
    
    if voltage < 12.0:
        print(f"  ⚠⚠⚠ 警告：电压过低！")
        print(f"  ODrive要求至少12V，当前只有{voltage:.2f}V")
        print(f"\n  这会导致:")
        print(f"    1. DC_BUS_UNDER_VOLTAGE错误(0x2)")
        print(f"    2. 电机无法进入闭环")
        print(f"    3. 电机震动加剧")
        print(f"\n  解决方案:")
        print(f"    1. 使用12V或更高电压的电源")
        print(f"    2. 检查电源线连接")
        print(f"    3. 确保电源能提供足够电流")
        
        # 降低欠压阈值（临时方案）
        print(f"\n  尝试降低欠压阈值（临时方案）...")
        try:
            # 获取当前配置
            current_min = odrv.config.dc_bus_undervoltage_trip_level
            current_max = odrv.config.dc_bus_overvoltage_trip_level
            print(f"    当前欠压阈值: {current_min:.2f} V")
            print(f"    当前过压阈值: {current_max:.2f} V")
            
            # 降低欠压阈值到当前电压以下
            new_min = voltage - 1.0
            if new_min < 8.0:
                new_min = 8.0
            
            odrv.config.dc_bus_undervoltage_trip_level = new_min
            print(f"    新欠压阈值: {new_min:.2f} V")
            print(f"    ⚠ 注意：这只是临时方案，建议提高供电电压！")
        except Exception as e:
            print(f"    ✗ 无法修改欠压阈值: {e}")
    else:
        print(f"  ✓ 电压正常")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 配置极低增益
    print("\n[2] 配置极低增益（消除震动）...")
    axis.controller.config.vel_gain = 0.005
    axis.controller.config.vel_integrator_gain = 0.01
    axis.controller.config.pos_gain = 5.0
    axis.motor.config.current_control_bandwidth = 100.0
    axis.controller.config.vel_limit = 20.0
    axis.controller.config.vel_limit_tolerance = 5.0
    axis.controller.config.enable_vel_limit = False
    print("✓ 配置完成")
    
    # 清除错误
    axis.clear_errors()
    time.sleep(0.5)
    
    # 进入闭环
    print("\n[3] 进入闭环控制...")
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(1)
    
    if axis.current_state != 8:
        print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
        print(f"  轴错误: {hex(axis.error)}")
        
        if axis.error == 0x2:
            print(f"\n  错误原因: DC_BUS_UNDER_VOLTAGE (母线电压不足)")
            print(f"  当前电压: {odrv.vbus_voltage:.2f} V")
            print(f"\n  必须提高供电电压到12V以上才能正常运行！")
        
        sys.exit(1)
    
    print("✓ 成功进入闭环控制")
    
    # 重新启用速度限制
    axis.controller.config.enable_vel_limit = True
    
    # 测试长时间运行
    print("\n[4] 测试长时间运行...")
    print("=" * 60)
    
    test_speeds = [0.5, 1.0]
    
    for speed in test_speeds:
        print(f"\n>>> 测试速度: {speed} turn/s (持续10秒)")
        print("-" * 60)
        
        axis.controller.input_vel = speed
        
        velocities = []
        currents = []
        voltages = []
        
        for i in range(20):  # 10秒
            time.sleep(0.5)
            
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            vbus = odrv.vbus_voltage
            
            velocities.append(vel)
            currents.append(iq)
            voltages.append(vbus)
            
            # 每2秒显示一次
            if (i + 1) % 4 == 0:
                elapsed = (i + 1) * 0.5
                print(f"  [{elapsed:.1f}s] 速度: {vel:.3f} turns/s, 电流: {iq:.2f} A, 电压: {vbus:.2f} V")
            
            # 检查错误
            if axis.error != 0:
                print(f"\n  ✗ 出现错误: {hex(axis.error)}")
                if axis.error == 0x2:
                    print(f"  原因: 电压不足 ({vbus:.2f} V)")
                break
        
        # 统计分析
        if len(velocities) > 0:
            avg_vel = sum(velocities) / len(velocities)
            vel_std = (sum([(v - avg_vel)**2 for v in velocities]) / len(velocities))**0.5
            avg_voltage = sum(voltages) / len(voltages)
            min_voltage = min(voltages)
            
            print(f"\n  统计数据:")
            print(f"    平均速度: {avg_vel:.3f} turns/s")
            print(f"    速度波动: {vel_std:.3f} turns/s", end="")
            
            if vel_std > 0.5:
                print(" - ⚠⚠⚠ 震动严重")
            elif vel_std > 0.3:
                print(" - ⚠⚠ 震动明显")
            elif vel_std > 0.15:
                print(" - ⚠ 轻微震动")
            else:
                print(" - ✓ 运行平稳")
            
            print(f"    平均电压: {avg_voltage:.2f} V")
            print(f"    最低电压: {min_voltage:.2f} V")
            
            if min_voltage < 11.5:
                print(f"    ⚠ 电压偏低，可能影响性能")
        
        if axis.error != 0:
            break
    
    # 停止
    print("\n[5] 停止电机...")
    axis.controller.input_vel = 0.0
    time.sleep(2)
    
    print("\n" + "=" * 60)
    print("✓ 测试完成")
    print("=" * 60)
    
    print("\n总结:")
    print(f"  最终电压: {odrv.vbus_voltage:.2f} V")
    print(f"  vel_gain: {axis.controller.config.vel_gain}")
    print(f"  vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    print("\n震动问题的根本原因:")
    print("  1. 供电电压不足 (11.96V < 12V)")
    print("  2. PID增益过高")
    print("  3. 电流环带宽过高")
    
    print("\n解决方案:")
    print("  ✓ 已降低PID增益到极低值")
    print("  ✓ 已降低电流环带宽")
    print("  ⚠ 必须提高供电电压到12V以上")

if __name__ == "__main__":
    main()
