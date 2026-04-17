#!/usr/bin/env python3
"""
最全面的 ODrive 诊断脚本
测试节点0的速度控制和力矩控制，找出具体问题
"""

import odrive
import time
import sys

def print_separator(title=""):
    print("\n" + "=" * 70)
    if title:
        print(f"  {title}")
        print("=" * 70)

def check_power_supply(odrv):
    """检查电源状态"""
    print_separator("电源检查")
    vbus = odrv.vbus_voltage
    print(f"母线电压: {vbus:.2f} V")
    
    if vbus < 12.0:
        print("⚠️  警告：电压过低（< 12V）")
        return False
    elif vbus < 18.0:
        print("⚠️  注意：电压偏低（< 18V），可能影响性能")
        return True
    else:
        print("✓ 电压正常")
        return True

def check_axis_status(odrv, axis_num=0):
    """检查轴状态"""
    print_separator(f"节点{axis_num}状态检查")
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"当前状态: {axis.current_state}")
    print(f"  0=UNDEFINED, 1=IDLE, 8=CLOSED_LOOP_CONTROL")
    
    print(f"\n错误状态:")
    print(f"  轴错误: 0x{axis.error:08X}")
    print(f"  控制器错误: 0x{axis.controller.error:08X}")
    print(f"  电机错误: 0x{axis.motor.error:08X}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    
    print(f"\n校准状态:")
    print(f"  电机已校准: {axis.motor.is_calibrated}")
    print(f"  编码器就绪: {axis.encoder.is_ready}")
    
    print(f"\n配置参数:")
    print(f"  速度限制: {axis.controller.config.vel_limit} turns/s")
    print(f"  电流限制: {axis.motor.config.current_lim} A")
    print(f"  位置增益: {axis.controller.config.pos_gain}")
    print(f"  速度增益: {axis.controller.config.vel_gain}")
    print(f"  速度积分增益: {axis.controller.config.vel_integrator_gain}")
    
    print(f"\n电机参数:")
    print(f"  电机类型: {axis.motor.config.motor_type}")
    print(f"  极对数: {axis.motor.config.pole_pairs}")
    print(f"  相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
    print(f"  相电感: {axis.motor.config.phase_inductance:.9f} H")
    
    print(f"\n编码器参数:")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  模式: {axis.encoder.config.mode}")
    
    has_errors = (axis.error != 0 or axis.controller.error != 0 or 
                  axis.motor.error != 0 or axis.encoder.error != 0)
    
    return not has_errors

def clear_errors(odrv, axis_num=0):
    """清除错误"""
    print(f"\n清除节点{axis_num}的所有错误...")
    axis = getattr(odrv, f'axis{axis_num}')
    axis.error = 0
    axis.controller.error = 0
    axis.motor.error = 0
    axis.encoder.error = 0
    time.sleep(0.5)
    print("✓ 错误已清除")

def enter_closed_loop(odrv, axis_num=0):
    """进入闭环控制"""
    print(f"\n尝试进入闭环控制...")
    axis = getattr(odrv, f'axis{axis_num}')
    
    axis.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if axis.current_state == 8:
        print("✓ 成功进入闭环状态")
        return True
    else:
        print(f"❌ 无法进入闭环状态")
        print(f"  当前状态: {axis.current_state}")
        print(f"  轴错误: 0x{axis.error:08X}")
        print(f"  控制器错误: 0x{axis.controller.error:08X}")
        print(f"  电机错误: 0x{axis.motor.error:08X}")
        return False

def test_velocity_control(odrv, axis_num=0, target_vel=1.0):
    """测试速度控制"""
    print_separator(f"测试节点{axis_num}速度控制")
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"目标速度: {target_vel} turns/s")
    print(f"速度限制: {axis.controller.config.vel_limit} turns/s")
    
    # 设置速度控制模式
    print("\n设置控制模式为速度控制...")
    axis.controller.config.control_mode = 2  # VELOCITY_CONTROL
    time.sleep(0.1)
    
    # 记录初始状态
    vbus_before = odrv.vbus_voltage
    print(f"发送命令前母线电压: {vbus_before:.2f} V")
    
    # 发送速度命令
    print(f"发送速度命令: {target_vel} turns/s")
    axis.controller.input_vel = target_vel
    
    # 监控2秒
    print("\n监控运行状态（2秒）:")
    for i in range(4):
        time.sleep(0.5)
        vel = axis.encoder.vel_estimate
        vbus = odrv.vbus_voltage
        iq = axis.motor.current_control.Iq_measured
        ctrl_err = axis.controller.error
        
        print(f"  [{i*0.5:.1f}s] 速度: {vel:6.2f} turns/s, "
              f"电压: {vbus:5.2f}V, 电流: {iq:6.2f}A, "
              f"错误: 0x{ctrl_err:08X}")
        
        if ctrl_err != 0:
            print(f"\n❌ 速度控制出现错误: 0x{ctrl_err:08X}")
            axis.controller.input_vel = 0
            return False
    
    # 停止
    print("\n停止电机...")
    axis.controller.input_vel = 0
    time.sleep(1)
    
    print("✓ 速度控制测试成功")
    return True

def test_torque_control(odrv, axis_num=0, target_torque=0.5):
    """测试力矩控制"""
    print_separator(f"测试节点{axis_num}力矩控制")
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"目标力矩: {target_torque} Nm")
    print(f"电流限制: {axis.motor.config.current_lim} A")
    print(f"速度限制: {axis.controller.config.vel_limit} turns/s")
    
    # 设置力矩控制模式
    print("\n设置控制模式为力矩控制...")
    axis.controller.config.control_mode = 1  # TORQUE_CONTROL
    time.sleep(0.1)
    
    # 记录初始状态
    vbus_before = odrv.vbus_voltage
    print(f"发送命令前母线电压: {vbus_before:.2f} V")
    
    # 发送力矩命令
    print(f"发送力矩命令: {target_torque} Nm")
    axis.controller.input_torque = target_torque
    
    # 监控2秒
    print("\n监控运行状态（2秒）:")
    for i in range(4):
        time.sleep(0.5)
        vel = axis.encoder.vel_estimate
        vbus = odrv.vbus_voltage
        iq = axis.motor.current_control.Iq_measured
        ctrl_err = axis.controller.error
        axis_err = axis.error
        
        print(f"  [{i*0.5:.1f}s] 速度: {vel:6.2f} turns/s, "
              f"电压: {vbus:5.2f}V, 电流: {iq:6.2f}A, "
              f"控制器错误: 0x{ctrl_err:08X}, 轴错误: 0x{axis_err:08X}")
        
        if ctrl_err != 0 or axis_err != 0:
            print(f"\n❌ 力矩控制出现错误!")
            print(f"  控制器错误: 0x{ctrl_err:08X}")
            print(f"  轴错误: 0x{axis_err:08X}")
            print(f"  电机错误: 0x{axis.motor.error:08X}")
            
            # 分析错误原因
            if axis_err == 0x200:
                print("  → CONTROLLER_FAILED")
            if ctrl_err == 0x01:
                print("  → OVERSPEED (速度超限)")
            if axis.motor.error == 0x10:
                print("  → CONTROL_DEADLINE_MISSED")
            
            axis.controller.input_torque = 0
            return False
    
    # 停止
    print("\n停止电机...")
    axis.controller.input_torque = 0
    time.sleep(1)
    
    print("✓ 力矩控制测试成功")
    return True

def test_different_torque_values(odrv, axis_num=0):
    """测试不同的力矩值"""
    print_separator(f"测试节点{axis_num}不同力矩值")
    axis = getattr(odrv, f'axis{axis_num}')
    
    torque_values = [0.1, 0.3, 0.5, 1.0]
    
    for torque in torque_values:
        print(f"\n--- 测试力矩: {torque} Nm ---")
        
        # 清除可能的错误
        axis.error = 0
        axis.controller.error = 0
        
        # 设置力矩控制
        axis.controller.config.control_mode = 1
        time.sleep(0.1)
        
        # 发送命令
        axis.controller.input_torque = torque
        time.sleep(1)
        
        # 检查状态
        vel = axis.encoder.vel_estimate
        vbus = odrv.vbus_voltage
        ctrl_err = axis.controller.error
        axis_err = axis.error
        
        print(f"  速度: {vel:.2f} turns/s")
        print(f"  电压: {vbus:.2f} V")
        print(f"  控制器错误: 0x{ctrl_err:08X}")
        print(f"  轴错误: 0x{axis_err:08X}")
        
        # 停止
        axis.controller.input_torque = 0
        time.sleep(0.5)
        
        if ctrl_err != 0 or axis_err != 0:
            print(f"  ❌ 力矩 {torque} Nm 时出现错误")
            return torque
        else:
            print(f"  ✓ 力矩 {torque} Nm 正常")
    
    return None

def main():
    print_separator("ODrive 最全面诊断")
    print("目标：找出力矩控制和速度控制的具体问题")
    
    # 1. 连接 ODrive
    print("\n[步骤1] 连接 ODrive...")
    try:
        odrv = odrive.find_any(timeout=10)
        print("✓ 已连接到 ODrive")
    except Exception as e:
        print(f"❌ 无法连接到 ODrive: {e}")
        return False
    
    # 2. 检查电源
    print("\n[步骤2] 检查电源...")
    if not check_power_supply(odrv):
        print("⚠️  电源电压过低，可能影响测试结果")
    
    # 3. 检查节点0状态
    print("\n[步骤3] 检查节点0状态...")
    check_axis_status(odrv, 0)
    
    # 4. 清除错误
    print("\n[步骤4] 清除错误...")
    clear_errors(odrv, 0)
    
    # 5. 进入闭环
    print("\n[步骤5] 进入闭环...")
    if not enter_closed_loop(odrv, 0):
        print("❌ 无法进入闭环，测试终止")
        return False
    
    # 6. 测试速度控制
    print("\n[步骤6] 测试速度控制...")
    vel_result = test_velocity_control(odrv, 0, 1.0)
    
    # 7. 测试力矩控制
    print("\n[步骤7] 测试力矩控制...")
    torque_result = test_torque_control(odrv, 0, 0.5)
    
    # 8. 如果力矩控制失败，测试不同力矩值
    if not torque_result:
        print("\n[步骤8] 力矩控制失败，测试不同力矩值...")
        # 重新进入闭环
        clear_errors(odrv, 0)
        if enter_closed_loop(odrv, 0):
            failed_torque = test_different_torque_values(odrv, 0)
            if failed_torque:
                print(f"\n发现：力矩值 >= {failed_torque} Nm 时出现问题")
    
    # 9. 退出闭环
    print("\n[步骤9] 退出闭环...")
    odrv.axis0.requested_state = 1  # IDLE
    time.sleep(0.5)
    
    # 10. 最终总结
    print_separator("诊断总结")
    print(f"速度控制: {'✓ 正常' if vel_result else '❌ 失败'}")
    print(f"力矩控制: {'✓ 正常' if torque_result else '❌ 失败'}")
    
    if vel_result and not torque_result:
        print("\n结论：速度控制正常，但力矩控制有问题")
        print("\n可能原因：")
        print("1. 力矩控制导致速度超过限制")
        print("2. 力矩控制导致电流过大，电压跌落")
        print("3. 力矩控制的PID参数不合适")
        print("4. 力矩控制模式配置问题")
    elif not vel_result and not torque_result:
        print("\n结论：速度控制和力矩控制都有问题")
    elif vel_result and torque_result:
        print("\n结论：速度控制和力矩控制都正常！")
    
    return vel_result and torque_result

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n操作已取消")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
