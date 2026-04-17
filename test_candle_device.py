#!/usr/bin/env python3
"""测试 candleLight 设备访问"""

import libusb_package
import usb.backend.libusb1
import usb.core
import usb.util

VID = 0x1D50
PID = 0x606F

BACKEND = usb.backend.libusb1.get_backend(find_library=libusb_package.find_library)

print("正在查找 candleLight 设备...")
matches = list(usb.core.find(idVendor=VID, idProduct=PID, backend=BACKEND, find_all=True) or [])

if not matches:
    print("❌ 未找到 candleLight 设备 (VID:0x1D50 PID:0x606F)")
    print("请检查：")
    print("  1. USB 设备是否已连接")
    print("  2. 设备驱动是否正确安装")
    exit(1)

print(f"✅ 找到 {len(matches)} 个 candleLight 设备")

for i, dev in enumerate(matches):
    print(f"\n设备 {i}:")
    print(f"  Bus: {dev.bus}")
    print(f"  Address: {dev.address}")
    
    try:
        # 尝试设置配置
        dev.set_configuration()
        print("  ✅ 成功设置配置")
        
        # 尝试获取配置信息
        cfg = dev.get_active_configuration()
        print(f"  配置: {cfg.bConfigurationValue}")
        
        # 尝试获取接口
        intf = cfg[(0, 0)]
        print(f"  接口: {intf.bInterfaceNumber}")
        
        # 尝试分离内核驱动（如果需要）
        try:
            if dev.is_kernel_driver_active(0):
                print("  检测到内核驱动，尝试分离...")
                dev.detach_kernel_driver(0)
                print("  ✅ 成功分离内核驱动")
        except (NotImplementedError, usb.core.USBError) as e:
            print(f"  内核驱动操作: {e}")
        
        # 尝试声明接口
        usb.util.claim_interface(dev, intf.bInterfaceNumber)
        print("  ✅ 成功声明接口")
        
        # 释放接口
        usb.util.release_interface(dev, intf.bInterfaceNumber)
        print("  ✅ 成功释放接口")
        
        print(f"\n✅✅✅ 设备 {i} 完全可访问！驱动安装正确！")
        
    except usb.core.USBError as e:
        print(f"  ❌ USB 错误: {e}")
        print(f"  错误代码: {e.errno}")
        if e.errno == 13:
            print("  → 这是权限错误，请尝试：")
            print("     1. 以管理员身份运行此脚本")
            print("     2. 使用 Zadig 重新安装 WinUSB 驱动")
        elif e.errno == 16:
            print("  → 设备正在被其他程序使用")
            print("     请关闭其他可能占用设备的程序")
    except Exception as e:
        print(f"  ❌ 未知错误: {e}")

print("\n测试完成！")
