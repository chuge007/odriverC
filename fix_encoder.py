#!/usr/bin/env python3
"""
ODrive 编码器快速诊断和修复脚本
"""
import odrive
import time
import sys

def print_section(title):
    """打印分节标题"""
    print("\n" + "="*60)
    print(f"  {title}")
    print("="*60)

def check_connection():
    """检查 ODrive 连接"""
    print_section("1. 检查 ODrive 连接")
    try:
        print("正在搜索 ODrive...")
        odrv0 = odrive.find_any(timeout=10)
        print(f"✓ 已连接到 ODrive")
        print(f"  序列号: {odrv0.serial_number}")
        print(f"  固件版本: {odrv0.fw_version_major}.{odrv0.fw_version_minor}.{odrv0.fw_version_revision}")
        print(f"  母线电压: {odrv0.vbus_voltage:.2f} V")
        return odrv0
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return None

def check_encoder_config(axis):
    """检查编码器配置"""
    print_section("2. 检查编码器配置")
    try:
        print(f"编码器模式: {axis.encoder.config.mode}")
        print(f"CPR (每转脉冲数): {axis.encoder.config.cpr}")
        print(f"带宽: {axis.encoder.config.bandwidth} Hz")
        print(f"使用索引: {axis.encoder.config.use_index}")
        print(f"编码器就绪: {axis.encoder.is_ready}")
        print(f"编码器错误: 0x{axis.encoder.error:08X}")
        
        if axis.encoder.error != 0:
            print(f"⚠ 编码器有错误！")
            return False
        return True
    except Exception as e:
        print(f"✗ 检查失败: {e}")
        return False

def test_encoder_connection(axis):
    """测试编码器连接"""
    print_section("3. 测试编码器连接")
    try:
        count1 = axis.encoder.shadow_count
        print(f"当前编码器计数: {count1}")
        print("\n请手动转动电机轴 1-2 圈...")
        print("等待 5 秒...")
        time.sleep(5)
        
        count2 = axis.encoder.shadow_count
        print(f"新的编码器计数: {count2}")
        diff = abs(count2 - count1)
        print(f"计数变化: {diff}")
        
        if diff > 100:
            print(f"✓ 编码器连接正常 (变化 {diff} 个计数)")
            return True
        else:
            print(f"✗ 编码器可能未连接或配置错误 (仅变化 {diff} 个计数)")
            return False
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        return False

def check_motor_config(axis):
    """检查电机配置"""
    print_section("4. 检查电机配置")
    try:
        print(f"极对数: {axis.motor.config.pole_pairs}")
        print(f"相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
        print(f"相电感: {axis.motor.config.phase_inductance:.9f} H")
        print(f"电流限制: {axis.motor.config.current_lim} A")
        print(f"电机类型: {axis.motor.config.motor_type}")
        print(f"电机错误: 0x{axis.motor.error:08X}")
        
        if axis.motor.error != 0:
            print(f"⚠ 电机有错误！")
            return False
        return True
    except Exception as e:
        print(f"✗ 检查失败: {e}")
        return False

def check_axis_state(axis):
    """检查轴状态"""
    print_section("5. 检查轴状态")
    try:
        state_names = {
            0: "UNDEFINED",
            1: "IDLE",
            2: "STARTUP_SEQUENCE",
            3: "FULL_CALIBRATION_SEQUENCE",
            4: "MOTOR_CALIBRATION",
            7: "ENCODER_INDEX_SEARCH",
            8: "CLOSED_LOOP_CONTROL"
        }
        
        state = axis.current_state
        state_name = state_names.get(state, f"UNKNOWN({state})")
        print(f"当前状态: {state_name}")
        print(f"轴错误: 0x{axis.error:08X}")
        print(f"流程结果: {axis.procedure_result}")
        
        if axis.error != 0:
            print(f"⚠ 轴有错误！")
            return False
        return True
    except Exception as e:
        print(f"✗ 检查失败: {e}")
        return False

def configure_encoder(axis, cpr=8192):
    """配置编码器"""
    print_section("6. 配置编码器")
    try:
        print(f"设置编码器参数...")
        axis.encoder.config.mode = 0  # INCREMENTAL
        axis.encoder.config.cpr = cpr
        axis.encoder.config.bandwidth = 1000
        axis.encoder.config.use_index = True
        
        print(f"✓ 编码器配置完成:")
        print(f"  模式: INCREMENTAL (0)")
        print(f"  CPR: {cpr}")
        print(f"  带宽: 1000 Hz")
        print(f"  使用索引: True")
        return True
    except Exception as e:
        print(f"✗ 配置失败: {e}")
        return False

def calibrate_motor(axis):
    """校准电机"""
    print_section("7. 校准电机")
    try:
        print("开始电机校准...")
        print("电机将会转动，请确保电机可以自由转动！")
        time.sleep(2)
        
        axis.requested_state = 4  # MOTOR_CALIBRATION
        print("校准中", end="", flush=True)
        
        timeout = 30
        start_time = time.time()
        while axis.current_state != 1:  # 等待回到 IDLE
            if time.time() - start_time > timeout:
                print("\n✗ 校准超时")
                return False
            time.sleep(0.5)
            print(".", end="", flush=True)
        
        print()
        if axis.motor.error != 0:
            print(f"✗ 电机校准失败")
            print(f"  错误代码: 0x{axis.motor.error:08X}")
            return False
        else:
            print(f"✓ 电机校准成功")
            print(f"  相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
            print(f"  相电感: {axis.motor.config.phase_inductance:.9f} H")
            return True
    except Exception as e:
        print(f"\n✗ 校准失败: {e}")
        return False

def calibrate_encoder(axis):
    """校准编码器"""
    print_section("8. 校准编码器")
    try:
        print("开始编码器索引搜索...")
        print("电机将会转动寻找索引信号...")
        time.sleep(2)
        
        axis.requested_state = 7  # ENCODER_INDEX_SEARCH
        print("搜索中", end="", flush=True)
        
        timeout = 30
        start_time = time.time()
        while axis.current_state != 1:
            if time.time() - start_time > timeout:
                print("\n✗ 搜索超时")
                return False
            time.sleep(0.5)
            print(".", end="", flush=True)
        
        print()
        if axis.encoder.error != 0:
            print(f"✗ 编码器校准失败")
            print(f"  错误代码: 0x{axis.encoder.error:08X}")
            return False
        else:
            print(f"✓ 编码器校准成功")
            print(f"  编码器已就绪: {axis.encoder.is_ready}")
            return True
    except Exception as e:
        print(f"\n✗ 校准失败: {e}")
        return False

def try_without_index(odrv0, axis):
    """尝试禁用索引搜索"""
    print_section("9. 尝试无索引模式")
    try:
        print("禁用索引搜索...")
        axis.encoder.config.use_index = False
        axis.config.startup_encoder_index_search = False
        
        print("保存配置并重启...")
        odrv0.save_configuration()
        print("等待重启...")
        time.sleep(3)
        
        print("重新连接...")
        odrv0 = odrive.find_any(timeout=10)
        axis = odrv0.axis0
        
        print("✓ 已切换到无索引模式")
        
        # 重新校准电机
        if calibrate_motor(axis):
            print("\n✓ 无索引模式下校准成功！")
            return True
        else:
            print("\n✗ 无索引模式下校准仍然失败")
            return False
    except Exception as e:
        print(f"✗ 操作失败: {e}")
        return False

def test_closed_loop(axis):
    """测试闭环控制"""
    print_section("10. 测试闭环控制")
    try:
        print("尝试进入闭环控制...")
        axis.requested_state = 8  # CLOSED_LOOP_CONTROL
        time.sleep(1)
        
        if axis.current_state == 8:
            print("✓ 成功进入闭环控制！")
            print("\n测试速度控制...")
            axis.controller.config.control_mode = 2  # VELOCITY_CONTROL
            axis.controller.input_vel = 2  # 2 转/秒
            time.sleep(2)
            axis.controller.input_vel = 0
            time.sleep(1)
            
            # 返回 IDLE
            axis.requested_state = 1
            time.sleep(1)
            print("✓ 闭环测试成功！")
            return True
        else:
            print(f"✗ 未能进入闭环，当前状态: {axis.current_state}")
            print(f"  轴错误: 0x{axis.error:08X}")
            return False
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        return False

def main():
    """主函数"""
    print("\n" + "="*60)
    print("  ODrive 编码器诊断和修复工具")
    print("="*60)
    
    # 1. 连接 ODrive
    odrv0 = check_connection()
    if not odrv0:
        print("\n无法连接到 ODrive，请检查连接后重试")
        return
    
    axis = odrv0.axis0
    
    # 清除所有错误
    print("\n清除所有错误...")
    try:
        odrv0.clear_errors()
    except AttributeError:
        # 0.5.x 版本使用不同的方法
        for axis in [odrv0.axis0, odrv0.axis1]:
            axis.error = 0
            axis.motor.error = 0
            axis.encoder.error = 0
            axis.controller.error = 0
    time.sleep(0.5)
    
    # 2-5. 检查配置和状态
    check_encoder_config(axis)
    encoder_connected = test_encoder_connection(axis)
    check_motor_config(axis)
    check_axis_state(axis)
    
    if not encoder_connected:
        print("\n⚠ 警告: 编码器连接测试失败")
        response = input("是否继续尝试配置和校准? (y/n): ")
        if response.lower() != 'y':
            return
    
    # 6. 配置编码器
    cpr_input = input("\n请输入编码器 CPR (默认 8192，直接回车使用默认值): ")
    cpr = int(cpr_input) if cpr_input.strip() else 8192
    
    if not configure_encoder(axis, cpr):
        print("\n配置失败，退出")
        return
    
    # 7. 校准电机
    if not calibrate_motor(axis):
        print("\n电机校准失败")
        print("可能的原因:")
        print("  1. 电机未正确连接")
        print("  2. 电源电压不足")
        print("  3. 电机参数配置错误")
        return
    
    # 8. 校准编码器
    if not calibrate_encoder(axis):
        print("\n编码器校准失败")
        response = input("是否尝试禁用索引搜索? (y/n): ")
        if response.lower() == 'y':
            if try_without_index(odrv0, axis):
                # 重新获取连接
                odrv0 = odrive.find_any(timeout=10)
                axis = odrv0.axis0
            else:
                print("\n所有尝试都失败了")
                return
        else:
            return
    
    # 保存配置
    print_section("保存配置")
    print("保存配置到 ODrive...")
    odrv0.save_configuration()
    print("✓ 配置已保存")
    
    # 10. 测试闭环
    test_closed_loop(axis)
    
    # 总结
    print_section("诊断完成")
    print("✓ 编码器配置和校准已完成")
    print("\n现在可以在 Qt 程序中:")
    print("  1. 连接到 ODrive")
    print("  2. 直接点击'进入闭环'")
    print("  3. 开始控制电机")
    print("\n如果仍有问题，请查看上面的错误信息")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断")
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
