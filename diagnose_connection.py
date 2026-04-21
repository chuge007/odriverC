#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ODrive连接诊断工具
详细检查ODrive连接问题
"""

import sys
import time

def check_odrive_import():
    """检查odrive库是否正确安装"""
    print("="*60)
    print("1. 检查odrive库")
    print("="*60)
    try:
        import odrive
        print(f"✓ odrive库已安装")
        print(f"  版本: {odrive.__version__ if hasattr(odrive, '__version__') else '未知'}")
        return True
    except ImportError as e:
        print(f"✗ odrive库未安装: {e}")
        print("\n请运行: pip install odrive")
        return False

def check_usb_devices():
    """检查USB设备"""
    print("\n" + "="*60)
    print("2. 检查USB设备")
    print("="*60)
    try:
        import usb.core
        devices = list(usb.core.find(find_all=True))
        print(f"找到 {len(devices)} 个USB设备")
        
        # 查找ODrive设备 (VID: 0x1209, PID: 0x0D32 或 0x0D33)
        odrive_devices = []
        for dev in devices:
            if dev.idVendor == 0x1209 and dev.idProduct in [0x0D32, 0x0D33]:
                odrive_devices.append(dev)
                print(f"\n✓ 找到ODrive设备:")
                print(f"  VID: 0x{dev.idVendor:04X}")
                print(f"  PID: 0x{dev.idProduct:04X}")
                print(f"  总线: {dev.bus}")
                print(f"  地址: {dev.address}")
        
        if not odrive_devices:
            print("\n✗ 未找到ODrive设备")
            print("\n可能的原因:")
            print("  1. ODrive未连接到USB")
            print("  2. 驱动程序未正确安装")
            print("  3. 设备处于DFU模式")
            return False
        
        return True
        
    except ImportError:
        print("✗ pyusb库未安装")
        print("\n请运行: pip install pyusb")
        return False
    except Exception as e:
        print(f"✗ 检查USB设备时出错: {e}")
        return False

def try_find_odrive():
    """尝试查找ODrive"""
    print("\n" + "="*60)
    print("3. 尝试连接ODrive")
    print("="*60)
    
    try:
        import odrive
        import odrive.enums
        
        print("正在搜索ODrive设备 (超时30秒)...")
        print("提示: 如果长时间无响应，请按Ctrl+C中断")
        
        odrv = odrive.find_any(timeout=30)
        
        if odrv is None:
            print("\n✗ 未找到ODrive设备")
            return None
        
        print("\n✓ 成功连接到ODrive!")
        
        # 读取基本信息
        try:
            print(f"\n设备信息:")
            print(f"  序列号: {hex(odrv.serial_number)}")
            print(f"  固件版本: {odrv.fw_version_major}.{odrv.fw_version_minor}.{odrv.fw_version_revision}")
            print(f"  硬件版本: v{odrv.hw_version_major}.{odrv.hw_version_minor}")
            print(f"  总线电压: {odrv.vbus_voltage:.2f}V")
        except Exception as e:
            print(f"  ⚠ 读取设备信息时出错: {e}")
        
        return odrv
        
    except Exception as e:
        print(f"\n✗ 连接失败: {e}")
        import traceback
        traceback.print_exc()
        return None

def check_axis_basic(odrv):
    """检查轴的基本状态"""
    print("\n" + "="*60)
    print("4. 检查轴状态")
    print("="*60)
    
    for i in range(2):
        print(f"\nAxis {i}:")
        try:
            axis = getattr(odrv, f'axis{i}')
            
            # 错误状态
            print(f"  错误代码:")
            print(f"    Axis: 0x{axis.error:X}")
            print(f"    Motor: 0x{axis.motor.error:X}")
            print(f"    Encoder: 0x{axis.encoder.error:X}")
            print(f"    Controller: 0x{axis.controller.error:X}")
            
            # 当前状态
            state_names = {
                0: "UNDEFINED", 1: "IDLE", 2: "STARTUP_SEQUENCE",
                3: "FULL_CALIBRATION_SEQUENCE", 4: "MOTOR_CALIBRATION",
                5: "ENCODER_INDEX_SEARCH", 6: "ENCODER_OFFSET_CALIBRATION",
                7: "CLOSED_LOOP_CONTROL", 8: "LOCKIN_SPIN",
                9: "ENCODER_DIR_FIND", 10: "HOMING"
            }
            state = axis.current_state
            state_name = state_names.get(state, f"UNKNOWN({state})")
            print(f"  当前状态: {state_name}")
            
            # 校准状态
            print(f"  校准状态:")
            print(f"    电机已校准: {axis.motor.is_calibrated}")
            print(f"    编码器就绪: {axis.encoder.is_ready}")
            
        except Exception as e:
            print(f"  ✗ 读取Axis{i}状态失败: {e}")

def main():
    """主函数"""
    print("\n" + "="*60)
    print("  ODrive连接诊断工具")
    print("="*60)
    
    # 1. 检查odrive库
    if not check_odrive_import():
        return 1
    
    # 2. 检查USB设备
    usb_ok = check_usb_devices()
    
    # 3. 尝试连接ODrive
    odrv = try_find_odrive()
    
    if odrv is None:
        print("\n" + "="*60)
        print("  诊断结果: 连接失败")
        print("="*60)
        print("\n请检查:")
        print("  1. ODrive是否通过USB连接到电脑")
        print("  2. USB线是否支持数据传输（不是仅充电线）")
        print("  3. 设备管理器中是否识别到设备")
        print("  4. 驱动程序是否正确安装")
        print("  5. 尝试更换USB端口")
        print("  6. 尝试重启ODrive（断电重连）")
        return 1
    
    # 4. 检查轴状态
    check_axis_basic(odrv)
    
    print("\n" + "="*60)
    print("  诊断结果: 连接成功")
    print("="*60)
    print("\n✓ ODrive板子已正常连接")
    print("\n你现在可以:")
    print("  1. 使用 odrivetool 进行交互式调试")
    print("  2. 运行 python check_odrive_status.py 进行完整检查")
    print("  3. 使用GUI程序进行控制")
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
