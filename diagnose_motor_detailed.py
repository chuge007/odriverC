#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""详细诊断电机状态"""

import odrive
import sys
import time

# 状态名称映射
AXIS_STATE_NAMES = {
    0: "UNDEFINED",
    1: "IDLE",
    2: "STARTUP_SEQUENCE",
    3: "FULL_CALIBRATION_SEQUENCE",
    4: "MOTOR_CALIBRATION",
    5: "ENCODER_INDEX_SEARCH",
    6: "ENCODER_OFFSET_CALIBRATION",
    7: "CLOSED_LOOP_CONTROL",
    8: "LOCKIN_SPIN",
    9: "ENCODER_DIR_FIND",
    10: "HOMING",
    11: "ENCODER_HALL_POLARITY_CALIBRATION",
    12: "ENCODER_HALL_PHASE_CALIBRATION",
}

def diagnose_motor(odrv, axis_num):
    """详细诊断单个电机"""
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"\n{'='*60}")
    print(f"节点 {axis_num} 详细诊断")
    print(f"{'='*60}")
    
    # 1. 基本状态
    print(f"\n【基本状态】")
    state_name = AXIS_STATE_NAMES.get(axis.current_state, 'UNKNOWN')
    print(f"  当前状态: {axis.current_state} ({state_name})")
    print(f"  轴错误: 0x{axis.error:08X}")
    if axis.error:
        print(f"    详细: {bin(axis.error)}")
    print(f"  电机错误: 0x{axis.motor.error:08X}")
    if axis.motor.error:
        print(f"    详细: {bin(axis.motor.error)}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    if axis.encoder.error:
        print(f"    详细: {bin(axis.encoder.error)}")
    print(f"  控制器错误: 0x{axis.controller.error:08X}")
    if axis.controller.error:
        print(f"    详细: {bin(axis.controller.error)}")
    
    # 2. 校准状态
    print(f"\n【校准状态】")
    print(f"  电机已校准: {axis.motor.is_calibrated}")
    print(f"  编码器已校准: {axis.encoder.is_ready}")
    print(f"  编码器偏移: {axis.encoder.config.offset}")
    
    # 3. 编码器配置
    print(f"\n【编码器配置】")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  模式: {axis.encoder.config.mode}")
    print(f"  当前位置: {axis.encoder.pos_estimate} 圈")
    print(f"  当前速度: {axis.encoder.vel_estimate} 圈/秒")
    
    # 4. 电机配置
    print(f"\n【电机配置】")
    print(f"  极对数: {axis.motor.config.pole_pairs}")
    print(f"  电阻: {axis.motor.config.phase_resistance} Ω")
    print(f"  电感: {axis.motor.config.phase_inductance} H")
    print(f"  电流限制: {axis.motor.config.current_lim} A")
    print(f"  校准电流: {axis.motor.config.calibration_current} A")
    
    # 5. 控制器配置
    print(f"\n【控制器配置】")
    print(f"  控制模式: {axis.controller.config.control_mode}")
    print(f"  输入模式: {axis.controller.config.input_mode}")
    print(f"  速度限制: {axis.controller.config.vel_limit} 圈/秒")
    print(f"  速度增益: {axis.controller.config.vel_gain}")
    print(f"  速度积分增益: {axis.controller.config.vel_integrator_gain}")
    print(f"  位置增益: {axis.controller.config.pos_gain}")
    
    # 6. 电源状态
    print(f"\n【电源状态】")
    print(f"  母线电压: {odrv.vbus_voltage:.2f} V")
    print(f"  母线电流: {axis.motor.current_control.Ibus:.2f} A")
    
    # 7. 诊断建议
    print(f"\n【诊断建议】")
    issues = []
    
    if not axis.motor.is_calibrated:
        issues.append("⚠ 电机未校准 - 需要运行电机校准")
    
    if not axis.encoder.is_ready:
        issues.append("⚠ 编码器未就绪 - 需要运行编码器校准")
    
    if axis.error != 0:
        issues.append(f"⚠ 轴错误: 0x{axis.error:08X} - 需要清除错误")
    
    if axis.motor.error != 0:
        issues.append(f"⚠ 电机错误: 0x{axis.motor.error:08X} - 需要清除错误")
    
    if axis.encoder.error != 0:
        issues.append(f"⚠ 编码器错误: 0x{axis.encoder.error:08X} - 需要清除错误")
    
    if axis.controller.error != 0:
        issues.append(f"⚠ 控制器错误: 0x{axis.controller.error:08X} - 需要清除错误")
    
    if odrv.vbus_voltage < 11.0:
        issues.append(f"⚠ 母线电压过低: {odrv.vbus_voltage:.2f}V - 检查电源")
    
    if axis.encoder.config.cpr == 0:
        issues.append("⚠ CPR未配置 - 需要设置编码器CPR")
    
    if issues:
        for issue in issues:
            print(f"  {issue}")
    else:
        print(f"  ✓ 未发现明显问题")
    
    return len(issues) == 0

def main():
    print("正在连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    print(f"\n母线电压: {odrv.vbus_voltage:.2f} V")
    
    # 诊断节点0
    node0_ok = diagnose_motor(odrv, 0)
    
    # 诊断节点1
    node1_ok = diagnose_motor(odrv, 1)
    
    # 总结
    print(f"\n{'='*60}")
    print(f"诊断总结")
    print(f"{'='*60}")
    print(f"节点0: {'✓ 正常' if node0_ok else '✗ 有问题'}")
    print(f"节点1: {'✓ 正常' if node1_ok else '✗ 有问题'}")
    
    if not node0_ok or not node1_ok:
        print(f"\n建议操作:")
        print(f"1. 清除所有错误: odrv.clear_errors()")
        print(f"2. 如果未校准，运行完整校准:")
        print(f"   axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE")
        print(f"3. 保存配置: odrv.save_configuration()")
        print(f"4. 重启: odrv.reboot()")
    
    return 0 if (node0_ok and node1_ok) else 1

if __name__ == "__main__":
    sys.exit(main())
