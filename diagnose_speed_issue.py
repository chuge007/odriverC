#!/usr/bin/env python3
"""
诊断节点0速度太快的问题
"""
import odrive
import time

def main():
    print("="*60)
    print("  节点0速度问题诊断工具")
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
    
    print("\n" + "="*60)
    print("  速度配置分析")
    print("="*60)
    
    # 控制器配置
    vel_limit = axis.controller.config.vel_limit
    vel_ramp_rate = axis.controller.config.vel_ramp_rate
    pos_gain = axis.controller.config.pos_gain
    vel_gain = axis.controller.config.vel_gain
    vel_integrator_gain = axis.controller.config.vel_integrator_gain
    
    print(f"\n控制器配置:")
    print(f"  速度限制 (vel_limit): {vel_limit} 转/秒")
    print(f"  加速度限制 (vel_ramp_rate): {vel_ramp_rate} 转/秒²")
    print(f"  位置增益 (pos_gain): {pos_gain} (转/秒)/转")
    print(f"  速度增益 (vel_gain): {vel_gain}")
    print(f"  速度积分增益 (vel_integrator_gain): {vel_integrator_gain}")
    
    # 分析速度设置
    print("\n" + "="*60)
    print("  速度分析")
    print("="*60)
    
    if vel_limit > 5.0:
        print(f"\n⚠ 速度限制过高: {vel_limit} 转/秒")
        print(f"  建议: 降低到 1.0-2.0 转/秒")
        print(f"  原因: 速度太快难以控制")
    elif vel_limit > 2.0:
        print(f"\n⚠ 速度限制较高: {vel_limit} 转/秒")
        print(f"  建议: 如果觉得太快，可以降低到 1.0 转/秒")
    elif vel_limit >= 0.5:
        print(f"\n✓ 速度限制合理: {vel_limit} 转/秒")
    else:
        print(f"\n⚠ 速度限制过低: {vel_limit} 转/秒")
        print(f"  问题: 速度太慢可能导致无法完成动作")
        print(f"  建议: 提高到 1.0 转/秒")
    
    if vel_ramp_rate > 2.0:
        print(f"\n⚠ 加速度过高: {vel_ramp_rate} 转/秒²")
        print(f"  建议: 降低到 0.5-1.0 转/秒²")
        print(f"  原因: 加速太快会导致突然启动")
    elif vel_ramp_rate >= 0.2:
        print(f"\n✓ 加速度合理: {vel_ramp_rate} 转/秒²")
    else:
        print(f"\n⚠ 加速度过低: {vel_ramp_rate} 转/秒²")
        print(f"  问题: 加速太慢")
    
    if pos_gain > 20.0:
        print(f"\n⚠ 位置增益过高: {pos_gain}")
        print(f"  建议: 降低到 5.0-10.0")
        print(f"  原因: 增益太高会导致响应过快、震荡")
    elif pos_gain >= 5.0:
        print(f"\n✓ 位置增益合理: {pos_gain}")
    else:
        print(f"\n⚠ 位置增益过低: {pos_gain}")
        print(f"  问题: 响应太慢")
    
    # 性能估算
    print("\n" + "="*60)
    print("  性能估算")
    print("="*60)
    
    print(f"\n以当前配置 (速度限制 {vel_limit} 转/秒):")
    print(f"  - 移动 1 转需要: 约 {1/vel_limit:.1f} 秒")
    print(f"  - 移动 5 转需要: 约 {5/vel_limit:.1f} 秒")
    print(f"  - 移动 10 转需要: 约 {10/vel_limit:.1f} 秒")
    print(f"  - 加速到最大速度需要: 约 {vel_limit/vel_ramp_rate:.1f} 秒")
    
    # 问题诊断
    print("\n" + "="*60)
    print("  问题诊断")
    print("="*60)
    
    issues = []
    
    if vel_limit > 2.0:
        issues.append(f"速度限制过高 ({vel_limit} 转/秒)")
    
    if vel_ramp_rate > 1.0:
        issues.append(f"加速度过高 ({vel_ramp_rate} 转/秒²)")
    
    if pos_gain > 15.0:
        issues.append(f"位置增益过高 ({pos_gain})")
    
    if vel_gain > 0.5:
        issues.append(f"速度增益可能过高 ({vel_gain})")
    
    if issues:
        print("\n发现以下可能导致速度太快的问题:")
        for i, issue in enumerate(issues, 1):
            print(f"  {i}. {issue}")
        
        print("\n" + "="*60)
        print("  建议的解决方案")
        print("="*60)
        
        print("\n推荐配置（安全且可控）:")
        print("  - 速度限制: 1.0 转/秒")
        print("  - 加速度: 0.5 转/秒²")
        print("  - 位置增益: 8.0")
        
        print("\n如果需要更慢:")
        print("  - 速度限制: 0.5 转/秒")
        print("  - 加速度: 0.3 转/秒²")
        print("  - 位置增益: 5.0")
        
        print("\n如果需要更快:")
        print("  - 速度限制: 2.0 转/秒")
        print("  - 加速度: 1.0 转/秒²")
        print("  - 位置增益: 10.0")
        
        response = input("\n是否应用推荐配置（1.0 转/秒）? (y/n): ")
        if response.lower() == 'y':
            print("\n应用推荐配置...")
            axis.controller.config.vel_limit = 1.0
            axis.controller.config.vel_ramp_rate = 0.5
            axis.controller.config.pos_gain = 8.0
            
            print("保存配置...")
            odrv0.save_configuration()
            print("✓ 配置已保存，ODrive 将重启")
            print("\n请等待几秒后重新测试")
    else:
        print("\n✓ 当前配置看起来合理")
        print("\n如果仍然觉得速度太快，可能的原因:")
        print("  1. Qt 程序发送的位置目标值太大（如 -10 转）")
        print("  2. 使用速度控制模式时发送的速度值太大")
        print("  3. 电机负载太小，实际速度超过设定值")
        
        print("\n建议:")
        print("  1. 在 Qt 程序中使用更小的位置值（如 -1 转）")
        print("  2. 在速度控制模式下使用更小的速度值（如 0.5 转/秒）")
        print("  3. 检查 Qt 程序发送的 CAN 命令")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
