# iGameVis_Server.py
# iGameVis MCP Server - 直接与 iGameVis 通信的 MCP 服务器
from mcp.server.fastmcp import FastMCP, Context, Image
import socket
import json
import asyncio
import logging
import tempfile
import struct
from dataclasses import dataclass
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, List
import os
import sys
from pathlib import Path
import base64
from datetime import datetime

# 导入统一配置
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config

# ============================================================================
# Configuration (从 config.py 导入)
# ============================================================================

# iGameVis 连接配置
DEFAULT_HOST = config.IGAMEVIS_HOST
DEFAULT_PORT = config.IGAMEVIS_PORT

# 日志配置
LOG_LEVEL = config.LOG_LEVEL

# 超时配置
CONNECTION_TIMEOUT = config.MCP_CONNECTION_TIMEOUT
COMMAND_TIMEOUT = config.MCP_COMMAND_TIMEOUT

# 文件操作配置
DEFAULT_SCREENSHOT_WIDTH = config.DEFAULT_SCREENSHOT_WIDTH
DEFAULT_SCREENSHOT_HEIGHT = config.DEFAULT_SCREENSHOT_HEIGHT
DEFAULT_IMAGE_QUALITY = config.DEFAULT_IMAGE_QUALITY

# 重连配置
MAX_RECONNECT_ATTEMPTS = config.MAX_RECONNECT_ATTEMPTS
RECONNECT_DELAY = config.RECONNECT_DELAY

# ============================================================================
# Logging Setup
# ============================================================================

# Configure logging - 输出到文件，因为 stdout 被 MCP 协议占用
log_file = os.path.join(tempfile.gettempdir(), "igamevis_mcp_server.log")
logging.basicConfig(
    level=getattr(logging, LOG_LEVEL), 
    format=config.LOG_FORMAT,
    handlers=[
        logging.FileHandler(log_file, mode='a', encoding='utf-8'),
        # 不输出到 stderr，避免干扰 MCP 协议
    ]
)
logger = logging.getLogger("iGameVisMCPServer")
logger.info(f"===== iGameVis MCP Server 启动 ===== 日志文件: {log_file}")

# 根据配置决定是否关闭 MCP 库的调试日志
if not config.SHOW_MCP_DEBUG_LOGS:
    logging.getLogger('mcp').setLevel(logging.WARNING)
    logging.getLogger('mcp.server').setLevel(logging.WARNING)

# ============================================================================
# Connection Management
# ============================================================================

@dataclass
class iGameVisConnection:
    host: str
    port: int
    sock: socket.socket = None
    
    def connect(self) -> bool:
        """Connect to the iGameVis application socket server"""
        if self.sock:
            return True
            
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            logger.info(f"Connected to iGameVis at {self.host}:{self.port}")
            return True
        except Exception as e:
            logger.error(f"Failed to connect to iGameVis: {str(e)}")
            self.sock = None
            return False
    
    def disconnect(self):
        """Disconnect from the iGameVis application"""
        if self.sock:
            try:
                self.sock.close()
            except Exception as e:
                logger.error(f"Error disconnecting from iGameVis: {str(e)}")
            finally:
                self.sock = None

    def receive_full_response(self, sock, buffer_size=8192):
        """Receive the complete response from iGameVis"""
        try:
            # 首先接收消息长度（4字节）
            length_data = b''
            while len(length_data) < 4:
                chunk = sock.recv(4 - len(length_data))
                if not chunk:
                    raise Exception("Connection closed while receiving length")
                length_data += chunk
            
            # 解析消息长度
            message_length = struct.unpack('I', length_data)[0]
            logger.info(f"Expecting message of length: {message_length}")
            
            # 接收完整消息
            message_data = b''
            while len(message_data) < message_length:
                remaining = message_length - len(message_data)
                chunk = sock.recv(min(buffer_size, remaining))
                if not chunk:
                    raise Exception("Connection closed while receiving message")
                message_data += chunk
            
            logger.info(f"Received complete response ({len(message_data)} bytes)")
            return message_data
            
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            raise

    def send_command(self, command_type: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
        """Send a command to iGameVis and return the response"""
        if not self.sock and not self.connect():
            raise ConnectionError("Not connected to iGameVis")
        
        # 构建命令，使用与原 server.py 相同的格式
        command = {
            "type": command_type,
            "content": params or {},
            "timestamp": datetime.now().isoformat()
        }
        
        try:
            # Log the command being sent
            logger.info(f"📤 [SEND] Action: {command_type}")
            if params:
                logger.info(f"📦 [PARAMS] {json.dumps(params, ensure_ascii=False)[:200]}")  # 最多打印200字符
            
            # 序列化命令
            command_json = json.dumps(command, ensure_ascii=False)
            command_bytes = command_json.encode('utf-8')
            
            logger.info(f"📨 [JSON] 发送 {len(command_bytes)} 字节: {command_json[:150]}...")  # 打印前150字符
            
            # 发送消息长度（4字节）+ 消息内容
            length_bytes = struct.pack('I', len(command_bytes))
            self.sock.sendall(length_bytes + command_bytes)
            logger.info(f"✅ [SENT] 命令已发送，等待响应...")
            
            # 设置超时
            self.sock.settimeout(COMMAND_TIMEOUT)
            
            # 接收响应
            response_data = self.receive_full_response(self.sock)
            logger.info(f"📥 [RECV] 收到 {len(response_data)} 字节响应")
            
            # 解析响应
            response = json.loads(response_data.decode('utf-8'))
            response_type = response.get('type', 'unknown')
            logger.info(f"📋 [RESPONSE] Type: {response_type}")
            
            # 打印响应内容（简短版本）
            content_str = str(response.get('content', ''))[:200]
            logger.info(f"💬 [CONTENT] {content_str}")
            
            return response
            
        except socket.timeout:
            logger.error("Socket timeout while waiting for response from iGameVis")
            self.sock = None
            raise Exception("Timeout waiting for iGameVis response - try simplifying your request")
        except (ConnectionError, BrokenPipeError, ConnectionResetError) as e:
            logger.error(f"Socket connection error: {str(e)}")
            self.sock = None
            raise Exception(f"Connection to iGameVis lost: {str(e)}")
        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON response from iGameVis: {str(e)}")
            if 'response_data' in locals() and response_data:
                logger.error(f"Raw response (first 200 bytes): {response_data[:200]}")
            raise Exception(f"Invalid response from iGameVis: {str(e)}")
        except Exception as e:
            logger.error(f"Error communicating with iGameVis: {str(e)}")
            self.sock = None
            raise Exception(f"Communication error with iGameVis: {str(e)}")

# ============================================================================
# Server Lifecycle Management
# ============================================================================

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Manage server startup and shutdown lifecycle"""
    try:
        logger.info("iGameVisMCP server starting up")
        
        # Try to connect to iGameVis on startup to verify it's available
        try:
            igamevis = get_igamevis_connection()
            logger.info("Successfully connected to iGameVis on startup")
        except Exception as e:
            logger.warning(f"Could not connect to iGameVis on startup: {str(e)}")
            logger.warning("Make sure iGameVis is running before using the tools")
        
        yield {}
    finally:
        # Clean up the global connection on shutdown
        global _igamevis_connection
        if _igamevis_connection:
            logger.info("Disconnecting from iGameVis on shutdown")
            _igamevis_connection.disconnect()
            _igamevis_connection = None
        logger.info("iGameVisMCP server shut down")

# Create the MCP server with lifespan support
mcp = FastMCP(
    "iGameVisMCP",
    lifespan=server_lifespan
)

# Global connection for resources and tools
_igamevis_connection = None

def get_igamevis_connection():
    """Get or create a persistent iGameVis connection"""
    global _igamevis_connection
    
    # If we have an existing connection, check if it's still valid
    if _igamevis_connection is not None:
        try:
            # Send a ping command to check if connection is alive
            result = _igamevis_connection.send_command("ping")
            return _igamevis_connection
        except Exception as e:
            # Connection is dead, close it and create a new one
            logger.warning(f"Existing connection is no longer valid: {str(e)}")
            try:
                _igamevis_connection.disconnect()
            except:
                pass
            _igamevis_connection = None
    
    # Create a new connection if needed
    if _igamevis_connection is None:
        host = os.getenv("IGAMEVIS_HOST", DEFAULT_HOST)
        port = int(os.getenv("IGAMEVIS_PORT", DEFAULT_PORT))
        _igamevis_connection = iGameVisConnection(host=host, port=port)
        if not _igamevis_connection.connect():
            logger.error("Failed to connect to iGameVis")
            _igamevis_connection = None
            raise Exception("Could not connect to iGameVis. Make sure iGameVis is running.")
        logger.info("Created new persistent connection to iGameVis")
    
    return _igamevis_connection

# ============================================================================
# Utility Functions
# ============================================================================

def get_desktop_path() -> Path:
    """Get the desktop path for the current user."""
    return Path(os.path.join(os.path.expanduser('~'), 'Desktop'))

def normalize_view_type(view_type: str) -> str:
    """
    标准化视角类型，支持多种表示方法
    Args:
        view_type (str): 输入的视角类型
    Returns:
        str: 标准化后的视角类型，如果无效则返回空字符串
    """
    if not view_type:
        return ""

    # 转换为小写并去除空格
    view_type = view_type.lower().strip()

    # 视角映射表：输入 -> 标准输出
    view_mapping = {
        # 标准名称
        "reset": "reset",
        "front": "front",
        "back": "back",
        "left": "left",
        "right": "right",
        "top": "top",
        "bottom": "bottom",
        "isometric": "isometric",

        # 坐标轴简写形式
        "+x": "right",      # 正X方向 = 右视图
        "-x": "left",       # 负X方向 = 左视图
        "+y": "top",        # 正Y方向 = 顶视图
        "-y": "bottom",     # 负Y方向 = 底视图
        "+z": "back",       # 正Z方向 = 后视图
        "-z": "front",      # 负Z方向 = 前视图

        # 完整坐标轴表示
        "positive_x": "right",
        "negative_x": "left",
        "positive_y": "top",
        "negative_y": "bottom",
        "positive_z": "back",
        "negative_z": "front",

        # 其他常见别名
        "pos_x": "right",
        "neg_x": "left",
        "pos_y": "top",
        "neg_y": "bottom",
        "pos_z": "back",
        "neg_z": "front",

        # 中文支持
        "正x": "right",
        "负x": "left",
        "正y": "top",
        "负y": "bottom",
        "正z": "back",
        "负z": "front",
        "重置": "reset",
        "前视": "front",
        "后视": "back",
        "左视": "left",
        "右视": "right",
        "顶视": "top",
        "底视": "bottom",
        "等轴测": "isometric"
    }

    return view_mapping.get(view_type, "")


def format_tool_result(result: Dict[str, Any], default_message: str = "Operation completed successfully") -> str:
    """
    统一格式化工具返回结果
    
    Args:
        result: iGameVis 返回的结果字典，格式：{"type": "success"|"failure", "content": str, "timestamp": str}
        default_message: 默认成功消息
    
    Returns:
        格式化后的字符串结果
    
    Note:
        iGameVis (C++) 返回格式固定：
        - type: "success" (成功) 或 "failure" (失败)
        - content: QString，总是字符串类型（即使内容是 JSON 格式也是字符串）
        - timestamp: ISO 格式时间戳
    """
    result_type = result.get("type", "unknown")
    content = result.get("content", default_message)
    logger.info(f"result_type: {result_type}, content: {content}")
    # 如果是失败类型，添加错误前缀让 AI 明确知道操作失败
    if result_type == "failure":
        return f"❌ Operation processed with failure: {content}"
    else:
        # 成功情况，直接返回内容
        return content

def parse_igamevis_result_with_images(result: Dict[str, Any], default_message: str = "Operation completed successfully") -> List[Any]:
    """
    解析 iGameVis 返回结果，自动提取文本和图像（通用函数）
    
    Args:
        result: iGameVis 返回的结果字典
        default_message: 默认成功消息
    
    Returns:
        List[TextContent | ImageContent]: MCP 标准内容列表
    
    Note:
        图像字段命名规范（固定格式）：
        - 单图：image_base64
        - 多图：image_base64_0, image_base64_1, image_base64_2, ...
    """
    from mcp.types import TextContent, ImageContent
    
    result_type = result.get("type", "unknown")
    content = result.get("content", default_message)
    
    # 失败情况
    if result_type == "failure":
        return [TextContent(type="text", text=f"❌ 操作失败: {content}")]
    
    # 尝试解析 JSON
    try:
        content_json = json.loads(content)
        if not isinstance(content_json, dict):
            return [TextContent(type="text", text=str(content))]
        
        contents = []
        images = []  # [(key, base64), ...]
        text_parts = []
        
        # 分离文本和图像字段
        for key, value in content_json.items():
            if key == "image_base64" or key.startswith("image_base64_"):
                # 图像字段
                if value:
                    # 移除 data URI 前缀（如果有）
                    base64_data = value.split(",", 1)[1] if value.startswith("data:") else value
                    images.append((key, base64_data))
            else:
                # 文本字段
                text_parts.append(f"{key}: {json.dumps(value, ensure_ascii=False) if isinstance(value, (dict, list)) else value}")
        
        # 添加文本
        if text_parts:
            contents.append(TextContent(type="text", text="\n".join(text_parts)))
        
        # 添加图像（按字段名排序）
        for key, base64_data in sorted(images):
            contents.append(ImageContent(type="image", data=base64_data, mimeType="image/png"))
            logger.info(f"添加图像: {key}")
        
        return contents if contents else [TextContent(type="text", text=content)]
        
    except (json.JSONDecodeError, TypeError):
        return [TextContent(type="text", text=str(content))]

# ============================================================================
# File Operations Tools
# ============================================================================

@mcp.tool()
def get_user_desktop_path() -> str:
    """Get the desktop path for the current user."""
    try:
        desktop_path = get_desktop_path()
        return f"桌面路径: {str(desktop_path)}"
    except Exception as e:
        return f"获取桌面路径时发生错误: {e}"

@mcp.tool()
def find_desktop_files(extensions: list) -> str:
    """Find files with specified extensions on the desktop.
    Args:
        extensions (list): List of file extensions to search for (e.g. ['txt', 'pdf', 'doc'])
    Returns:
        str: A formatted string containing the count and list of found files with their full paths
    """
    desktop_path = get_desktop_path()
    try:
        search_patterns = [f'*.{ext.lower().strip(".")}' for ext in extensions]
        matching_files = []
        for pattern in search_patterns:
            matching_files.extend(list(desktop_path.glob(pattern)))

        if not matching_files:
            return f"No files with extensions {extensions} found on desktop."

        files_by_ext = {}
        for file in matching_files:
            ext = file.suffix.lower().strip('.')
            if ext not in files_by_ext:
                files_by_ext[ext] = []
            files_by_ext[ext].append(file)

        output = [f"Found {len(matching_files)} files:"]
        for ext, files in files_by_ext.items():
            output.append(f"\n{ext.upper()} files ({len(files)}):")
            for file in sorted(files, key=lambda x: x.name):
                output.append(f"- {file.name} (路径: {str(file)})")

        return "\n".join(output)
    except Exception as e:
        return f"Error accessing desktop: {e}"

@mcp.tool()
def find_files_in_path(directory_path: str, extensions: list = None) -> str:
    """Find files in a specified directory path.
    Args:
        directory_path (str): The directory path to search in
        extensions (list, optional): List of file extensions to search for
    Returns:
        str: A formatted string containing the count and list of found files with their full paths
    """
    try:
        search_path = Path(directory_path)
        
        if not search_path.exists():
            return f"Directory '{directory_path}' does not exist."
        
        if not search_path.is_dir():
            return f"'{directory_path}' is not a directory."
        
        if extensions is None:
            matching_files = [f for f in search_path.iterdir() if f.is_file()]
        else:
            search_patterns = [f'*.{ext.lower().strip(".")}' for ext in extensions]
            matching_files = []
            for pattern in search_patterns:
                matching_files.extend(list(search_path.glob(pattern)))
        
        if not matching_files:
            if extensions:
                return f"No files with extensions {extensions} found in '{directory_path}'."
            else:
                return f"No files found in '{directory_path}'."
        
        if extensions:
            files_by_ext = {}
            for file in matching_files:
                ext = file.suffix.lower().strip('.')
                if ext not in files_by_ext:
                    files_by_ext[ext] = []
                files_by_ext[ext].append(file)
            
            output = [f"Found {len(matching_files)} files in '{directory_path}':"]
            for ext, files in files_by_ext.items():
                output.append(f"\n{ext.upper()} files ({len(files)}):")
                for file in sorted(files, key=lambda x: x.name):
                    output.append(f"- {file.name} (路径: {str(file).replace('\\', '/')})")
        else:
            output = [f"Found {len(matching_files)} files in '{directory_path}':"]
            for file in sorted(matching_files, key=lambda x: x.name):
                output.append(f"- {file.name} (路径: {str(file).replace('\\', '/')})")
        
        return "\n".join(output)
        
    except Exception as e:
        return f"Error accessing directory '{directory_path}': {e}"

@mcp.tool()
def get_desktop_file_path(filename: str) -> str:
    """Get the full path of a specific file on the desktop.
    Args:
        filename (str): The name of the file to search for (can be partial match)
    Returns:
        str: Full path of the file if found, or error message if not found
    """
    desktop_path = get_desktop_path()
    
    try:
        exact_file = desktop_path / filename
        if exact_file.exists():
            return str(exact_file).replace("\\", "/")
        
        matching_files = []
        for file in desktop_path.iterdir():
            if file.is_file() and filename.lower() in file.name.lower():
                matching_files.append(file)
        
        if not matching_files:
            return f"File '{filename}' not found on desktop."
        elif len(matching_files) == 1:
            return str(matching_files[0]).replace("\\", "/")
        else:
            file_list = '\n'.join([f"- {file.name}: {str(file).replace('\\', '/')}" for file in matching_files])
            return f"Multiple files matching '{filename}' found:\n{file_list}"
            
    except Exception as e:
        return f"Error searching for file: {e}"

@mcp.tool()
def open_file_with_path(file_path: str) -> str:
    """Open a file in the iGameVis application by providing its full path.
    Args:
        file_path (str): Full path to the file to open
    Returns:
        str: Result message from iGameVis or error message
    """
    try:
        file_path_obj = Path(file_path)
        if not file_path_obj.exists():
            return f"Error: File '{file_path}' does not exist."
        
        if not file_path_obj.is_file():
            return f"Error: '{file_path}' is not a file."
        
        # Send command to iGameVis
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("open_file", {
            "file_path": str(file_path_obj.absolute()).replace("\\", "/"),
            "file_name": file_path_obj.name,
            "file_extension": file_path_obj.suffix.lower()
        })
        
        return format_tool_result(result, "File opened successfully")
        
    except Exception as e:
        return f"Error opening file: {e}"

@mcp.tool()
def open_desktop_file(filename: str) -> str:
    """Find and open a specific file from the desktop in the iGameVis application.
    Args:
        filename (str): EXACT name of the file to find and open
    Returns:
        str: Result message from iGameVis or error message
    """
    desktop_path = get_desktop_path()
    
    try:
        exact_file = desktop_path / filename
        if exact_file.exists():
            return open_file_with_path(str(exact_file).replace("\\", "/"))
        
        matching_files = []
        for file in desktop_path.iterdir():
            if file.is_file() and filename.lower() in file.name.lower():
                matching_files.append(file)
        
        if not matching_files:
            return f"Error: File '{filename}' not found on desktop."
        elif len(matching_files) == 1:
            return open_file_with_path(str(matching_files[0]).replace("\\", "/"))
        else:
            file_list = '\n'.join([f"- {file.name}: {str(file).replace('\\', '/')}" for file in matching_files])
            return f"Multiple files matching '{filename}' found:\n{file_list}\n\nPlease use 'open_file_with_path' with the specific file path."
            
    except Exception as e:
        return f"Error searching for file: {e}"

@mcp.tool()
def open_file(file_path_or_name: str) -> str:
    """Open a file in the iGameVis application. This is the primary tool for opening files.
    Args:
        file_path_or_name (str): EXACT filename or full file path
    Returns:
        str: Result message from iGameVis or error message
    """
    try:
        file_path_obj = Path(file_path_or_name)
        if file_path_obj.exists() and file_path_obj.is_file():
            return open_file_with_path(str(file_path_obj).replace("\\", "/"))
        
        return open_desktop_file(file_path_or_name)
        
    except Exception as e:
        return f"Error opening file: {e}"

# ============================================================================
# Model Information Tools
# ============================================================================

@mcp.tool()
def get_model_info() -> List[Any]:
    """获取当前模型信息（包含描述和图像）"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("get_model_info", {})
        
        # 使用通用函数自动解析文本和图像
        return parse_igamevis_result_with_images(result, "No model information available")
    except Exception as e:
        from mcp.types import TextContent
        return [TextContent(type="text", text=f"Error getting model info: {e}")]



# ============================================================================
# Camera Control Tools
# ============================================================================

@mcp.tool()
def camera_control(
    control_type: str,
    view_type: str = "reset",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    factor: float = 1.0,
    angle_x: float = 0.0,
    angle_y: float = 0.0,
    angle_z: float = 0.0
) -> str:
    """相机控制工具 - 统一的相机操作接口，支持专业坐标轴表示法

    特别适用于开发人员使用专业术语，如：
    - 用户说"切换到+X视角" → 使用 view_type="+x"
    - 用户说"看-Z方向" → 使用 view_type="-z"
    - 用户说"正Y视图" → 使用 view_type="+y"

    Args:
        control_type (str): 控制类型，可选值：
            - "view": 视角控制，需要 view_type 参数
            - "position": 设置相机位置，需要 x, y, z 参数
            - "target": 设置相机目标点，需要 x, y, z 参数
            - "zoom": 缩放相机，需要 factor 参数
            - "rotate": 旋转相机，需要 angle_x, angle_y, angle_z 参数
        view_type (str): 视角类型，用于 control_type="view"，支持多种表示方法：
            标准视角名称：
            - "reset": 重置相机视角到默认位置
            - "front": 前视图（-Z方向）
            - "back": 后视图（+Z方向）
            - "left": 左视图（-X方向）
            - "right": 右视图（+X方向）
            - "top": 顶视图（+Y方向）
            - "bottom": 底视图（-Y方向）
            - "isometric": 等轴测视图
            专业坐标轴表示法：
            - "+x", "positive_x": 正X方向视图（右视图）
            - "-x", "negative_x": 负X方向视图（左视图）
            - "+y", "positive_y": 正Y方向视图（顶视图）
            - "-y", "negative_y": 负Y方向视图（底视图）
            - "+z", "positive_z": 正Z方向视图（后视图）
            - "-z", "negative_z": 负Z方向视图（前视图）
        x (float): X坐标，用于 control_type="position" 或 "target"
        y (float): Y坐标，用于 control_type="position" 或 "target"
        z (float): Z坐标，用于 control_type="position" 或 "target"
        factor (float): 缩放因子，用于 control_type="zoom"，>1为放大，<1为缩小
        angle_x (float): X轴旋转角度（度），用于 control_type="rotate"
        angle_y (float): Y轴旋转角度（度），用于 control_type="rotate"
        angle_z (float): Z轴旋转角度（度），用于 control_type="rotate"
    Returns:
        str: Result message from iGameVis or error message
    """
    valid_control_types = ["view", "position", "target", "zoom", "rotate"]

    if control_type not in valid_control_types:
        return f"Error: Invalid control type '{control_type}'. Valid types are: {', '.join(valid_control_types)}"

    # 构建数据对象
    data = {"control_type": control_type}

    if control_type == "view":
        # 标准化视角类型，支持多种表示方法
        normalized_view_type = normalize_view_type(view_type)
        if not normalized_view_type:
            valid_examples = [
                "标准名称: reset, front, back, left, right, top, bottom, isometric",
                "坐标轴表示: +x, -x, +y, -y, +z, -z",
                "完整表示: positive_x, negative_x, positive_y, negative_y, positive_z, negative_z"
            ]
            return f"Error: Invalid view type '{view_type}'. Valid formats:\n" + "\n".join(valid_examples)
        data["view_type"] = normalized_view_type

    elif control_type == "position":
        data.update({"x": x, "y": y, "z": z})

    elif control_type == "target":
        data.update({"x": x, "y": y, "z": z})

    elif control_type == "zoom":
        if factor <= 0:
            return "Error: Zoom factor must be greater than 0"
        data["factor"] = factor

    elif control_type == "rotate":
        data.update({"angle_x": angle_x, "angle_y": angle_y, "angle_z": angle_z})

    try:
        # Send command to iGameVis
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("camera_control", data)
        
        return format_tool_result(result, "Camera control executed successfully")
    except Exception as e:
        return f"Error controlling camera: {e}"

# ============================================================================
# File Operations Tools
# ============================================================================

@mcp.tool()
def save_file_as(file_path: str = "", file_name: str = "") -> str:
    """另存为文件到指定路径
    Args:
        file_path (str): 保存文件的完整路径
        file_name (str): 文件名（可选，如果file_path已包含文件名）
    """
    try:
        data = {}
        if file_path:
            data["file_path"] = file_path
        if file_name:
            data["file_name"] = file_name
            
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("save_file_as", data)
        
        return format_tool_result(result, "File saved successfully")
    except Exception as e:
        return f"Error saving file: {e}"

@mcp.tool()
def save_screenshot(file_path: str, width: int = DEFAULT_SCREENSHOT_WIDTH, height: int = DEFAULT_SCREENSHOT_HEIGHT) -> str:
    """保存屏幕截图
    Args:
        file_path (str): 保存截图的完整路径
        width (int): 截图宽度，默认1920
        height (int): 截图高度，默认1080
    """
    try:
        data = {
            "file_path": file_path,
            "width": width,
            "height": height
        }
        
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("save_screenshot", data)
        
        return format_tool_result(result, "Screenshot saved successfully")
    except Exception as e:
        return f"Error saving screenshot: {e}"

# ============================================================================
# View and Display Control Tools
# ============================================================================

@mcp.tool()
def change_background_color(r: int, g: int, b: int) -> str:
    """更改背景颜色
    Args:
        r (int): 红色分量 (0-255)
        g (int): 绿色分量 (0-255)
        b (int): 蓝色分量 (0-255)
    """
    try:
        data = {
            "r": max(0, min(255, r)),
            "g": max(0, min(255, g)),
            "b": max(0, min(255, b))
        }
        
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("change_background_color", data)
        
        return format_tool_result(result, "Background color changed successfully")
    except Exception as e:
        return f"Error changing background color: {e}"

@mcp.tool()
def toggle_colorbar() -> str:
    """切换颜色条的显示/隐藏"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("toggle_colorbar", {})
        
        return format_tool_result(result, "Colorbar toggled successfully")
    except Exception as e:
        return f"Error toggling colorbar: {e}"

@mcp.tool()
def change_camera_type(camera_type: str) -> str:
    """更改相机类型
    Args:
        camera_type (str): 相机类型，可选值：
            - "orthographic": 正交投影
            - "perspective": 透视投影
    """
    valid_types = ["orthographic", "perspective"]
    if camera_type.lower() not in valid_types:
        return f"Error: Invalid camera type '{camera_type}'. Valid types are: {', '.join(valid_types)}"

    try:
        data = {"camera_type": camera_type.lower()}
        
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("change_camera_type", data)
        
        return format_tool_result(result, "Camera type changed successfully")
    except Exception as e:
        return f"Error changing camera type: {e}"

# ============================================================================
# Model Operations Tools
# ============================================================================

@mcp.tool()
def delete_current_model() -> str:
    """删除当前选中的模型"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("delete_current_model", {})
        
        return format_tool_result(result, "Model deleted successfully")
    except Exception as e:
        return f"Error deleting model: {e}"

@mcp.tool()
def show_model_tree() -> str:
    """显示模型树窗口"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("show_model_tree", {})
        
        return format_tool_result(result, "Model tree window shown")
    except Exception as e:
        return f"Error showing model tree: {e}"

@mcp.tool()
def show_scalar_field() -> str:
    """显示标量场窗口"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("show_scalar_field", {})
        
        return format_tool_result(result, "Scalar field window shown")
    except Exception as e:
        return f"Error showing scalar field: {e}"

@mcp.tool()
def show_vector_field() -> str:
    """显示矢量场窗口"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("show_vector_field", {})
        
        return format_tool_result(result, "Vector field window shown")
    except Exception as e:
        return f"Error showing vector field: {e}"

@mcp.tool()
def show_tensor_field() -> str:
    """显示张量场窗口"""
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("show_tensor_field", {})
        
        return format_tool_result(result, "Tensor field window shown")
    except Exception as e:
        return f"Error showing tensor field: {e}"

# ============================================================================
# Interaction Mode Tools
# ============================================================================

@mcp.tool()
def change_interaction_mode(mode: str) -> str:
    """更改交互模式
    Args:
        mode (str): 交互模式，可选值：
            - "basic": 基本模式（默认）
            - "point_selection": 点选择模式
            - "face_selection": 面选择模式
    """
    valid_modes = ["basic", "point_selection", "face_selection"]
    if mode.lower() not in valid_modes:
        return f"Error: Invalid interaction mode '{mode}'. Valid modes are: {', '.join(valid_modes)}"

    try:
        data = {"mode": mode.lower()}
        
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("change_interaction_mode", data)
        
        return format_tool_result(result, "Interaction mode changed successfully")
    except Exception as e:
        return f"Error changing interaction mode: {e}"

# ============================================================================
# Algorithm Processing Tools
# ============================================================================

@mcp.tool()
def get_model_eight_views(image_size: dict = None, quality: str = DEFAULT_IMAGE_QUALITY) -> str:
    """
    获取当前模型的八个视角图像
    
    Args:
        image_size (dict): 图像尺寸，格式: {"width": 800, "height": 600}
        quality (str): 图像质量，"high" 或 "normal"
    
    Returns:
        str: 操作结果信息
    """
    # 设置默认参数
    if image_size is None:
        image_size = {"width": 800, "height": 600}
    
    # 验证参数
    if not isinstance(image_size, dict) or "width" not in image_size or "height" not in image_size:
        return "错误：image_size 必须包含 width 和 height"
    
    if quality not in ["high", "normal"]:
        return "错误：quality 必须是 'high' 或 'normal'"
    
    try:
        data = {
            "image_size": image_size,
            "quality": quality
        }
        
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("get_model_eight_views", data)
        
        return format_tool_result(result, "Eight views captured successfully")
    except Exception as e:
        return f"Error capturing eight views: {e}"

@mcp.tool()
def apply_mesh_filter(filter_type: str, **parameters) -> str:
    """应用网格处理算法
    Args:
        filter_type (str): 算法类型，可选值：
            - "curvature": 计算曲率
            - "gradient": 计算梯度
            - "simplification": 网格简化
            - "smoothing": 拉普拉斯平滑
        **parameters: 算法参数（根据不同算法类型而定）
    """
    valid_filters = ["curvature", "gradient", "simplification", "smoothing"]
    if filter_type.lower() not in valid_filters:
        return f"Error: Invalid filter type '{filter_type}'. Valid types are: {', '.join(valid_filters)}"

    try:
        data = {"filter_type": filter_type.lower()}
        # 添加参数
        for key, value in parameters.items():
            data[key] = value

        igamevis = get_igamevis_connection()
        result = igamevis.send_command("apply_mesh_filter", data)
        
        return format_tool_result(result, "Mesh filter applied successfully")
    except Exception as e:
        return f"Error applying mesh filter: {e}"

# ============================================================================
# Screenshot Tool
# ============================================================================

@mcp.tool()
def get_viewport_screenshot(max_size: int = 800) -> Image:
    """
    Capture a screenshot of the current iGameVis viewport.
    
    Parameters:
    - max_size: Maximum size in pixels for the largest dimension (default: 800)
    
    Returns the screenshot as an Image.
    """
    try:
        # Create temp file path
        temp_dir = tempfile.gettempdir()
        temp_path = os.path.join(temp_dir, f"igamevis_screenshot_{os.getpid()}.png")
        
        # Send command to iGameVis to save screenshot
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("save_screenshot", {
            "file_path": temp_path.replace("\\", "/"),
            "width": max_size,
            "height": max_size
        })
        
        # Check if screenshot was saved successfully
        if result.get("type") == "failure":
            raise Exception(result.get("content", "Unknown error"))
        
        # Wait a moment for file to be written
        import time
        time.sleep(0.5)
        
        if not os.path.exists(temp_path):
            raise Exception("Screenshot file was not created")
        
        # Read the file
        with open(temp_path, 'rb') as f:
            image_bytes = f.read()
        
        # Delete the temp file
        os.remove(temp_path)
        
        return Image(data=image_bytes, format="png")
        
    except Exception as e:
        logger.error(f"Error capturing screenshot: {str(e)}")
        raise Exception(f"Screenshot failed: {str(e)}")

# ============================================================================
# Prompts for AI Assistant
# ============================================================================

@mcp.prompt()
def igamevis_assistant_guide() -> str:
    """General guidance for iGameVis AI assistant"""
    return """You are an intelligent assistant for iGameVis, a professional 3D visualization application used in engineering, scientific research, and technical analysis.

## Your Role
- Help users operate iGameVis efficiently and professionally
- Provide expert guidance on 3D visualization workflows
- Explain technical concepts in an accessible way
- Troubleshoot issues systematically
- Suggest best practices for different use cases

## Core Principles
1. **User-Centric**: Always prioritize user needs and goals
2. **Professional**: Maintain technical accuracy and professional standards
3. **Educational**: Explain concepts and reasoning behind recommendations
4. **Efficient**: Suggest optimal workflows and time-saving approaches
5. **Adaptive**: Adjust recommendations based on user expertise level and domain

## General Workflow Approach
1. **Understand the Task**: Clarify user goals and context
2. **Assess the Data**: Analyze model characteristics and requirements
3. **Plan the Approach**: Suggest systematic steps to achieve goals
4. **Execute Efficiently**: Use appropriate tools and settings
5. **Validate Results**: Verify outputs and suggest improvements
6. **Document Process**: Help users save and document their work

## Domain Expertise
- **Engineering/CAD**: Focus on precision, measurements, and technical accuracy
- **Scientific Simulation**: Emphasize data validation and physical interpretation
- **Biomedical**: Consider anatomical conventions and medical standards
- **Architecture**: Balance aesthetics with structural considerations

## Communication Style
- Use clear, professional language
- Explain technical terms when necessary
- Provide step-by-step guidance
- Offer alternatives when appropriate
- Anticipate follow-up questions

## Problem-Solving Approach
- Start with simple solutions before complex ones
- Verify basic prerequisites (file existence, format compatibility, etc.)
- Consider system resources and performance limitations
- Suggest preventive measures for future issues
- Provide educational context for solutions

Remember: Your goal is to make iGameVis accessible and powerful for users across different domains and expertise levels."""

# ============================================================================
# Main Execution
# ============================================================================

def main():
    """Run the MCP server"""
    logger.info("Starting iGameVis MCP Server...")
    logger.info(f"Configuration: Host={DEFAULT_HOST}, Port={DEFAULT_PORT}")
    logger.info(f"Timeouts: Connection={CONNECTION_TIMEOUT}s, Command={COMMAND_TIMEOUT}s")
    mcp.run()

if __name__ == "__main__":
    main()
