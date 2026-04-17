#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复电机错误脚本
针对诊断报告中的问题进行修复
"""

import odrive
import time
import sys

# 导入枚举类型
try:
    from odrive.enums import AxisState, ControlMode, InputMode
except ImportError:
    # 如果导入失败，使用数值
    class AxisState:
        IDLE = 1
        CLOSED_LOOP_CONTROL = 8
    
    class ControlMode:
        VOLTAGE_CONTROL = 0
        TORQUE_CONTROL = 1
        VELOCITY_CONTROL = 2
        POSITION_CONTROL = 3
    
    class InputMode:
        INACTIVE = 0
        PASSTHROUGH = 1

def find_odrive():
    """查找ODrive设备"""
    print("正在查找ODrive设备...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 找到ODrive设备，序列号: {odrv.serial_number}")
        return odrv
    except Exception as e:
        print(f"✗ 未找到ODrive设备: {e}")
        return None

def clear_errors(axis):
    """清除错误"""
    print("\n[1] 清除错误...")
    try:
        axis.clear_errors()
        time.sleep(0.5)
        print("✓ 错误已清除")
        return True
    except Exception as e:
        print(f"✗ 清除错误失败: {e}")
        return False

def check_errors(axis):
    """检查当前错误状态"""
    print("\n[2] 检查错误状态...")
    errors = {
        "轴错误": hex(axis.error),
        "电机错误": hex(axis.motor.error),
        "编码器错误": hex(axis.encoder.error),
        "控制器错误": hex(axis.controller.error)
    }
    
    has_error = False
    for name, value in errors.items():
        if value != "0x0":
            print(f"✗ {name}: {value}")
            has_error = True
        else:
            print(f"✓ {name}: 无错误")
    
    return not has_error

def configure_limits(axis):
    """配置速度和电流限制"""
    print("\n[3] 配置速度和电流限制...")
    try:
        # 设置合理的速度限制 (根据你的应用调整)
        axis.controller.config.vel_limit = 10.0  # 10 turns/s
        print(f"✓ 速度限制设置为: {axis.controller.config.vel_limit} turns/s")
        
        # 设置合理的电流限制
        axis.motor.config.current_lim = 10.0  # 10A
        print(f"✓ 电流限制设置为: {axis.motor.config.current_lim} A")
        
        # 设置控制器电流限制
        axis.controller.config.vel_limit = 10.0
        print(f"✓ 控制器速度限制: {axis.controller.config.vel_limit} turns/s")
        
        return True
    except Exception as e:
        print(f"✗ 配置限制失败: {e}")
        return False

def configure_pid_gains(axis):
    """配置PID增益"""
    print("\n[4] 配置PID增益...")
    try:
        # 设置位置增益
        axis.controller.config.pos_gain = 20.0
        print(f"✓ 位置增益: {axis.controller.config.pos_gain}")
        
        # 设置速度增益
        axis.controller.config.vel_gain = 0.16
        print(f"✓ 速度增益: {axis.controller.config.vel_gain}")
        
        # 设置速度积分增益 (重要！诊断报告显示为0)
        axis.controller.config.vel_integrator_gain = 0.32
        print(f"✓ 速度积分增益: {axis.controller.config.vel_integrator_gain}")
        
        return True
    except Exception as e:
        print(f"✗ 配置PID增益失败: {e}")
        return False

def configure_control_mode(axis):
    """配置控制模式"""
    print("\n[5] 配置控制模式...")
    try:
        # 设置为速度控制模式
        axis.controller.config.control_mode = ControlMode.VELOCITY_CONTROL
        print(f"✓ 控制模式: VELOCITY_CONTROL")
        
        # 设置输入模式为直通
        axis.controller.config.input_mode = InputMode.PASSTHROUGH
        print(f"✓ 输入模式: PASSTHROUGH")
        
        return True
    except Exception as e:
        print(f"✗ 配置控制模式失败: {e}")
        return False

def enter_closed_loop(axis):
    """进入闭环控制"""
    print("\n[6] 进入闭环控制...")
    try:
        axis.requested_state = AxisState.CLOSED_LOOP_CONTROL
        print("等待进入闭环...")
        
        # 等待状态切换
        timeout = 10
        start_time = time.time()
        while time.time() - start_time < timeout:
            if axis.current_state == AxisState.CLOSED_LOOP_CONTROL:
                print("✓ 成功进入闭环控制")
                return True
            time.sleep(0.1)
        
        print(f"✗ 进入闭环超时，当前状态: {axis.current_state}")
        print(f"  轴错误: {hex(axis.error)}")
        return False
        
    except Exception as e:
        print(f"✗ 进入闭环失败: {e}")
        return False

def test_motor(axis):
    """测试电机运行"""
    print("\n[7] 测试电机运行...")
    try:
        # 设置低速运行
        print("设置速度: 2 turns/s")
        axis.controller.input_vel = 2.0
        time.sleep(2)
        
        print(f"当前速度: {axis.encoder.vel_estimate:.2f} turns/s")
        print(f"当前电流: {axis.motor.current_control.Iq_measured:.2f} A")
        
        # 停止
        print("停止电机...")
        axis.controller.input_vel = 0.0
        time.sleep(1)
        
        print("✓ 电机测试完成")
        return True
        
    except Exception as e:
        print(f"✗ 电机测试失败: {e}")
        return False

def main():
    print("=" * 60)
    print("ODrive 电机错误修复脚本")
    print("=" * 60)
    
    # 查找设备
    odrv = find_odrive()
    if not odrv:
        sys.exit(1)
    
    # 选择轴 (根据诊断报告，这是节点0)
    axis = odrv.axis0
    
    # 执行修复步骤
    steps = [
        ("清除错误", lambda: clear_errors(axis)),
        ("检查错误状态", lambda: check_errors(axis)),
        ("配置限制", lambda: configure_limits(axis)),
        ("配置PID增益", lambda: configure_pid_gains(axis)),
        ("配置控制模式", lambda: configure_control_mode(axis)),
    ]
    
    for step_name, step_func in steps:
        if not step_func():
            print(f"\n✗ 修复失败于步骤: {step_name}")
            print("\n建议:")
            print("1. 检查电机连接")
            print("2. 确认编码器已校准")
            print("3. 检查供电电压 (当前11.96V偏低)")
            sys.exit(1)
    
    # 尝试进入闭环
    print("\n" + "=" * 60)
    print("准备进入闭环控制")
    print("=" * 60)
    
    if enter_closed_loop(axis):
        # 测试电机
        test_motor(axis)
        
        print("\n" + "=" * 60)
        print("✓ 修复完成！")
        print("=" * 60)
        print("\n最终状态:")
        print(f"  轴状态: {axis.current_state}")
        print(f"  速度限制: {axis.controller.config.vel_limit} turns/s")
        print(f"  电流限制: {axis.motor.config.current_lim} A")
        print(f"  位置增益: {axis.controller.config.pos_gain}")
        print(f"  速度增益: {axis.controller.config.vel_gain}")
        print(f"  速度积分增益: {axis.controller.config.vel_integrator_gain}")
    else:
        print("\n✗ 无法进入闭环控制")
        print("\n可能原因:")
        print("1. 编码器未校准 - 运行: odrv0.axis0.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION")
        print("2. 电机未校准 - 运行: odrv0.axis0.requested_state = AXIS_STATE_MOTOR_CALIBRATION")
        print("3. 供电电压不足 - 当前11.96V，建议至少12V以上")
        print("4. 电机或编码器连接问题")

if __name__ == "__main__":
    main()
