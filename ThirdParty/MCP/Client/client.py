import asyncio
import os
import json
import sys
import time
from http.client import responses
from typing import Optional, List
from contextlib import AsyncExitStack
from datetime import datetime
import re
from openai import OpenAI
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
import logging

# 导入配置
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config

# 配置日志
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format=config.LOG_FORMAT
)
logger = logging.getLogger(__name__)

# 关闭MCP相关的调试日志
logging.getLogger('mcp').setLevel(logging.WARNING)
logging.getLogger('mcp.client').setLevel(logging.WARNING)
logging.getLogger('mcp.server').setLevel(logging.WARNING)

class MCPClient:
    """MCPClient manages connections to MCP server."""

    def __init__(self):
        self.name: str = "MyTest"
        self.exit_stack = AsyncExitStack()
        
        # 使用config.py中的配置
        self.base_url = config.BASE_URL
        self.model = config.MODEL
        self.openai_api_key = config.API_KEY
        
        if not self.openai_api_key:
            raise RuntimeError("API_KEY not set in config.py")
            
        self.client = OpenAI(
            api_key=self.openai_api_key,
            base_url=self.base_url
        )
        self.session: Optional[ClientSession] = None
        self._cleanup_lock: asyncio.Lock = asyncio.Lock()
        # 添加对话历史记录
        self.conversation_history = []
        # 创建会话文件夹
        self.session_folder = self._create_session_folder()
        # 缓存工具列表，避免重复获取
        self._cached_tools = None
        self._cached_response = None

    def _create_session_folder(self) -> str:
        """创建新的会话文件夹"""
        # 获取项目根目录路径
        current_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(current_dir)  # 上一级目录就是项目根目录
        base_dir = os.path.join(project_root, "history")
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        session_folder = os.path.join(base_dir, f"session_{timestamp}")
        os.makedirs(session_folder, exist_ok=True)
        return session_folder

    def _generate_filename(self, query: str, index: int) -> str:
        """生成文件名"""
        # 清理查询文本
        safe_query = clean_filename(query)
        
        # 如果清理后的文本为空或太短，使用默认名称
        if len(safe_query) < 3:
            safe_query = "question"
        
        # 限制长度
        safe_query = safe_query[:20]  # 减少长度避免路径问题
        
        # 添加序号和时间戳
        timestamp = datetime.now().strftime('%H%M%S')
        return f"{index:03d}_{safe_query}_{timestamp}.txt"

    async def connect_to_server(self, server_script_path: str):
        is_python = server_script_path.endswith(".py")
        if not is_python:
            raise ValueError(f"{server_script_path} is not a Python file")

        # 获取服务器脚本的绝对路径
        server_script_path = os.path.abspath(server_script_path)
        if not os.path.exists(server_script_path):
            raise FileNotFoundError(f"Server script not found: {server_script_path}")

        # 获取服务器脚本所在目录
        server_dir = os.path.dirname(server_script_path)
        
        # 设置 Python 解释器路径
        if os.name == 'nt':  # Windows
            python_path = sys.executable
        else:
            python_path = 'python'

        # 设置环境变量
        env = os.environ.copy()  # 复制当前环境变量
        env["PYTHONPATH"] = server_dir  # 添加服务器目录到 Python 路径
        if "PYTHONIOENCODING" not in env:
            env["PYTHONIOENCODING"] = "utf-8"  # 确保正确的编码

        server_params = StdioServerParameters(
            command=python_path,
            args=["-u", server_script_path],  # 添加 -u 参数，禁用输出缓冲
            env=env,
            cwd=server_dir
        )

        try:
            print(f"\n正在启动服务器: {server_script_path}")
            print(f"工作目录: {server_dir}")
            print(f"Python 解释器: {python_path}")

            stdio_transport = await self.exit_stack.enter_async_context(
                stdio_client(server_params)
            )
            self.stdio, self.write = stdio_transport
            self.session = await self.exit_stack.enter_async_context(
                ClientSession(self.stdio, self.write)
            )

            # print("正在初始化服务器连接...")
            await self.session.initialize()

            response = await self.session.list_tools()
            tools = response.tools
            # print("\n已连接到服务器，支持以下工具:", [tool.name for tool in tools])

        except Exception as e:
            print(f"\n连接错误: {e}")
            if hasattr(e, '__cause__') and e.__cause__:
                print(f"原因: {e.__cause__}")
            await self.cleanup()
            raise



    async def process_query(self, query: str, images: list = None) -> str:
        """处理查询，按照简化的逻辑：
        1. 不需要调用工具 -> 直接让AI回答
        2. 需要调用工具且返回command -> 直接返回JSON
        3. 需要调用工具且返回非command -> 结合工具结果让AI回答
        """
        try:
            # 检查session是否已连接
            if not self.session:
                raise RuntimeError("MCP session not initialized")
            
            # 获取可用工具列表（使用缓存避免重复连接）
            if self._cached_tools is None:
                response = await self.session.list_tools()
                self._cached_response = response
                self._cached_tools = [
                    {
                        "type": "function",
                        "function": {
                            "name": tool.name,
                            "description": tool.description,
                            "parameters": tool.inputSchema
                        }
                    } for tool in response.tools
                ]
            else:
                response = self._cached_response
            available_tools = self._cached_tools

            # 构建消息
            messages = []
            
            # 添加系统消息
            tools_description = []
            for tool in response.tools:
                name = tool.name
                desc = tool.description.split('.')[0] if '.' in tool.description else tool.description
                tools_description.append(f"- 使用 {name} 工具可以{desc}")

            system_message = {
                "role": "system",
                "content": (
                    "你是一个智能的3D模型处理助手，可以帮助用户操作和分析3D模型。\n"
                    "你有以下可用的工具：\n"
                    f"{chr(10).join(tools_description)}\n\n"
                    "## 重要行为准则：\n"
                    "- **不要**只是告诉用户如何使用工具，而是**直接调用工具**为用户完成任务\n"
                    "- **不要**提供操作步骤说明，而是**直接执行操作**\n\n"
                    "## 工作流程：\n"
                    "1. 理解用户需求\n"
                    "2. 立即调用相应的工具\n"
                    "3. 基于工具结果给用户友好的回复\n\n"
                    "记住：你的任务是**执行操作**，不是**指导操作**。"
                )
            }
            messages.append(system_message)
            
            # 构建用户消息
            if images and len(images) > 0:
                # 包含图像的消息
                user_content = [{"type": "text", "text": query}]
                
                # 处理图像数据
                for image_info in images:
                    image_data_b64 = image_info.get("data", "")
                    if image_data_b64 and image_data_b64 not in ["no_renderer", "image_null", ""]:
                        user_content.append({
                            "type": "image_url",
                            "image_url": {
                                "url": f"data:image/png;base64,{image_data_b64}"
                            }
                        })
                
                messages.append({"role": "user", "content": user_content})
            else:
                # 纯文本消息
                messages.append({"role": "user", "content": query})

            # 调用大模型（包含工具调用支持）
            response = self.client.chat.completions.create(
                model=self.model,
                messages=messages,
                tools=available_tools,
                tool_choice="auto",
                max_tokens=config.MAX_TOKENS,
                temperature=config.TEMPERATURE
            )
            
            # 处理模型响应
            message = response.choices[0].message
            
            # 情况1：不需要调用工具 - 直接返回AI回答
            if not message.tool_calls:
                print(f"[DEBUG] 无工具调用")
                return message.content
            
            # 情况2和3：需要调用工具
            print(f"[DEBUG] 开始执行工具调用...")
            
            # 执行所有工具调用
            for tool_call in message.tool_calls:
                tool_name = tool_call.function.name
                tool_args = json.loads(tool_call.function.arguments)
                
                print(f"=== 准备调用工具 ===")
                print(f"工具名称: {tool_name}")
                print(f"工具参数: {tool_args}")
                
                # 调用工具
                result = await self.session.call_tool(tool_name, tool_args)
                tool_output = result.content[0].text
                
                # 检查工具返回是否是command类型
                try:
                    json_data = json.loads(tool_output)
                    if isinstance(json_data, dict) and json_data.get("type") == "command":
                        # 情况2：工具返回command - 直接返回JSON，不添加到messages
                        print(f"=== 检测到工具返回JSON命令，交由桥梁服务器处理 ===")
                        print(f"命令内容: {tool_output}")
                        print(f"[DEBUG] 工具调用完成，结果长度: {len(tool_output)}")
                        return tool_output
                except:
                    # 不是JSON格式，继续处理
                    pass
                
                # 只有非command类型才添加到对话历史
                messages.append({
                    "role": "tool",
                    "tool_call_id": tool_call.id,
                    "name": tool_name,
                    "content": tool_output
                })
            
            # 情况3：工具返回非command - 结合工具结果让AI回答
            print(f"[DEBUG] 工具调用完成，结果长度: {len('combined_result')}")
            response = self.client.chat.completions.create(
                model=self.model,
                messages=messages,
                max_tokens=config.MAX_TOKENS,
                temperature=config.TEMPERATURE
            )
            return response.choices[0].message.content

        except Exception as e:
            error_message = f"处理统一查询时发生错误: {str(e)}"
            print(f"\n{error_message}")
            return error_message


    async def call_tool(self, tool_name: str, tool_args: dict) -> str:
        """
        直接调用MCP工具
        Args:
            tool_name: 工具名称
            tool_args: 工具参数
        Returns:
            str: 工具执行结果
        """
        try:
            if not self.session:
                raise RuntimeError("MCP session not initialized")
            
            print(f"=== 直接调用工具 ===")
            print(f"工具名称: {tool_name}")
            print(f"工具参数: {tool_args}")
            
            # 调用工具
            result = await self.session.call_tool(tool_name, tool_args)
            tool_output = result.content[0].text
            
            print(f"工具输出长度: {len(tool_output)}")
            
            return tool_output
            
        except Exception as e:
            error_message = f"调用工具 {tool_name} 时发生错误: {str(e)}"
            print(f"\n{error_message}")
            return error_message

    async def chat_loop(self):
        # 初始化提示信息
        print(f"\n MCP 客户端已经启动！使用模型: {self.model}")
        print("输入 'exit' 退出！")

        # 等待用户输入
        while True:
            try:
                query = input("\n 用户: ").strip()
                if(query == "exit"):
                    break

                # 处理用户的提问，并返回结果
                response = await self.process_query(query)
                print(f"\n AI: {response}")

            except Exception as e:
                print(f"\n Error: {str(e)}")

    async def cleanup(self) -> None:
        """Clean up server resources."""
        async with self._cleanup_lock:
            try:
                await self.exit_stack.aclose()
                self.session = None
            except Exception as e:
                logging.error(f"Error during cleanup of server {self.name}: {e}")

def clean_filename(text: str) -> str:
    """
    Clean a string to make it suitable for use as a filename.
    Removes or replaces characters that are not allowed in filenames.

    Args:
        text (str): The input text to be cleaned

    Returns:
        str: A cleaned string safe to use as a filename
    """
    import re
    
    # 首先移除所有不可打印字符和控制字符
    # 只保留ASCII可打印字符（32-126）和常见的Unicode字符
    cleaned = ''.join(char for char in text if ord(char) >= 32 and ord(char) <= 126)
    
    # Remove or replace characters that are not allowed in filenames
    # Windows: \ / : * ? " < > |
    # Unix: / (forward slash)
    # Common: \ / : * ? " < > |
    invalid_chars = r'[\\/:*?"<>|]'
    # Replace invalid characters with underscore
    safe_text = re.sub(invalid_chars, '_', cleaned)
    
    # Remove leading/trailing spaces and dots
    safe_text = safe_text.strip('. ')
    
    # 如果清理后为空，使用默认名称
    if not safe_text:
        safe_text = "question"
    
    # Limit length to avoid too long filenames
    safe_text = safe_text[:30]
    
    return safe_text

async def main(server_script_path):
    client = MCPClient()
    try:
        # 检查服务器脚本路径
        if not os.path.isabs(server_script_path):
            server_script_path = os.path.abspath(os.path.join(os.path.dirname(__file__), server_script_path))
        
        if not os.path.exists(server_script_path):
            raise FileNotFoundError(f"找不到服务器脚本: {server_script_path}")
            
        logger.info(f"使用服务器脚本: {server_script_path}")
        await client.connect_to_server(server_script_path)
        await client.chat_loop()
    except Exception as e:
        logger.error(f"运行错误: {e}", exc_info=True)
        raise
    finally:
        await client.cleanup()

if __name__ == "__main__":
    path = "../Servers/server.py"
    try:
        asyncio.run(main(path))
    except KeyboardInterrupt:
        print("\n程序已被用户停止")
    except Exception as e:
        logger.error(f"程序错误: {e}", exc_info=True)
        sys.exit(1)

