#!/usr/bin/env python3
"""
自动修复节点0速度太快的问题
"""
import odrive
import time

def main():
    print("="*60)
    print("  自动修复节点0速度问题")
    print("="*60)
    
    print("\n正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any(timeout=5)
        print(f"✓ 已连接: {odrv0.serial_number}")
    except:
        print("✗ 无法连接到 ODrive")
        return
    
    axis = odrv0.axis0
    
    # 清除错误
    print("\n清除所有错误...")
    axis.error = 0
    axis.motor.error = 0
    axis.encoder.error = 0
    axis.controller.error = 0
    time.sleep(0.5)
    print("✓ 错误已清除")
    
    # 显示当前配置
    print("\n当前配置:")
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    print(f"  加速度: {axis.controller.config.vel_ramp_rate} 转/秒²")
    print(f"  位置增益: {axis.controller.config.pos_gain}")
    
    # 应用推荐配置
    print("\n应用推荐配置...")
    axis.controller.config.vel_limit = 1.0
    axis.controller.config.vel_ramp_rate = 0.5
    axis.controller.config.pos_gain = 8.0
    
    print("✓ 新配置:")
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    print(f"  加速度: {axis.controller.config.vel_ramp_rate} 转/秒²")
    print(f"  位置增益: {axis.controller.config.pos_gain}")
    
    # 保存配置
    print("\n保存配置...")
    odrv0.save_configuration()
    print("✓ 配置已保存")
    
    print("\n等待 ODrive 重启...")
    time.sleep(3)
    
    # 重新连接验证
    print("重新连接验证...")
    try:
        odrv0 = odrive.find_any(timeout=10)
        axis = odrv0.axis0
        print("✓ 已重新连接")
        
        print("\n验证新配置:")
        print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
        print(f"  加速度: {axis.controller.config.vel_ramp_rate} 转/秒²")
        print(f"  位置增益: {axis.controller.config.pos_gain}")
        
        print("\n" + "="*60)
        print("  修复完成！")
        print("="*60)
        
        print("\n现在电机应该:")
        print("  - 速度: 1.0 转/秒（可控）")
        print("  - 加速: 0.5 转/秒²（平滑）")
        print("  - 移动 1 转需要约 1 秒")
        print("  - 移动 10 转需要约 10 秒")
        
        print("\n可以在 Qt 程序中测试了！")
        
    except Exception as e:
        print(f"重新连接失败: {e}")
        print("配置已保存，请手动重启 ODrive 或等待几秒")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
