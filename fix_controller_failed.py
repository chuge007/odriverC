#!/usr/bin/env python3
"""
自动修复 CONTROLLER_FAILED 错误
解决电机震动问题
"""

import odrive
from odrive.enums import *
import time
import sys

def print_header(title):
    """打印标题"""
    print("\n" + "="*60)
    print(f"  {title}")
    print("="*60)

def main():
    print_header("ODrive CONTROLLER_FAILED 自动修复工具")
    
    # 连接 ODrive
    print("\n正在连接 ODrive...")
    try:
        odrv0 = odrive.find_any()
        print(f"✅ 已连接到 ODrive")
        print(f"   序列号: {odrv0.serial_number}")
        print(f"   固件版本: {odrv0.fw_version_major}.{odrv0.fw_version_minor}.{odrv0.fw_version_revision}")
    except Exception as e:
        print(f"❌ 无法连接到 ODrive: {e}")
        return
    
    # 选择轴
    print("\n请选择要修复的轴:")
    print("  0 - axis0")
    print("  1 - axis1")
    
    try:
        axis_num = int(input("输入轴编号 (0 或 1): "))
        if axis_num not in [0, 1]:
            print("无效的轴编号！")
            return
    except:
        print("无效的输入！")
        return
    
    axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
    print(f"\n已选择 axis{axis_num}")
    
    # 步骤1：检查当前错误
    print_header("步骤1：检查当前错误")
    print(f"轴错误: 0x{axis.error:08X}")
    print(f"电机错误: 0x{axis.motor.error:08X}")
    print(f"编码器错误: 0x{axis.encoder.error:08X}")
    print(f"控制器错误: 0x{axis.controller.error:08X}")
    
    if axis.error == 0x00004000:
        print("✅ 检测到 CONTROLLER_FAILED 错误 (0x00004000)")
    elif axis.error != 0:
        print(f"⚠️ 检测到其他错误: 0x{axis.error:08X}")
    else:
        print("✅ 当前没有错误")
    
    # 步骤2：清除错误
    print_header("步骤2：清除所有错误")
    odrv0.clear_errors()
    time.sleep(0.5)
    
    print(f"轴错误: 0x{axis.error:08X}")
    if axis.error == 0:
        print("✅ 错误已清除")
    else:
        print(f"⚠️ 仍有错误: 0x{axis.error:08X}")
    
    # 步骤3：检查电源电压
    print_header("步骤3：检查电源电压")
    try:
        vbus = odrv0.vbus_voltage
        print(f"总线电压: {vbus:.2f} V")
        
        if vbus < 12.0:
            print("❌ 电压过低！必须 ≥ 12V")
            print("   请检查电源连接")
            return
        elif vbus < 18.0:
            print("⚠️ 电压偏低，建议 ≥ 24V")
            print("   可能导致电流不足")
        else:
            print("✅ 电压正常")
    except Exception as e:
        print(f"⚠️ 无法读取电压: {e}")
    
    # 步骤4：检查当前配置
    print_header("步骤4：检查当前配置")
    print(f"电流限制: {axis.motor.config.current_lim:.1f} A")
    print(f"速度限制: {axis.controller.config.vel_limit:.1f} 圈/秒")
    print(f"位置增益: {axis.controller.config.pos_gain:.3f}")
    print(f"速度增益: {axis.controller.config.vel_gain:.6f}")
    
    # 步骤5：调整配置
    print_header("步骤5：调整配置")
    
    need_save = False
    
    # 检查电流限制
    if axis.motor.config.current_lim < 15.0:
        print(f"⚠️ 电流限制过低 ({axis.motor.config.current_lim:.1f} A)")
        print("   建议增加到 20A")
        
        response = input("是否增加电流限制到 20A? (y/n): ").lower()
        if response == 'y':
            axis.motor.config.current_lim = 20.0
            print("✅ 电流限制已设置为 20A")
            need_save = True
        else:
            print("⏭️ 跳过电流限制调整")
    else:
        print(f"✅ 电流限制正常 ({axis.motor.config.current_lim:.1f} A)")
    
    # 检查速度限制
    if axis.controller.config.vel_limit > 15.0:
        print(f"⚠️ 速度限制较高 ({axis.controller.config.vel_limit:.1f} 圈/秒)")
        print("   建议降低到 10 圈/秒")
        
        response = input("是否降低速度限制到 10 圈/秒? (y/n): ").lower()
        if response == 'y':
            axis.controller.config.vel_limit = 10.0
            print("✅ 速度限制已设置为 10 圈/秒")
            need_save = True
        else:
            print("⏭️ 跳过速度限制调整")
    else:
        print(f"✅ 速度限制正常 ({axis.controller.config.vel_limit:.1f} 圈/秒)")
    
    # 保存配置
    if need_save:
        print("\n保存配置并重启...")
        response = input("确认保存配置? (y/n): ").lower()
        if response == 'y':
            try:
                odrv0.save_configuration()
                print("✅ 配置已保存")
                print("正在重启 ODrive...")
                odrv0.reboot()
                print("⏳ 等待重启完成（10秒）...")
                time.sleep(10)
                
                # 重新连接
                print("正在重新连接...")
                odrv0 = odrive.find_any()
                axis = odrv0.axis0 if axis_num == 0 else odrv0.axis1
                print("✅ 重新连接成功")
            except Exception as e:
                print(f"❌ 保存配置失败: {e}")
                return
        else:
            print("⏭️ 跳过保存配置")
    
    # 步骤6：测试闭环控制
    print_header("步骤6：测试闭环控制")
    
    # 清除错误
    odrv0.clear_errors()
    time.sleep(0.5)
    
    print("尝试进入闭环控制...")
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    
    # 等待状态切换
    print("⏳ 等待2秒...")
    time.sleep(2)
    
    # 检查状态
    print(f"当前状态: {axis.current_state}")
    print(f"轴错误: 0x{axis.error:08X}")
    
    if axis.error != 0:
        print(f"❌ 进入闭环失败！错误: 0x{axis.error:08X}")
        
        if axis.error == 0x00004000:
            print("\n⚠️ 仍然是 CONTROLLER_FAILED 错误")
            print("\n可能的原因:")
            print("  1. 电源电压不足（需要 ≥ 24V）")
            print("  2. 电机负载过大")
            print("  3. 编码器连接问题")
            print("  4. 电机相线连接问题")
            print("\n建议:")
            print("  1. 检查电源电压和容量")
            print("  2. 断开机械负载重新测试")
            print("  3. 检查所有连接线")
        
        return
    
    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL:
        print("✅ 成功进入闭环控制！")
    else:
        print(f"⚠️ 状态异常: {axis.current_state}")
        return
    
    # 步骤7：低速测试
    print_header("步骤7：低速测试")
    
    print("发送低速命令: 0.5 圈/秒")
    axis.controller.input_vel = 0.5
    
    print("⏳ 运行5秒...")
    for i in range(5):
        time.sleep(1)
        vel = axis.encoder.vel_estimate
        iq = axis.motor.current_control.Iq_measured
        print(f"  [{i+1}s] 速度: {vel:.3f} 圈/秒, Iq: {iq:.3f} A")
        
        # 检查错误
        if axis.error != 0:
            print(f"❌ 出现错误: 0x{axis.error:08X}")
            break
    
    # 停止
    print("\n停止电机...")
    axis.controller.input_vel = 0.0
    time.sleep(1)
    
    # 检查最终状态
    print(f"最终错误: 0x{axis.error:08X}")
    
    if axis.error == 0:
        print("✅ 测试成功！电机运行正常")
    else:
        print(f"❌ 测试失败，错误: 0x{axis.error:08X}")
    
    # 退出闭环
    print("\n退出闭环控制...")
    axis.requested_state = AXIS_STATE_IDLE
    time.sleep(1)
    
    # 总结
    print_header("修复总结")
    
    if axis.error == 0:
        print("✅ 修复成功！")
        print("\n建议:")
        print("  1. 在 Qt 程序中清除错误后重新测试")
        print("  2. 避免频繁切换控制模式")
        print("  3. 从低速开始逐步增加速度")
        print("  4. 确保电源电压稳定")
    else:
        print("❌ 修复未完成")
        print("\n需要进一步检查:")
        print("  1. 电源电压和容量")
        print("  2. 机械负载")
        print("  3. 编码器连接")
        print("  4. 电机相线连接")
    
    print("\n" + "="*60)
    print("修复完成！")
    print("="*60)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
