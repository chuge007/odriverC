# Codex 自定义 API 配置问题解决方案

## 问题描述
虽然已经配置了自定义 API，但 Codex 仍然显示使用限额已用尽的提示。

## 已完成的配置

### 1. 配置文件位置
`C:\Users\a\.codex\config.toml`

### 2. 当前配置内容
```toml
base_url = "https://realmrouter.cn/v1"
api_key = "sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO"
model = "gpt-5.3-codex"

model_context_window = 1000000
model_auto_compact_token_limit = 900000
model_reasoning_effort = "high"
approval_policy = "never"
sandbox_mode = "workspace-write"
network_access = true

[windows]
sandbox = "elevated"

[projects.'D:\dev\odriveCar']
trust_level = "trusted"
```

### 3. API 测试结果
✅ API 连接成功（状态码 200）
✅ 模型 gpt-5.3-codex 可正常使用
✅ 令牌使用正常

## 解决方案

### 方案 1：重启 VS Code（推荐）

1. **完全关闭 VS Code**
   - 关闭所有 VS Code 窗口
   - 确保任务管理器中没有 Code.exe 进程

2. **重新启动 VS Code**
   - 重新打开 VS Code
   - Codex 扩展会重新加载配置文件

3. **验证配置是否生效**
   - 尝试使用 Codex
   - 如果仍显示限额提示，继续下一个方案

### 方案 2：重新加载 Codex 扩展

1. **打开命令面板**
   - 按 `Ctrl+Shift+P` (Windows)
   - 或 `Cmd+Shift+P` (Mac)

2. **重新加载扩展**
   - 输入 "Developer: Reload Window"
   - 按回车执行

3. **或者重启扩展**
   - 输入 "Extensions: Restart Extensions"
   - 按回车执行

### 方案 3：检查配置文件路径

Codex 可能在其他位置查找配置文件。请检查以下位置：

1. **用户主目录**
   ```
   C:\Users\a\.codex\config.toml
   ```

2. **项目目录**
   ```
   D:\dev\odriveCar\.codex\config.toml
   ```

3. **VS Code 配置目录**
   ```
   %APPDATA%\Code\User\.codex\config.toml
   ```

### 方案 4：手动设置环境变量（备用）

如果上述方案都不起作用，可以尝试设置环境变量：

```bash
# 在 PowerShell 中执行
$env:CODEX_API_BASE = "https://realmrouter.cn/v1"
$env:CODEX_API_KEY = "sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO"
$env:CODEX_MODEL = "gpt-5.3-codex"
```

然后从该 PowerShell 窗口启动 VS Code：
```bash
code
```

### 方案 5：检查 Codex 扩展版本

1. 打开扩展面板（Ctrl+Shift+X）
2. 搜索 "Codex"
3. 检查是否有更新
4. 如果有更新，先更新扩展
5. 更新后重启 VS Code

## 常见问题

### Q1: 为什么 API 测试成功但 Codex 还是不工作？
A: Codex 扩展需要重新加载才能读取新的配置文件。必须重启 VS Code 或重新加载窗口。

### Q2: 配置文件应该放在哪里？
A: 通常是 `C:\Users\<用户名>\.codex\config.toml`，但某些版本可能在其他位置查找。

### Q3: 如何确认配置已生效？
A: 重启后，尝试使用 Codex。如果不再显示限额提示，说明配置已生效。

### Q4: 如果还是不行怎么办？
A: 
1. 检查 Codex 扩展的输出日志（查看 -> 输出 -> 选择 Codex）
2. 查看是否有错误信息
3. 确认 API 密钥是否有效
4. 联系 Codex 支持团队

## 可用的模型列表

根据 API 测试，你的账户可以访问以下模型：
- ✅ gpt-5.3-codex（当前配置）
- gpt-5.2-codex
- gpt-5.4
- gpt-5.2
- claude-sonnet-4-5
- claude-sonnet-4
- claude-haiku-4.5
- claude-opus-4-6-thinking

如果需要更换模型，修改配置文件中的 `model` 字段即可。

## 下一步操作

**立即执行：**
1. 保存所有工作
2. 完全关闭 VS Code
3. 重新启动 VS Code
4. 测试 Codex 是否正常工作

如果问题仍然存在，请查看 VS Code 的输出面板中的 Codex 日志以获取更多信息。
