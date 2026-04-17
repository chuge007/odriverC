#!/usr/bin/env python3
"""
修复电机控制失败问题
- 清除错误
- 重新校准节点0
- 修复节点1的LOCKIN_SPIN状态
"""

import odrive
from odrive.enums import *
import time

def main():
    print("="*60)
    print("🔧 修复电机控制失败")
    print("="*60)
    
    try:
        print("\n📡 连接 ODrive...")
        odrv = odrive.find_any()
        print(f"✅ 已连接: {hex(odrv.serial_number)}")
        
        # 清除所有错误
        print("\n🧹 清除所有错误...")
        odrv.axis0.error = 0
        odrv.axis0.motor.error = 0
        odrv.axis0.encoder.error = 0
        odrv.axis0.controller.error = 0
        odrv.axis1.error = 0
        odrv.axis1.motor.error = 0
        odrv.axis1.encoder.error = 0
        odrv.axis1.controller.error = 0
        time.sleep(0.5)
        print("✅ 错误已清除")
        
        # 检查节点1状态
        print("\n🔍 检查节点1状态...")
        if odrv.axis1.current_state == AXIS_STATE_LOCKIN_SPIN:
            print("⚠ 节点1卡在LOCKIN_SPIN状态，正在修复...")
            odrv.axis1.requested_state = AXIS_STATE_IDLE
            time.sleep(1)
            print("✅ 节点1已设置为IDLE")
        
        # 检查节点0校准状态
        print("\n🔍 检查节点0校准状态...")
        motor_calibrated = odrv.axis0.motor.is_calibrated
        encoder_ready = odrv.axis0.encoder.is_ready
        
        print(f"  电机已校准: {motor_calibrated}")
        print(f"  编码器已就绪: {encoder_ready}")
        
        if not motor_calibrated or not encoder_ready:
            print("\n⚙️ 节点0需要校准，开始完整校准序列...")
            print("⏳ 这将需要约15-20秒...")
            
            odrv.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
            
            # 等待校准完成
            start_time = time.time()
            while odrv.axis0.current_state != AXIS_STATE_IDLE:
                elapsed = time.time() - start_time
                state = odrv.axis0.current_state
                print(f"  [{elapsed:.1f}s] 当前状态: {state}", end='\r')
                
                if elapsed > 30:
                    print("\n❌ 校准超时")
                    break
                
                time.sleep(0.5)
            
            print("\n")
            
            # 检查校准结果
            if odrv.axis0.motor.is_calibrated and odrv.axis0.encoder.is_ready:
                print("✅ 节点0校准成功！")
                
                # 保存配置
                print("\n💾 保存配置...")
                try:
                    odrv.axis0.motor.config.pre_calibrated = True
                    odrv.axis0.encoder.config.pre_calibrated = True
                    print("✅ 配置已保存（设置为预校准）")
                except Exception as e:
                    print(f"⚠ 保存配置失败: {e}")
            else:
                print("❌ 节点0校准失败")
                print(f"  轴错误: 0x{odrv.axis0.error:08X}")
                print(f"  电机错误: 0x{odrv.axis0.motor.error:08X}")
                print(f"  编码器错误: 0x{odrv.axis0.encoder.error:08X}")
                return
        else:
            print("✅ 节点0已校准，无需重新校准")
        
        # 测试进入闭环控制
        print("\n🔄 测试进入闭环控制...")
        
        print("  节点0...")
        odrv.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(1)
        
        if odrv.axis0.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print("  ✅ 节点0进入闭环成功")
            # 返回IDLE
            odrv.axis0.requested_state = AXIS_STATE_IDLE
            time.sleep(0.5)
        else:
            print(f"  ❌ 节点0进入闭环失败，状态: {odrv.axis0.current_state}")
            print(f"     错误: 0x{odrv.axis0.error:08X}")
        
        print("\n  节点1...")
        odrv.axis1.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
        time.sleep(1)
        
        if odrv.axis1.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
            print("  ✅ 节点1进入闭环成功")
            # 返回IDLE
            odrv.axis1.requested_state = AXIS_STATE_IDLE
            time.sleep(0.5)
        else:
            print(f"  ❌ 节点1进入闭环失败，状态: {odrv.axis1.current_state}")
            print(f"     错误: 0x{odrv.axis1.error:08X}")
        
        print("\n" + "="*60)
        print("✅ 修复完成！")
        print("="*60)
        print("\n现在可以使用以下命令启动电机:")
        print("  python auto_start_motor.py")
        print("\n或在Qt程序中测试电机控制")
        
    except Exception as e:
        print(f"\n❌ 修复失败: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
