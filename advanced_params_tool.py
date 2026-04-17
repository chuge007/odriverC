#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive 高级参数工具
用于读取和设置ODrive的核心监控参数和控制参数
"""

import odrive
from odrive.enums import *
import sys
import time

def print_separator(title=""):
    """打印分隔线"""
    if title:
        print(f"\n{'='*60}")
        print(f"  {title}")
        print(f"{'='*60}")
    else:
        print(f"{'='*60}")

def read_core_monitoring_params(axis):
    """读取核心监控参数（5个必看参数）"""
    print_separator("核心监控参数（实时数据）")
    
    try:
        # 1. 电源电压
        vbus = axis.parent.vbus_voltage
        print(f"1. 电源电压 (vbus_voltage):        {vbus:.2f} V")
        
        # 2. 位置估算
        pos = axis.pos_estimate
        print(f"2. 位置估算 (pos_estimate):        {pos:.3f} 圈")
        
        # 3. 速度估算
        vel = axis.vel_estimate
        print(f"3. 速度估算 (vel_estimate):        {vel:.3f} 圈/秒")
        
        # 4. Iq设定值
        iq_setpoint = axis.motor.current_control.Iq_setpoint
        print(f"4. Iq设定 (Iq_setpoint):           {iq_setpoint:.3f} A")
        
        # 5. Iq测量值
        iq_measured = axis.motor.current_control.Iq_measured
        print(f"5. Iq测量 (Iq_measured):           {iq_measured:.3f} A")
        
        return True
    except Exception as e:
        print(f"❌ 读取监控参数失败: {e}")
        return False

def read_pid_params(axis):
    """读取PID参数（5个必调参数）"""
    print_separator("PID控制参数")
    
    try:
        # 1. 位置增益
        pos_gain = axis.controller.config.pos_gain
        print(f"1. 位置增益 (pos_gain):            {pos_gain:.3f}")
        
        # 2. 速度增益
        vel_gain = axis.controller.config.vel_gain
        print(f"2. 速度增益 (vel_gain):            {vel_gain:.6f}")
        
        # 3. 速度积分增益
        vel_int_gain = axis.controller.config.vel_integrator_gain
        print(f"3. 速度积分增益 (vel_integrator_gain): {vel_int_gain:.6f}")
        
        # 4. 电流限制
        current_lim = axis.motor.config.current_lim
        print(f"4. 电流限制 (current_lim):         {current_lim:.3f} A")
        
        # 5. 速度限制
        vel_limit = axis.controller.config.vel_limit
        print(f"5. 速度限制 (vel_limit):           {vel_limit:.3f} 圈/秒")
        
        return True
    except Exception as e:
        print(f"❌ 读取PID参数失败: {e}")
        return False

def read_encoder_params(axis):
    """读取编码器参数"""
    print_separator("编码器参数")
    
    try:
        # 配置参数
        cpr = axis.encoder.config.cpr
        mode = axis.encoder.config.mode
        print(f"CPR (每圈脉冲数):                  {cpr}")
        print(f"模式 (mode):                       {mode}")
        
        # 实时数据
        shadow_count = axis.encoder.shadow_count
        count_in_cpr = axis.encoder.count_in_cpr
        print(f"Shadow Count:                      {shadow_count}")
        print(f"Count in CPR:                      {count_in_cpr}")
        
        return True
    except Exception as e:
        print(f"❌ 读取编码器参数失败: {e}")
        return False

def read_motor_params(axis):
    """读取电机参数"""
    print_separator("电机参数")
    
    try:
        pole_pairs = axis.motor.config.pole_pairs
        torque_constant = axis.motor.config.torque_constant
        motor_current_lim = axis.motor.config.current_lim
        
        print(f"极对数 (pole_pairs):               {pole_pairs}")
        print(f"力矩常数 (torque_constant):        {torque_constant:.6f} Nm/A")
        print(f"电机电流限制 (current_lim):        {motor_current_lim:.3f} A")
        
        return True
    except Exception as e:
        print(f"❌ 读取电机参数失败: {e}")
        return False

def read_trajectory_params(axis):
    """读取轨迹控制参数"""
    print_separator("轨迹控制参数")
    
    try:
        vel_limit = axis.trap_traj.config.vel_limit
        accel_limit = axis.trap_traj.config.accel_limit
        decel_limit = axis.trap_traj.config.decel_limit
        
        print(f"速度限制 (vel_limit):              {vel_limit:.3f} 圈/秒")
        print(f"加速度限制 (accel_limit):          {accel_limit:.3f} 圈/秒²")
        print(f"减速度限制 (decel_limit):          {decel_limit:.3f} 圈/秒²")
        
        return True
    except Exception as e:
        print(f"❌ 读取轨迹参数失败: {e}")
        return False

def set_pid_params(axis, pos_gain=None, vel_gain=None, vel_integrator_gain=None):
    """设置PID参数"""
    print_separator("设置PID参数")
    
    try:
        if pos_gain is not None:
            axis.controller.config.pos_gain = pos_gain
            print(f"✓ 设置 pos_gain = {pos_gain}")
        
        if vel_gain is not None:
            axis.controller.config.vel_gain = vel_gain
            print(f"✓ 设置 vel_gain = {vel_gain}")
        
        if vel_integrator_gain is not None:
            axis.controller.config.vel_integrator_gain = vel_integrator_gain
            print(f"✓ 设置 vel_integrator_gain = {vel_integrator_gain}")
        
        print("\n⚠️  参数已设置，但未保存到flash。")
        print("   如需永久保存，请运行: odrv0.save_configuration()")
        
        return True
    except Exception as e:
        print(f"❌ 设置PID参数失败: {e}")
        return False

def set_limits(axis, vel_limit=None, current_lim=None):
    """设置限制参数"""
    print_separator("设置限制参数")
    
    try:
        if vel_limit is not None:
            axis.controller.config.vel_limit = vel_limit
            print(f"✓ 设置 vel_limit = {vel_limit} 圈/秒")
        
        if current_lim is not None:
            axis.motor.config.current_lim = current_lim
            print(f"✓ 设置 current_lim = {current_lim} A")
        
        print("\n⚠️  参数已设置，但未保存到flash。")
        print("   如需永久保存，请运行: odrv0.save_configuration()")
        
        return True
    except Exception as e:
        print(f"❌ 设置限制参数失败: {e}")
        return False

def monitor_realtime(axis, duration=10):
    """实时监控核心参数"""
    print_separator(f"实时监控（{duration}秒）")
    print("按 Ctrl+C 停止监控\n")
    
    try:
        start_time = time.time()
        while time.time() - start_time < duration:
            vbus = axis.parent.vbus_voltage
            pos = axis.pos_estimate
            vel = axis.vel_estimate
            iq_set = axis.motor.current_control.Iq_setpoint
            iq_meas = axis.motor.current_control.Iq_measured
            
            print(f"\r电压:{vbus:5.1f}V | 位置:{pos:7.3f}圈 | 速度:{vel:6.2f}圈/s | Iq:{iq_set:5.2f}/{iq_meas:5.2f}A", end='')
            time.sleep(0.1)
        
        print("\n")
        return True
    except KeyboardInterrupt:
        print("\n\n监控已停止")
        return True
    except Exception as e:
        print(f"\n❌ 监控失败: {e}")
        return False

def interactive_menu(axis):
    """交互式菜单"""
    while True:
        print_separator("ODrive 高级参数工具")
        print("1. 读取核心监控参数（5个必看）")
        print("2. 读取PID参数（5个必调）")
        print("3. 读取编码器参数")
        print("4. 读取电机参数")
        print("5. 读取轨迹控制参数")
        print("6. 读取所有参数")
        print("7. 设置PID参数")
        print("8. 设置限制参数")
        print("9. 实时监控（10秒）")
        print("0. 退出")
        print_separator()
        
        choice = input("请选择操作 (0-9): ").strip()
        
        if choice == '1':
            read_core_monitoring_params(axis)
        elif choice == '2':
            read_pid_params(axis)
        elif choice == '3':
            read_encoder_params(axis)
        elif choice == '4':
            read_motor_params(axis)
        elif choice == '5':
            read_trajectory_params(axis)
        elif choice == '6':
            read_core_monitoring_params(axis)
            read_pid_params(axis)
            read_encoder_params(axis)
            read_motor_params(axis)
            read_trajectory_params(axis)
        elif choice == '7':
            print("\n设置PID参数（留空跳过）:")
            pos_gain = input("pos_gain (当前推荐: 8.0): ").strip()
            vel_gain = input("vel_gain (当前推荐: 0.05): ").strip()
            vel_int = input("vel_integrator_gain (当前推荐: 0.2): ").strip()
            
            set_pid_params(
                axis,
                float(pos_gain) if pos_gain else None,
                float(vel_gain) if vel_gain else None,
                float(vel_int) if vel_int else None
            )
        elif choice == '8':
            print("\n设置限制参数（留空跳过）:")
            vel_lim = input("vel_limit (圈/秒): ").strip()
            cur_lim = input("current_lim (A): ").strip()
            
            set_limits(
                axis,
                float(vel_lim) if vel_lim else None,
                float(cur_lim) if cur_lim else None
            )
        elif choice == '9':
            monitor_realtime(axis, 10)
        elif choice == '0':
            print("退出程序")
            break
        else:
            print("❌ 无效选择，请重试")
        
        input("\n按回车继续...")

def main():
    """主函数"""
    print_separator("ODrive 高级参数工具")
    print("正在连接 ODrive...")
    
    try:
        # 连接ODrive
        odrv0 = odrive.find_any()
        print(f"✓ 已连接到 ODrive")
        print(f"  硬件版本: {odrv0.hw_version_major}.{odrv0.hw_version_minor}")
        print(f"  固件版本: {odrv0.fw_version_major}.{odrv0.fw_version_minor}.{odrv0.fw_version_revision}")
        
        # 选择轴
        if len(sys.argv) > 1:
            axis_num = int(sys.argv[1])
        else:
            axis_num = 0
        
        axis = getattr(odrv0, f'axis{axis_num}')
        print(f"  使用轴: axis{axis_num}")
        
        # 启动交互式菜单
        interactive_menu(axis)
        
    except Exception as e:
        print(f"❌ 错误: {e}")
        print("\n请确保:")
        print("1. ODrive已通过USB连接")
        print("2. 已安装odrive库: pip install odrive")
        print("3. ODrive固件版本兼容")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
