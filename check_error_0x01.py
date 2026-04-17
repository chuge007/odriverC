#!/usr/bin/env python3
"""
检查错误 0x00000001
"""

import odrive
from odrive.enums import *
import time

print("="*60)
print("  检查错误 0x00000001")
print("="*60)

# 连接
print("\n正在连接 ODrive...")
odrv0 = odrive.find_any()
print("✅ 已连接")

axis = odrv0.axis0

print("\n" + "="*60)
print("  详细错误信息")
print("="*60)

print(f"轴错误: 0x{axis.error:08X}")
print(f"电机错误: 0x{axis.motor.error:08X}")
print(f"编码器错误: 0x{axis.encoder.error:08X}")
print(f"控制器错误: 0x{axis.controller.error:08X}")

# 0x00000001 通常是 AXIS_ERROR_INVALID_STATE
print("\n错误 0x00000001 = AXIS_ERROR_INVALID_STATE")
print("这意味着轴无法进入请求的状态")

print("\n可能的原因:")
print("1. 电机未校准")
print("2. 编码器未准备好")
print("3. 缺少必要的配置")

print("\n" + "="*60)
print("  检查校准状态")
print("="*60)

print(f"电机已校准: {axis.motor.is_calibrated}")
print(f"编码器已准备: {axis.encoder.is_ready}")
print(f"编码器索引已找到: {axis.encoder.index_found}")

print(f"\n相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
print(f"相电感: {axis.motor.config.phase_inductance:.9f} H")

if axis.motor.config.phase_resistance < 0.001:
    print("⚠️ 电机未校准！需要运行校准")

print("\n" + "="*60)
print("  检查配置")
print("="*60)

print(f"电机类型: {axis.motor.config.motor_type}")
print(f"编码器模式: {axis.encoder.config.mode}")
print(f"编码器CPR: {axis.encoder.config.cpr}")
print(f"极对数: {axis.motor.config.pole_pairs}")

print("\n" + "="*60)
print("  解决方案")
print("="*60)

if not axis.motor.is_calibrated:
    print("\n电机未校准，需要运行校准:")
    print("1. 确保电机可以自由转动（无负载）")
    print("2. 运行: axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE")
    print("3. 等待校准完成（约10-30秒）")
    print("4. 保存配置: odrv0.save_configuration()")
    
    response = input("\n是否现在运行校准? (y/n): ").lower()
    if response == 'y':
        print("\n开始校准...")
        print("⚠️ 请确保电机可以自由转动！")
        time.sleep(2)
        
        axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
        print("校准中，请等待...")
        
        # 等待校准完成
        while axis.current_state != AXIS_STATE_IDLE:
            time.sleep(1)
            print(f"  状态: {axis.current_state}")
        
        print("\n校准完成！")
        print(f"轴错误: 0x{axis.error:08X}")
        
        if axis.error == 0:
            print("✅ 校准成功！")
            
            save = input("\n保存配置? (y/n): ").lower()
            if save == 'y':
                odrv0.save_configuration()
                print("✅ 配置已保存")
        else:
            print(f"❌ 校准失败，错误: 0x{axis.error:08X}")
else:
    print("\n✅ 电机已校准")
    print("\n可能需要:")
    print("1. 清除错误: axis.error = 0")
    print("2. 检查编码器连接")
    print("3. 确保配置正确")
