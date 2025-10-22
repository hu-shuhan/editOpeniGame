# MCP项目配置文件

# 套接字服务器配置
SOCKET_HOST = "127.0.0.1"
SOCKET_PORT = 8080  # 修改这里来改变端口号
MAX_CONNECTIONS = 5

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
    "MODEL": "deepseek-v3",
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
LOG_LEVEL = "WARNING"  # WARNING 级别，只显示警告和错误
LOG_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'

# 模型选择菜单
MODEL_MENU = {
    "1": ("qwen", "阿里云通义千问"),
    "2": ("gemini", "Google Gemini"),
    "3": ("openai", "OpenAI GPT"),
    "4": ("claude", "Anthropic Claude"),
    "5": ("nuwa", "Nuwa")
} 