#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动启动已校准的电机（无需交互）
适用于已经校准过的电机，直接进入闭环控制
"""

import odrive
from odrive.enums import *
import sys
import time

def auto_start_motor(odrv, axis_num):
    """自动启动单个电机（跳过校准）"""
    axis = getattr(odrv, f'axis{axis_num}')
    
    print(f"\n{'='*60}")
    print(f"启动节点 {axis_num}")
    print(f"{'='*60}")
    
    try:
        # 1. 清除错误
        print("清除错误...")
        axis.clear_errors()
        time.sleep(0.5)
        
        # 2. 检查校准状态
        print(f"电机已校准: {axis.motor.is_calibrated}")
        print(f"编码器已就绪: {axis.encoder.is_ready}")
        
        if not axis.motor.is_calibrated or not axis.encoder.is_ready:
            print(f"✗ 电机未校准！")
            return False
        
        # 3. 设置保守的控制参数
        print("设置控制参数...")
        axis.controller.config.vel_limit = 10.0
        axis.controller.config.vel_gain = 0.02
        axis.controller.config.vel_integrator_gain = 0.1
        axis.controller.config.pos_gain = 8.0
        axis.controller.config.control_mode = CONTROL_MODE_POSITION_CONTROL
        axis.controller.config.input_mode = INPUT_MODE_PASSTHROUGH
        
        # 4. 设置限制
        print("设置安全限制...")
        axis.motor.config.current_lim = 10.0
        axis.controller.config.vel_limit = 10.0
        
        # 5. 进入闭环控制
        print("进入闭环控制...")
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print(f"✓ 成功！状态={axis.current_state}, 位置={axis.encoder.pos_estimate:.3f}圈")
            return True
        else:
            print(f"✗ 失败！状态={axis.current_state}")
            if axis.error != 0:
                print(f"  轴错误: 0x{axis.error:08X}")
            if axis.motor.error != 0:
                print(f"  电机错误: 0x{axis.motor.error:08X}")
            if axis.encoder.error != 0:
                print(f"  编码器错误: 0x{axis.encoder.error:08X}")
            if axis.controller.error != 0:
                print(f"  控制器错误: 0x{axis.controller.error:08X}")
            return False
            
    except Exception as e:
        print(f"✗ 异常: {e}")
        return False

def main():
    print("="*60)
    print("ODrive 自动启动（已校准电机）")
    print("="*60)
    
    # 连接ODrive
    print("\n连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    # 启动每个轴
    results = {}
    for axis_num in [0, 1]:
        results[axis_num] = auto_start_motor(odrv, axis_num)
    
    # 总结
    print(f"\n{'='*60}")
    print("总结")
    print(f"{'='*60}")
    for axis_num, success in results.items():
        status = "✓ 成功" if success else "✗ 失败"
        print(f"节点 {axis_num}: {status}")
    
    if all(results.values()):
        print("\n🎉 所有节点已进入闭环控制！")
        return 0
    else:
        print("\n⚠ 部分节点启动失败")
        return 1

if __name__ == "__main__":
    sys.exit(main())
