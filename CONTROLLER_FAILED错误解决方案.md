# CONTROLLER_FAILED 错误解决方案

## 错误含义

当电机进入闭环时出现 **CONTROLLER_FAILED** 错误（错误代码 0x00000200），表示 ODrive 的控制器模块检测到异常情况。

## 常见原因

### 1. **控制器参数配置不当**
- 位置/速度/电流增益（Kp, Ki, Kd）设置不合理
- 控制器输出超出限制范围
- 控制器计算出现数值异常（NaN, Inf）

### 2. **编码器反馈问题**
- 编码器信号质量差
- 编码器CPR配置错误
- 编码器偏移未正确校准

### 3. **电机参数不匹配**
- 电机极对数配置错误
- 电流限制设置过高或过低
- 速度限制设置不合理

### 4. **电源问题**
- 母线电压不稳定
- 电流供应不足
- 电压超出ODrive工作范围（8-56V）

### 5. **控制模式切换问题**
- 从IDLE直接切换到闭环，但控制器未准备好
- 控制模式和输入模式不匹配

## 诊断步骤

### 1. 检查错误详情
在高级页面的状态监控区域查看：
- **轴错误详情**：查看具体的错误标志
- **当前错误详情**：查看ODrive系统错误
- **母线电压/电流**：确认电源状态
- **编码器位置/速度**：确认编码器工作正常

### 2. 检查控制器配置
```python
# 通过 odrivetool 检查
odrv0.axis0.controller.config.pos_gain  # 位置增益
odrv0.axis0.controller.config.vel_gain  # 速度增益
odrv0.axis0.controller.config.vel_integrator_gain  # 速度积分增益
```

### 3. 检查限制设置
```python
odrv0.axis0.controller.config.vel_limit  # 速度限制
odrv0.axis0.motor.config.current_lim  # 电流限制
```

## 解决方案

### 方案1：使用程序内置的诊断修复功能

在高级页面点击 **"诊断修复"** 按钮，程序会自动：
1. 清除所有错误
2. 设置安全的限制参数（速度=10圈/秒，电流=10A）
3. 尝试进入闭环状态
4. 刷新状态显示

### 方案2：手动调整控制器参数

#### 降低控制器增益
```python
# 保守的控制器参数
odrv0.axis0.controller.config.pos_gain = 20.0  # 位置增益
odrv0.axis0.controller.config.vel_gain = 0.16  # 速度增益
odrv0.axis0.controller.config.vel_integrator_gain = 0.32  # 速度积分增益
```

#### 设置合理的限制
```python
# 安全的限制值
odrv0.axis0.controller.config.vel_limit = 10.0  # 10圈/秒
odrv0.axis0.motor.config.current_lim = 10.0  # 10A
odrv0.axis0.motor.config.current_lim_margin = 8.0  # 8A
```

### 方案3：重新校准编码器

如果是编码器问题导致的：
```python
# 1. 清除错误
odrv0.clear_errors()

# 2. 重新进行编码器偏移校准
odrv0.axis0.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION

# 3. 等待校准完成后检查结果
odrv0.axis0.encoder.config.pre_calibrated = True
odrv0.save_configuration()
```

### 方案4：检查电源

确保：
- 母线电压在 12-48V 范围内（推荐24V）
- 电源能提供足够的电流（至少 10A per axis）
- 电源线连接牢固，无松动

### 方案5：渐进式进入闭环

不要直接从 IDLE 进入闭环，而是：
```python
# 1. 清除错误
odrv0.clear_errors()

# 2. 先设置低限制
odrv0.axis0.controller.config.vel_limit = 2.0
odrv0.axis0.motor.config.current_lim = 5.0

# 3. 进入闭环
odrv0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL

# 4. 等待稳定后再逐步提高限制
```

## 在程序中的操作步骤

### 使用高级页面

1. **打开高级页面**
   - 点击主界面的"高级"按钮

2. **选择问题节点**
   - 在"调试板子"下拉框选择板子
   - 在"调试节点"下拉框选择节点0

3. **查看状态监控**
   - 右侧"状态监控（完整信息）"区域会显示所有错误详情
   - 重点查看：
     - 轴错误详情
     - 当前错误详情
     - 母线电压/电流
     - 编码器位置/速度

4. **尝试修复**
   - 点击"清除错误"按钮
   - 点击"诊断修复"按钮（自动修复）
   - 或手动调整"运行限制"中的参数
   - 点击"进入闭环"按钮

5. **观察结果**
   - 查看状态监控区域的实时更新
   - 查看系统日志中的详细信息

## 预防措施

1. **首次使用新电机**
   - 先进行完整校准（电机校准 + 编码器校准）
   - 使用保守的控制器参数
   - 逐步提高性能参数

2. **定期检查**
   - 检查编码器连接
   - 检查电源电压稳定性
   - 检查电机温度

3. **使用安全限制**
   - 速度限制不要超过电机额定转速
   - 电流限制不要超过电机额定电流
   - 留有安全余量（80%额定值）

## 相关错误代码

- `0x00000001` - INVALID_STATE：状态转换无效
- `0x00000040` - MOTOR_FAILED：电机故障
- `0x00000100` - ENCODER_FAILED：编码器故障
- `0x00000200` - **CONTROLLER_FAILED**：控制器故障
- `0x00000800` - WATCHDOG_TIMER_EXPIRED：看门狗超时

## 总结

CONTROLLER_FAILED 通常是由于控制器参数配置不当或反馈信号异常导致的。通过：
1. 使用程序的"诊断修复"功能快速修复
2. 或手动调整控制器参数和限制
3. 确保编码器和电源正常工作

大多数情况下都能解决问题。如果问题持续存在，可能需要重新进行完整的电机和编码器校准。
