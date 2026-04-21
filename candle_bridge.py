import argparse
import json
import struct
import sys
import threading
import time
import re

import libusb_package
import usb.backend.libusb1
import usb.core
import usb.util


VID = 0x1D50
PID = 0x606F

GS_USB_BREQ_HOST_FORMAT = 0
GS_USB_BREQ_BITTIMING = 1
GS_USB_BREQ_MODE = 2

GS_CAN_MODE_RESET = 0
GS_CAN_MODE_START = 1

CAN_EFF_FLAG = 0x80000000
CAN_RTR_FLAG = 0x40000000
CAN_ERR_FLAG = 0x20000000
CAN_EFF_MASK = 0x1FFFFFFF
CAN_SFF_MASK = 0x000007FF

BACKEND = usb.backend.libusb1.get_backend(find_library=libusb_package.find_library)


def emit(event, **kwargs):
    payload = {"event": event}
    payload.update(kwargs)
    print(json.dumps(payload), flush=True)


BITRATES = {
    1000000: (1, 2, 1, 1, 16),
    500000: (1, 4, 1, 1, 16),
    250000: (1, 12, 2, 1, 12),
    125000: (1, 12, 2, 1, 24),
}


def list_candle_interfaces():
    matches = list(usb.core.find(idVendor=VID, idProduct=PID, backend=BACKEND, find_all=True) or [])
    return [f"candle{i}" for i in range(len(matches))]


class CandleDevice:
    def __init__(self, channel_name, bitrate):
        self.channel_name = channel_name
        self.bitrate = bitrate
        self.dev = None
        self.stop_event = threading.Event()
        self.reader = None
        self.out_ep = 0x02
        self.in_ep = 0x81

    def open(self):
        matches = list(usb.core.find(idVendor=VID, idProduct=PID, backend=BACKEND, find_all=True) or [])
        if not matches:
            raise RuntimeError(
                "candleLight/gs_usb adapter VID 1D50 PID 606F not found. "
                "If your USB-CAN module appears as a COM port, use AUTO/SLCAN instead. "
                "If it is a candleLight adapter on Windows, make sure WinUSB/libusb is installed."
            )

        channel_index = 0
        match = re.search(r"(\d+)$", self.channel_name or "")
        if match:
            channel_index = int(match.group(1))
        if channel_index < 0 or channel_index >= len(matches):
            raise RuntimeError(
                f"Requested {self.channel_name}, but only {len(matches)} candleLight adapter(s) were detected"
            )

        self.dev = matches[channel_index]

        # 改进的设备初始化流程
        try:
            # 先尝试分离内核驱动
            if self.dev.is_kernel_driver_active(0):
                try:
                    self.dev.detach_kernel_driver(0)
                    time.sleep(0.1)
                except (NotImplementedError, usb.core.USBError):
                    pass
        except (NotImplementedError, usb.core.USBError):
            pass

        # 重置设备
        try:
            self.dev.reset()
            time.sleep(0.5)  # 增加等待时间，确保设备完全重置
        except usb.core.USBError as e:
            # 如果重置失败，尝试继续
            emit("log", message=f"Device reset warning: {e}")

        # 设置配置
        max_retries = 3
        for attempt in range(max_retries):
            try:
                self.dev.set_configuration()
                time.sleep(0.1)
                break
            except usb.core.USBError as e:
                if attempt == max_retries - 1:
                    raise RuntimeError(f"Failed to set USB configuration after {max_retries} attempts: {e}")
                time.sleep(0.2)

        cfg = self.dev.get_active_configuration()
        intf = cfg[(0, 0)]

        # 释放可能存在的占用
        try:
            usb.util.release_interface(self.dev, intf.bInterfaceNumber)
            time.sleep(0.1)
        except usb.core.USBError:
            pass

        # 声明接口
        max_retries = 3
        for attempt in range(max_retries):
            try:
                usb.util.claim_interface(self.dev, intf.bInterfaceNumber)
                time.sleep(0.1)
                break
            except usb.core.USBError as e:
                if attempt == max_retries - 1:
                    raise RuntimeError(f"Failed to claim USB interface after {max_retries} attempts: {e}")
                time.sleep(0.2)

        endpoints = list(intf.endpoints())
        for endpoint in endpoints:
            if usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_IN:
                self.in_ep = endpoint.bEndpointAddress
            else:
                self.out_ep = endpoint.bEndpointAddress

        # 发送控制命令时增加重试机制
        max_retries = 3
        for attempt in range(max_retries):
            try:
                self.ctrl_out(GS_USB_BREQ_HOST_FORMAT, struct.pack("<I", 0x0000BEEF))
                break
            except usb.core.USBError as e:
                if attempt == max_retries - 1:
                    raise RuntimeError(f"Failed to set host format after {max_retries} attempts: {e}")
                time.sleep(0.3)
        
        for attempt in range(max_retries):
            try:
                self.ctrl_out(GS_USB_BREQ_MODE, struct.pack("<2I", GS_CAN_MODE_RESET, 0))
                break
            except usb.core.USBError as e:
                if attempt == max_retries - 1:
                    raise RuntimeError(f"Failed to reset CAN mode after {max_retries} attempts: {e}")
                time.sleep(0.3)

        timing = BITRATES.get(self.bitrate)
        if timing is None:
            raise RuntimeError(f"Unsupported bitrate {self.bitrate}")
        self.ctrl_out(GS_USB_BREQ_BITTIMING, struct.pack("<5I", *timing))
        self.ctrl_out(GS_USB_BREQ_MODE, struct.pack("<2I", GS_CAN_MODE_START, 0))

        self.reader = threading.Thread(target=self.read_loop, daemon=True)
        self.reader.start()

    def close(self):
        self.stop_event.set()
        if self.dev is None:
            return
        try:
            self.ctrl_out(GS_USB_BREQ_MODE, struct.pack("<2I", GS_CAN_MODE_RESET, 0))
        except Exception:
            pass
        try:
            usb.util.release_interface(self.dev, 0)
        except Exception:
            pass
        usb.util.dispose_resources(self.dev)
        self.dev = None

    def ctrl_out(self, request, data):
        # 增加超时时间并添加重试逻辑
        max_retries = 2
        for attempt in range(max_retries):
            try:
                return self.dev.ctrl_transfer(0x41, request, 0, 0, data, timeout=2000)
            except usb.core.USBError as e:
                if attempt == max_retries - 1:
                    raise
                time.sleep(0.2)

    def send(self, frame_id, data_hex, remote=False, extended=False):
        data = bytes.fromhex(data_hex) if data_hex else b""
        if extended:
            can_id = frame_id & CAN_EFF_MASK
            can_id |= CAN_EFF_FLAG
        else:
            can_id = frame_id & CAN_SFF_MASK
        if remote:
            can_id |= CAN_RTR_FLAG
        frame = struct.pack("<IIBBBB8s", 0xFFFFFFFF, can_id, len(data), 0, 0, 0, data.ljust(8, b"\x00"))
        self.dev.write(self.out_ep, frame, timeout=1000)

    def read_loop(self):
        while not self.stop_event.is_set():
            try:
                raw = bytes(self.dev.read(self.in_ep, 32, timeout=200))
            except usb.core.USBError:
                continue
            if len(raw) < 20:
                continue
            echo_id, can_id, can_dlc, channel, flags, reserved, data = struct.unpack("<IIBBBB8s", raw[:20])
            remote = bool(can_id & CAN_RTR_FLAG)
            frame_id = can_id & (CAN_EFF_MASK if (can_id & CAN_EFF_FLAG) else CAN_SFF_MASK)
            emit("rx",
                 id=frame_id,
                 dlc=int(can_dlc),
                 remote=remote,
                 channel=int(channel),
                 data=data[:can_dlc].hex())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel", default="candle0")
    parser.add_argument("--bitrate", type=int, default=250000)
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()

    if args.list:
        print(json.dumps({
            "interfaces": list_candle_interfaces(),
            "error": ""
        }), flush=True)
        return 0

    candle = CandleDevice(args.channel, args.bitrate)
    try:
        candle.open()
    except Exception as exc:
        import traceback
        emit("error", message=f"{type(exc).__name__}: {str(exc)}")
        emit("log", message=f"Traceback: {traceback.format_exc()}")
        return 1

    emit("ready", channel=args.channel, bitrate=args.bitrate)

    try:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError as exc:
                emit("error", message=f"Invalid JSON command: {exc}")
                continue

            cmd = obj.get("cmd")
            if cmd == "stop":
                break
            if cmd == "send":
                try:
                    candle.send(int(obj.get("id", 0)),
                                obj.get("data", ""),
                                bool(obj.get("remote", False)),
                                bool(obj.get("extended", False)))
                except Exception as exc:
                    emit("error", message=f"Send failed: {exc}")
            else:
                emit("log", message=f"Unknown command: {cmd}")
    except (BrokenPipeError, EOFError, OSError) as exc:
        # 正常的管道关闭，不报错
        pass

    candle.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
