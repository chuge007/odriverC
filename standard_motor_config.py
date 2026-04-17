#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive电机标准配置脚本
适用于新电机，一键配置即可使用
"""

import odrive
from odrive.enums import *
import sys
import time

# ============ 标准配置参数 ============
STANDARD_CONFIG = {
    # 电机参数（根据你的电机型号调整）
    'pole_pairs': 7,  # 极对数
    'calibration_current': 10.0,  # 校准电流 (A)
    'current_lim': 10.0,  # 电流限制 (A)
    'current_lim_margin': 8.0,  # 电流限制余量 (A)
    
    # 编码器参数
    'cpr': 8192,  # 每转计数 (根据你的编码器)
    'encoder_mode': ENCODER_MODE_INCREMENTAL,
    
    # 控制器参数（保守设置，稳定可靠）
    'vel_limit': 10.0,  # 速度限制 (圈/秒)
    'vel_gain': 0.02,  # 速度增益（降低以提高稳定性）
    'vel_integrator_gain': 0.1,  # 速度积分增益（降低以避免超时）
    'pos_gain': 8.0,  # 位置增益
    
    # 控制模式
    'control_mode': CONTROL_MODE_POSITION_CONTROL,
    'input_mode': INPUT_MODE_PASSTHROUGH,
}

def configure_axis(axis, axis_num):
    """配置单个轴的标准参数"""
    print(f"\n{'='*60}")
    print(f"配置节点 {axis_num}")
    print(f"{'='*60}")
    
    try:
        # 1. 清除所有错误
        print("步骤1: 清除错误...")
        axis.clear_errors()
        time.sleep(0.3)
        
        # 2. 配置电机
        print("步骤2: 配置电机参数...")
        axis.motor.config.pole_pairs = STANDARD_CONFIG['pole_pairs']
        axis.motor.config.calibration_current = STANDARD_CONFIG['calibration_current']
        axis.motor.config.current_lim = STANDARD_CONFIG['current_lim']
        axis.motor.config.current_lim_margin = STANDARD_CONFIG['current_lim_margin']
        axis.motor.config.motor_type = MOTOR_TYPE_HIGH_CURRENT
        axis.motor.config.resistance_calib_max_voltage = 2.0
        print(f"  ✓ 极对数: {STANDARD_CONFIG['pole_pairs']}")
        print(f"  ✓ 电流限制: {STANDARD_CONFIG['current_lim']} A")
        
        # 3. 配置编码器
        print("步骤3: 配置编码器...")
        axis.encoder.config.cpr = STANDARD_CONFIG['cpr']
        axis.encoder.config.mode = STANDARD_CONFIG['encoder_mode']
        axis.encoder.config.bandwidth = 1000
        print(f"  ✓ CPR: {STANDARD_CONFIG['cpr']}")
        
        # 4. 配置控制器（保守参数）
        print("步骤4: 配置控制器...")
        axis.controller.config.vel_limit = STANDARD_CONFIG['vel_limit']
        axis.controller.config.vel_gain = STANDARD_CONFIG['vel_gain']
        axis.controller.config.vel_integrator_gain = STANDARD_CONFIG['vel_integrator_gain']
        axis.controller.config.pos_gain = STANDARD_CONFIG['pos_gain']
        axis.controller.config.control_mode = STANDARD_CONFIG['control_mode']
        axis.controller.config.input_mode = STANDARD_CONFIG['input_mode']
        print(f"  ✓ 速度限制: {STANDARD_CONFIG['vel_limit']} 圈/秒")
        print(f"  ✓ 速度增益: {STANDARD_CONFIG['vel_gain']}")
        print(f"  ✓ 速度积分增益: {STANDARD_CONFIG['vel_integrator_gain']}")
        
        # 5. 启用校准后自动进入闭环
        print("步骤5: 配置启动行为...")
        axis.config.startup_motor_calibration = False  # 不在启动时校准电机
        axis.config.startup_encoder_index_search = False  # 不搜索索引
        axis.config.startup_encoder_offset_calibration = False  # 不在启动时校准编码器偏移
        axis.config.startup_closed_loop_control = False  # 不自动进入闭环（手动控制更安全）
        axis.config.startup_homing = False
        print(f"  ✓ 启动行为已配置（手动控制模式）")
        
        # 6. 配置制动电阻（如果有）
        print("步骤6: 配置制动电阻...")
        axis.config.enable_brake_resistor = False  # 如果没有制动电阻，设为False
        print(f"  ✓ 制动电阻: 禁用")
        
        print(f"\n✓ 节点 {axis_num} 配置完成！")
        return True
        
    except Exception as e:
        print(f"\n✗ 配置节点 {axis_num} 失败: {e}")
        return False

def calibrate_axis(axis, axis_num):
    """执行完整校准"""
    print(f"\n{'='*60}")
    print(f"校准节点 {axis_num}")
    print(f"{'='*60}")
    
    try:
        # 清除错误
        axis.clear_errors()
        time.sleep(0.3)
        
        print("开始完整校准序列...")
        print("  ⚠ 请确保电机可以自由转动！")
        print("  ⚠ 校准过程约需20-30秒...")
        
        axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
        
        # 等待校准完成
        start_time = time.time()
        while axis.current_state != AXIS_STATE_IDLE:
            if time.time() - start_time > 60:
                print(f"  ✗ 校准超时")
                return False
            
            if axis.error != 0:
                print(f"  ✗ 校准失败，错误代码: 0x{axis.error:08X}")
                print(f"     电机错误: 0x{axis.motor.error:08X}")
                print(f"     编码器错误: 0x{axis.encoder.error:08X}")
                return False
            
            time.sleep(0.5)
            print(".", end="", flush=True)
        
        print()
        
        # 检查校准结果
        if axis.motor.is_calibrated and axis.encoder.is_ready:
            print(f"  ✓ 校准成功！")
            print(f"     电机已校准: {axis.motor.is_calibrated}")
            print(f"     编码器已就绪: {axis.encoder.is_ready}")
            return True
        else:
            print(f"  ✗ 校准未完成")
            print(f"     电机已校准: {axis.motor.is_calibrated}")
            print(f"     编码器已就绪: {axis.encoder.is_ready}")
            return False
            
    except Exception as e:
        print(f"\n✗ 校准节点 {axis_num} 失败: {e}")
        return False

def test_closed_loop(axis, axis_num):
    """测试闭环控制"""
    print(f"\n{'='*60}")
    print(f"测试节点 {axis_num} 闭环控制")
    print(f"{'='*60}")
    
    try:
        # 清除错误
        axis.clear_errors()
        time.sleep(0.3)
        
        print("进入闭环控制...")
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print(f"  ✓ 成功进入闭环控制！")
            
            # 测试小幅度运动
            print("  测试小幅度运动...")
            current_pos = axis.encoder.pos_estimate
            axis.controller.input_pos = current_pos + 0.5  # 移动0.5圈
            time.sleep(2)
            
            new_pos = axis.encoder.pos_estimate
            if abs(new_pos - (current_pos + 0.5)) < 0.1:
                print(f"  ✓ 运动测试成功！")
                
                # 返回原位
                axis.controller.input_pos = current_pos
                time.sleep(2)
                return True
            else:
                print(f"  ⚠ 运动测试未达到预期位置")
                return False
        else:
            print(f"  ✗ 未能进入闭环控制")
            if axis.error != 0:
                print(f"     错误代码: 0x{axis.error:08X}")
            return False
            
    except Exception as e:
        print(f"\n✗ 测试失败: {e}")
        return False

def main():
    print("="*60)
    print("ODrive 标准配置脚本")
    print("="*60)
    print("\n此脚本将:")
    print("1. 配置标准参数（保守稳定）")
    print("2. 执行完整校准")
    print("3. 测试闭环控制")
    print("4. 保存配置到ODrive")
    print("\n⚠ 请确保:")
    print("  - 电机可以自由转动")
    print("  - 编码器正确连接")
    print("  - 电源电压充足 (>11V)")
    
    input("\n按回车键继续...")
    
    # 连接ODrive
    print("\n正在连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V")
        
        if odrv.vbus_voltage < 11.0:
            print(f"  ⚠ 警告: 母线电压过低！建议 >11V")
            if input("  是否继续? (y/n): ").lower() != 'y':
                return 1
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    # 配置和校准每个轴
    results = {}
    for axis_num in [0, 1]:
        axis = getattr(odrv, f'axis{axis_num}')
        
        # 配置
        if not configure_axis(axis, axis_num):
            results[axis_num] = False
            continue
        
        # 校准
        if not calibrate_axis(axis, axis_num):
            results[axis_num] = False
            continue
        
        # 测试
        if not test_closed_loop(axis, axis_num):
            results[axis_num] = False
            continue
        
        results[axis_num] = True
    
    # 保存配置
    print(f"\n{'='*60}")
    print("保存配置")
    print(f"{'='*60}")
    
    if any(results.values()):
        print("保存配置到ODrive...")
        try:
            odrv.save_configuration()
            print("✓ 配置已保存")
            print("\n⚠ 建议重启ODrive以应用所有设置")
            if input("是否立即重启? (y/n): ").lower() == 'y':
                print("正在重启...")
                odrv.reboot()
                print("✓ 已重启")
        except Exception as e:
            print(f"✗ 保存失败: {e}")
    
    # 总结
    print(f"\n{'='*60}")
    print("配置总结")
    print(f"{'='*60}")
    for axis_num, success in results.items():
        status = "✓ 成功" if success else "✗ 失败"
        print(f"节点 {axis_num}: {status}")
    
    if all(results.values()):
        print("\n🎉 所有节点配置成功！")
        print("现在可以使用Qt程序控制电机了。")
        return 0
    else:
        print("\n⚠ 部分节点配置失败，请检查硬件连接。")
        return 1

if __name__ == "__main__":
    sys.exit(main())
