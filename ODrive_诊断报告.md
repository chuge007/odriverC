# ODrive 电机驱动问题诊断报告

## 问题描述
ODrive 设备无法正常连接，odrivetool 报错：`AssertionError` in `libusb0.py`

## 诊断结果

### 1. USB 设备检测
✅ **ODrive 设备已被系统识别**
- VID:PID = 0x1209:0x0d32 (标准 ODrive USB ID)
- 设备物理连接正常

### 2. 问题根源
❌ **USB 驱动配置问题**
- Windows 使用了错误的 USB 驱动（libusb0）
- ODrive 需要使用 WinUSB 或 libusb-win32 驱动
- 当前驱动无法正确获取设备配置

## 解决方案

### 方案 1：使用 Zadig 重新安装 USB 驱动（推荐）

1. **下载 Zadig 工具**
   - 访问：https://zadig.akeo.ie/
   - 下载最新版本

2. **安装正确的驱动**
   ```
   步骤：
   a. 运行 Zadig（以管理员身份）
   b. 点击 Options -> List All Devices
   c. 在下拉列表中找到 "ODrive 3.x" 或 "Bulk Interface"
   d. 在驱动选择框中选择 "WinUSB" 或 "libusb-win32"
   e. 点击 "Replace Driver" 或 "Install Driver"
   f. 等待安装完成
   ```

3. **重新连接设备**
   - 拔出 ODrive USB 线
   - 等待 5 秒
   - 重新插入
   - 运行 `odrivetool`

### 方案 2：使用串口模式（临时方案）

如果 USB 驱动问题难以解决，可以使用串口通信：

1. **检查可用串口**
   ```bash
   python -c "from serial.tools import list_ports; [print(p.device) for p in list_ports.comports()]"
   ```

2. **使用串口连接**
   ```bash
   odrivetool --serial-port COM3  # 替换为实际端口号
   ```

### 方案 3：检查 ODrive 配置

连接成功后，检查以下配置：

```python
# 在 odrivetool 中执行
odrv0.axis0.motorconfig.pole_pairs  # 检查极对数
odrv0.axis0.motor.config.resistance_calib_max_voltage  # 最大校准电压
odrv0.axis0.motor.config.current_lim  # 电流限制
odrv0.axis0.encoder.config.cpr  # 编码器 CPR

# 常见配置问题
# 1. 极对数设置错误
# 2. 编码器 CPR 不匹配
# 3. 电流限制过低
# 4. 校准未完成
```

## 常见电机配置参数

### 典型配置示例（根据你的电机调整）

```python
# 电机配置
odrv0.axis0.motor.config.pole_pairs = 7  # 根据电机规格
odrv0.axis0.motor.config.resistance_calib_max_voltage = 4.0
odrv0.axis0.motor.config.requested_current_range = 25.0
odrv0.axis0.motor.config.current_lim = 10.0  # 根据电机额定电流

# 编码器配置
odrv0.axis0.encoder.config.cpr = 8192  # 根据编码器规格
odrv0.axis0.encoder.config.mode = ENCODER_MODE_INCREMENTAL

# 控制器配置
odrv0.axis0.controller.config.vel_limit = 2.0  # 速度限制（转/秒）
odrv0.axis0.controller.config.control_mode = CONTROL_MODE_VELOCITY_CONTROL

# 保存配置
odrv0.save_configuration()
odrv0.reboot()
```

## 校准步骤

```python
# 1. 电机校准
odrv0.axis0.requested_state = AXIS_STATE_MOTOR_CALIBRATION
# 等待完成，检查错误
odrv0.axis0.error

# 2. 编码器校准
odrv0.axis0.requested_state = AXIS_STATE_ENCODER_INDEX_SEARCH
# 或全校准
odrv0.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE

# 3. 进入闭环控制
odrv0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL

# 4. 测试
odrv0.axis0.controller.input_vel = 1.0  # 1 转/秒
```

## 错误代码参考

如果出现错误，检查：
```python
hex(odrv0.axis0.error)  # 轴错误
hex(odrv0.axis0.motor.error)  # 电机错误
hex(odrv0.axis0.encoder.error)  # 编码器错误
```

常见错误：
- `0x001`: 初始化失败
- `0x0002`: 系统级别错误
- `0x004`: 时序错误
- `0x008`: 缺少估计值
- `0x0040`: 电机失败
- `0x0080`: 编码器失败

## 下一步操作

1. ✅ 首先使用 Zadig 修复 USB 驱动
2. ✅ 重新连接并运行 `odrivetool`
3. ✅ 检查电机和编码器配置
4. ✅ 执行校准序列
5. ✅ 测试闭环控制

## 参考资源

- ODrive 官方文档：https://docs.odriverobotics.com/
- MKS ODrive GitHub：https://github.com/makerbase-motor/ODrive-MKS
- 技术支持 Q 群：732557609
