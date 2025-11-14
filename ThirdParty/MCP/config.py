# =============================================================================
# 多模型配置
# =============================================================================

# 模型选择 - 修改这里来切换不同的模型
SELECTED_MODEL = "nuwa"  # 可选: "qwen", "gemini", "openai", "claude", "nuwa"

# 通义千问配置
QWEN_CONFIG = {
    "API_KEY": "sk-8002cd18b7194f85b33e1307fdd85a6f",
    "BASE_URL": "https://dashscope.aliyuncs.com/compatible-mode/v1",
    "MODEL": "qwen-vl-plus",  # 使用支持视觉的模型
    "MAX_TOKENS": 1000,
    "TEMPERATURE": 0.7
}

# Google Gemini配置
GEMINI_CONFIG = {
    "API_KEY": "AIzaSyCjCdrekIxLLOa5iHPBVMsLOfLmM_jIDh4",
    "BASE_URL": "https://generativelanguage.googleapis.com/v1beta",
    "MODEL": "gemini-1.5-flash",
    "MAX_TOKENS": 1000,
    "TEMPERATURE": 0.7
}

# OpenAI配置
OPENAI_CONFIG = {
    "API_KEY": "your-openai-api-key",
    "BASE_URL": "https://api.openai.com/v1",
    "MODEL": "gpt-3.5-turbo",
    "MAX_TOKENS": 1000,
    "TEMPERATURE": 0.7
}

# Claude配置
CLAUDE_CONFIG = {
    "API_KEY": "your-claude-api-key",
    "BASE_URL": "https://api.anthropic.com",
    "MODEL": "claude-3-sonnet-20240229",
    "MAX_TOKENS": 1000,
    "TEMPERATURE": 0.7
}

NUWA_CONFIG = {
    "API_KEY": "sk-jYrjDnqiZ894oHHkJlmtKTsCP4L4RJCz0o7isE5oJ4vZJ90S",
    "BASE_URL": "https://api.nuwaapi.com/v1",
    "MODEL": "gpt-4o",
    "MAX_TOKENS": 1000,
    "TEMPERATURE": 0.7
}

# 模型配置映射
MODEL_CONFIGS = {
    "qwen": QWEN_CONFIG,
    "gemini": GEMINI_CONFIG,
    "openai": OPENAI_CONFIG,
    "claude": CLAUDE_CONFIG,
    "nuwa": NUWA_CONFIG
}

# 当前选择的模型配置
current_config = MODEL_CONFIGS[SELECTED_MODEL]

# AI模型配置（自动从选择的模型获取）
API_KEY = current_config["API_KEY"]
BASE_URL = current_config["BASE_URL"]
MODEL = current_config["MODEL"]
MAX_TOKENS = current_config["MAX_TOKENS"]
TEMPERATURE = current_config["TEMPERATURE"]

# 超时设置
MCP_SERVER_TIMEOUT = 30  # MCP服务器连接超时时间（秒）
SOCKET_TIMEOUT = 60      # 套接字超时时间（秒）

# 日志配置
LOG_LEVEL = "ERROR"  # DEBUG, INFO, WARNING, ERROR
LOG_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
SHOW_MCP_DEBUG_LOGS = False  # 是否显示 MCP 库的调试日志（用于调试 MCP 通信问题）

# =============================================================================
# iGameVis MCP Server 配置
# =============================================================================

# iGameVis 应用程序连接配置
IGAMEVIS_HOST = "localhost"
IGAMEVIS_PORT = 12345

# MCP Server 超时配置
MCP_CONNECTION_TIMEOUT = 30.0  # 连接超时时间（秒）
MCP_COMMAND_TIMEOUT = 30.0     # 命令执行超时时间（秒）

# 截图和文件操作配置
DEFAULT_SCREENSHOT_WIDTH = 1920
DEFAULT_SCREENSHOT_HEIGHT = 1080
DEFAULT_IMAGE_QUALITY = "high"  # high, normal

# 重连配置
MAX_RECONNECT_ATTEMPTS = 3
RECONNECT_DELAY = 1.0  # 重连延迟（秒）
