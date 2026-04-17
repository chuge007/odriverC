#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
重启ODrive并修复震动问题
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("重启ODrive并修复震动问题")
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
    print(f"\n[诊断] 当前状态:")
    print(f"  轴状态: {axis.current_state}")
    print(f"  轴错误: {hex(axis.error)}")
    print(f"  速度估计: {axis.encoder.vel_estimate} turns/s")
    print(f"  位置估计: {axis.encoder.pos_estimate} turns")
    
    # 清除错误
    print("\n[1] 清除错误...")
    axis.clear_errors()
    time.sleep(0.5)
    
    # 擦除配置，恢复出厂设置
    print("\n[2] 擦除配置，恢复出厂设置...")
    print("⚠ 这将清除所有保存的配置！")
    
    try:
        odrv.erase_configuration()
        print("✓ 配置已擦除")
        print("\n正在重启ODrive...")
        time.sleep(3)
    except Exception as e:
        print(f"✗ 擦除配置失败: {e}")
        print("\n手动重启ODrive:")
        print("  1. 断开USB连接")
        print("  2. 等待3秒")
        print("  3. 重新连接USB")
        print("  4. 再次运行此脚本")
        sys.exit(1)
    
    # 重新查找设备
    print("\n[3] 重新查找ODrive设备...")
    for i in range(10):
        try:
            odrv = odrive.find_any()
            print(f"✓ 找到ODrive设备")
            break
        except:
            print(f"  尝试 {i+1}/10...")
            time.sleep(2)
    else:
        print("✗ 无法重新连接ODrive")
        sys.exit(1)
    
    axis = odrv.axis0
    
    # 配置电机参数
    print("\n[4] 配置电机参数...")
    
    # 基本配置
    axis.motor.config.current_lim = 10.0
    axis.motor.config.calibration_current = 5.0
    print(f"✓ current_lim: {axis.motor.config.current_lim}")
    print(f"✓ calibration_current: {axis.motor.config.calibration_current}")
    
    # 控制器配置（极低增益以消除震动）
    axis.controller.config.control_mode = 2  # VELOCITY_CONTROL
    axis.controller.config.input_mode = 1    # PASSTHROUGH
    axis.controller.config.vel_limit = 20.0
    axis.controller.config.vel_limit_tolerance = 5.0  # 增大容差
    axis.controller.config.enable_vel_limit = True
    
    axis.controller.config.pos_gain = 5.0
    axis.controller.config.vel_gain = 0.005  # 极低
    axis.controller.config.vel_integrator_gain = 0.01  # 极低
    
    print(f"✓ control_mode: VELOCITY_CONTROL")
    print(f"✓ vel_limit: {axis.controller.config.vel_limit}")
    print(f"✓ vel_limit_tolerance: {axis.controller.config.vel_limit_tolerance}")
    print(f"✓ pos_gain: {axis.controller.config.pos_gain}")
    print(f"✓ vel_gain: {axis.controller.config.vel_gain}")
    print(f"✓ vel_integrator_gain: {axis.controller.config.vel_integrator_gain}")
    
    # 降低电流控制带宽
    axis.motor.config.current_control_bandwidth = 100.0
    print(f"✓ current_control_bandwidth: {axis.motor.config.current_control_bandwidth}")
    
    # 保存配置
    print("\n[5] 保存配置...")
    odrv.save_configuration()
    print("✓ 配置已保存")
    
    print("\n[6] 重启ODrive以应用配置...")
    try:
        odrv.reboot()
        print("✓ ODrive正在重启...")
        time.sleep(5)
    except:
        pass
    
    # 重新连接
    print("\n[7] 重新连接ODrive...")
    for i in range(10):
        try:
            odrv = odrive.find_any()
            print(f"✓ 已重新连接")
            break
        except:
            print(f"  尝试 {i+1}/10...")
            time.sleep(2)
    else:
        print("✗ 无法重新连接ODrive")
        print("\n请手动:")
        print("1. 运行校准: python -c \"import odrive; odrv=odrive.find_any(); odrv.axis0.requested_state=3\"")
        print("2. 运行测试: python fix_vibration_final.py")
        sys.exit(1)
    
    axis = odrv.axis0
    
    print("\n" + "=" * 60)
    print("✓ 配置完成")
    print("=" * 60)
    
    print("\n下一步:")
    print("1. 如果电机未校准，运行校准:")
    print("   python -c \"import odrive; odrv=odrive.find_any(); odrv.axis0.requested_state=3\"")
    print("\n2. 然后测试闭环驱动:")
    print("   python fix_vibration_final.py")

if __name__ == "__main__":
    main()
