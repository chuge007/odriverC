#!/usr/bin/env python3
"""
降低节点0电机的速度和加速度
"""
import odrive
import time

def main():
    print("正在连接 ODrive...")
    odrv0 = odrive.find_any(timeout=5)
    print(f"✓ 已连接: {odrv0.serial_number}")
    
    axis = odrv0.axis0
    
    print("\n当前速度和加速度限制:")
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
    
    # 降低速度和加速度
    print("\n设置新的限制（更慢、更平滑）:")
    
    # 速度限制：从默认值降低到 2 转/秒（很慢）
    axis.controller.config.vel_limit = 2.0
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    
    # 加速度限制：设置为 1 转/秒²（很平滑的加速）
    axis.controller.config.vel_ramp_rate = 1.0
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    
    # 降低位置增益（使位置控制更柔和）
    if axis.controller.config.pos_gain > 10:
        axis.controller.config.pos_gain = 10.0
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
        print("  - 最大速度: 2 转/秒（很慢）")
        print("  - 加速度: 1 转/秒²（很平滑）")
        print("  - 运动更加可控和安全")
        
        print("\n如果还是太快，可以进一步降低:")
        print("  - 速度限制可以设置为 0.5 或 1.0 转/秒")
        print("  - 加速度可以设置为 0.5 转/秒²")
        
    except Exception as e:
        print(f"重新连接失败: {e}")
        print("请手动重启 ODrive 或等待几秒后再试")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
