#!/usr/bin/env python3
"""
修复电机震动 - PID参数调优
电机能转但有震动，通常是PID参数不合适
"""

import odrive
from odrive.enums import *
import time

def main():
    print("="*60)
    print("  电机震动修复 - PID调优工具")
    print("="*60)
    
    # 连接
    print("\n正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any()
        print("✅ 已连接")
    except Exception as e:
        print(f"❌ 无法连接: {e}")
        return
    
    # 选择轴
    axis_num = int(input("\n输入轴编号 (0 或 1): "))
    axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
    
    print(f"\n已选择 axis{axis_num}")
    
    # 显示当前参数
    print("\n" + "="*60)
    print("  当前PID参数")
    print("="*60)
    print(f"位置增益 (pos_gain): {axis.controller.config.pos_gain:.3f}")
    print(f"速度增益 (vel_gain): {axis.controller.config.vel_gain:.6f}")
    print(f"速度积分增益 (vel_integrator_gain): {axis.controller.config.vel_integrator_gain:.6f}")
    print(f"电流限制: {axis.motor.config.current_lim:.1f} A")
    print(f"速度限制: {axis.controller.config.vel_limit:.1f} 圈/秒")
    
    # 常见震动原因和解决方案
    print("\n" + "="*60)
    print("  常见震动原因")
    print("="*60)
    print("1. 速度增益过高 → 降低 vel_gain")
    print("2. 速度积分增益过高 → 降低 vel_integrator_gain")
    print("3. 位置增益过高 → 降低 pos_gain")
    print("4. 电流限制不足 → 增加 current_lim")
    
    # 推荐的保守参数
    print("\n" + "="*60)
    print("  推荐的保守参数（适合大多数电机）")
    print("="*60)
    print("pos_gain: 5.0 ~ 10.0")
    print("vel_gain: 0.02 ~ 0.1")
    print("vel_integrator_gain: 0.1 ~ 0.5")
    print("current_lim: 15.0 ~ 30.0 A")
    
    # 询问是否应用推荐参数
    print("\n是否应用推荐的保守参数？")
    print("这些参数通常能消除震动，但响应可能较慢")
    
    response = input("\n应用推荐参数? (y/n): ").lower()
    
    if response == 'y':
        print("\n正在应用参数...")
        
        # 保守的PID参数
        axis.controller.config.pos_gain = 8.0
        axis.controller.config.vel_gain = 0.05
        axis.controller.config.vel_integrator_gain = 0.2
        
        # 确保电流限制足够
        if axis.motor.config.current_lim < 15.0:
            axis.motor.config.current_lim = 20.0
            print(f"✅ 电流限制已增加到 20A")
        
        print("✅ PID参数已更新:")
        print(f"   pos_gain = {axis.controller.config.pos_gain:.3f}")
        print(f"   vel_gain = {axis.controller.config.vel_gain:.6f}")
        print(f"   vel_integrator_gain = {axis.controller.config.vel_integrator_gain:.6f}")
        
        # 保存配置
        save = input("\n保存配置到ODrive? (y/n): ").lower()
        if save == 'y':
            odrv0.save_configuration()
            print("✅ 配置已保存")
            print("正在重启...")
            odrv0.reboot()
            print("⏳ 等待10秒...")
            time.sleep(10)
            
            # 重新连接
            odrv0 = odrive.find_any()
            axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
            print("✅ 重新连接成功")
        else:
            print("⏭️ 参数未保存（重启后会恢复）")
    else:
        print("\n⏭️ 跳过参数调整")
    
    # 测试
    print("\n" + "="*60)
    print("  测试电机")
    print("="*60)
    
    test = input("\n是否测试电机? (y/n): ").lower()
    
    if test == 'y':
        # 清除错误
        odrv0.clear_errors()
        time.sleep(0.5)
        
        # 进入闭环
        print("\n进入闭环控制...")
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.error != 0:
            print(f"❌ 错误: 0x{axis.error:08X}")
            return
        
        print("✅ 已进入闭环")
        
        # 低速测试
        print("\n发送低速命令: 0.5 圈/秒")
        axis.controller.input_vel = 0.5
        
        print("运行5秒，观察震动...")
        for i in range(5):
            time.sleep(1)
            vel = axis.encoder.vel_estimate
            iq = axis.motor.current_control.Iq_measured
            print(f"  [{i+1}s] 速度: {vel:.3f} 圈/秒, Iq: {iq:.3f} A")
        
        # 停止
        print("\n停止...")
        axis.controller.input_vel = 0.0
        time.sleep(1)
        axis.requested_state = AXIS_STATE_IDLE
        
        print("\n" + "="*60)
        print("  测试完成")
        print("="*60)
        
        print("\n如果仍然震动:")
        print("  1. 进一步降低 vel_gain (例如 0.02)")
        print("  2. 进一步降低 vel_integrator_gain (例如 0.1)")
        print("  3. 检查编码器连接是否牢固")
        print("  4. 检查电机相线连接")
        print("  5. 运行 monitor_vibration_realtime.py 查看详细数据")
    
    print("\n" + "="*60)
    print("完成！")
    print("="*60)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n错误: {e}")
        import traceback
        traceback.print_exc()
