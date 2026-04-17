#!/usr/bin/env python3
"""
修复力矩和速度控制的 OVERSPEED 错误
问题：速度限制太低（12 turns/s）导致 CONTROLLER_FAILED
解决：增加速度限制并优化 PID 参数
"""

import odrive
import time
import sys

def main():
    print("=" * 60)
    print("修复 ODrive 力矩/速度控制 OVERSPEED 错误")
    print("=" * 60)
    
    # 1. 连接 ODrive
    print("\n[1/7] 正在连接 ODrive...")
    try:
        odrv = odrive.find_any(timeout=10)
        print("✓ 已连接到 ODrive")
    except Exception as e:
        print(f"❌ 无法连接到 ODrive: {e}")
        return False
    
    # 2. 显示当前错误状态
    print("\n[2/7] 当前错误状态:")
    print(f"  轴错误: 0x{odrv.axis0.error:08X}")
    print(f"  控制器错误: 0x{odrv.axis0.controller.error:08X} (OVERSPEED)")
    print(f"  电机错误: 0x{odrv.axis0.motor.error:08X}")
    
    # 3. 显示当前配置
    print("\n[3/7] 当前配置:")
    print(f"  速度限制: {odrv.axis0.controller.config.vel_limit} turns/s (太低！)")
    print(f"  位置增益: {odrv.axis0.controller.config.pos_gain} (太高！)")
    print(f"  速度增益: {odrv.axis0.controller.config.vel_gain}")
    print(f"  速度积分增益: {odrv.axis0.controller.config.vel_integrator_gain}")
    
    # 4. 清除错误
    print("\n[4/7] 清除所有错误...")
    odrv.axis0.error = 0
    odrv.axis0.controller.error = 0
    odrv.axis0.motor.error = 0
    odrv.axis0.encoder.error = 0
    time.sleep(0.5)
    print("✓ 错误已清除")
    
    # 5. 修改配置
    print("\n[5/7] 修改配置参数...")
    
    # 增加速度限制（关键修复！）
    old_vel_limit = odrv.axis0.controller.config.vel_limit
    odrv.axis0.controller.config.vel_limit = 50.0
    print(f"  速度限制: {old_vel_limit} → 50.0 turns/s")
    
    # 降低位置增益
    old_pos_gain = odrv.axis0.controller.config.pos_gain
    odrv.axis0.controller.config.pos_gain = 1.0
    print(f"  位置增益: {old_pos_gain} → 1.0")
    
    # 调整速度增益
    old_vel_gain = odrv.axis0.controller.config.vel_gain
    odrv.axis0.controller.config.vel_gain = 0.02
    print(f"  速度增益: {old_vel_gain} → 0.02")
    
    # 关闭速度积分（先测试）
    old_vel_int = odrv.axis0.controller.config.vel_integrator_gain
    odrv.axis0.controller.config.vel_integrator_gain = 0.0
    print(f"  速度积分增益: {old_vel_int} → 0.0")
    
    print("✓ 配置已更新")
    
    # 6. 测试进入闭环
    print("\n[6/7] 测试进入闭环...")
    odrv.axis0.requested_state = 8  # CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if odrv.axis0.current_state == 8 and odrv.axis0.error == 0:
        print("✓ 成功进入闭环状态！")
        
        # 7. 测试速度控制
        print("\n[7/7] 测试速度控制...")
        print("  设置速度: 2.0 turns/s")
        odrv.axis0.controller.config.control_mode = 2  # VELOCITY_CONTROL
        odrv.axis0.controller.input_vel = 2.0
        time.sleep(2)
        
        current_vel = odrv.axis0.encoder.vel_estimate
        controller_error = odrv.axis0.controller.error
        
        print(f"  当前速度: {current_vel:.2f} turns/s")
        print(f"  控制器错误: 0x{controller_error:08X}")
        
        if controller_error == 0:
            print("✓ 速度控制测试成功！")
        else:
            print("❌ 速度控制仍有错误")
        
        # 停止电机
        odrv.axis0.controller.input_vel = 0.0
        time.sleep(1)
        
        # 测试力矩控制
        print("\n  测试力矩控制...")
        print("  设置力矩: 0.5 Nm")
        odrv.axis0.controller.config.control_mode = 1  # TORQUE_CONTROL
        odrv.axis0.controller.input_torque = 0.5
        time.sleep(1)
        
        current_vel = odrv.axis0.encoder.vel_estimate
        controller_error = odrv.axis0.controller.error
        
        print(f"  当前速度: {current_vel:.2f} turns/s")
        print(f"  控制器错误: 0x{controller_error:08X}")
        
        if controller_error == 0:
            print("✓ 力矩控制测试成功！")
        else:
            print("❌ 力矩控制仍有错误")
        
        # 停止电机
        odrv.axis0.controller.input_torque = 0.0
        odrv.axis0.requested_state = 1  # IDLE
        time.sleep(0.5)
        
        print("\n" + "=" * 60)
        print("修复完成！")
        print("=" * 60)
        print("\n建议:")
        print("1. 如果测试成功，保存配置: odrv.save_configuration()")
        print("2. 在 Qt 程序中使用前，确保电机已进入闭环状态")
        print("3. 发送控制命令前，设置足够高的速度限制")
        print("\n是否保存配置？(y/n): ", end='')
        
        try:
            response = input().strip().lower()
            if response == 'y':
                print("\n保存配置并重启...")
                odrv.save_configuration()
                print("✓ 配置已保存")
                print("正在重启 ODrive...")
                odrv.reboot()
                print("✓ 重启完成")
            else:
                print("\n配置未保存（重启后会恢复原配置）")
        except:
            print("\n配置未保存")
        
        return True
    else:
        print(f"❌ 无法进入闭环状态")
        print(f"  当前状态: {odrv.axis0.current_state}")
        print(f"  轴错误: 0x{odrv.axis0.error:08X}")
        return False

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
