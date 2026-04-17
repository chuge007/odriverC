#!/usr/bin/env python3
"""
电机震动深度诊断脚本
使用 odrivetool 检查震动的根本原因
"""

import odrive
from odrive.enums import *
import time
import sys

def print_section(title):
    """打印分节标题"""
    print("\n" + "="*60)
    print(f"  {title}")
    print("="*60)

def check_errors(axis):
    """检查所有错误"""
    print_section("错误检查")
    
    print(f"轴错误 (axis.error): 0x{axis.error:08X}")
    if axis.error != 0:
        print("  ⚠️ 轴有错误！")
    
    print(f"电机错误 (motor.error): 0x{axis.motor.error:08X}")
    if axis.motor.error != 0:
        print("  ⚠️ 电机有错误！")
    
    print(f"编码器错误 (encoder.error): 0x{axis.encoder.error:08X}")
    if axis.encoder.error != 0:
        print("  ⚠️ 编码器有错误！")
    
    print(f"控制器错误 (controller.error): 0x{axis.controller.error:08X}")
    if axis.controller.error != 0:
        print("  ⚠️ 控制器有错误！")
    
    print(f"无感错误 (sensorless_estimator.error): 0x{axis.sensorless_estimator.error:08X}")

def check_encoder(axis):
    """检查编码器状态"""
    print_section("编码器状态")
    
    print(f"编码器模式: {axis.encoder.config.mode}")
    print(f"CPR配置: {axis.encoder.config.cpr}")
    print(f"当前位置估算: {axis.encoder.pos_estimate:.3f} 圈")
    print(f"当前速度估算: {axis.encoder.vel_estimate:.3f} 圈/秒")
    print(f"影子计数: {axis.encoder.shadow_count}")
    print(f"CPR计数: {axis.encoder.count_in_cpr}")
    
    # 检查编码器是否准备好
    if not axis.encoder.is_ready:
        print("  ⚠️ 编码器未准备好！")
    else:
        print("  ✅ 编码器已准备好")

def check_motor_config(axis):
    """检查电机配置"""
    print_section("电机配置")
    
    print(f"电机类型: {axis.motor.config.motor_type}")
    print(f"极对数: {axis.motor.config.pole_pairs}")
    print(f"相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
    print(f"相电感: {axis.motor.config.phase_inductance:.9f} H")
    print(f"力矩常数: {axis.motor.config.torque_constant:.6f} Nm/A")
    
    # 检查校准状态
    if axis.motor.config.phase_resistance < 0.001:
        print("  ⚠️ 相电阻未校准！")
    if axis.motor.config.phase_inductance < 0.000001:
        print("  ⚠️ 相电感未校准！")

def check_controller_config(axis):
    """检查控制器配置"""
    print_section("控制器配置")
    
    print(f"控制模式: {axis.controller.config.control_mode}")
    print(f"输入模式: {axis.controller.config.input_mode}")
    print(f"位置增益: {axis.controller.config.pos_gain:.3f}")
    print(f"速度增益: {axis.controller.config.vel_gain:.6f}")
    print(f"速度积分增益: {axis.controller.config.vel_integrator_gain:.6f}")
    
    # 检查增益是否合理
    if axis.controller.config.pos_gain > 50:
        print("  ⚠️ 位置增益可能过高！建议 < 50")
    if axis.controller.config.vel_gain > 1.0:
        print("  ⚠️ 速度增益可能过高！建议 < 1.0")

def check_limits(axis):
    """检查限制设置"""
    print_section("限制设置")
    
    print(f"速度限制: {axis.controller.config.vel_limit:.3f} 圈/秒")
    print(f"电流限制: {axis.motor.config.current_lim:.3f} A")
    print(f"电流限制余量: {axis.motor.config.current_lim_margin:.3f} A")
    
    if axis.controller.config.vel_limit < 1.0:
        print("  ⚠️ 速度限制过低！")
    if axis.motor.config.current_lim < 10.0:
        print("  ⚠️ 电流限制可能过低！")

def check_current_state(axis):
    """检查当前运行状态"""
    print_section("当前运行状态")
    
    print(f"轴状态: {axis.current_state}")
    print(f"位置设定: {axis.controller.pos_setpoint:.3f} 圈")
    print(f"速度设定: {axis.controller.vel_setpoint:.3f} 圈/秒")
    print(f"力矩设定: {axis.controller.torque_setpoint:.3f} Nm")
    
    print(f"\nIq设定: {axis.motor.current_control.Iq_setpoint:.3f} A")
    print(f"Iq测量: {axis.motor.current_control.Iq_measured:.3f} A")
    print(f"Id设定: {axis.motor.current_control.Id_setpoint:.3f} A")
    print(f"Id测量: {axis.motor.current_control.Id_measured:.3f} A")
    
    # 检查电流是否异常
    iq_error = abs(axis.motor.current_control.Iq_setpoint - axis.motor.current_control.Iq_measured)
    if iq_error > 2.0:
        print(f"  ⚠️ Iq跟踪误差过大: {iq_error:.3f} A")

def check_power_supply(axis):
    """检查电源状态"""
    print_section("电源状态")
    
    print(f"母线电压: {axis.motor.current_control.v_current_control_integral_d:.3f} V")
    
    # 尝试获取总线电压（不同版本可能不同）
    try:
        vbus = axis.vbus_voltage
        print(f"总线电压: {vbus:.3f} V")
        if vbus < 12.0:
            print("  ⚠️ 电压过低！可能导致欠压保护")
        elif vbus < 18.0:
            print("  ⚠️ 电压偏低，建议 ≥ 24V")
    except:
        pass

def monitor_vibration(axis, duration=5):
    """监控震动 - 记录位置和速度的波动"""
    print_section(f"震动监控 ({duration}秒)")
    
    print("正在记录位置和速度数据...")
    
    positions = []
    velocities = []
    iq_values = []
    timestamps = []
    
    start_time = time.time()
    
    while time.time() - start_time < duration:
        try:
            pos = axis.encoder.pos_estimate
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            
            positions.append(pos)
            velocities.append(vel)
            iq_values.append(iq)
            timestamps.append(time.time() - start_time)
            
            time.sleep(0.01)  # 100Hz采样
        except KeyboardInterrupt:
            break
    
    # 分析数据
    if len(positions) > 10:
        import statistics
        
        pos_std = statistics.stdev(positions)
        vel_std = statistics.stdev(velocities)
        iq_std = statistics.stdev(iq_values)
        
        print(f"\n数据统计 (采样点: {len(positions)}):")
        print(f"位置标准差: {pos_std:.6f} 圈")
        print(f"速度标准差: {vel_std:.6f} 圈/秒")
        print(f"Iq标准差: {iq_std:.6f} A")
        
        # 判断震动程度
        print("\n震动评估:")
        if vel_std > 0.5:
            print(f"  🔴 速度波动严重！标准差 = {vel_std:.3f} 圈/秒")
        elif vel_std > 0.1:
            print(f"  🟡 速度有明显波动。标准差 = {vel_std:.3f} 圈/秒")
        else:
            print(f"  ✅ 速度相对稳定。标准差 = {vel_std:.3f} 圈/秒")
        
        if iq_std > 2.0:
            print(f"  🔴 电流波动严重！标准差 = {iq_std:.3f} A")
        elif iq_std > 0.5:
            print(f"  🟡 电流有明显波动。标准差 = {iq_std:.3f} A")
        else:
            print(f"  ✅ 电流相对稳定。标准差 = {iq_std:.3f} A")

def main():
    print("="*60)
    print("  ODrive 电机震动深度诊断工具")
    print("="*60)
    
    # 连接 ODrive
    print("\n正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any()
        print(f"✅ 已连接到 ODrive")
        print(f"   序列号: {odrv0.serial_number}")
        print(f"   固件版本: {odrv0.fw_version_major}.{odrv0.fw_version_minor}.{odrv0.fw_version_revision}")
    except Exception as e:
        print(f"❌ 无法连接到 ODrive: {e}")
        print("\n请确保:")
        print("  1. ODrive 已通过 USB 连接")
        print("  2. 已安装 odrive 工具: pip install odrive")
        return
    
    # 选择轴
    print("\n请选择要诊断的轴:")
    print("  0 - axis0")
    print("  1 - axis1")
    
    try:
        axis_num = int(input("输入轴编号 (0 或 1): "))
        if axis_num not in [0, 1]:
            print("无效的轴编号！")
            return
    except:
        print("无效的输入！")
        return
    
    axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
    print(f"\n已选择 axis{axis_num}")
    
    # 执行诊断
    check_errors(axis)
    check_encoder(axis)
    check_motor_config(axis)
    check_controller_config(axis)
    check_limits(axis)
    check_current_state(axis)
    check_power_supply(axis)
    
    # 如果电机在闭环状态，监控震动
    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
        print("\n电机正在闭环控制中，开始监控震动...")
        monitor_vibration(axis, duration=5)
    else:
        print(f"\n电机当前状态: {axis.current_state}")
        print("电机未在闭环状态，跳过震动监控")
        print("\n如需监控震动，请:")
        print("  1. 清除错误: odrv0.clear_errors()")
        print("  2. 进入闭环: odrv0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL")
        print("  3. 重新运行此脚本")
    
    # 总结
    print_section("诊断总结")
    
    has_issues = False
    
    if axis.error != 0:
        print("🔴 发现轴错误")
        has_issues = True
    
    if axis.motor.error != 0:
        print("🔴 发现电机错误")
        has_issues = True
    
    if axis.encoder.error != 0:
        print("🔴 发现编码器错误")
        has_issues = True
    
    if axis.controller.error != 0:
        print("🔴 发现控制器错误")
        has_issues = True
    
    if axis.controller.config.pos_gain > 50:
        print("🟡 位置增益可能过高")
        has_issues = True
    
    if axis.controller.config.vel_gain > 1.0:
        print("🟡 速度增益可能过高")
        has_issues = True
    
    if axis.motor.config.current_lim < 10.0:
        print("🟡 电流限制可能过低")
        has_issues = True
    
    if not has_issues:
        print("✅ 未发现明显的配置问题")
        print("\n如果仍然震动，可能的原因:")
        print("  1. 机械负载不平衡")
        print("  2. 编码器安装不牢固")
        print("  3. 电源纹波过大")
        print("  4. PID参数需要微调")
    
    print("\n" + "="*60)
    print("诊断完成！")
    print("="*60)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
