#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Qt程序控制参数辅助工具
用于读取和设置ODrive的控制/PID相关参数，供Qt程序调用
"""

import json
import sys

import odrive


def get_axis(node_id=0):
    odrv = odrive.find_any(timeout=3)
    return odrv.axis0 if int(node_id) == 0 else odrv.axis1


def axis_params(axis):
    return {
        "success": True,
        "pos_gain": float(axis.controller.config.pos_gain),
        "vel_gain": float(axis.controller.config.vel_gain),
        "vel_integrator_gain": float(axis.controller.config.vel_integrator_gain),
        "vel_limit": float(axis.controller.config.vel_limit),
        "vel_limit_tolerance": float(axis.controller.config.vel_limit_tolerance),
        "input_filter_bandwidth": float(axis.controller.config.input_filter_bandwidth),
        "inertia": float(axis.controller.config.inertia),
        "torque_ramp_rate": float(axis.controller.config.torque_ramp_rate),
        "enable_vel_limit": bool(axis.controller.config.enable_vel_limit),
        "enable_torque_mode_vel_limit": bool(axis.controller.config.enable_torque_mode_vel_limit),
        "current_limit": float(axis.motor.config.current_lim),
        "torque_limit": float(axis.motor.config.torque_lim),
        "traj_vel_limit": float(axis.trap_traj.config.vel_limit),
        "traj_accel_limit": float(axis.trap_traj.config.accel_limit),
        "traj_decel_limit": float(axis.trap_traj.config.decel_limit),
    }


def read_params(node_id=0):
    """读取完整控制参数"""
    try:
        axis = get_axis(node_id)
        print(json.dumps(axis_params(axis)))
        return 0
    except Exception as e:
        print(json.dumps({
            "success": False,
            "error": str(e)
        }))
        return 1


def set_params(node_id, params):
    """设置完整控制参数，仅对传入字段生效"""
    try:
        axis = get_axis(node_id)

        controller_map = {
            "pos_gain": "pos_gain",
            "vel_gain": "vel_gain",
            "vel_integrator_gain": "vel_integrator_gain",
            "vel_limit": "vel_limit",
            "vel_limit_tolerance": "vel_limit_tolerance",
            "input_filter_bandwidth": "input_filter_bandwidth",
            "inertia": "inertia",
            "torque_ramp_rate": "torque_ramp_rate",
            "enable_vel_limit": "enable_vel_limit",
            "enable_torque_mode_vel_limit": "enable_torque_mode_vel_limit",
        }

        for key, attr in controller_map.items():
            if key not in params:
                continue
            value = params[key]
            if key.startswith("enable_"):
                setattr(axis.controller.config, attr, bool(value))
            else:
                setattr(axis.controller.config, attr, float(value))

        if "current_limit" in params:
            axis.motor.config.current_lim = float(params["current_limit"])

        if "torque_limit" in params:
            axis.motor.config.torque_lim = float(params["torque_limit"])

        if "traj_vel_limit" in params:
            axis.trap_traj.config.vel_limit = float(params["traj_vel_limit"])

        if "traj_accel_limit" in params:
            axis.trap_traj.config.accel_limit = float(params["traj_accel_limit"])

        if "traj_decel_limit" in params:
            axis.trap_traj.config.decel_limit = float(params["traj_decel_limit"])

        print(json.dumps(axis_params(axis)))
        return 0
    except Exception as e:
        print(json.dumps({
            "success": False,
            "error": str(e)
        }))
        return 1


def set_pid_legacy(node_id, pos_gain, vel_gain, vel_integrator_gain):
    """兼容旧版仅设置3个PID参数的调用方式"""
    return set_params(node_id, {
        "pos_gain": pos_gain,
        "vel_gain": vel_gain,
        "vel_integrator_gain": vel_integrator_gain,
    })


def main():
    if len(sys.argv) < 2:
        print(json.dumps({
            "success": False,
            "error": "Usage: qt_pid_helper.py <read|set|set_json> [args]"
        }))
        return 1

    command = sys.argv[1]

    if command == "read":
        node_id = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        return read_params(node_id)

    if command == "set":
        if len(sys.argv) < 6:
            print(json.dumps({
                "success": False,
                "error": "Usage: qt_pid_helper.py set <node_id> <pos_gain> <vel_gain> <vel_integrator_gain>"
            }))
            return 1

        node_id = int(sys.argv[2])
        pos_gain = float(sys.argv[3])
        vel_gain = float(sys.argv[4])
        vel_integrator_gain = float(sys.argv[5])
        return set_pid_legacy(node_id, pos_gain, vel_gain, vel_integrator_gain)

    if command == "set_json":
        if len(sys.argv) < 4:
            print(json.dumps({
                "success": False,
                "error": "Usage: qt_pid_helper.py set_json <node_id> <json_payload>"
            }))
            return 1

        node_id = int(sys.argv[2])
        payload = json.loads(sys.argv[3])
        if not isinstance(payload, dict):
            print(json.dumps({
                "success": False,
                "error": "json_payload must be an object"
            }))
            return 1
        return set_params(node_id, payload)

    print(json.dumps({
        "success": False,
        "error": f"Unknown command: {command}"
    }))
    return 1


if __name__ == "__main__":
    sys.exit(main())
