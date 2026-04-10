# 如何切换 Cline 到自定义 API 模型

## 当前问题
你看到错误：
```
You've hit your usage limit. Upgrade to Plus to continue using Codex 
(https://chatgpt.com/explore/plus), or try again at Apr 14th, 2026 4:20 PM.
```

这是因为 Cline 当前使用的是默认的 ChatGPT API（已达到限额），而不是你配置的自定义 API。

## 解决方案：切换到自定义模型

### 步骤 1：找到模型选择器

在 Cline 聊天界面的**顶部**，你会看到：
```
┌─────────────────────────────────────┐
│ [模型名称 ▼]  [设置图标]           │  ← 这里！
├─────────────────────────────────────┤
│                                     │
│  聊天内容区域                        │
│                                     │
└─────────────────────────────────────┘
```

### 步骤 2：点击模型选择器

点击顶部显示当前模型名称的下拉框（可能显示为 "GPT-4" 或其他默认模型）

### 步骤 3：选择自定义模型

在弹出的列表中，你应该看到：
- ✅ **GPT-5.3-Codex** ← 推荐选择这个
- ✅ **GPT-5.4**
- ✅ **GPT-5.2-Codex**
- （可能还有其他默认模型）

选择任何一个带 ✅ 的模型，这些都是你的自定义 API 模型。

### 步骤 4：验证切换成功

选择后，顶部应该显示你选择的模型名称（如 "GPT-5.3-Codex"）。

现在发送一条测试消息，如果收到回复且没有限额错误，说明切换成功！

## 如果看不到自定义模型

### 方法 1：重启 VS Code
1. 完全关闭 VS Code（所有窗口）
2. 重新打开 VS Code
3. 打开 Cline，查看模型选择器

### 方法 2：检查配置文件

确认配置文件内容正确：

**文件位置：** `C:\Users\a\.continue\config.yaml`

**应该包含：**
```yaml
models:
  - title: GPT-5.3-Codex
    provider: openai
    model: gpt-5.3-codex
    apiKey: sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO
    apiBase: https://realmrouter.cn/v1
```

### 方法 3：通过设置界面添加模型

1. 点击 Cline 界面右上角的**齿轮图标**（设置）
2. 在设置界面中，找到 "Models" 部分
3. 点击 "Add Model" 或 "+"
4. 填写：
   - **Provider:** OpenAI
   - **Model:** gpt-5.3-codex
   - **API Key:** sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO
   - **Base URL:** https://realmrouter.cn/v1
5. 保存
6. 在模型选择器中选择新添加的模型

## 常见问题

### Q: 我找不到模型选择器
**A:** 模型选择器通常在 Cline 聊天界面的顶部。如果看不到，尝试：
- 重启 VS Code
- 检查 Cline 扩展是否是最新版本
- 点击 Cline 图标重新打开界面

### Q: 选择了自定义模型，但还是显示限额错误
**A:** 可能的原因：
1. 配置文件格式错误 - 检查 YAML 缩进
2. API 密钥无效 - 运行 `python test_api_config.py` 测试
3. 需要重启 VS Code 使配置生效

### Q: 模型列表中没有我的自定义模型
**A:** 
1. 确认 `config.yaml` 文件内容正确
2. 重启 VS Code
3. 或者通过设置界面手动添加模型

## 验证配置

运行测试脚本确认 API 可用：
```bash
python test_api_config.py
```

应该看到：
```
✓ API 连接成功！
✓ 测试通过！你的 API 配置正常工作。
```

## 配置文件位置总结

- **Cline 主配置（YAML）：** `C:\Users\a\.continue\config.yaml` ✅ 已更新
- **Cline 备用配置（JSON）：** `C:\Users\a\.continue\config.json` ✅ 已更新
- **旧版配置（TOML）：** `C:\Users\a\.codex\config.toml` ✅ 已更新

## 下一步

**立即执行：**
1. 在 Cline 界面顶部找到模型选择器
2. 点击并选择 "GPT-5.3-Codex"
3. 发送测试消息验证

如果还有问题，请告诉我具体看到了什么，我会继续帮你解决！
