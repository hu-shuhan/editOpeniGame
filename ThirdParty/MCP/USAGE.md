# iGame MCP AI助手使用说明

## 概述
本MCP (Model Control Protocol) 系统为iGameVis提供AI聊天助手功能，支持多种AI模型。

## 快速开始

### 1. 安装Python依赖
```bash
# 运行依赖安装脚本
install_dependencies.bat
```

### 2. 启动MCP服务器
```bash
# 运行MCP桥梁服务器
start_bridge.bat
```

### 3. 在iGameVis中使用
1. 启动iGameVis程序
2. 打开AI工具菜单 → 显示AI聊天助手
3. 点击"连接服务器"按钮
4. 开始与AI对话

## 配置说明

### 切换AI模型
编辑 `config.py` 文件中的 `SELECTED_MODEL` 参数：

```python
SELECTED_MODEL = "qwen"    # 阿里云通义千问 (默认)
SELECTED_MODEL = "gemini"  # Google Gemini
SELECTED_MODEL = "openai"  # OpenAI GPT
SELECTED_MODEL = "claude"  # Anthropic Claude
```

### API密钥配置
在 `config.py` 中配置对应模型的API密钥：

```python
# 通义千问配置
QWEN_CONFIG = {
    "API_KEY": "你的API密钥",
    "MODEL": "qwen-plus",
    # ...其他配置
}
```

## 网络配置

### 服务器地址
- **主机**: 127.0.0.1 (本地)
- **端口**: 8080
- **协议**: TCP Socket

### 防火墙设置
确保端口8080未被防火墙阻止

## MCP工具功能

本系统支持以下MCP工具调用：

### 1. get_chat_history
获取聊天历史记录
```json
{
    "tool_name": "get_chat_history",
    "parameters": {}
}
```

### 2. clear_chat
清空聊天记录
```json
{
    "tool_name": "clear_chat", 
    "parameters": {}
}
```

### 3. send_message
发送消息到聊天界面
```json
{
    "tool_name": "send_message",
    "parameters": {
        "message": "要发送的消息内容"
    }
}
```

## 故障排除

### 常见问题

1. **Python未找到**
   - 安装Python 3.13或更高版本
   - 确保Python已添加到系统PATH

2. **连接失败**
   - 检查MCP服务器是否正在运行
   - 确认端口8080未被占用
   - 检查防火墙设置

3. **AI响应慢**
   - 检查网络连接
   - 确认API密钥有效
   - 尝试切换到其他AI模型

### 日志查看
MCP服务器运行时会在控制台显示详细日志，包括：
- 客户端连接/断开信息
- 消息收发记录
- 错误信息

## 系统架构

```
iGameVis应用程序
    ↓ (Socket连接)
MCP桥梁服务器 (bridge_server.py)
    ↓ (MCP协议)
AI模型服务器 (通义千问/Gemini/OpenAI/Claude)
```

## 支持信息

- **开发者**: iGame团队
- **版本**: 1.0.0
- **支持的AI模型**: 通义千问、Google Gemini、OpenAI GPT、Anthropic Claude
- **兼容系统**: Windows 10/11 