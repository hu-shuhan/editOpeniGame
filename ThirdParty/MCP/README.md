# iGame MCP AI助手

这是iGameVis项目的MCP (Model Control Protocol) AI助手组件，为iGameVis提供智能聊天和工具调用功能。

## 🏗️ 项目架构

```
iGameVis应用程序 ←→ Socket通信 ←→ bridge_server.py ←→ client.py ←→ MCP协议 ←→ server.py
```

- **iGameVis** - 主应用程序（C++ Qt），内置AI聊天助手Widget
- **bridge_server.py** - MCP桥梁服务器，处理Socket通信
- **client.py** - MCP客户端，连接AI模型服务
- **server.py** - MCP工具服务器，提供各种工具功能

## 🚀 功能特点

### AI助手功能
- 🤖 集成阿里云通义千问大模型
- 💬 智能对话问答
- 🔧 工具调用支持

### CAD辅助工具
- 📁 桌面文件统计和搜索
- 🎯 网格文件识别（obj, vtk, ply, stl等）
- 📊 网格文件标量场分析
- 🔍 多格式文件查找

### 桥梁通信
- 🌐 TCP套接字（与iGameVis客户端通信）
- 📡 MCP协议（内部工具调用）
- 🔄 完全复用现有MCP功能

## 📦 安装依赖

```bash
# 进入MCP目录
cd ThirdParty/MCP

# 运行依赖安装脚本
install_dependencies.bat

# 或手动安装
pip install fastmcp>=0.2.0 python-dotenv>=1.0.0 openai>=1.0.0 mcp>=1.0.0
```

## ⚙️ 配置

项目使用`config.py`文件进行配置，支持多种AI模型：

```python
# 模型选择 - 修改这里来切换不同的模型
SELECTED_MODEL = "qwen"  # 可选: "qwen", "gemini", "openai", "claude"

# 套接字服务器配置  
SOCKET_HOST = "127.0.0.1"
SOCKET_PORT = 8080

# 各模型的API密钥配置在对应的CONFIG部分
```

### 支持的AI模型：
- ✅ **阿里云通义千问** (Qwen) - 默认模型
- ✅ **Google Gemini** - 需要API密钥  
- ⚠️ **OpenAI GPT** - 需要配置API密钥
- ⚠️ **Anthropic Claude** - 需要配置API密钥

> 详细配置说明请参考 `CONFIG_NOTES.md`

## 🎮 使用方法

### 标准使用流程

1. **安装Python依赖**
```bash
install_dependencies.bat
```

2. **启动MCP桥梁服务器**
```bash
# 使用批处理文件启动（推荐）
start_bridge.bat

# 或直接启动Python脚本
python bridge_server.py
```

3. **在iGameVis中使用AI助手**
- 启动iGameVis应用程序
- 打开 "AI工具" 菜单 → "显示AI聊天助手"
- 点击 "连接服务器" 按钮
- 开始与AI进行对话

### 快捷方式
- **快捷键**: `Ctrl + Alt + A` 快速打开AI聊天助手
- **菜单路径**: AI工具 → 显示AI聊天助手

## 🔗 iGameVis集成架构

MCP桥梁服务器与iGameVis应用程序通过Socket通信：

### 通信架构
```
iGameVis AI聊天Widget (Qt C++)
    ↓ Windows Socket API
MCP桥梁服务器 (Python)
    ↓ MCP协议
AI模型服务 (通义千问/Gemini等)
```

### 通信协议
使用简单的二进制协议：
1. 发送消息长度（4字节整数，小端序）
2. 发送消息内容（UTF-8字符串）
3. 接收回复长度（4字节整数，小端序）  
4. 接收回复内容（UTF-8字符串）

### JSON工具调用支持
MCP系统支持JSON格式的工具调用：
```json
{
    "tool_name": "get_chat_history",
    "parameters": {}
}
```

## 🛠️ 可用工具

### 内置MCP工具函数
1. **get_chat_history** - 获取聊天历史记录
2. **clear_chat** - 清空聊天记录  
3. **send_message** - 发送消息到聊天界面

### 扩展工具（通过MCP服务器）
1. **count_desktop_mesh_files** - 统计桌面网格文件数量
2. **list_desktop_mesh_files** - 列出桌面所有网格文件
3. **find_desktop_files** - 查找指定扩展名的文件
4. **get_scalar_count** - 分析网格文件的标量场数量

### 使用示例（在iGameVis AI聊天助手中输入）
- "帮我统计桌面上有几个网格文件"
- "桌面上有哪些vtk文件？"
- "分析box.vtk文件有几个标量场"
- "桌面上有几个pdf文件"
- "清空聊天记录"
- "显示聊天历史"

## 📁 项目结构

```
ThirdParty/MCP/
├── config.py              # 🔧 配置文件（多模型支持）
├── bridge_server.py       # 🔗 MCP桥梁服务器
├── main.py                # 主入口文件
├── start_bridge.bat       # Windows启动脚本（增强版）
├── install_dependencies.bat # Python依赖安装脚本
├── USAGE.md               # 详细使用说明
├── CONFIG_NOTES.md        # 配置说明和安全提醒
├── README.md              # 项目说明文档
├── Client/
│   └── client.py          # MCP客户端实现
├── Servers/
│   └── server.py          # MCP工具服务器
└── history/               # 对话历史记录
```

## 🎯 核心优势

1. **无缝集成** - 完美集成到iGameVis应用程序中
2. **多模型支持** - 支持通义千问、Gemini、OpenAI、Claude等主流AI模型
3. **即插即用** - 简单的安装和配置过程
4. **工具扩展** - 支持MCP工具调用和JSON函数调用
5. **用户友好** - 现代化的聊天界面，支持消息复制、窗口缩放等
6. **高性能** - 异步处理，支持并发连接

## 🚨 注意事项

- 确保在`config.py`中设置正确的API密钥
- 防火墙要允许端口8080的本地连接
- 需要Python 3.13或更高版本
- 桥梁服务器必须先启动，再在iGameVis中连接
- API密钥请妥善保管，避免泄露

## 🐛 故障排除

1. **连接失败** - 检查MCP桥梁服务器是否正在运行
2. **Python错误** - 运行`install_dependencies.bat`安装依赖
3. **API错误** - 检查config.py中的API_KEY配置和网络连接
4. **端口被占用** - 修改config.py中的SOCKET_PORT为其他端口
5. **聊天无响应** - 检查AI模型服务是否可用，尝试切换其他模型

## 📖 更多信息

- 详细使用说明：`USAGE.md`
- 配置安全提醒：`CONFIG_NOTES.md` 
- iGameVis官方文档：请查看项目主目录

---
*iGame MCP AI助手 - 让你的CAD工作更智能*