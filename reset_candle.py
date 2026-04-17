#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
重置 candleLight USB 设备以解决 Pipe error
"""

import usb.core
import usb.util
import sys
import time

def reset_candle_device():
    """查找并重置 candleLight 设备"""
    print("正在查找 candleLight 设备...")
    
    # candleLight 设备的 VID/PID
    devices = list(usb.core.find(find_all=True, idVendor=0x1d50, idProduct=0x606f))
    
    if not devices:
        print("❌ 未找到 candleLight 设备")
        return False
    
    print(f"✅ 找到 {len(devices)} 个 candleLight 设备")
    
    for i, dev in enumerate(devices):
        print(f"\n设备 {i}:")
        print(f"  Bus: {dev.bus}")
        print(f"  Address: {dev.address}")
        
        try:
            # 尝试重置设备
            print("  正在重置设备...")
            dev.reset()
            print("  ✅ 设备重置成功")
            time.sleep(1)  # 等待设备重新枚举
            
        except usb.core.USBError as e:
            print(f"  ⚠️ 重置失败: {e}")
            
            # 尝试分离内核驱动并重置
            try:
                if dev.is_kernel_driver_active(0):
                    print("  正在分离内核驱动...")
                    dev.detach_kernel_driver(0)
                    time.sleep(0.5)
                
                print("  再次尝试重置...")
                dev.reset()
                print("  ✅ 设备重置成功")
                time.sleep(1)
                
            except Exception as e2:
                print(f"  ❌ 重置失败: {e2}")
                continue
    
    print("\n✅ 重置完成！请尝试重新连接。")
    return True

if __name__ == "__main__":
    try:
        reset_candle_device()
    except Exception as e:
        print(f"❌ 发生错误: {e}")
        sys.exit(1)
