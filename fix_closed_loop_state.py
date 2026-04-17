#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""修复闭环状态问题"""

import odrive
import sys
import time

def main():
    print("正在连接 ODrive...")
    try:
        odrv = odrive.find_any()
        print(f"✓ 已连接: {hex(odrv.serial_number)}")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1
    
    print(f"母线电压: {odrv.vbus_voltage:.2f} V\n")
    
    # 检查两个节点
    for axis_num in [0, 1]:
        axis = getattr(odrv, f'axis{axis_num}')
        print(f"节点 {axis_num}:")
        print(f"  当前状态: {axis.current_state}")
        print(f"  电机已校准: {axis.motor.is_calibrated}")
        print(f"  编码器已就绪: {axis.encoder.is_ready}")
        print(f"  轴错误: 0x{axis.error:08X}")
        
        if axis.error != 0:
            print(f"  ⚠ 清除错误...")
            axis.clear_errors()
            time.sleep(0.5)
        
        # 如果电机已校准且编码器就绪，进入闭环控制
        if axis.motor.is_calibrated and axis.encoder.is_ready:
            print(f"  → 请求进入闭环控制状态 (AXIS_STATE_CLOSED_LOOP_CONTROL = 8)...")
            axis.requested_state = 8  # AXIS_STATE_CLOSED_LOOP_CONTROL
            time.sleep(2)
            
            print(f"  ✓ 新状态: {axis.current_state}")
            if axis.current_state == 8:
                print(f"  ✓ 成功进入闭环控制！")
            else:
                print(f"  ✗ 未能进入闭环控制，当前状态: {axis.current_state}")
                if axis.error != 0:
                    print(f"  ✗ 错误代码: 0x{axis.error:08X}")
        else:
            print(f"  ⚠ 电机未校准或编码器未就绪，无法进入闭环")
            if not axis.motor.is_calibrated:
                print(f"     - 需要运行电机校准")
            if not axis.encoder.is_ready:
                print(f"     - 需要运行编码器校准")
        
        print()
    
    print("完成！")
    return 0

if __name__ == "__main__":
    sys.exit(main())
