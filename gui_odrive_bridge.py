#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GUI 专用 ODrive USB 桥接脚本

提供以下非交互命令：
1. 读取 PID 参数
2. 设置 PID 参数
3. 诊断并尝试修复 AXIS_ERROR_INVALID_STATE (0x00000001)
4. 整机初始化（标准配置 + 校准 + 闭环测试 + 保存）

输出设计目标：
- 普通日志直接 print，供 Qt 日志面板与控制台显示
- 机器可解析结果使用 RESULT key=value 形式
"""

import argparse
import sys
import time

import odrive
from odrive.enums import *


STANDARD_CONFIG = {
    "pole_pairs": 7,
    "calibration_current": 10.0,
    "current_lim": 10.0,
    "current_lim_margin": 8.0,
    "cpr": 8192,
    "encoder_mode": ENCODER_MODE_INCREMENTAL,
    "vel_limit": 10.0,
    "vel_gain": 0.02,
    "vel_integrator_gain": 0.1,
    "pos_gain": 8.0,
    "control_mode": CONTROL_MODE_POSITION_CONTROL,
    "input_mode": INPUT_MODE_PASSTHROUGH,
}


def log(message):
    print(message, flush=True)


def connect_odrive():
    log("正在连接 ODrive...")
    odrv = odrive.find_any()
    log(f"✅ 已连接 ODrive: serial=0x{odrv.serial_number:X}")
    log(f"母线电压: {odrv.vbus_voltage:.2f} V")
    return odrv


def get_axis(odrv, axis_num):
    return getattr(odrv, f"axis{axis_num}")


def emit_pid_result(axis):
    print(f"RESULT pos_gain={axis.controller.config.pos_gain}", flush=True)
    print(f"RESULT vel_gain={axis.controller.config.vel_gain}", flush=True)
    print(
        f"RESULT vel_integrator_gain={axis.controller.config.vel_integrator_gain}",
        flush=True,
    )


def cmd_read_pid(args):
    odrv = connect_odrive()
    axis = get_axis(odrv, args.axis)

    log(f"读取 axis{args.axis} PID 参数...")
    pos_gain = axis.controller.config.pos_gain
    vel_gain = axis.controller.config.vel_gain
    vel_integrator_gain = axis.controller.config.vel_integrator_gain

    log(f"pos_gain = {pos_gain}")
    log(f"vel_gain = {vel_gain}")
    log(f"vel_integrator_gain = {vel_integrator_gain}")

    emit_pid_result(axis)
    return 0


def cmd_set_pid(args):
    odrv = connect_odrive()
    axis = get_axis(odrv, args.axis)

    log(f"设置 axis{args.axis} PID 参数...")
    axis.controller.config.pos_gain = args.pos_gain
    axis.controller.config.vel_gain = args.vel_gain
    axis.controller.config.vel_integrator_gain = args.vel_integrator_gain

    log(f"✅ pos_gain = {args.pos_gain}")
    log(f"✅ vel_gain = {args.vel_gain}")
    log(f"✅ vel_integrator_gain = {args.vel_integrator_gain}")

    if args.save:
        log("保存配置到 flash...")
        odrv.save_configuration()
        log("✅ 配置已保存")
    else:
        log("⚠️ 未保存到 flash，仅当前运行时生效")

    emit_pid_result(axis)
    return 0


def wait_until_idle(axis, timeout=60.0):
    start = time.time()
    while axis.current_state != AXIS_STATE_IDLE:
        if time.time() - start > timeout:
            return False
        time.sleep(0.5)
        log(f"当前状态: {axis.current_state}")
    return True


def cmd_diagnose_fix(args):
    odrv = connect_odrive()
    axis = get_axis(odrv, args.axis)

    log("=" * 60)
    log(f"诊断 axis{args.axis} 错误")
    log("=" * 60)

    log(f"轴错误: 0x{axis.error:08X}")
    log(f"电机错误: 0x{axis.motor.error:08X}")
    log(f"编码器错误: 0x{axis.encoder.error:08X}")
    log(f"控制器错误: 0x{axis.controller.error:08X}")

    log("")
    log("错误 0x00000001 = AXIS_ERROR_INVALID_STATE")
    log("这意味着轴无法进入请求的状态")

    log("")
    log("检查校准状态:")
    log(f"电机已校准: {axis.motor.is_calibrated}")
    log(f"编码器已准备: {axis.encoder.is_ready}")
    log(f"编码器索引已找到: {axis.encoder.index_found}")
    log(f"相电阻: {axis.motor.config.phase_resistance:.6f} Ω")
    log(f"相电感: {axis.motor.config.phase_inductance:.9f} H")

    log("")
    log("检查配置:")
    log(f"电机类型: {axis.motor.config.motor_type}")
    log(f"编码器模式: {axis.encoder.config.mode}")
    log(f"编码器CPR: {axis.encoder.config.cpr}")
    log(f"极对数: {axis.motor.config.pole_pairs}")

    tried_fix = False

    if not axis.motor.is_calibrated or axis.motor.config.phase_resistance < 0.001:
        tried_fix = True
        log("")
        log("检测到电机未校准，开始执行完整校准...")
        log("⚠️ 请确保电机可以自由转动！")
        time.sleep(1.0)

        axis.clear_errors()
        time.sleep(0.3)
        axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
        log("校准中，请等待...")

        if not wait_until_idle(axis, timeout=90.0):
            log("❌ 校准超时")
            return 1

        log("校准完成")
        log(f"校准后轴错误: 0x{axis.error:08X}")

        if axis.error == 0 and axis.motor.is_calibrated:
            log("✅ 校准成功")
            if args.save:
                log("保存配置到 flash...")
                odrv.save_configuration()
                log("✅ 配置已保存")
        else:
            log("❌ 校准失败")
            return 1
    else:
        log("")
        log("电机已校准，尝试清除错误...")
        tried_fix = True
        axis.clear_errors()
        time.sleep(0.5)
        log(f"清错后轴错误: 0x{axis.error:08X}")

    log("")
    log("尝试进入闭环控制...")
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(1.5)
    log(f"当前状态: {axis.current_state}")
    log(f"当前轴错误: 0x{axis.error:08X}")

    if axis.current_state == AXIS_STATE_CLOSED_LOOP_CONTROL and axis.error == 0:
        log("✅ 诊断修复成功，已进入闭环控制")
        print("RESULT diagnose_fix=success", flush=True)
        return 0

    if tried_fix:
        log("⚠️ 已执行自动修复，但仍未成功进入闭环")
    else:
        log("⚠️ 未执行任何修复动作")

    print("RESULT diagnose_fix=failed", flush=True)
    return 1


def configure_axis(axis, axis_num):
    log("")
    log("=" * 60)
    log(f"配置节点 {axis_num}")
    log("=" * 60)

    axis.clear_errors()
    time.sleep(0.3)

    log("步骤1: 配置电机参数...")
    axis.motor.config.pole_pairs = STANDARD_CONFIG["pole_pairs"]
    axis.motor.config.calibration_current = STANDARD_CONFIG["calibration_current"]
    axis.motor.config.current_lim = STANDARD_CONFIG["current_lim"]
    axis.motor.config.current_lim_margin = STANDARD_CONFIG["current_lim_margin"]
    axis.motor.config.motor_type = MOTOR_TYPE_HIGH_CURRENT
    axis.motor.config.resistance_calib_max_voltage = 2.0

    log("步骤2: 配置编码器...")
    axis.encoder.config.cpr = STANDARD_CONFIG["cpr"]
    axis.encoder.config.mode = STANDARD_CONFIG["encoder_mode"]
    axis.encoder.config.bandwidth = 1000

    log("步骤3: 配置控制器...")
    axis.controller.config.vel_limit = STANDARD_CONFIG["vel_limit"]
    axis.controller.config.vel_gain = STANDARD_CONFIG["vel_gain"]
    axis.controller.config.vel_integrator_gain = STANDARD_CONFIG["vel_integrator_gain"]
    axis.controller.config.pos_gain = STANDARD_CONFIG["pos_gain"]
    axis.controller.config.control_mode = STANDARD_CONFIG["control_mode"]
    axis.controller.config.input_mode = STANDARD_CONFIG["input_mode"]

    log("步骤4: 配置启动行为...")
    axis.config.startup_motor_calibration = False
    axis.config.startup_encoder_index_search = False
    axis.config.startup_encoder_offset_calibration = False
    axis.config.startup_closed_loop_control = False
    axis.config.startup_homing = False

    log("步骤5: 配置制动电阻...")
    axis.config.enable_brake_resistor = False

    log(
        f"✅ 节点 {axis_num} 参数配置完成: "
        f"pos_gain={STANDARD_CONFIG['pos_gain']}, "
        f"vel_gain={STANDARD_CONFIG['vel_gain']}, "
        f"vel_integrator_gain={STANDARD_CONFIG['vel_integrator_gain']}"
    )
    return True


def calibrate_axis(axis, axis_num):
    log("")
    log("=" * 60)
    log(f"校准节点 {axis_num}")
    log("=" * 60)

    axis.clear_errors()
    time.sleep(0.3)
    axis.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
    log("开始完整校准序列...")

    start_time = time.time()
    while axis.current_state != AXIS_STATE_IDLE:
        if time.time() - start_time > 90:
            log("❌ 校准超时")
            return False
        if axis.error != 0:
            log(f"❌ 校准失败，轴错误: 0x{axis.error:08X}")
            log(f"电机错误: 0x{axis.motor.error:08X}")
            log(f"编码器错误: 0x{axis.encoder.error:08X}")
            return False
        time.sleep(0.5)
        log(f"校准状态: {axis.current_state}")

    if axis.motor.is_calibrated and axis.encoder.is_ready:
        log("✅ 校准成功")
        return True

    log("❌ 校准未完成")
    log(f"电机已校准: {axis.motor.is_calibrated}")
    log(f"编码器已就绪: {axis.encoder.is_ready}")
    return False


def test_closed_loop(axis, axis_num):
    log("")
    log("=" * 60)
    log(f"测试节点 {axis_num} 闭环控制")
    log("=" * 60)

    axis.clear_errors()
    time.sleep(0.3)
    axis.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(2.0)

    if axis.current_state != AXIS_STATE_CLOSED_LOOP_CONTROL:
        log("❌ 未能进入闭环控制")
        log(f"当前状态: {axis.current_state}")
        log(f"轴错误: 0x{axis.error:08X}")
        return False

    current_pos = axis.encoder.pos_estimate
    axis.controller.input_pos = current_pos + 0.2
    time.sleep(1.5)
    new_pos = axis.encoder.pos_estimate
    log(f"测试前位置: {current_pos:.3f}")
    log(f"测试后位置: {new_pos:.3f}")

    axis.controller.input_pos = current_pos
    time.sleep(1.0)

    if abs(new_pos - (current_pos + 0.2)) < 0.2:
        log("✅ 闭环测试成功")
        return True

    log("⚠️ 闭环已进入，但位置响应未达到预期")
    return True


def cmd_initialize_machine(args):
    odrv = connect_odrive()

    if odrv.vbus_voltage < 11.0:
        log(f"⚠️ 母线电压较低: {odrv.vbus_voltage:.2f} V")

    axes = [0, 1] if args.all_axes else [args.axis]
    results = {}

    for axis_num in axes:
        axis = get_axis(odrv, axis_num)
        try:
            configure_axis(axis, axis_num)
            if not calibrate_axis(axis, axis_num):
                results[axis_num] = False
                continue
            if not test_closed_loop(axis, axis_num):
                results[axis_num] = False
                continue
            results[axis_num] = True
        except Exception as exc:
            log(f"❌ 节点 {axis_num} 初始化失败: {exc}")
            results[axis_num] = False

    if any(results.values()) and args.save:
        log("")
        log("保存配置到 ODrive...")
        odrv.save_configuration()
        log("✅ 配置已保存")

    log("")
    log("=" * 60)
    log("整机初始化结果")
    log("=" * 60)
    for axis_num in axes:
        ok = results.get(axis_num, False)
        log(f"节点 {axis_num}: {'✅ 成功' if ok else '❌ 失败'}")

    all_ok = all(results.get(axis_num, False) for axis_num in axes)
    print(f"RESULT initialize_machine={'success' if all_ok else 'failed'}", flush=True)
    return 0 if all_ok else 1


def build_parser():
    parser = argparse.ArgumentParser(description="GUI ODrive bridge")
    subparsers = parser.add_subparsers(dest="command", required=True)

    parser_read_pid = subparsers.add_parser("read_pid")
    parser_read_pid.add_argument("--axis", type=int, default=0)

    parser_set_pid = subparsers.add_parser("set_pid")
    parser_set_pid.add_argument("--axis", type=int, default=0)
    parser_set_pid.add_argument("--pos-gain", type=float, required=True)
    parser_set_pid.add_argument("--vel-gain", type=float, required=True)
    parser_set_pid.add_argument("--vel-integrator-gain", type=float, required=True)
    parser_set_pid.add_argument("--save", action="store_true")

    parser_fix = subparsers.add_parser("diagnose_fix")
    parser_fix.add_argument("--axis", type=int, default=0)
    parser_fix.add_argument("--save", action="store_true")

    parser_init = subparsers.add_parser("initialize_machine")
    parser_init.add_argument("--axis", type=int, default=0)
    parser_init.add_argument("--all-axes", action="store_true")
    parser_init.add_argument("--save", action="store_true")

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    try:
        if args.command == "read_pid":
            return cmd_read_pid(args)
        if args.command == "set_pid":
            return cmd_set_pid(args)
        if args.command == "diagnose_fix":
            return cmd_diagnose_fix(args)
        if args.command == "initialize_machine":
            return cmd_initialize_machine(args)
        parser.error("unknown command")
        return 1
    except Exception as exc:
        log(f"❌ 执行失败: {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
