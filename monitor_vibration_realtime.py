#!/usr/bin/env python3
"""
实时监控电机震动
找出震动的具体原因
"""

import odrive
from odrive.enums import *
import time
import sys

def main():
    print("="*60)
    print("  实时震动监控工具")
    print("="*60)
    
    # 连接 ODrive
    print("\n正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any()
        print(f"✅ 已连接")
    except Exception as e:
        print(f"❌ 无法连接: {e}")
        return
    
    # 选择轴
    axis_num = int(input("\n输入轴编号 (0 或 1): "))
    axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
    
    print(f"\n开始监控 axis{axis_num}...")
    print("按 Ctrl+C 停止\n")
    
    print("时间(s) | 速度(圈/s) | Iq设定(A) | Iq测量(A) | 误差(A) | 位置(圈) | 状态")
    print("-" * 90)
    
    start_time = time.time()
    last_pos = axis.encoder.pos_estimate
    
    try:
        while True:
            elapsed = time.time() - start_time
            
            # 读取数据
            vel = axis.encoder.vel_estimate
            iq_setpoint = axis.motor.current_control.Iq_setpoint
            iq_measured = axis.motor.current_control.Iq_measured
            iq_error = abs(iq_setpoint - iq_measured)
            pos = axis.encoder.pos_estimate
            state = axis.current_state
            
            # 计算位置变化
            pos_change = pos - last_pos
            last_pos = pos
            
            # 显示数据
            print(f"{elapsed:6.2f}  | {vel:10.3f} | {iq_setpoint:9.3f} | {iq_measured:9.3f} | {iq_error:7.3f} | {pos:8.3f} | {state}")
            
            # 检测异常
            if abs(vel) > 0.1 and iq_error > 1.5:
                print(f"  ⚠️ 电流跟踪误差过大: {iq_error:.3f} A")
            
            if abs(vel) > 0.1 and abs(pos_change) > 0.01:
                print(f"  ⚠️ 位置跳变: {pos_change:.6f} 圈")
            
            time.sleep(0.1)  # 10Hz 更新
            
    except KeyboardInterrupt:
        print("\n\n监控停止")
        
        # 统计分析
        print("\n" + "="*60)
        print("  建议")
        print("="*60)
        
        print("\n如果看到:")
        print("  1. Iq误差持续 > 1.5A → 增加电流限制或降低速度")
        print("  2. 速度波动大 → 调整速度增益 (vel_gain)")
        print("  3. 位置频繁跳变 → 检查编码器连接")
        print("  4. Iq频繁震荡 → 降低速度积分增益 (vel_integrator_gain)")

if __name__ == "__main__":
    main()
