#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复 CONTROL_DEADLINE_MISSED 错误
"""

import odrive
from odrive.enums import *
import time

def fix_control_deadline():
    """修复控制周期超时错误"""
    print("正在连接 ODrive...")
    
    try:
        odrv = odrive.find_any()
        print(f"✅ 已连接: {odrv.serial_number}")
        print(f"母线电压: {odrv.vbus_voltage:.2f} V")
        
        # 显示当前错误
        print(f"\n当前错误状态:")
        print(f"  axis.error = 0x{odrv.axis0.error:08X}")
        print(f"  motor.error = 0x{odrv.axis0.motor.error:08X}")
        print(f"  controller.error = 0x{odrv.axis0.controller.error:08X}")
        
        # 1. 清除所有错误
        print("\n步骤1: 清除所有错误...")
        odrv.axis0.error = 0
        odrv.axis0.motor.error = 0
        odrv.axis0.encoder.error = 0
        odrv.axis0.controller.error = 0
        time.sleep(0.5)
        print("  ✅ 错误已清除")
        
        # 2. 检查并调整控制器配置
        print("\n步骤2: 检查控制器配置...")
        print(f"  当前控制模式: {odrv.axis0.controller.config.control_mode}")
        print(f"  当前输入模式: {odrv.axis0.controller.config.input_mode}")
        
        # 3. 降低控制器增益以减少CPU负载
        print("\n步骤3: 优化控制器参数...")
        
        # 保存当前值
        original_pos_gain = odrv.axis0.controller.config.pos_gain
        original_vel_gain = odrv.axis0.controller.config.vel_gain
        original_vel_integrator_gain = odrv.axis0.controller.config.vel_integrator_gain
        
        print(f"  原始位置增益: {original_pos_gain}")
        print(f"  原始速度增益: {original_vel_gain}")
        print(f"  原始速度积分增益: {original_vel_integrator_gain}")
        
        # 适当降低增益
        odrv.axis0.controller.config.pos_gain = min(original_pos_gain, 20.0)
        odrv.axis0.controller.config.vel_gain = min(original_vel_gain, 0.16)
        odrv.axis0.controller.config.vel_integrator_gain = min(original_vel_integrator_gain, 0.32)
        
        print(f"  新位置增益: {odrv.axis0.controller.config.pos_gain}")
        print(f"  新速度增益: {odrv.axis0.controller.config.vel_gain}")
        print(f"  新速度积分增益: {odrv.axis0.controller.config.vel_integrator_gain}")
        
        # 4. 设置合理的速度和电流限制
        print("\n步骤4: 设置安全限制...")
        odrv.axis0.controller.config.vel_limit = 10.0  # 降低速度限制
        odrv.axis0.motor.config.current_lim = 10.0
        print(f"  速度限制: {odrv.axis0.controller.config.vel_limit} 圈/秒")
        print(f"  电流限制: {odrv.axis0.motor.config.current_lim} A")
        
        # 5. 保存配置
        print("\n步骤5: 保存配置...")
        try:
            odrv.save_configuration()
            time.sleep(2)
            print("  ✅ 配置已保存")
        except Exception as e:
            print(f"  ⚠️ 保存配置失败: {e}")
            print("  配置将在重启后丢失")
        
        time.sleep(1)
        
        # 6. 尝试进入闭环
        print("\n步骤6: 尝试进入闭环...")
        odrv.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        # 检查状态
        current_state = odrv.axis0.current_state
        current_error = odrv.axis0.error
        
        print(f"  当前状态: {current_state}")
        print(f"  当前错误: 0x{current_error:08X}")
        
        if current_state == AXIS_STATE_CLOSED_LOOP_CONTROL and current_error == 0:
            print("\n✅ 成功！节点0已进入闭环状态")
            
            # 测试小幅度运动
            print("\n测试小幅度运动...")
            odrv.axis0.controller.input_pos = 0.1
            time.sleep(1)
            pos = odrv.axis0.encoder.pos_estimate
            print(f"  位置: {pos:.3f} 圈")
            
            odrv.axis0.controller.input_pos = 0
            time.sleep(1)
            
            print("\n✅ 修复完成！电机工作正常")
            return True
        else:
            print(f"\n❌ 仍然无法进入闭环")
            print(f"  axis.error = 0x{odrv.axis0.error:08X}")
            print(f"  motor.error = 0x{odrv.axis0.motor.error:08X}")
            print(f"  controller.error = 0x{odrv.axis0.controller.error:08X}")
            
            if odrv.axis0.motor.error & 0x00000010:
                print("\n⚠️ CONTROL_DEADLINE_MISSED 错误仍然存在")
                print("可能需要:")
                print("1. 检查ODrive固件版本")
                print("2. 降低控制频率")
                print("3. 检查电机和编码器连接")
            
            return False
            
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    import sys
    success = fix_control_deadline()
    sys.exit(0 if success else 1)
