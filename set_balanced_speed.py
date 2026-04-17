#!/usr/bin/env python3
"""
设置平衡的速度参数 - 既不太快也不太慢
"""
import odrive
import time

def main():
    print("正在连接 ODrive...")
    odrv0 = odrive.find_any(timeout=5)
    print(f"✓ 已连接: {odrv0.serial_number}")
    
    axis = odrv0.axis0
    
    print("\n当前配置:")
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
    
    # 设置平衡的速度参数
    print("\n设置平衡的速度参数:")
    print("目标：既安全又能在合理时间内完成动作")
    
    # 速度限制：1.0 转/秒（平衡值）
    # 移动 10 转需要 10 秒，这是合理的
    axis.controller.config.vel_limit = 1.0
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    
    # 加速度限制：0.5 转/秒²（平滑但不太慢）
    axis.controller.config.vel_ramp_rate = 0.5
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    
    # 位置增益：8.0（适中）
    axis.controller.config.pos_gain = 8.0
    print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
    
    # 保存配置
    print("\n保存配置...")
    odrv0.save_configuration()
    print("✓ 配置已保存")
    
    print("\n等待重启...")
    time.sleep(3)
    
    # 重新连接
    print("重新连接...")
    try:
        odrv0 = odrive.find_any(timeout=10)
        axis = odrv0.axis0
        print("✓ 已重新连接")
        
        print("\n新的配置:")
        print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
        print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
        print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
        
        print("\n✓ 配置完成！")
        print("\n现在电机应该:")
        print("  - 速度: 1.0 转/秒（平衡）")
        print("  - 加速度: 0.5 转/秒²（平滑）")
        print("  - 移动 10 转需要约 10 秒（合理）")
        print("  - 既安全又能完成动作")
        
        print("\n性能分析:")
        print("  - 移动 1 转：约 1 秒")
        print("  - 移动 5 转：约 5 秒")
        print("  - 移动 10 转：约 10 秒")
        print("  - 加速时间：约 2 秒（从 0 到 1 转/秒）")
        
    except Exception as e:
        print(f"重新连接失败: {e}")
        print("请等待几秒后手动检查")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
