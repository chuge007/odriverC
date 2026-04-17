#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复节点1的错误
错误代码 0x00000028 = 0x00000020 + 0x00000008
"""

import odrive
from odrive.enums import *
import sys
import time

def fix_node1(odrv):
    """修复节点1"""
    axis = odrv.axis1
    
    print("="*60)
    print("修复节点1")
    print("="*60)
    
    # 1. 显示当前状态
    print(f"\n当前状态:")
    print(f"  轴错误: 0x{axis.error:08X}")
    print(f"  电机错误: 0x{axis.motor.error:08X}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    print(f"  控制器错误: 0x{axis.controller.error:08X}")
    
    # 2. 清除所有错误
    print(f"\n步骤1: 清除所有错误...")
    axis.clear_errors()
    time.sleep(1)
    
    # 3. 重新配置参数（更保守）
    print(f"步骤2: 重新配置参数（更保守）...")
    
    # 降低电流限制
    axis.motor.config.current_lim = 8.0
    axis.motor.config.current_lim_margin = 6.0
    print(f"  电流限制: 8A")
    
    # 降低速度限制和增益
    axis.controller.config.vel_limit = 5.0  # 降低到5圈/秒
    axis.controller.config.vel_gain = 0.01  # 进一步降低增益
    axis.controller.config.vel_integrator_gain = 0.05  # 进一步降低积分增益
    print(f"  速度限制: 5 圈/秒")
    print(f"  速度增益: 0.01")
    print(f"  积分增益: 0.05")
    
    # 4. 尝试进入闭环
    print(f"\n步骤3: 尝试进入闭环...")
    for attempt in range(3):
        print(f"  尝试 {attempt + 1}/3...")
        
        axis.clear_errors()
        time.sleep(0.5)
        
        axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(2)
        
        if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print(f"  ✓ 成功进入闭环控制！")
            print(f"     状态: {axis.current_state}")
            print(f"     位置: {axis.encoder.pos_estimate:.3f} 圈")
            return True
        else:
            print(f"  ✗ 失败，状态: {axis.current_state}")
            if axis.error != 0:
                print(f"     错误: 0x{axis.error:08X}")
    
    # 5. 如果还是失败，尝试重新校准编码器偏移
    print(f"\n步骤4: 尝试重新校准编码器偏移...")
    axis.clear_errors()
    time.sleep(0.5)
    
    axis.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION
    print(f"  校准中...")
    
    start_time = time.time()
    while axis.current_state != AXIS_STATE_IDLE:
        if time.time() - start_time > 30:
            print(f"  ✗ 校准超时")
            return False
        
        if axis.error != 0:
            print(f"  ✗ 校准失败: 0x{axis.error:08X}")
            return False
        
        time.sleep(0.5)
        print(".", end="", flush=True)
    
    print()
    print(f"  ✓ 编码器校准完成")
    
    # 6. 再次尝试进入闭环
    print(f"\n步骤5: 再次尝试进入闭环...")
    axis.clear_errors()
    time.sleep(0.5)
    
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(2)
    
    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
        print(f"  ✓ 成功进入闭环控制！")
        return True
    else:
        print(f"  ✗ 仍然失败")
        print(f"     状态: {axis.current_state}")
        print(f"     错误: 0x{axis.error:08X}")
        return False

def main():
    print("连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
        print(f"  母线电压: {odrv.vbus_voltage:.2f} V\n")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    if fix_node1(odrv):
        print(f"\n{'='*60}")
        print("✓ 节点1修复成功！")
        print(f"{'='*60}")
        
        # 保存配置
        if input("\n是否保存配置? (y/n): ").lower() == 'y':
            print("保存配置...")
            odrv.save_configuration()
            print("✓ 已保存")
        
        return 0
    else:
        print(f"\n{'='*60}")
        print("✗ 节点1修复失败")
        print(f"{'='*60}")
        print("\n可能的原因:")
        print("  1. 编码器连接问题")
        print("  2. 电机连接问题")
        print("  3. 硬件故障")
        print("\n建议:")
        print("  1. 检查编码器连接")
        print("  2. 检查电机连接")
        print("  3. 尝试更换电机或编码器")
        return 1

if __name__ == "__main__":
    sys.exit(main())
