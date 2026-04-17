# Kilo Code 插件配置说明（第三方中转 API）

## 已完成的配置

已在 `.vscode/settings.json` 中配置了 Kilo Code 插件，使用第三方中转 API（兼容 OpenAI 格式）。

## 配置参数说明

- **apiEndpoint**: 你的第三方中转 API 端点地址
- **apiKey**: 你的 API 密钥
- **model**: 使用的 AI 模型（根据你的中转服务支持的模型）
- **maxTokens**: `4096` - 最大生成 token 数
- **temperature**: `0.7` - 生成温度（0-1，越高越有创造性）

## 当前配置

配置文件位置：`.vscode/settings.json`

```json
{
  "kilocode.apiEndpoint": "https://your-api-endpoint.com/v1",
  "kilocode.apiKey": "sk-Fh7wd***************************************zNbO",
  "kilocode.model": "gpt-4-turbo-preview",
  "kilocode.maxTokens": 4096,
  "kilocode.temperature": 0.7,
  "kilocode.enableAutoCompletion": true,
  "kilocode.enableInlineChat": true
}
```

## 修改配置

### 1. 修改 API 端点

将 `kilocode.apiEndpoint` 的值改为你的第三方中转 API 地址：

```json
{
  "kilocode.apiEndpoint": "https://你的中转API地址/v1"
}
```

常见的第三方中转 API 格式：
- `https://api.example.com/v1`
- `https://your-domain.com/openai/v1`
- `https://proxy.example.com/v1`

**注意**：确保端点地址以 `/v1` 结尾（如果你的中转服务要求）

### 2. 修改 API 密钥

将 `kilocode.apiKey` 的值改为你的实际密钥：

```json
{
  "kilocode.apiKey": "你的完整API密钥"
}
```

### 3. 选择模型

根据你的中转服务支持的模型，修改 `kilocode.model`：

```json
{
  "kilocode.model": "gpt-4"                    // GPT-4
  "kilocode.model": "gpt-4-turbo-preview"      // GPT-4 Turbo
  "kilocode.model": "gpt-3.5-turbo"            // GPT-3.5
  "kilocode.model": "claude-3-opus-20240229"   // 如果支持 Claude
}
```

## 使用 Kilo Code

配置完成后：

1. **保存配置文件**
2. **完全重启 VSCode**（关闭所有窗口后重新打开）
3. 使用 Kilo Code 的各项功能：
   - 代码补全
   - 内联聊天
   - 代码解释
   - 代码重构建议

## 验证配置

重启 VSCode 后，尝试使用 Kilo Code 功能。如果配置正确，应该能正常工作。

### 检查清单

- ✅ API 端点地址正确（包含协议 https://）
- ✅ API 密钥完整且有效
- ✅ 模型名称与中转服务支持的模型匹配
- ✅ 网络可以访问中转 API 地址
- ✅ 已完全重启 VSCode

## 故障排除

### 问题：401 Unauthorized / Invalid API Key

**可能原因**：
1. API 密钥错误或已过期
2. API 端点地址不正确
3. 中转服务的密钥格式要求不同

**解决方案**：
1. 检查 API 密钥是否完整复制（没有多余空格）
2. 确认 API 端点地址正确
3. 联系你的中转服务提供商确认密钥格式
4. 尝试在浏览器或 Postman 中测试 API 是否可用

### 问题：连接超时或无法连接

**解决方案**：
1. 检查网络连接
2. 确认防火墙没有阻止该域名
3. 尝试在浏览器中访问 API 端点测试连通性
4. 如果使用代理，确保代理配置正确

### 问题：模型不支持

**解决方案**：
1. 查看你的中转服务文档，确认支持的模型列表
2. 修改配置文件中的 `kilocode.model` 为支持的模型名称
3. 常见模型：`gpt-4`, `gpt-4-turbo-preview`, `gpt-3.5-turbo`

### 问题：响应速度慢

**解决方案**：
1. 可能是中转服务的网络延迟，属于正常现象
2. 可以尝试切换到更快的模型（如 `gpt-3.5-turbo`）
3. 调整 `maxTokens` 参数减少响应长度

## 自定义配置

### 调整生成参数

```json
{
  "kilocode.temperature": 0.3,    // 更确定性的输出（0.0-1.0）
  "kilocode.maxTokens": 2048,     // 减少响应长度以提高速度
  "kilocode.enableAutoCompletion": false  // 禁用自动补全以减少 API 调用
}
```

### 推荐配置

**代码生成场景**（更精确）：
```json
{
  "kilocode.temperature": 0.2,
  "kilocode.maxTokens": 2048
}
```

**创意编程场景**（更灵活）：
```json
{
  "kilocode.temperature": 0.8,
  "kilocode.maxTokens": 4096
}
```

## 测试 API 连接

你可以使用 curl 命令测试你的中转 API 是否正常：

```bash
curl https://你的中转API地址/v1/chat/completions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer 你的API密钥" \
  -d '{
    "model": "gpt-3.5-turbo",
    "messages": [{"role": "user", "content": "Hello"}]
  }'
```

如果返回正常的 JSON 响应，说明 API 配置正确。

## 安全建议

1. **不要将包含 API 密钥的配置文件提交到 Git**
   - 将 `.vscode/settings.json` 添加到 `.gitignore`
   
2. **使用环境变量**（可选）
   ```json
   {
     "kilocode.apiKey": "${env:MY_API_KEY}"
   }
   ```

3. **定期更换 API 密钥**

## 需要帮助？

如果遇到问题：
1. 检查中转服务提供商的文档
2. 确认 API 配额是否充足
3. 查看 VSCode 的输出面板（Output）中的错误信息
4. 联系中转服务的技术支持
