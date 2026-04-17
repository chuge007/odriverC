#!/usr/bin/env python3
"""
快速检查 ODrive 当前状态
"""
import odrive
import sys

def main():
    print("正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any(timeout=5)
    except:
        print("✗ 无法连接到 ODrive")
        return
    
    print(f"✓ 已连接: {odrv0.serial_number}")
    print(f"母线电压: {odrv0.vbus_voltage:.2f} V")
    print()
    
    # 检查 axis0
    axis = odrv0.axis0
    
    state_names = {
        0: "UNDEFINED",
        1: "IDLE",
        2: "STARTUP_SEQUENCE",
        3: "FULL_CALIBRATION_SEQUENCE",
        4: "MOTOR_CALIBRATION",
        7: "ENCODER_INDEX_SEARCH",
        8: "CLOSED_LOOP_CONTROL"
    }
    
    print("=" * 60)
    print("  Axis 0 状态")
    print("=" * 60)
    print(f"当前状态: {state_names.get(axis.current_state, 'UNKNOWN')} ({axis.current_state})")
    print(f"轴错误: 0x{axis.error:08X}")
    print(f"电机错误: 0x{axis.motor.error:08X}")
    print(f"编码器错误: 0x{axis.encoder.error:08X}")
    print(f"控制器错误: 0x{axis.controller.error:08X}")
    print()
    
    # 解析错误
    if axis.error != 0:
        print("⚠ 轴错误详情:")
        errors = []
        if axis.error & 0x00000001: errors.append("INVALID_STATE")
        if axis.error & 0x00000040: errors.append("MOTOR_FAILED")
        if axis.error & 0x00000080: errors.append("SENSORLESS_ESTIMATOR_FAILED")
        if axis.error & 0x00000100: errors.append("ENCODER_FAILED")
        if axis.error & 0x00000200: errors.append("CONTROLLER_FAILED")
        if axis.error & 0x00000800: errors.append("WATCHDOG_TIMER_EXPIRED")
        if axis.error & 0x00004000: errors.append("ESTOP_REQUESTED")
        if axis.error & 0x00040000: errors.append("OVER_TEMP")
        for err in errors:
            print(f"  - {err}")
        print()
    
    if axis.motor.error != 0:
        print("⚠ 电机错误详情:")
        errors = []
        if axis.motor.error & 0x00000001: errors.append("PHASE_RESISTANCE_OUT_OF_RANGE")
        if axis.motor.error & 0x00000002: errors.append("PHASE_INDUCTANCE_OUT_OF_RANGE")
        if axis.motor.error & 0x00000008: errors.append("DRV_FAULT")
        if axis.motor.error & 0x00000010: errors.append("CONTROL_DEADLINE_MISSED")
        if axis.motor.error & 0x00000040: errors.append("MODULATION_MAGNITUDE")
        if axis.motor.error & 0x00000100: errors.append("CURRENT_SENSE_SATURATION")
        if axis.motor.error & 0x00001000: errors.append("CURRENT_LIMIT_VIOLATION")
        for err in errors:
            print(f"  - {err}")
        print()
    
    if axis.encoder.error != 0:
        print("⚠ 编码器错误详情:")
        errors = []
        if axis.encoder.error & 0x00000001: errors.append("UNSTABLE_GAIN")
        if axis.encoder.error & 0x00000002: errors.append("CPR_POLEPAIRS_MISMATCH")
        if axis.encoder.error & 0x00000004: errors.append("NO_RESPONSE")
        if axis.encoder.error & 0x00000008: errors.append("UNSUPPORTED_ENCODER_MODE")
        if axis.encoder.error & 0x00000010: errors.append("ILLEGAL_HALL_STATE")
        if axis.encoder.error & 0x00000020: errors.append("INDEX_NOT_FOUND_YET")
        for err in errors:
            print(f"  - {err}")
        print()
    
    # 编码器配置
    print("编码器配置:")
    print(f"  模式: {axis.encoder.config.mode}")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  带宽: {axis.encoder.config.bandwidth} Hz")
    print(f"  使用索引: {axis.encoder.config.use_index}")
    print(f"  编码器就绪: {axis.encoder.is_ready}")
    print()
    
    # 电机配置
    print("电机配置:")
    print(f"  极对数: {axis.motor.config.pole_pairs}")
    print(f"  相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
    print(f"  相电感: {axis.motor.config.phase_inductance:.9f} H")
    print(f"  电流限制: {axis.motor.config.current_lim} A")
    print()
    
    # 建议
    print("=" * 60)
    if axis.error == 0 and axis.motor.error == 0 and axis.encoder.error == 0:
        if axis.current_state == 8:
            print("✓ 系统正常，已在闭环控制状态")
            print("\n可以直接控制电机")
        elif axis.current_state == 1:
            print("✓ 系统正常，处于空闲状态")
            print("\n可以点击'进入闭环'开始控制")
        else:
            print(f"系统正在执行操作: {state_names.get(axis.current_state, 'UNKNOWN')}")
            print("请等待操作完成")
    else:
        print("⚠ 系统有错误，需要处理")
        print("\n建议操作:")
        print("1. 点击'清除错误'")
        print("2. 点击'进入空闲'")
        if axis.encoder.error != 0:
            print("3. 检查编码器连接")
            print("4. 使用 fix_encoder.py 脚本重新配置")
        else:
            print("3. 重新尝试操作")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
