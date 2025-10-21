# MCP配置说明

## API密钥安全提醒

⚠️ **重要安全提醒**：
- config.py文件包含API密钥，请注意保护
- 如果将代码提交到版本控制系统，建议：
  1. 将API密钥移动到环境变量
  2. 或将config.py添加到.gitignore
  3. 或创建config_template.py作为模板

## 当前配置状态

### 已配置的模型
- ✅ 阿里云通义千问 (Qwen) - 默认
- ✅ Google Gemini
- ⚠️ OpenAI GPT - 需要配置API密钥
- ⚠️ Anthropic Claude - 需要配置API密钥

### 配置说明

1. **切换模型**：修改config.py中的`SELECTED_MODEL`
2. **API密钥**：在对应模型配置中修改`API_KEY`
3. **模型参数**：可调整温度、最大令牌数等

### 性能建议

- **温度 (TEMPERATURE)**: 0.7 (创造性) | 0.1 (准确性)
- **最大令牌 (MAX_TOKENS)**: 1000 (平衡) | 2000+ (长文本)

## 使用流程

1. 根据需要配置API密钥
2. 选择要使用的模型
3. 运行install_dependencies.bat安装依赖
4. 运行start_bridge.bat启动服务器
5. 在iGameVis中连接并使用 