#!/usr/bin/env python3
"""
验证震动修复效果
"""

import odrive
from odrive.enums import *
import time

print("="*60)
print("  验证震动修复效果")
print("="*60)

print("\n等待 ODrive 重启完成...")
time.sleep(5)

print("正在连接 ODrive...")
try:
    odrv0 = odrive.find_any()
    print("✅ 已连接")
except Exception as e:
    print(f"❌ 无法连接: {e}")
    print("请等待几秒后手动重试")
    exit(1)

axis = odrv0.axis0

print("\n" + "="*60)
print("  验证新参数")
print("="*60)
print(f"位置增益: {axis.controller.config.pos_gain:.3f}")
print(f"速度增益: {axis.controller.config.vel_gain:.6f} ← 已降低")
print(f"速度积分增益: {axis.controller.config.vel_integrator_gain:.6f} ← 已降低")
print(f"电流限制: {axis.motor.config.current_lim:.1f} A ← 已增加")

print("\n" + "="*60)
print("  测试电机")
print("="*60)

# 清除错误
odrv0.clear_errors()
time.sleep(0.5)

print("\n进入闭环控制...")
axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
time.sleep(2)

if axis.error != 0:
    print(f"❌ 错误: 0x{axis.error:08X}")
    exit(1)

print("✅ 已进入闭环")

# 测试不同速度
speeds = [0.5, 1.0, 2.0]

for speed in speeds:
    print(f"\n测试速度: {speed} 圈/秒")
    axis.controller.input_vel = speed
    
    print("运行3秒...")
    for i in range(3):
        time.sleep(1)
        vel = axis.encoder.vel_estimate
        iq = axis.motor.current_control.Iq_measured
        iq_setpoint = axis.motor.current_control.Iq_setpoint
        iq_error = abs(iq_setpoint - iq)
        
        print(f"  [{i+1}s] 速度: {vel:.3f}, Iq: {iq:.3f}A, 误差: {iq_error:.3f}A")
        
        if iq_error > 1.5:
            print(f"    ⚠️ 电流误差较大")

# 停止
print("\n停止电机...")
axis.controller.input_vel = 0.0
time.sleep(1)
axis.requested_state = AXIS_STATE_IDLE

print("\n" + "="*60)
print("  测试完成")
print("="*60)

print("\n✅ 修复总结:")
print("  1. 速度增益从 0.083 降低到 0.050")
print("  2. 速度积分增益从 0.320 降低到 0.200")
print("  3. 电流限制从 10A 增加到 20A")
print("\n如果震动明显减少，说明修复成功！")
print("如果仍有震动，可以进一步降低 vel_gain 到 0.02")
