# ODrive UI 显示功能改进方案

## 当前UI状态分析

### 已有的状态显示（18项）
根据 mainwindow.h 中的成员变量，当前UI已经显示：

1. **m_axisErrorValue** - 轴错误代码
2. **m_axisErrorDetailValue** - 轴错误详情
3. **m_activeErrorsValue** - 当前错误代码
4. **m_activeErrorsDetailValue** - 当前错误详情
5. **m_disarmReasonValue** - 失能原因代码
6. **m_disarmReasonDetailValue** - 失能原因详情
7. **m_axisStateValue** - 轴状态
8. **m_procedureResultValue** - 流程结果
9. **m_trajectoryDoneValue** - 轨迹完成状态
10. **m_positionValue** - 位置
11. **m_velocityValue** - 速度
12. **m_busVoltageValue** - 母线电压
13. **m_busCurrentValue** - 母线电流
14. **m_iqValue** - Iq 电流
15. **m_temperatureValue** - 温度
16. **m_torqueValue** - 力矩
17. **m_powerValue** - 功率
18. **m_lastUpdateValue** - 最后更新时间

---

## 可以添加的UI显示功能

### 优先级1：高优先级错误诊断信息（必须添加）

这些信息对于诊断 CONTROLLER_FAILED 等问题至关重要：

#### 1. 电机错误显示
```cpp
QLabel *m_motorErrorValue;           // 电机错误代码
QLabel *m_motorErrorDetailValue;     // 电机错误详情
```
**显示内容**：
- 错误代码：0x00000000
- 错误详情：解码后的电机错误描述

**用途**：诊断电机硬件问题、相电阻/电感测量失败等

#### 2. 编码器错误显示
```cpp
QLabel *m_encoderErrorValue;         // 编码器错误代码
QLabel *m_encoderErrorDetailValue;   // 编码器错误详情
```
**显示内容**：
- 错误代码：0x00000000
- 错误详情：解码后的编码器错误描述

**用途**：诊断编码器连接、CPR配置、校准问题

#### 3. 控制器错误显示
```cpp
QLabel *m_controllerErrorValue;      // 控制器错误代码
QLabel *m_controllerErrorDetailValue; // 控制器错误详情
```
**显示内容**：
- 错误代码：0x00000000
- 错误详情：解码后的控制器错误描述

**用途**：诊断 CONTROLLER_FAILED、超速、超限等问题

#### 4. 编码器计数显示
```cpp
QLabel *m_encoderShadowCountValue;   // 编码器影子计数
QLabel *m_encoderCPRCountValue;      // 编码器CPR计数
```
**显示内容**：
- 影子计数：12345
- CPR计数：6789

**用途**：实时监控编码器原始数据，验证编码器工作状态

---

### 优先级2：当前运行参数显示（重要）

显示当前生效的控制参数，便于确认设置是否正确：

#### 5. 当前控制模式显示
```cpp
QLabel *m_currentControlModeValue;   // 当前控制模式
QLabel *m_currentInputModeValue;     // 当前输入模式
```
**显示内容**：
- 控制模式：位置控制 (3)
- 输入模式：梯形轨迹 (5)

**用途**：确认当前使用的控制模式，避免模式混淆

#### 6. 当前限制值显示
```cpp
QLabel *m_currentVelocityLimitValue; // 当前速度限制
QLabel *m_currentCurrentLimitValue;  // 当前电流限制
```
**显示内容**：
- 速度限制：10.0 圈/秒
- 电流限制：20.0 A

**用途**：确认当前生效的限制值，诊断速度/电流限制问题

---

### 优先级3：高级控制参数显示（可选）

#### 7. 轨迹参数显示
```cpp
QLabel *m_trajVelLimitValue;         // 轨迹速度限制
QLabel *m_trajAccelLimitValue;       // 轨迹加速限制
QLabel *m_trajDecelLimitValue;       // 轨迹减速限制
QLabel *m_trajInertiaValue;          // 轨迹惯性
```
**显示内容**：
- 轨迹速度限制：5.0 圈/秒
- 轨迹加速限制：2.0 圈/秒²
- 轨迹减速限制：2.0 圈/秒²
- 轨迹惯性：0.0

**用途**：监控梯形轨迹模式的参数设置

#### 8. 增益参数显示
```cpp
QLabel *m_posGainValue;              // 位置增益
QLabel *m_velGainValue;              // 速度增益
QLabel *m_velIntegratorGainValue;    // 速度积分增益
```
**显示内容**：
- 位置增益：20.0
- 速度增益：0.16
- 速度积分增益：0.32

**用途**：监控PID参数，调试控制性能

#### 9. 无感估算显示
```cpp
QLabel *m_sensorlessPosValue;        // 无感位置估算
QLabel *m_sensorlessVelValue;        // 无感速度估算
QLabel *m_sensorlessErrorValue;      // 无感错误
```
**显示内容**：
- 无感位置：1.234 圈
- 无感速度：0.000 圈/秒
- 无感错误：0x00000000

**用途**：对比有感和无感数据，诊断无感控制问题

---

### 优先级4：高级控制功能（可选）

在控制面板中添加新的控制按钮和输入框：

#### 10. 轨迹控制组
```cpp
// UI元素
QDoubleSpinBox *m_trajVelLimitSpin;      // 轨迹速度限制输入
QDoubleSpinBox *m_trajAccelLimitSpin;    // 轨迹加速限制输入
QDoubleSpinBox *m_trajDecelLimitSpin;    // 轨迹减速限制输入
QDoubleSpinBox *m_trajInertiaSpin;       // 轨迹惯性输入
QPushButton *m_applyTrajParamsButton;    // 应用轨迹参数按钮

// 槽函数
void applyTrajParams();
```

**功能**：设置梯形轨迹模式的运动参数

#### 11. 增益调整组
```cpp
// UI元素
QDoubleSpinBox *m_posGainSpin;           // 位置增益输入
QDoubleSpinBox *m_velGainSpin;           // 速度增益输入
QDoubleSpinBox *m_velIntegratorGainSpin; // 速度积分增益输入
QPushButton *m_applyGainsButton;         // 应用增益按钮

// 槽函数
void applyGains();
```

**功能**：在线调整PID参数

#### 12. 高级控制按钮
```cpp
// UI元素
QPushButton *m_startAnticoggingButton;   // 启动反齿隙校准
QPushButton *m_rebootODriveButton;       // 重启ODrive
QSpinBox *m_linearCountSpin;             // 编码器计数输入
QPushButton *m_setLinearCountButton;     // 设置编码器计数

// 槽函数
void startAnticogging();
void rebootODrive();
void setLinearCount();
```

**功能**：高级校准和系统控制

---

### 优先级5：实时图表显示（未来扩展）

使用 QCustomPlot 或 QChart 添加实时曲线：

#### 13. 位置-时间曲线
```cpp
QCustomPlot *m_positionPlot;
```
**显示内容**：实时位置变化曲线

#### 14. 速度-时间曲线
```cpp
QCustomPlot *m_velocityPlot;
```
**显示内容**：实时速度变化曲线

#### 15. 电流-时间曲线
```cpp
QCustomPlot *m_currentPlot;
```
**显示内容**：实时电流变化曲线

#### 16. 温度-时间曲线
```cpp
QCustomPlot *m_temperaturePlot;
```
**显示内容**：FET和电机温度变化曲线

#### 17. 功率-时间曲线
```cpp
QCustomPlot *m_powerPlot;
```
**显示内容**：电功率和机械功率变化曲线

---

## 实现建议

### 阶段1：添加高优先级错误诊断信息（立即实现）

**修改文件**：
- mainwindow.h - 添加新的 QLabel 成员变量
- mainwindow.cpp - 在 buildUi() 中创建标签
- mainwindow.cpp - 在 updateStatusPanel() 中更新显示

**代码示例**：

```cpp
// mainwindow.h 中添加
QLabel *m_motorErrorValue;
QLabel *m_motorErrorDetailValue;
QLabel *m_encoderErrorValue;
QLabel *m_encoderErrorDetailValue;
QLabel *m_controllerErrorValue;
QLabel *m_controllerErrorDetailValue;
QLabel *m_encoderShadowCountValue;
QLabel *m_encoderCPRCountValue;

// mainwindow.cpp 的 buildUi() 中添加
m_motorErrorValue = createValueLabel();
m_motorErrorDetailValue = createValueLabel();
// ... 其他标签

// mainwindow.cpp 的 updateStatusPanel() 中添加
const ODriveMotorController::AxisStatus &s = m_controller->status(currentDebugNodeId());

m_motorErrorValue->setText(QString("0x%1").arg(s.motorError, 8, 16, QLatin1Char('0')).toUpper());
m_motorErrorDetailValue->setText(decodeMotorError(s.motorError));

m_encoderErrorValue->setText(QString("0x%1").arg(s.encoderError, 8, 16, QLatin1Char('0')).toUpper());
m_encoderErrorDetailValue->setText(decodeEncoderError(s.encoderError));

m_controllerErrorValue->setText(QString("0x%1").arg(s.controllerError, 8, 16, QLatin1Char('0')).toUpper());
m_controllerErrorDetailValue->setText(decodeControllerError(s.controllerError));

m_encoderShadowCountValue->setText(QString::number(s.encoderShadowCount));
m_encoderCPRCountValue->setText(QString::number(s.encoderCountInCPR));
```

### 阶段2：添加当前运行参数显示（近期实现）

类似阶段1，添加当前控制模式、输入模式和限制值的显示。

### 阶段3：添加高级控制功能（可选）

在高级面板中添加轨迹控制和增益调整功能。

### 阶段4：添加实时图表（未来扩展）

需要引入 QCustomPlot 库或使用 Qt Charts 模块。

---

## UI布局建议

### 方案1：扩展现有状态面板

在现有的状态监控区域下方添加新的分组：

```
┌─ 状态监控 ─────────────────────┐
│ [现有18项状态]                  │
├─ 错误诊断 ─────────────────────┤
│ 电机错误：    0x00000000        │
│ 编码器错误：  0x00000000        │
│ 控制器错误：  0x00000000        │
│ 编码器计数：  12345 / 6789      │
├─ 当前参数 ─────────────────────┤
│ 控制模式：    位置控制 (3)      │
│ 输入模式：    梯形轨迹 (5)      │
│ 速度限制：    10.0 圈/秒        │
│ 电流限制：    20.0 A            │
└─────────────────────────────────┘
```

### 方案2：添加新的选项卡

在主窗口中添加选项卡，分离不同类型的信息：

```
┌─ 选项卡 ────────────────────────┐
│ [基本状态] [错误诊断] [高级参数] [图表] │
└─────────────────────────────────┘
```

### 方案3：使用可折叠的分组框

使用 QGroupBox 的可折叠功能，节省空间：

```
▼ 基本状态 (18项)
▼ 错误诊断 (8项)
▶ 高级参数 (12项)
▶ 实时图表 (5项)
```

---

## 总结

### 必须添加（优先级1）
- 电机错误显示
- 编码器错误显示
- 控制器错误显示
- 编码器计数显示

### 建议添加（优先级2）
- 当前控制模式显示
- 当前输入模式显示
- 当前限制值显示

### 可选添加（优先级3-5）
- 轨迹参数显示和控制
- 增益参数显示和调整
- 无感估算显示
- 实时图表

这些UI改进将大大增强程序的诊断能力和易用性！
