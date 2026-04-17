# ODrive 完整状态信息和控制选项

## 当前程序已实现的状态信息（17项）

### 错误状态（6项）
1. ✅ **轴错误** (Axis Error) - 0x00000000
2. ✅ **轴错误详情** - 解码后的错误描述
3. ✅ **当前错误** (Active Errors / ODrive Error) - 0x00000000
4. ✅ **当前错误详情** - 解码后的ODrive错误描述
5. ✅ **失能原因** (Disarm Reason) - 0x00000000
6. ✅ **失能详情** - 解码后的失能原因

### 状态信息（3项）
7. ✅ **轴状态** (Axis State) - IDLE (1)
8. ✅ **流程结果** (Procedure Result) - SUCCESS (0)
9. ✅ **轨迹完成** (Trajectory Done) - 是/否

### 位置和速度（2项）
10. ✅ **位置** (Position) - 1.234 圈
11. ✅ **速度** (Velocity) - 0.000 圈/秒

### 电气参数（4项）
12. ✅ **母线电压** (Bus Voltage) - 12.00 V
13. ✅ **母线电流** (Bus Current) - 0.123 A
14. ✅ **Iq 设定/测量** - 0.000 / 0.000 A
15. ✅ **FET/电机温度** - 25.0 / 25.0 摄氏度

### 力矩和功率（2项）
16. ✅ **目标/估算力矩** - 0.000 / 0.000 Nm
17. ✅ **电功率/机械功率** - 0.00 / 0.00 W

### 时间戳（1项）
18. ✅ **最后更新时间** - 2026-04-16 10:35

---

## ODrive CAN协议支持的所有状态信息

### 可以添加的额外状态信息

#### 1. 编码器详细信息
- ❌ **编码器影子计数** (Encoder Shadow Count) - CMD 0x0A
- ❌ **编码器CPR计数** (Encoder Count in CPR) - CMD 0x0A
- ❌ **编码器错误** (Encoder Error) - CMD 0x04
- ❌ **无感估算位置** (Sensorless Pos Estimate) - CMD 0x15
- ❌ **无感估算速度** (Sensorless Vel Estimate) - CMD 0x15
- ❌ **无感错误** (Sensorless Error) - CMD 0x05

#### 2. 电机详细信息
- ❌ **电机错误** (Motor Error) - CMD 0x03
- ❌ **电机错误标志** (Motor Error Flag) - 来自心跳消息

#### 3. 控制器详细信息
- ❌ **控制器错误** (Controller Error) - CMD 0x1D
- ❌ **控制器错误标志** (Controller Error Flag) - 来自心跳消息
- ❌ **当前控制模式** (Control Mode) - 需要记录
- ❌ **当前输入模式** (Input Mode) - 需要记录

#### 4. 增益参数（只读）
- ❌ **位置增益** (Position Gain) - 需要通过USB读取
- ❌ **速度增益** (Velocity Gain) - 需要通过USB读取
- ❌ **速度积分增益** (Velocity Integrator Gain) - 需要通过USB读取

#### 5. 限制参数（当前设置）
- ❌ **当前速度限制** (Current Velocity Limit) - 需要记录
- ❌ **当前电流限制** (Current Current Limit) - 需要记录

#### 6. 轨迹参数
- ❌ **轨迹速度限制** (Traj Vel Limit) - 需要记录
- ❌ **轨迹加速限制** (Traj Accel Limit) - 需要记录
- ❌ **轨迹减速限制** (Traj Decel Limit) - 需要记录
- ❌ **轨迹惯性** (Traj Inertia) - 需要记录

#### 7. ADC电压
- ❌ **ADC电压** (ADC Voltage) - CMD 0x01C (需要指定GPIO引脚)

---

## 当前程序已实现的控制选项

### 轴状态控制（6项）
1. ✅ **进入空闲** (Idle) - 按钮
2. ✅ **电机校准** (Motor Calibration) - 按钮
3. ✅ **整轴校准** (Full Calibration) - 按钮
4. ✅ **进入闭环** (Closed Loop) - 按钮
5. ✅ **急停** (Estop) - 按钮
6. ✅ **清除错误** (Clear Errors) - 按钮

### 控制模式设置（2项）
7. ✅ **控制模式** (Control Mode) - 下拉框（位置/速度/力矩/电压）
8. ✅ **输入模式** (Input Mode) - 下拉框（透传/速度斜坡/位置滤波/梯形轨迹/力矩斜坡/未激活）

### 位置控制（4项）
9. ✅ **目标位置** - 输入框
10. ✅ **速度前馈** - 输入框
11. ✅ **力矩前馈** - 输入框
12. ✅ **发送位置命令** - 按钮

### 速度控制（3项）
13. ✅ **目标速度** - 输入框
14. ✅ **力矩前馈** - 输入框
15. ✅ **发送速度命令** - 按钮

### 力矩控制（2项）
16. ✅ **目标力矩** - 输入框
17. ✅ **发送力矩命令** - 按钮

### 运行限制（3项）
18. ✅ **速度限制** - 输入框
19. ✅ **电流限制** - 输入框
20. ✅ **应用限制** - 按钮

### 其他控制（1项）
21. ✅ **刷新状态** - 按钮

---

## ODrive CAN协议支持的所有控制选项

### 可以添加的额外控制选项

#### 1. 节点配置
- ❌ **设置节点ID** (Set Axis Node ID) - CMD 0x06

#### 2. 轨迹控制
- ❌ **设置轨迹速度限制** (Set Traj Vel Limit) - CMD 0x11
- ❌ **设置轨迹加速限制** (Set Traj Accel Limits) - CMD 0x12
- ❌ **设置轨迹惯性** (Set Traj Inertia) - CMD 0x13

#### 3. 增益调整
- ❌ **设置位置增益** (Set Position Gain) - CMD 0x1A
- ❌ **设置速度增益** (Set Vel Gains) - CMD 0x1B

#### 4. 编码器控制
- ❌ **设置线性计数** (Set Linear Count) - CMD 0x19

#### 5. 校准控制
- ❌ **启动反齿隙校准** (Start Anticogging) - CMD 0x010

#### 6. 系统控制
- ❌ **重启ODrive** (Reboot ODrive) - CMD 0x016

---

## 建议的UI改进方案

### 方案1：在现有状态监控区域添加更多信息

**新增状态项（优先级高）：**
1. **电机错误** - 显示详细的电机错误代码
2. **编码器错误** - 显示编码器错误代码
3. **控制器错误** - 显示控制器错误代码
4. **编码器计数** - 显示原始编码器计数
5. **当前控制模式** - 显示当前使用的控制模式
6. **当前输入模式** - 显示当前使用的输入模式
7. **当前速度限制** - 显示当前生效的速度限制
8. **当前电流限制** - 显示当前生效的电流限制

### 方案2：添加新的控制选项组

**轨迹控制组：**
- 轨迹速度限制输入框
- 轨迹加速限制输入框
- 轨迹减速限制输入框
- 轨迹惯性输入框
- 应用轨迹参数按钮

**增益调整组：**
- 位置增益输入框
- 速度增益输入框
- 速度积分增益输入框
- 应用增益按钮

**高级控制组：**
- 启动反齿隙校准按钮
- 重启ODrive按钮
- 设置编码器计数输入框

### 方案3：添加实时图表

**可视化显示：**
- 位置-时间曲线
- 速度-时间曲线
- 电流-时间曲线
- 温度-时间曲线
- 功率-时间曲线

---

## 实现优先级

### 高优先级（立即实现）
1. ✅ 电机错误显示
2. ✅ 编码器错误显示
3. ✅ 控制器错误显示
4. ✅ 当前控制模式显示
5. ✅ 当前输入模式显示
6. ✅ 编码器计数显示

### 中优先级（近期实现）
7. ⏳ 轨迹控制参数设置
8. ⏳ 增益参数调整
9. ⏳ 当前限制值显示

### 低优先级（未来实现）
10. 📋 实时图表显示
11. 📋 反齿隙校准
12. 📋 ADC电压监控
13. 📋 无感估算显示

---

## 技术实现说明

### 需要在 odrivemotorcontroller.h 中添加

```cpp
// 在 AxisStatus 结构体中添加
struct AxisStatus {
    // 现有字段...
    
    // 新增字段
    quint32 motorError = 0;           // 电机错误
    quint32 encoderError = 0;         // 编码器错误
    quint32 controllerError = 0;      // 控制器错误
    qint32 encoderShadowCount = 0;    // 编码器影子计数
    qint32 encoderCountInCPR = 0;     // 编码器CPR计数
    quint8 currentControlMode = 0;    // 当前控制模式
    quint8 currentInputMode = 0;      // 当前输入模式
    float currentVelocityLimit = 0.0f; // 当前速度限制
    float currentCurrentLimit = 0.0f;  // 当前电流限制
};

// 新增命令ID
enum CommandId {
    // 现有命令...
    GetMotorError = 0x03,
    GetEncoderError = 0x04,
    GetSensorlessError = 0x05,
    GetEncoderCount = 0x0A,
    SetTrajVelLimit = 0x11,
    SetTrajAccelLimits = 0x12,
    SetTrajInertia = 0x13,
    GetSensorlessEstimates = 0x15,
    RebootODrive = 0x16,
    GetADCVoltage = 0x01C,
    GetControllerError = 0x1D,
    SetLinearCount = 0x19,
    SetPositionGain = 0x1A,
    SetVelGains = 0x1B
};
```

### 需要在 mainwindow.cpp 中添加

```cpp
// 在状态监控区域添加新的标签
m_motorErrorValue = createValueLabel();
m_encoderErrorValue = createValueLabel();
m_controllerErrorValue = createValueLabel();
m_encoderCountValue = createValueLabel();
m_currentControlModeValue = createValueLabel();
m_currentInputModeValue = createValueLabel();
m_currentVelocityLimitValue = createValueLabel();
m_currentCurrentLimitValue = createValueLabel();
```

---

## 总结

当前程序已经实现了ODrive CAN协议中最核心的状态监控（17项）和控制功能（21项）。

**可以进一步添加的功能：**
- 6项额外的错误状态信息
- 8项编码器和控制器详细信息
- 4项轨迹控制参数
- 3项增益调整功能
- 多项高级控制选项

建议优先实现电机错误、编码器错误、控制器错误和编码器计数的显示，这些信息对于诊断问题非常重要。
