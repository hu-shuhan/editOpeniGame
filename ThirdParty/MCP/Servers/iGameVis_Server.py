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


@mcp.tool()
def get_current_attribute() -> List[Any]:
    """
    获取当前绘制的云图属性的详细信息

    返回当前正在显示的属性（云图）的完整信息，包括：
    - 属性名称
    - 属性类型（标量/矢量/张量）
    - 属性维度信息
    - 当前绘制的维度
    - 数值范围
    - 所属（点/单元）
    - 数据数量

    使用场景：
    - 查询当前云图显示的是什么属性
    - 了解属性的数值范围
    - 查看属性的维度信息
    - 分析属性的详细特征

    Returns:
        包含属性详细信息的文本描述
    """
    try:
        igamevis = get_igamevis_connection()
        result = igamevis.send_command("get_current_attribute", {})

        # 使用通用函数自动解析文本和图像
        return parse_igamevis_result_with_images(result, "No current attribute information available")
    except Exception as e:
        from mcp.types import TextContent
        return [TextContent(type="text", text=f"Error getting current attribute: {e}")]


# ============================================================================
# Camera Control Tools
# ============================================================================

@mcp.tool()
def camera_control(
    control_type: str,
    view_type: str = "",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    factor: float = 1.0,
    angle_x: float = 0.0,
    angle_y: float = 0.0,
    angle_z: float = 0.0,
    angle: float = 0.0,  # 屏幕相对旋转使用
) -> str:
    """
    相机控制工具（Camera Control）

    参数说明：
        control_type:
            - "view"          切换视角（需要 view_type）
            - "position"      设置相机位置（需要 x,y,z）
            - "target"        设置相机焦点（需要 x,y,z）
            - "zoom"          缩放相机（需要 factor）
            - "rotate"        绝对旋转（需要 angle_x, angle_y, angle_z）
            - "rotate_screen" 相对旋转，视角顺时针旋转的角度（angle [-180,180]）

        view_type:
            当 control_type="view" 时使用，取值：
            "reset", "front", "back", "left", "right", "top", "bottom", "isometric"

        x, y, z:
            位置或目标坐标，仅当 control_type 为 "position" 或 "target" 时使用。

        factor:
            缩放因子，>1 表示放大，<1 表示缩小，仅当 control_type="zoom" 时使用。

        angle_x, angle_y, angle_z:
            绝对旋转角度，仅当 control_type="rotate" 时使用。

        angle:
            相对当前视角的顺时针旋转角度，仅当 control_type="rotate_screen" 时使用。
            按用户语义直接取值，逆时针需要取反，并放缩到[-180,180]

    💡 AI 使用指南：
        - 只选择 control_type 和对应参数。
        - 屏幕旋转仅输出 angle，绝对旋转仅输出 angle_x/y/z。
        - 不推理或映射视角词，直接输出 view_type。
        - 仅返回有效参数组合。

    示例：
        "切换到前视图" → {"control_type": "view", "view_type": "front"}
        "重置相机"     → {"control_type": "view", "view_type": "reset"}
        "移动到 (10,5,0)" → {"control_type": "position", "x": 10, "y": 5, "z": 0}
        "放大两倍"     → {"control_type": "zoom", "factor": 2.0}
        "绕Z轴旋转90°" → {"control_type": "rotate", "angle_x": 0, "angle_y": 0, "angle_z": 90}
        "顺时针旋转90°" → {"control_type": "rotate_screen", "angle": 90}
        "逆时针旋转90°" → {"control_type": "rotate_screen", "angle": -90}
    """
    try:
        igamevis = get_igamevis_connection()
        data = {
            "control_type": control_type,
            "view_type": view_type,
            "x": x,
            "y": y,
            "z": z,
            "factor": factor,
            "angle_x": angle_x,
            "angle_y": angle_y,
            "angle_z": angle_z,
            "angle": angle
        }
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

@mcp.tool()
def apply_mesh_clip_filter(    
    pos_x: float = 0.0,
    pos_y: float = 0.0,
    pos_z: float = 0.0,
    normal_x: float = 0.0,
    normal_y: float = 0.0,
    normal_z: float = 0.0
) -> str:
    """对当前网格应用裁剪滤波器（Clip Filter）

    Args:
        pos_x (float): 裁剪平面上的点 X 坐标
        pos_y (float): 裁剪平面上的点 Y 坐标
        pos_z (float): 裁剪平面上的点 Z 坐标
        normal_x (float): 裁剪平面的法向量 X 分量
        normal_y (float): 裁剪平面的法向量 Y 分量
        normal_z (float): 裁剪平面的法向量 Z 分量
    """
    try:
        # 组织命令参数
        data = {
            "pos_x": pos_x,
            "pos_y": pos_y,
            "pos_z": pos_z,
            "normal_x": normal_x,
            "normal_y": normal_y,
            "normal_z": normal_z,
        }

        # 获取 iGameVis 连接实例
        igamevis = get_igamevis_connection()

        # 发送命令
        result = igamevis.send_command("apply_mesh_clip_filter", data)

        # 格式化并返回结果
        return format_tool_result(result, "Mesh clip filter applied successfully")

    except Exception as e:
        return f"Error applying mesh clip filter: {e}"

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
