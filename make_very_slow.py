#!/usr/bin/env python3
"""
将电机速度降到非常慢
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
    print(f"  速度增益: {axis.controller.config.vel_gain}")
    print(f"  速度积分增益: {axis.controller.config.vel_integrator_gain}")
    
    # 设置非常慢的速度
    print("\n设置非常慢的速度参数:")
    
    # 速度限制：0.5 转/秒（非常慢）
    axis.controller.config.vel_limit = 0.5
    print(f"  速度限制: {axis.controller.config.vel_limit} 转/秒")
    
    # 加速度限制：0.2 转/秒²（非常平滑）
    axis.controller.config.vel_ramp_rate = 0.2
    print(f"  加速度限制: {axis.controller.config.vel_ramp_rate} 转/秒²")
    
    # 降低位置增益（使位置控制更柔和）
    axis.controller.config.pos_gain = 5.0
    print(f"  位置增益: {axis.controller.config.pos_gain} (转/秒)/转")
    
    # 降低速度增益（使速度控制更柔和）
    current_vel_gain = axis.controller.config.vel_gain
    if current_vel_gain > 0.01:
        axis.controller.config.vel_gain = current_vel_gain * 0.5
        print(f"  速度增益: {axis.controller.config.vel_gain}")
    
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
        print("  - 最大速度: 0.5 转/秒（非常慢）")
        print("  - 加速度: 0.2 转/秒²（非常平滑）")
        print("  - 运动非常缓慢和可控")
        
        print("\n如果还是太快，可以手动设置更低的值:")
        print("  odrv0.axis0.controller.config.vel_limit = 0.2")
        print("  odrv0.axis0.controller.config.vel_ramp_rate = 0.1")
        
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
