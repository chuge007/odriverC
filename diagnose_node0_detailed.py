#!/usr/bin/env python3
"""
节点0详细诊断脚本
检查节点0无法进入闭环的原因
"""

import odrive
import time
import sys

# 定义状态枚举
AXIS_STATE_UNDEFINED = 0
AXIS_STATE_IDLE = 1
AXIS_STATE_STARTUP_SEQUENCE = 2
AXIS_STATE_FULL_CALIBRATION_SEQUENCE = 3
AXIS_STATE_MOTOR_CALIBRATION = 4
AXIS_STATE_ENCODER_INDEX_SEARCH = 6
AXIS_STATE_ENCODER_OFFSET_CALIBRATION = 7
AXIS_STATE_CLOSED_LOOP_CONTROL = 8
AXIS_STATE_LOCKIN_SPIN = 9
AXIS_STATE_ENCODER_DIR_FIND = 10
AXIS_STATE_HOMING = 11

AXIS_STATE_NAMES = {
    0: "UNDEFINED",
    1: "IDLE",
    2: "STARTUP_SEQUENCE",
    3: "FULL_CALIBRATION_SEQUENCE",
    4: "MOTOR_CALIBRATION",
    6: "ENCODER_INDEX_SEARCH",
    7: "ENCODER_OFFSET_CALIBRATION",
    8: "CLOSED_LOOP_CONTROL",
    9: "LOCKIN_SPIN",
    10: "ENCODER_DIR_FIND",
    11: "HOMING"
}

def diagnose_node0():
    print("=" * 60)
    print("节点0详细诊断")
    print("=" * 60)
    
    try:
        print("\n正在连接 ODrive...")
        odrv = odrive.find_any(timeout=10)
        if odrv is None:
            print("❌ 未找到 ODrive 设备")
            return False
        
        print("✓ 已连接到 ODrive")
        print(f"  序列号: {odrv.serial_number}")
        print(f"  固件版本: {odrv.fw_version_major}.{odrv.fw_version_minor}.{odrv.fw_version_revision}")
        
        # 检查节点0（axis0）
        axis = odrv.axis0
        motor = axis.motor
        encoder = axis.encoder
        controller = axis.controller
        
        print("\n" + "=" * 60)
        print("1. 轴状态检查")
        print("=" * 60)
        state_name = AXIS_STATE_NAMES.get(axis.current_state, 'UNKNOWN')
        print(f"  当前状态: {axis.current_state} ({state_name})")
        print(f"  请求状态: {axis.requested_state}")
        print(f"  轴错误: 0x{axis.error:08X}")
        if axis.error != 0:
            print(f"    错误详情: {hex(axis.error)}")
            # 解析错误位
            error_bits = {
                0x00000001: "INVALID_STATE",
                0x00000040: "MOTOR_FAILED",
                0x00000080: "SENSORLESS_ESTIMATOR_FAILED",
                0x00000100: "ENCODER_FAILED",
                0x00000200: "CONTROLLER_FAILED",
                0x00000800: "WATCHDOG_TIMER_EXPIRED",
                0x00001000: "MIN_ENDSTOP_PRESSED",
                0x00002000: "MAX_ENDSTOP_PRESSED",
                0x00004000: "ESTOP_REQUESTED",
                0x00020000: "HOMING_WITHOUT_ENDSTOP",
                0x00040000: "OVER_TEMP",
                0x00080000: "UNKNOWN_POSITION"
            }
            for bit, name in error_bits.items():
                if axis.error & bit:
                    print(f"      - {name}")
        
        print("\n" + "=" * 60)
        print("2. 电机状态检查")
        print("=" * 60)
        print(f"  电机错误: 0x{motor.error:08X}")
        if motor.error != 0:
            print(f"    错误详情: {hex(motor.error)}")
        print(f"  电机配置:")
        print(f"    极对数: {motor.config.pole_pairs}")
        print(f"    电阻: {motor.config.phase_resistance:.6f} Ω")
        print(f"    电感: {motor.config.phase_inductance:.9f} H")
        print(f"    电流限制: {motor.config.current_lim} A")
        print(f"    校准电流: {motor.config.calibration_current} A")
        print(f"  电机状态:")
        print(f"    是否已校准: {motor.is_calibrated}")
        try:
            print(f"    是否armed: {motor.is_armed}")
        except:
            pass
        try:
            print(f"    当前电流: Iq={motor.current_control.Iq_measured:.3f}A, Id={motor.current_control.Id_measured:.3f}A")
        except:
            pass
        try:
            print(f"    温度: FET={motor.fet_thermistor.temperature:.1f}°C, Motor={motor.motor_thermistor.temperature:.1f}°C")
        except:
            pass
        
        print("\n" + "=" * 60)
        print("3. 编码器状态检查")
        print("=" * 60)
        print(f"  编码器错误: 0x{encoder.error:08X}")
        if encoder.error != 0:
            print(f"    错误详情: {hex(encoder.error)}")
            error_bits = {
                0x00000001: "UNSTABLE_GAIN",
                0x00000002: "CPR_POLEPAIRS_MISMATCH",
                0x00000004: "NO_RESPONSE",
                0x00000008: "UNSUPPORTED_ENCODER_MODE",
                0x00000010: "ILLEGAL_HALL_STATE",
                0x00000020: "INDEX_NOT_FOUND_YET",
                0x00000040: "ABS_SPI_TIMEOUT",
                0x00000080: "ABS_SPI_COM_FAIL",
                0x00000100: "ABS_SPI_NOT_READY"
            }
            for bit, name in error_bits.items():
                if encoder.error & bit:
                    print(f"      - {name}")
        print(f"  编码器配置:")
        print(f"    CPR: {encoder.config.cpr}")
        print(f"    模式: {encoder.config.mode}")
        print(f"  编码器状态:")
        print(f"    是否已就绪: {encoder.is_ready}")
        print(f"    位置: {encoder.pos_estimate:.3f} 圈")
        print(f"    速度: {encoder.vel_estimate:.3f} 圈/秒")
        print(f"    Shadow count: {encoder.shadow_count}")
        print(f"    Count in CPR: {encoder.count_in_cpr}")
        
        print("\n" + "=" * 60)
        print("4. 控制器状态检查")
        print("=" * 60)
        print(f"  控制器错误: 0x{controller.error:08X}")
        if controller.error != 0:
            print(f"    错误详情: {hex(controller.error)}")
        print(f"  控制模式: {controller.config.control_mode}")
        print(f"  输入模式: {controller.config.input_mode}")
        print(f"  速度限制: {controller.config.vel_limit} 圈/秒")
        print(f"  位置增益: {controller.config.pos_gain}")
        print(f"  速度增益: {controller.config.vel_gain}")
        print(f"  速度积分增益: {controller.config.vel_integrator_gain}")
        
        print("\n" + "=" * 60)
        print("5. 总线状态检查")
        print("=" * 60)
        print(f"  总线电压: {odrv.vbus_voltage:.2f} V")
        print(f"  总线电流: {odrv.ibus:.3f} A")
        
        print("\n" + "=" * 60)
        print("6. 诊断建议")
        print("=" * 60)
        
        issues = []
        
        # 检查各种问题
        if axis.error != 0:
            issues.append(f"❌ 轴有错误: 0x{axis.error:08X}")
        
        if motor.error != 0:
            issues.append(f"❌ 电机有错误: 0x{motor.error:08X}")
        
        if encoder.error != 0:
            issues.append(f"❌ 编码器有错误: 0x{encoder.error:08X}")
            if encoder.error & 0x00000002:  # CPR_POLEPAIRS_MISMATCH
                issues.append(f"  ⚠️  CPR与极对数不匹配！")
                issues.append(f"     当前 CPR={encoder.config.cpr}, 极对数={motor.config.pole_pairs}")
                issues.append(f"     建议: CPR应该是极对数的整数倍")
            if encoder.error & 0x00000004:  # NO_RESPONSE
                issues.append(f"  ⚠️  编码器无响应！检查编码器连接")
        
        if not motor.is_calibrated:
            issues.append(f"⚠️  电机未校准")
        
        if not encoder.is_ready:
            issues.append(f"⚠️  编码器未就绪")
        
        if odrv.vbus_voltage < 12.0:
            issues.append(f"⚠️  总线电压过低: {odrv.vbus_voltage:.2f}V (建议 >12V)")
        
        if controller.error != 0:
            issues.append(f"❌ 控制器有错误: 0x{controller.error:08X}")
        
        if len(issues) == 0:
            print("✓ 未发现明显问题")
            print("\n可能的原因:")
            print("  1. 需要先进行电机校准")
            print("  2. 需要先进行编码器校准")
            print("  3. 检查CAN通信是否正常")
        else:
            print("发现以下问题:")
            for issue in issues:
                print(f"  {issue}")
        
        print("\n" + "=" * 60)
        print("7. 建议的修复步骤")
        print("=" * 60)
        
        if encoder.error & 0x00000002:  # CPR不匹配
            print("1. 修复CPR配置:")
            print(f"   odrv.axis0.encoder.config.cpr = {motor.config.pole_pairs * 8192}")
            print("   odrv.save_configuration()")
            print("   odrv.reboot()")
        
        if not motor.is_calibrated or not encoder.is_ready:
            print("2. 执行完整校准:")
            print("   odrv.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE")
            print("   # 等待校准完成")
            print("   time.sleep(15)")
        
        print("3. 清除错误:")
        print("   odrv.clear_errors()")
        
        print("4. 尝试进入闭环:")
        print("   odrv.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL")
        
        return True
        
    except Exception as e:
        print(f"\n❌ 诊断过程中出错: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = diagnose_node0()
    sys.exit(0 if success else 1)
