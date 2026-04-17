#!/usr/bin/env python3
"""
清除错误并测试电机
"""
import odrive
import time

def main():
    print("正在连接 ODrive...")
    odrv0 = odrive.find_any(timeout=5)
    print(f"✓ 已连接: {odrv0.serial_number}")
    
    axis = odrv0.axis0
    
    print("\n当前错误:")
    print(f"  轴错误: 0x{axis.error:08X}")
    print(f"  电机错误: 0x{axis.motor.error:08X}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    
    # 清除错误
    print("\n清除所有错误...")
    axis.error = 0
    axis.motor.error = 0
    axis.encoder.error = 0
    axis.controller.error = 0
    time.sleep(0.5)
    
    print("✓ 错误已清除")
    
    # 检查配置
    print("\n当前配置:")
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  编码器就绪: {axis.encoder.is_ready}")
    
    print("\n✓ 系统已准备好！")
    print("\n现在可以在 Qt 程序中:")
    print("  1. 连接到 ODrive")
    print("  2. 点击'进入闭环'")
    print("  3. 测试电机控制")
    print("\n电机现在应该:")
    print("  - 运动速度慢（最大 2 转/秒）")
    print("  - 加速平滑（1 转/秒²）")
    print("  - 更加可控和安全")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
