# MeshReport ReportGenerate Server

网格分析报告生成服务器。接收客户端上传的 VTK 网格文件，调用 LLM 生成分析报告（Word 文档）并回传。

目标机器上服务端与 C++ 端 `iGameMeshReportGenerator` / `iGameMeshReportClient` 配套使用。

---

## 要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10/11 或 Linux |
| Python | 3.10（本仓库亦已在 3.13 下运行通过） |
| 网络 | 需要能访问大模型 API（报告生成依赖云端 LLM，非离线） |
| 显存 | 无（纯 CPU 推理管道路径；渲染/可视化不依赖 GPU） |

> **和 P3SAM SplitServer 不同：本服务器不是离线模型。** 报告的文字分析与图片描述由云端大模型完成，
> 因此运行机器必须能访问 `PredefinedStrs/Config.py` 中配置的 API 域名（默认 `https://api.nuwaapi.com/v1`）。

---

## 配置（重要）

在运行前配置大模型 API。配置文件位于 `ReportGenerate/PredefinedStrs/Config.py`：

```python
# 对话模型
CHAT_API_BASE_URL = "https://api.nuwaapi.com/v1"
CHAT_API_KEY = "sk-xxxxxxxx"          # 改成你自己的 key
CHAT_MODEL = "gpt-5-mini"             # 可选其他模型

# Embedding / 图片模型
EMBEDDING_API_BASE_URL = "https://api.nuwaapi.com/v1"
EMBEDDING_API_KEY = "sk-xxxxxxxx"     # 通常与 CHAT_API_KEY 相同
IMG_API_BASE_URL = "https://api.nuwaapi.com/v1"
IMG_API_KEY = "sk-xxxxxxxx"
```

- 仓库自带的 `Config.py` 含一个示例 key，**仅作演示，请务必替换为真实 key**，否则请求会失败或额度被他人使用。
- 若使用 OpenAI 兼容接口，把三组 `*_API_BASE_URL` 指向你的 API 端点，`*_API_KEY` 填对应密钥。
- `POSTGRESQL_*` 项在报告生成流程中暂未使用，可保留默认，无需本地部署 PostgreSQL。

---

## 安装

### 1. 创建 Conda 环境

```bash
conda create -n reportgen python=3.10
conda activate reportgen
```

### 2. 安装依赖

```bash
pip install -r requirements.txt
```

等价的手动安装：

```bash
pip install vtk numpy scipy matplotlib python-docx pillow requests
pip install langchain langchain-openai langchain-chroma langchain-community sentence-transformers sqlalchemy pydantic
```

> `vtk` 在部分系统上需要较新的 pip 版本，若安装失败先 `pip install -U pip`。

---

## 启动服务器

```bash
cd ReportGenerate/Server
python mesh_report_server.py --host 127.0.0.1 --port 8766
```

### 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--host` | `127.0.0.1` | 监听地址。`0.0.0.0` 接受所有来源连接。 |
| `--port` | `8766` | 监听端口。 |

看到 `MeshReport server listening on 127.0.0.1:8766` 即就绪。按 `Ctrl+C` 停止。

---

## 测试

用打包附带的 `Test/mock_cpp_client.py` 模拟 C++ 客户端发送一个 VTK 文件：

```bash
cd ReportGenerate/Test
python mock_cpp_client.py <你的网格.vtk> --host 127.0.0.1 --port 8766
```

不指定 `<vtk文件>` 时，默认读 `Test/input.vtk`（示例文件需自行放置）。

成功时在 `Test/` 下生成 `report_<时间戳>.docx`；失败时打印服务器返回的错误信息。

---

## 通信协议

简单二进制 TCP 流，**小端序**。

**请求（客户端 -> 服务器）：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `vtk_size` | `uint32` (4 bytes) | VTK 数据字节长度 |
| `vtk_data` | `bytes` (vtk_size 字节) | 原始 VTK 文件内容 |

**成功响应（服务器 -> 客户端）：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `success` | `uint8` (1 byte) | 值 `1` |
| `report_size` | `uint32` (4 bytes) | 报告数据字节长度 |
| `report_data` | `bytes` (report_size 字节) | 报告文件内容（Word 文档） |

**失败响应（服务器 -> 客户端）：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `success` | `uint8` (1 byte) | 值 `0` |
| `msg_size` | `uint32` (4 bytes) | 错误信息字节长度 |
| `error_msg` | `bytes` (msg_size 字节) | UTF-8 编码的错误描述 |

---

## FAQ

**Q: 报告生成很慢 / 卡住不动**
A: 生成过程会调用多次云端 LLM（含图片分析），单次可能耗时数分钟到十几分钟。请确保网络通畅且 API 额度充足。

**Q: `ERROR: 连接超时` 或服务器无法启动**
A: 检查防火墙是否放行端口，或改用 `--host 127.0.0.1` 限制为本机访问。

**Q: `ImportError: No module named 'ReportGenerator'`**
A: 始终以 `ReportGenerate/Server/mesh_report_server.py` 为入口启动；脚本已自动把 `ReportGenerate/` 及其子目录加入 `sys.path`，不要在其它目录下裸 import。

**Q: 生成的报告是空的或用例报错**
A: 先确认 `Config.py` 中的 API key 有效、`CHAT_MODEL` 与 `IMG_MODEL` 可被你的 API 服务器识别。

**Q: 系统没有安装 conda，README 第 1 步无法执行**
A: 先安装 Miniconda（用户目录即可），再按 README 创建 `reportgen` 环境。

**Q: 包内没有 `Test/input.vtk` 示例文件**
A: README 已说明示例文件需自行放置。可用任意 VTK 网格，或先用 Python/VTK 生成一个最小八节点六面体测试文件。

**Q: 使用 DeepSeek 官方 API 时地址和模型名填什么**
A: DeepSeek 官方 OpenAI 兼容端点填 `https://api.deepseek.com/v1`；`deepseek-v4-flash-vision-exp` 是官方多模态模型名，可同时承担文本与图片理解。

**Q: `Config.py` 里的示例 key 能用吗**
A: 不能依赖演示 key，请替换为真实 key，并把 `*_API_BASE_URL` 指向你的 API 端点。

**Q: `EMBEDDING_*` 配置在报告生成中报错/未使用**
A: 当前报告流程以 `collection_name=None` 初始化，不会调用 Embedding；若后续启用 RAG 才需要可用的 embedding 服务。

**Q: 启动日志出现 LangChain 弃用警告或中文乱码**
A: 都是非致命问题；弃用警告不影响功能，Windows 控制台中文乱码只是 GBK/UTF-8 显示问题。
