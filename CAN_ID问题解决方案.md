# CAN ID 问题解决方案

## 问题诊断

你在Qt程序的自定义发送面板中发送了 `0x0CF` 帧，但这个地址是**错误的**。

### 问题分析

**发送的CAN ID**: `0x0CF` (207 十进制)

解码结果：
- **节点ID**: 6
- **命令ID**: 0x0F (Set_Limits)
- **Payload**: `04 00 00 00`

**问题**：你的ODrive配置的节点ID是 **16** 和 **1**，但你发送的命令是给**节点6**的！

这就是为什么电机没有反应 - 命令发送到了错误的节点。

---

## CAN ID 计算公式

ODrive的CAN ID格式：

```
CAN_ID = (node_id << 5) | command_id
```

或者用更简单的方式：

```
CAN_ID = (node_id × 32) + command_id
```

### 示例计算

**节点16, Set_Axis_State命令 (0x07)**:
```
= (16 << 5) | 0x07
= (16 × 32) + 7
= 512 + 7
= 519 (0x207)
```

---

## 正确的CAN ID

### 节点16 (0x10) 的常用命令

| 命令名称 | 命令ID | CAN ID (十六进制) | CAN ID (十进制) |
|------|---------------|
| Set_Axis_State | 0x07 | **0x207** | 519 |
| Set_Controller_Mode | 0x0B | **0x20B** | 523 |
| Set_Input_Pos | 0x0C | **0x20C** | 524 |
| Set_Input_Vel | 0x0D | **0x20D** | 525 |
| Set_Input_Torque | 0x0E | **0x20E** | 526 |
| Clear_Errors | 0x18 | **0x218** | 536 |

### 节点1 (0x01) 的常用命令

| 命令名称 | 命令ID | CAN ID (十六进制) | CAN ID (十进制) |
|---------|--------|------------------|-------------|
| Set_Axis_State | 0x07 | **0x027** | 39 |
| Set_Controller_Mode | 0x0B | **0x02B** | 43 |
| Set_Input_Pos | 0x0C | **0x02C** | 44 |
| Set_Input_Vel | 0x0D | **0x02D** | 45 |
| Set_Input_Torque | 0x0E | **0x02E** | 46 |
| Clear_Errors | 0x18 | **0x038** | 56 |

---

## 解决方案

### 在Qt程序的自定义发送面板中：

1. **检查 'Address' 输入框**
2. **确保输入正确的CAN ID**

### 示例：发送校准命令

#### 给节点16发送完整校准命令：

```
Address: 0x207 (或十进制 519)
DLC: 4
Payload: 04 00 00
```

解释：
- `0x207` = 节点16的Set_Axis_State命令
- `04 00 00 00` = 状态4 (FULL_CALIBRATION_SEQUENCE)

#### 给节点1发送完整校准命令：

```
Address: 0x027 (或十进制 39)
DLC: 4
Payload: 04 00 00 00
```

---

## 常用Axis State值

| 状态名称 | 值 | Payload (小端) |
|---------|---|------------|
| IDLE | 1 | `01 00 00 00` |
| MOTOR_CALIBRATION | 3 | `03 00 00` |
| FULL_CALIBRATION_SEQUENCE | 4 | `04 00 00 00` |
| ENCODER_OFFSET_CALIBRATION | 6 | `06 00 00` |
| CLOSED_LOOP_CONTROL | 8 | `08 00 00` |

---

## 测试步骤

### 1. 测试节点16进入闭环

```
Address: 0x207
DLC: 4
Payload: 08 00 00 00
```

### 2. 测试节点1进入闭环

```
Address: 0x027
DLC: 4
Payload: 08 00 00 00
```

### 3. 发送位置命令（节点16）

先设置控制模式：
```
Address: 0x20B
DLC: 8
Payload: 03 00 00 00 01 00 00 00
```
(控制模式3=位置控制, 输入模式1=Passthrough)

然后发送位置：
```
Address: 0x20C
DLC: 8
Payload: 00 80 3F 00 00 00 00
```
(位置1.0圈, 速度前馈0)

---

## 快速检查清单

- [ ] 确认ODrive的节点ID（你的是16和1）
- [ ] 使用正确的CAN ID公式计算地址
- [ ] 在Qt程序中输入正确的Address
- [ ] 确认Payload格式正确（小端字节序）
- [ ] 发送命令后检查接收窗口是否有响应

---

## 调试技巧

1. **查看日志中的心跳**：确认ODrive在线
   - 节点16的心跳：`0x201`
   - 节点1的心跳：`0x021`

2. **发送Get_Encoder_Estimates请求**：
   - 节点16：`0x209`
   - 节点1：`0x029`

3. **清除错误**：
   - 节点16：`0x218`, Payload: `00`
   - 节点1：`0x038`, Payload: `00`

---

## 总结

**问题根源**：使用了错误的CAN ID `0x0CF`（节点6），而不是正确的 `0x207`（节点16）或 `0x027`（节点1）。

**解决方法**：在Qt程序的自定义发送面板中，使用上面表格中列出的正确CAN ID。

**验证方法**：发送命令后，应该能在接收窗口看到ODrive的响应消息。
