import asyncio
import os
import json
import sys
import time
from typing import Optional, List
from contextlib import AsyncExitStack
from datetime import datetime
import re
from openai import OpenAI
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
import logging

# 设置Windows控制台编码为UTF-8
if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        sys.stderr.reconfigure(encoding='utf-8', errors='replace')
    except AttributeError:
        pass

# 导入配置
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config

# 配置日志（输出到stderr，供C++捕获；强制UTF-8避免乱码）
_log_handler = logging.StreamHandler(stream=open(sys.stderr.fileno(), mode='w', encoding='utf-8', closefd=False))
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format=config.LOG_FORMAT,
    handlers=[_log_handler]
)
logger = logging.getLogger(__name__)

# 根据配置决定是否关闭 MCP 库的调试日志
if not config.SHOW_MCP_DEBUG_LOGS:
    logging.getLogger('mcp').setLevel(logging.WARNING)
    logging.getLogger('mcp.client').setLevel(logging.WARNING)
    logging.getLogger('mcp.server').setLevel(logging.WARNING)

class iGameVisMCPClient:
    """iGameVis MCP Client - 专门用于与 iGameVis 应用程序交互的客户端"""

    def __init__(self):
        self.name: str = "iGameVis Assistant"
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
        self.available_tools = []
        self.available_prompts = []
        
        # 聊天历史记录
        self.chat_history: List[dict] = []
        # 历史记录文件路径
        history_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "history")
        os.makedirs(history_dir, exist_ok=True)
        self.history_file = os.path.join(history_dir, "chat_history.json")
        
        # 加载历史记录
        self.load_history()

    async def connect_to_server(self, server_script_path: str):
        """连接到 iGameVis MCP 服务器"""
        try:
            # 使用 stdio 连接到服务器
            # 在 Windows 上使用 sys.executable 获取当前 Python 解释器路径
            python_cmd = sys.executable
            
            logger.info(f"启动 MCP Server: {python_cmd} {server_script_path}")
            
            server_params = StdioServerParameters(
                command=python_cmd, 
                args=[server_script_path],
                env=None  # 使用当前环境变量
            )
            
            logger.info("正在建立 stdio 连接...")
            stdio_transport = await self.exit_stack.enter_async_context(
                stdio_client(server_params)
            )
            
            logger.info("正在创建 MCP 会话...")
            self.session = await self.exit_stack.enter_async_context(
                ClientSession(stdio_transport[0], stdio_transport[1])
            )
            
            logger.info("正在初始化会话...")
            await self.session.initialize()
            
            # 获取可用工具和提示词
            logger.info("正在获取可用工具列表...")
            tools_response = await self.session.list_tools()
            self.available_tools = [tool.name for tool in tools_response.tools]
            
            try:
                logger.info("正在获取可用提示词列表...")
                prompts_response = await self.session.list_prompts()
                self.available_prompts = [prompt.name for prompt in prompts_response.prompts]
            except Exception as e:
                logger.warning(f"Failed to get prompts: {e}")
                self.available_prompts = []
            
            logger.info(f"✅ Connected to iGameVis MCP server")
            logger.info(f"✅ Available tools: {len(self.available_tools)}")
            logger.info(f"✅ Available prompts: {len(self.available_prompts)}")
            
            return True
            
        except Exception as e:
            logger.error(f"❌ Failed to connect to server: {e}")
            logger.exception("详细错误信息:")
            return False

    async def get_system_context(self) -> str:
        """获取系统上下文信息"""
        context_parts = []
        
        # 获取并添加服务器提示词信息
        if self.available_prompts:
            for prompt_name in self.available_prompts:
                try:
                    prompt_response = await self.session.get_prompt(
                        name=prompt_name,
                        arguments={}
                    )
                    if prompt_response.messages:
                        for message in prompt_response.messages:
                            if hasattr(message.content, 'text'):
                                context_parts.append(message.content.text)
                            elif isinstance(message.content, str):
                                context_parts.append(message.content)
                except Exception as e:
                    logger.warning(f"Failed to get prompt {prompt_name}: {e}")
        
        # 如果没有服务器提示词，使用基本的角色描述
        if not context_parts:
            context_parts.append("你是 iGameVis 的智能助手，专门帮助用户操作 iGameVis 3D 可视化应用程序。")
        
        # 添加可用工具信息
        if self.available_tools:
            context_parts.append(f"\n## 可用工具 ({len(self.available_tools)} 个)")
            context_parts.append("你可以使用以下工具来帮助用户：")
            for tool in self.available_tools:
                context_parts.append(f"- {tool}")
        
        return "\n".join(context_parts)

    async def call_tool(self, tool_name: str, arguments: dict):
        """调用 MCP 工具"""
        try:
            if not self.session:
                raise RuntimeError("Not connected to MCP server")
            
            logger.info(f"Calling tool: {tool_name} with args: {arguments}")
            
            result = await self.session.call_tool(tool_name, arguments)
            
            if result.isError:
                logger.error(f"Tool call failed: {result.content}")
                return f"Error: {result.content}"
            else:
                logger.info(f"Tool call successful")
                # 打印工具返回的内容（截断长内容）
                result_str = str(result.content)
                if len(result_str) > 500:
                    logger.info(f"Tool result (truncated): {result_str[:500]}...")
                else:
                    logger.info(f"Tool result: {result_str}")
                return result.content
                
        except Exception as e:
            logger.error(f"Error calling tool {tool_name}: {e}")
            return f"Error calling tool: {e}"

    def parse_tool_calls(self, content: str) -> List[dict]:
        """解析 AI 响应中的工具调用"""
        tool_calls = []
        
        # 查找工具调用模式
        tool_call_pattern = r'<tool_call>\s*(\{.*?\})\s*</tool_call>'
        matches = re.findall(tool_call_pattern, content, re.DOTALL)
        
        for match in matches:
            try:
                tool_call = json.loads(match)
                if 'name' in tool_call:
                    tool_calls.append(tool_call)
            except json.JSONDecodeError as e:
                logger.warning(f"Failed to parse tool call: {e}")
        
        return tool_calls

    def load_history(self):
        """从文件加载聊天历史记录"""
        try:
            if os.path.exists(self.history_file):
                with open(self.history_file, 'r', encoding='utf-8') as f:
                    self.chat_history = json.load(f)
                logger.info(f"已加载 {len(self.chat_history)} 条历史记录")
            else:
                self.chat_history = []
                logger.info("历史记录文件不存在，创建新的历史记录")
        except Exception as e:
            logger.error(f"加载历史记录失败: {e}")
            self.chat_history = []

    def save_history(self):
        """保存聊天历史记录到文件"""
        try:
            with open(self.history_file, 'w', encoding='utf-8') as f:
                json.dump(self.chat_history, f, ensure_ascii=False, indent=2)
            logger.info(f"已保存 {len(self.chat_history)} 条历史记录")
        except Exception as e:
            logger.error(f"保存历史记录失败: {e}")

    def add_to_history(self, role: str, content: str, tool_calls=None, tool_call_id=None):
        """添加消息到历史记录"""
        message = {
            "role": role,
            "content": content,
            "timestamp": datetime.now().isoformat()
        }
        
        # 如果有工具调用，添加到消息中
        if tool_calls:
            message["tool_calls"] = tool_calls
        
        # 如果是工具响应，添加 tool_call_id
        if tool_call_id:
            message["tool_call_id"] = tool_call_id
        
        self.chat_history.append(message)
        
        # 限制历史记录长度，保留最近100条消息（避免上下文过长）
        max_history = 100
        if len(self.chat_history) > max_history:
            # 保留系统消息和最近的对话
            system_messages = [msg for msg in self.chat_history if msg.get("role") == "system"]
            other_messages = [msg for msg in self.chat_history if msg.get("role") != "system"]
            # 保留最近的对话
            recent_messages = other_messages[-max_history:]
            self.chat_history = system_messages + recent_messages
            logger.info(f"历史记录已截断，保留最近 {max_history} 条消息")

    async def process_user_message(self, user_message: str, stream_callback=None):
        """处理用户消息并返回 AI 响应，使用 MCP 协议进行工具调用
        
        Args:
            user_message: 用户输入的消息
            stream_callback: 可选的流式回调函数，接收每个生成的文本片段
                            如果提供，将实时调用此函数；否则等待完整响应后返回
        
        Returns:
            str: 完整的AI响应（如果使用流式，也会返回完整内容）
        """
        try:
            logger.info(f"开始处理用户消息: {user_message}")
            
            # 获取可用工具的详细信息
            logger.info("获取工具列表...")
            tools_response = await self.session.list_tools()
            
            # 转换为 OpenAI function calling 格式
            tools = []
            for tool in tools_response.tools:
                tool_schema = {
                    "type": "function",
                    "function": {
                        "name": tool.name,
                        "description": tool.description or "",
                        "parameters": tool.inputSchema if hasattr(tool, 'inputSchema') else {"type": "object", "properties": {}}
                    }
                }
                tools.append(tool_schema)
            
            logger.info(f"已加载 {len(tools)} 个工具")
            
            # 构建消息，包含历史记录
            messages = []
            
            # 添加系统消息（如果历史记录中没有）
            system_content = "你是 iGameVis 的智能助手，专门帮助用户操作 iGameVis 3D 可视化应用程序。"
            if not self.chat_history or not any(msg.get("role") == "system" for msg in self.chat_history):
                messages.append({"role": "system", "content": system_content})
            
            # 添加历史记录（排除系统消息，因为上面已经添加了）
            for hist_msg in self.chat_history:
                # 只添加非系统消息，并且过滤掉 timestamp 等额外字段
                if hist_msg.get("role") != "system":
                    clean_msg = {
                        "role": hist_msg.get("role"),
                        "content": hist_msg.get("content")
                    }
                    # 如果有工具调用，保留
                    if "tool_calls" in hist_msg:
                        clean_msg["tool_calls"] = hist_msg["tool_calls"]
                    # 如果是工具响应，保留 tool_call_id
                    if "tool_call_id" in hist_msg:
                        clean_msg["tool_call_id"] = hist_msg["tool_call_id"]
                    messages.append(clean_msg)
            
            # 添加当前用户消息
            messages.append({"role": "user", "content": user_message})
            
            # 添加用户消息到历史记录
            self.add_to_history("user", user_message)
            
            logger.info(f"构建消息列表，包含 {len(messages)} 条消息（含历史记录）")
            
            # 调用 AI，启用 function calling
            logger.info("调用 AI...")
            response = self.client.chat.completions.create(
                model=self.model,
                messages=messages,
                tools=tools,
                tool_choice="auto",  # 让 AI 自动决定是否调用工具
                temperature=0.1,
                max_tokens=4000,
                timeout=30.0
            )
            
            choice = response.choices[0]
            message = choice.message
            logger.info(f"AI 响应类型: {choice.finish_reason}")
            
            # 检查是否有工具调用
            if message.tool_calls:
                logger.info(f"AI 请求调用 {len(message.tool_calls)} 个工具")
                
                # 执行所有工具调用
                tool_messages = []
                captured_images = []  # 收集图像数据
                
                for tool_call in message.tool_calls:
                    tool_name = tool_call.function.name
                    tool_args = json.loads(tool_call.function.arguments)
                    
                    logger.info(f"执行工具: {tool_name}")
                    logger.info(f"参数: {tool_args}")
                    
                    try:
                        # 通过 MCP 调用工具
                        result = await asyncio.wait_for(
                            self.call_tool(tool_name, tool_args),
                            timeout=30.0
                        )
                        logger.info(f"工具 {tool_name} 执行成功")
                        
                        # 处理 MCP 返回的内容列表
                        text_parts = []
                        
                        # result 是 [TextContent(...), ImageContent(...), ...]
                        if hasattr(result, '__iter__'):
                            for item in result:
                                if hasattr(item, 'type'):
                                    if item.type == 'text' and hasattr(item, 'text'):
                                        text_parts.append(item.text)
                                    elif item.type == 'image' and hasattr(item, 'data'):
                                        # 提取图像数据
                                        base64_data = item.data
                                        mime_type = getattr(item, 'mimeType', 'image/png')
                                        
                                        # 构建 data URI（OpenAI API 需要完整的 data URI）
                                        if not base64_data.startswith("data:"):
                                            image_data = f"data:{mime_type};base64,{base64_data}"
                                        else:
                                            image_data = base64_data
                                        
                                        logger.info(f"检测到图像，数据长度: {len(base64_data) if isinstance(base64_data, str) else 'N/A'}")
                                        captured_images.append({
                                            "type": "image_url",
                                            "image_url": {"url": image_data}
                                        })
                                else:
                                    # 如果没有 type 属性，转为字符串
                                    text_parts.append(str(item))
                        else:
                            text_parts.append(str(result))
                        
                        # 构建工具响应消息（只包含文本）
                        result_text = "\n".join(text_parts) if text_parts else "操作完成"
                        tool_messages.append({
                            "role": "tool",
                            "tool_call_id": tool_call.id,
                            "content": result_text
                        })
                    except Exception as e:
                        logger.error(f"工具 {tool_name} 执行失败: {e}")
                        tool_messages.append({
                            "role": "tool",
                            "tool_call_id": tool_call.id,
                            "content": f"Error: {str(e)}"
                        })
                
                # 将工具调用和结果添加到对话历史
                assistant_msg = message.model_dump()
                messages.append(assistant_msg)
                messages.extend(tool_messages)
                
                # 添加助手消息（包含工具调用）到历史记录
                tool_calls_data = []
                if message.tool_calls:
                    for tc in message.tool_calls:
                        tool_calls_data.append({
                            "id": tc.id,
                            "type": "function",
                            "function": {
                                "name": tc.function.name,
                                "arguments": tc.function.arguments
                            }
                        })
                self.add_to_history("assistant", "", tool_calls=tool_calls_data if tool_calls_data else None)
                
                # 添加工具响应到历史记录
                for tool_msg in tool_messages:
                    self.add_to_history("tool", tool_msg.get("content", ""), tool_call_id=tool_msg.get("tool_call_id"))
                
                # 如果有捕获的图像，作为用户消息附加
                if captured_images:
                    logger.info(f"添加 {len(captured_images)} 张图像到对话")
                    image_content = [
                        {"type": "text", "text": "这是工具返回的图像，请帮我分析："}
                    ]
                    image_content.extend(captured_images)
                    messages.append({
                        "role": "user",
                        "content": image_content  # 使用列表格式支持多模态
                    })
                
                # 再次调用 AI，让它基于工具结果生成最终回复
                logger.info("获取最终响应...")
                # 如果有图像，使用更长的超时时间
                timeout = 120.0 if captured_images else 30.0
                logger.info(f"使用超时时间: {timeout}秒")
                
                # 使用流式输出
                if stream_callback:
                    logger.info("使用流式输出模式")
                    stream = self.client.chat.completions.create(
                        model=self.model,
                        messages=messages,
                        temperature=0.1,
                        max_tokens=4000,
                        timeout=timeout,
                        stream=True  # 启用流式输出
                    )
                    
                    full_content = ""
                    for chunk in stream:
                        if chunk.choices and len(chunk.choices) > 0:
                            delta = chunk.choices[0].delta
                            if hasattr(delta, 'content') and delta.content:
                                full_content += delta.content
                                # 调用回调函数，实时输出
                                await stream_callback(delta.content)
                    
                    # 添加最终AI响应到历史记录
                    if full_content:
                        self.add_to_history("assistant", full_content)
                    
                    # 保存历史记录
                    self.save_history()
                    
                    return full_content
                else:
                    # 非流式模式
                    final_response = self.client.chat.completions.create(
                        model=self.model,
                        messages=messages,
                        temperature=0.1,
                        max_tokens=4000,
                        timeout=timeout
                    )
                    final_content = final_response.choices[0].message.content
                    
                    # 添加最终AI响应到历史记录
                    if final_content:
                        self.add_to_history("assistant", final_content)
                    
                    # 保存历史记录
                    self.save_history()
                    
                    return final_content
            
            # 没有工具调用，直接返回 AI 响应（支持流式）
            if stream_callback:
                logger.info("使用流式输出模式（无工具调用）")
                # 重新调用以获取流式响应
                stream = self.client.chat.completions.create(
                    model=self.model,
                    messages=messages,
                    tools=tools,
                    tool_choice="auto",
                    temperature=0.1,
                    max_tokens=4000,
                    timeout=30.0,
                    stream=True
                )
                
                full_content = ""
                for chunk in stream:
                    if chunk.choices and len(chunk.choices) > 0:
                        delta = chunk.choices[0].delta
                        if hasattr(delta, 'content') and delta.content:
                            full_content += delta.content
                            await stream_callback(delta.content)
                
                # 添加最终AI响应到历史记录
                if full_content:
                    self.add_to_history("assistant", full_content)
                
                # 保存历史记录
                self.save_history()
                
                return full_content
            else:
                # 没有工具调用，直接返回 AI 响应
                response_content = message.content
                
                # 添加最终AI响应到历史记录
                if response_content:
                    self.add_to_history("assistant", response_content)
                
                # 保存历史记录
                self.save_history()
                
                return response_content
            
        except Exception as e:
            logger.error(f"Error processing message: {e}")
            # logger.exception("详细错误:")
            error_msg = f"处理消息时出错: {e}"
            
            # 添加错误消息到历史记录
            self.add_to_history("assistant", error_msg)
            self.save_history()
            
            if stream_callback:
                await stream_callback(error_msg)
            return error_msg

    async def start_interactive_session(self):
        """启动交互式会话"""
        print("=" * 60)
        print("iGameVis Intelligent Assistant")
        print("=" * 60)
        print("I am your iGameVis assistant, here to help you operate the 3D visualization application.")
        print("Type 'quit' or 'exit' to quit, 'help' for help.")
        print("=" * 60)
        
        while True:
            try:
                user_input = input("\n[You]: ").strip()
                
                if user_input.lower() in ['quit', 'exit', '退出']:
                    print("\n[INFO] Goodbye!")
                    break
                
                if not user_input:
                    continue
                
                print("\n[iGameVis Assistant]: ", end='', flush=True)
                
                # 定义流式回调函数，实时打印AI回答
                async def console_stream_callback(text_chunk):
                    """控制台流式输出回调"""
                    print(text_chunk, end='', flush=True)
                
                # 使用流式模式处理消息
                response = await self.process_user_message(
                    user_input, 
                    stream_callback=console_stream_callback
                )
                
                print()  # 换行
                
            except KeyboardInterrupt:
                print("\n\n[INFO] Goodbye!")
                break
            except Exception as e:
                print(f"\n[ERROR] Error: {e}")


    async def cleanup(self):
        """清理资源"""
        try:
            await self.exit_stack.aclose()
        except Exception as e:
            logger.error(f"Error during cleanup: {e}")

async def main():
    """主函数"""
    client = iGameVisMCPClient()
    
    try:
        # 连接到 iGameVis MCP 服务器
        server_path = os.path.join(
            os.path.dirname(os.path.dirname(__file__)), 
            "Servers", 
            "iGameVis_Server.py"
        )
        
        if not os.path.exists(server_path):
            print(f"[ERROR] Server file not found: {server_path}")
            return
        
        print("[INFO] Connecting to iGameVis MCP Server...")
        
        if not await client.connect_to_server(server_path):
            print("[ERROR] Failed to connect to server")
            return
        
        print("[OK] Connected successfully!")
        
        # 启动交互式会话
        await client.start_interactive_session()
        
    except Exception as e:
        print(f"[ERROR] Startup failed: {e}")
        logger.error(f"Startup error: {e}")
    finally:
        await client.cleanup()

if __name__ == "__main__":
    asyncio.run(main())
