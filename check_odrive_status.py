#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive板子状态检查工具
使用odrivetool检查板子是否正常运行
"""

import odrive
from odrive.enums import *
import sys
import time

def print_section(title):
    """打印分节标题"""
    print("\n" + "="*60)
    print(f"  {title}")
    print("="*60)

def check_connection():
    """检查ODrive连接"""
    print_section("1. 检查ODrive连接")
    try:
        print("正在搜索ODrive设备...")
        odrv = odrive.find_any(timeout=10)
        print(f"✓ 成功连接到ODrive")
        print(f"  序列号: {hex(odrv.serial_number)}")
        print(f"  固件版本: {odrv.fw_version_major}.{odrv.fw_version_minor}.{odrv.fw_version_revision}")
        return odrv
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        print("\n请检查:")
        print("  1. ODrive是否通过USB连接到电脑")
        print("  2. 驱动程序是否正确安装")
        print("  3. 设备管理器中是否识别到设备")
        return None

def check_voltage(odrv):
    """检查电源电压"""
    print_section("2. 检查电源电压")
    try:
        vbus = odrv.vbus_voltage
        print(f"  总线电压: {vbus:.2f}V")
        
        if vbus < 12:
            print(f"  ⚠ 警告: 电压过低 ({vbus:.2f}V < 12V)")
            print("  建议: 检查电源连接")
            return False
        elif vbus > 56:
            print(f"  ⚠ 警告: 电压过高 ({vbus:.2f}V > 56V)")
            return False
        else:
            print(f"  ✓ 电压正常")
            return True
    except Exception as e:
        print(f"  ✗ 读取电压失败: {e}")
        return False

def check_axis_errors(axis, axis_name):
    """检查轴错误"""
    errors = []
    
    # 检查轴错误
    if axis.error != 0:
        errors.append(f"Axis错误: 0x{axis.error:X}")
    
    # 检查电机错误
    if axis.motor.error != 0:
        errors.append(f"Motor错误: 0x{axis.motor.error:X}")
    
    # 检查编码器错误
    if axis.encoder.error != 0:
        errors.append(f"Encoder错误: 0x{axis.encoder.error:X}")
    
    # 检查控制器错误
    if axis.controller.error != 0:
        errors.append(f"Controller错误: 0x{axis.controller.error:X}")
    
    return errors

def check_axis_status(odrv, axis_num):
    """检查单个轴的状态"""
    print_section(f"3.{axis_num+1}. 检查Axis{axis_num}状态")
    
    axis = getattr(odrv, f'axis{axis_num}')
    
    # 检查错误
    errors = check_axis_errors(axis, f"axis{axis_num}")
    if errors:
        print(f"  ✗ 发现错误:")
        for err in errors:
            print(f"    - {err}")
    else:
        print(f"  ✓ 无错误")
    
    # 检查状态
    state = axis.current_state
    state_names = {
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
        12: "ENCODER_HALL_PHASE_CALIBRATION"
    }
    state_name = state_names.get(state, f"UNKNOWN({state})")
    print(f"  当前状态: {state_name}")
    
    # 检查配置
    print(f"\n  配置信息:")
    print(f"    电机类型: {axis.motor.config.motor_type}")
    print(f"    电流限制: {axis.motor.config.current_lim}A")
    print(f"    校准电流: {axis.motor.config.calibration_current}A")
    
    # 检查编码器
    print(f"\n  编码器信息:")
    print(f"    模式: {axis.encoder.config.mode}")
    print(f"    CPR: {axis.encoder.config.cpr}")
    if hasattr(axis.encoder, 'pos_estimate'):
        print(f"    位置估计: {axis.encoder.pos_estimate:.2f}")
    if hasattr(axis.encoder, 'vel_estimate'):
        print(f"    速度估计: {axis.encoder.vel_estimate:.2f}")
    
    # 检查控制器
    print(f"\n  控制器信息:")
    print(f"    控制模式: {axis.controller.config.control_mode}")
    print(f"    输入模式: {axis.controller.config.input_mode}")
    
    # 检查是否已校准
    is_calibrated = axis.motor.is_calibrated and axis.encoder.is_ready
    print(f"\n  校准状态:")
    print(f"    电机已校准: {'✓' if axis.motor.is_calibrated else '✗'}")
    print(f"    编码器就绪: {'✓' if axis.encoder.is_ready else '✗'}")
    
    return len(errors) == 0, is_calibrated

def check_can_config(odrv):
    """检查CAN配置"""
    print_section("4. 检查CAN配置")
    try:
        print(f"  CAN波特率: {odrv.can.config.baud_rate}")
        print(f"  CAN协议: {odrv.can.config.protocol}")
        
        # 检查各轴的CAN ID
        for i in range(2):
            axis = getattr(odrv, f'axis{i}')
            node_id = axis.config.can.node_id
            print(f"  Axis{i} CAN节点ID: {node_id}")
        
        print(f"  ✓ CAN配置正常")
        return True
    except Exception as e:
        print(f"  ⚠ 无法读取CAN配置: {e}")
        return False

def test_basic_function(odrv, axis_num=0):
    """测试基本功能"""
    print_section(f"5. 测试Axis{axis_num}基本功能")
    
    axis = getattr(odrv, f'axis{axis_num}')
    
    # 清除错误
    print("  清除错误...")
    try:
        odrv.clear_errors()
        time.sleep(0.5)
    except:
        pass
    
    # 检查是否需要校准
    if not (axis.motor.is_calibrated and axis.encoder.is_ready):
        print("  ⚠ 电机未校准，跳过功能测试")
        print("  建议: 先运行完整校准 (axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE)")
        return False
    
    # 尝试进入闭环控制
    print("  尝试进入闭环控制...")
    try:
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print("  ✓ 成功进入闭环控制")
            
            # 测试速度控制
            print("  测试速度控制 (1转/秒)...")
            axis.controller.config.control_mode = CONTROL_MODE_VELOCITY_CONTROL
            axis.controller.input_vel = 1
            time.sleep(2)
            
            vel = axis.encoder.vel_estimate
            print(f"    当前速度: {vel:.2f} 转/秒")
            
            # 停止
            axis.controller.input_vel = 0
            time.sleep(1)
            
            # 退出闭环
            axis.requested_state = AXIS_STATE_IDLE
            time.sleep(0.5)
            
            print("  ✓ 基本功能测试通过")
            return True
        else:
            print(f"  ✗ 无法进入闭环控制，当前状态: {axis.current_state}")
            errors = check_axis_errors(axis, f"axis{axis_num}")
            if errors:
                print("  错误信息:")
                for err in errors:
                    print(f"    - {err}")
            return False
            
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False

def generate_report(results):
    """生成诊断报告"""
    print_section("诊断报告总结")
    
    all_ok = all(results.values())
    
    if all_ok:
        print("\n  ✓✓✓ ODrive板子运行正常 ✓✓✓")
        print("\n  所有检查项目都通过了！")
    else:
        print("\n  ⚠⚠⚠ 发现问题 ⚠⚠⚠")
        print("\n  失败的检查项:")
        for check, passed in results.items():
            if not passed:
                print(f"    ✗ {check}")
        
        print("\n  建议操作:")
        if not results.get('connection'):
            print("    1. 检查USB连接")
            print("    2. 重新安装驱动")
        if not results.get('voltage'):
            print("    1. 检查电源连接")
            print("    2. 确认电源电压在12-56V范围内")
        if not results.get('axis0') or not results.get('axis1'):
            print("    1. 清除错误: odrv.clear_errors()")
            print("    2. 运行校准: axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE")
            print("    3. 检查电机和编码器连接")

def main():
    """主函数"""
    print("\n" + "="*60)
    print("  ODrive板子状态检查工具")
    print("  使用odrivetool检查板子是否正常运行")
    print("="*60)
    
    results = {}
    
    # 1. 检查连接
    odrv = check_connection()
    results['connection'] = odrv is not None
    
    if not odrv:
        generate_report(results)
        return 1
    
    # 2. 检查电压
    results['voltage'] = check_voltage(odrv)
    
    # 3. 检查各轴状态
    axis0_ok, axis0_calibrated = check_axis_status(odrv, 0)
    results['axis0'] = axis0_ok
    
    axis1_ok, axis1_calibrated = check_axis_status(odrv, 1)
    results['axis1'] = axis1_ok
    
    # 4. 检查CAN配置
    results['can'] = check_can_config(odrv)
    
    # 5. 测试基本功能（如果axis0已校准）
    if axis0_calibrated:
        results['function_test'] = test_basic_function(odrv, 0)
    else:
        print_section("5. 测试基本功能")
        print("  ⚠ 跳过功能测试（电机未校准）")
        results['function_test'] = None
    
    # 6. 生成报告
    generate_report(results)
    
    return 0 if all(v for v in results.values() if v is not None) else 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
