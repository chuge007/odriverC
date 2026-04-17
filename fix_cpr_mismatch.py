#!/usr/bin/env python3
"""
修复 CPR_POLEPAIRS_MISMATCH 错误
"""
import odrive
import time

def main():
    print("正在连接 ODrive...")
    odrv0 = odrive.find_any(timeout=5)
    print(f"✓ 已连接: {odrv0.serial_number}")
    
    axis = odrv0.axis0
    
    print("\n当前配置:")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  极对数: {axis.motor.config.pole_pairs}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    
    # 清除错误
    print("\n清除错误...")
    axis.error = 0
    axis.motor.error = 0
    axis.encoder.error = 0
    axis.controller.error = 0
    time.sleep(0.5)
    
    # 方案1：使用正确的 CPR（16384）
    print("\n方案1：设置 CPR 为 16384（之前检测到的值）")
    print("这是你的编码器实际的 CPR 值")
    
    axis.encoder.config.cpr = 16384
    print(f"✓ CPR 已设置为: {axis.encoder.config.cpr}")
    
    # 禁用索引搜索（因为之前设置为 False）
    axis.encoder.config.use_index = False
    axis.config.startup_encoder_index_search = False
    print("✓ 已禁用索引搜索")
    
    # 保存配置
    print("\n保存配置...")
    odrv0.save_configuration()
    print("✓ 配置已保存")
    
    print("\n等待重启...")
    time.sleep(3)
    
    # 重新连接
    print("重新连接...")
    odrv0 = odrive.find_any(timeout=10)
    axis = odrv0.axis0
    print("✓ 已重新连接")
    
    # 检查状态
    print("\n检查新状态:")
    print(f"  CPR: {axis.encoder.config.cpr}")
    print(f"  极对数: {axis.motor.config.pole_pairs}")
    print(f"  编码器错误: 0x{axis.encoder.error:08X}")
    print(f"  轴错误: 0x{axis.error:08X}")
    
    if axis.encoder.error == 0 and axis.error == 0:
        print("\n✓ 错误已修复！")
        print("\n现在可以:")
        print("1. 在 Qt 程序中连接到 ODrive")
        print("2. 点击'进入闭环'")
        print("3. 开始控制电机")
    else:
        print("\n⚠ 仍有错误")
        print(f"编码器错误: 0x{axis.encoder.error:08X}")
        print(f"轴错误: 0x{axis.error:08X}")
        
        if axis.encoder.error & 0x00000002:
            print("\n仍然有 CPR_POLEPAIRS_MISMATCH 错误")
            print("可能需要手动设置 CPR")
            print("\n尝试其他常见 CPR 值:")
            print("  - 2048")
            print("  - 4096")
            print("  - 8192")
            print("  - 16384")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
