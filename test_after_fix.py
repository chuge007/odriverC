#!/usr/bin/env python3
"""
修复后测试脚本
验证震动是否消失
"""

import odrive
from odrive.enums import *
import time

print("="*60)
print("  修复后测试 - 验证震动是否消失")
print("="*60)

# 连接
print("\n正在连接 ODrive...")
try:
    odrv0 = odrive.find_any()
    print("✅ 已连接")
except Exception as e:
    print(f"❌ 无法连接: {e}")
    exit(1)

axis = odrv0.axis0

# 显示当前配置
print("\n" + "="*60)
print("  当前配置")
print("="*60)
print(f"速度增益: {axis.controller.config.vel_gain:.6f}")
print(f"速度积分增益: {axis.controller.config.vel_integrator_gain:.6f}")
print(f"电流限制: {axis.motor.config.current_lim:.1f} A")
print(f"速度限制: {axis.controller.config.vel_limit:.1f} 圈/秒")

# 检查错误
print("\n" + "="*60)
print("  检查错误")
print("="*60)
print(f"轴错误: 0x{axis.error:08X}")
print(f"电机错误: 0x{axis.motor.error:08X}")
print(f"编码器错误: 0x{axis.encoder.error:08X}")
print(f"控制器错误: 0x{axis.controller.error:08X}")

if axis.error != 0:
    print("\n⚠️ 发现错误，正在清除...")
    # 使用正确的方法清除错误
    axis.error = 0
    axis.motor.error = 0
    axis.encoder.error = 0
    axis.controller.error = 0
    time.sleep(0.5)
    print("✅ 错误已清除")

# 进入闭环
print("\n" + "="*60)
print("  进入闭环控制")
print("="*60)

print("正在进入闭环...")
axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
time.sleep(2)

if axis.error != 0:
    print(f"❌ 进入闭环失败！错误: 0x{axis.error:08X}")
    exit(1)

print(f"✅ 已进入闭环 (状态: {axis.current_state})")

# 测试不同速度
print("\n" + "="*60)
print("  速度测试")
print("="*60)

test_speeds = [0.5, 1.0, 2.0, 3.0]

for speed in test_speeds:
    print(f"\n测试速度: {speed} 圈/秒")
    print("-" * 60)
    
    axis.controller.input_vel = speed
    
    # 运行3秒并监控
    for i in range(3):
        time.sleep(1)
        
        vel = axis.encoder.vel_estimate
        iq_setpoint = axis.motor.current_control.Iq_setpoint
        iq_measured = axis.motor.current_control.Iq_measured
        iq_error = abs(iq_setpoint - iq_measured)
        
        # 显示数据
        status = "✅" if iq_error < 1.0 else "⚠️"
        print(f"  [{i+1}s] {status} 速度: {vel:6.3f} 圈/秒 | Iq: {iq_measured:6.3f}A | 误差: {iq_error:5.3f}A")
        
        # 检查错误
        if axis.error != 0:
            print(f"  ❌ 出现错误: 0x{axis.error:08X}")
            break
    
    # 检查是否有错误
    if axis.error != 0:
        print(f"\n❌ 测试中断，错误: 0x{axis.error:08X}")
        break

# 停止电机
print("\n" + "="*60)
print("  停止电机")
print("="*60)

print("正在停止...")
axis.controller.input_vel = 0.0
time.sleep(2)

print("退出闭环...")
axis.requested_state = AXIS_STATE_IDLE
time.sleep(1)

# 最终检查
print("\n" + "="*60)
print("  测试结果")
print("="*60)

if axis.error == 0:
    print("✅ 测试成功！电机运行正常")
    print("\n观察结果:")
    print("  - 如果电流误差 < 1.0A，说明控制稳定")
    print("  - 如果速度稳定，说明震动已消除")
    print("  - 如果没有错误，说明配置正确")
else:
    print(f"❌ 测试失败，错误: 0x{axis.error:08X}")

print("\n" + "="*60)
print("  建议")
print("="*60)

print("\n如果震动已消除:")
print("  ✅ 修复成功！可以在 Qt 程序中正常使用")

print("\n如果仍有轻微震动:")
print("  1. 进一步降低 vel_gain 到 0.02")
print("  2. 运行 monitor_vibration_realtime.py 查看详细数据")
print("  3. 检查编码器和电机连接")

print("\n" + "="*60)
print("测试完成！")
print("="*60)
