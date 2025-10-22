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

# 导入配置
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config

# 配置日志
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format=config.LOG_FORMAT
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

    async def process_user_message(self, user_message: str) -> str:
        """处理用户消息并返回 AI 响应，使用 MCP 协议进行工具调用"""
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
            
            # 构建消息
            messages = [
                {"role": "system", "content": "你是 iGameVis 的智能助手，专门帮助用户操作 iGameVis 3D 可视化应用程序。"},
                {"role": "user", "content": user_message}
            ]
            
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
                        
                        # 处理结果，检测是否包含图像
                        result_str = str(result)
                        
                        # 尝试解析 JSON 并提取图像
                        try:
                            # result 可能是 [TextContent(type='text', text='...')]
                            # 需要提取 text 内容
                            if hasattr(result, '__iter__') and len(result) > 0:
                                first_item = result[0]
                                if hasattr(first_item, 'text'):
                                    result_str = first_item.text
                            
                            # 尝试解析为 JSON
                            result_json = json.loads(result_str)
                            
                            # 检查是否包含 image_base64
                            if isinstance(result_json, dict) and 'image_base64' in result_json:
                                image_base64 = result_json['image_base64']
                                logger.info(f"检测到图像数据，长度: {len(image_base64)}")
                                
                                # 保存图像供后续发送
                                captured_images.append({
                                    "type": "image_url",
                                    "image_url": {
                                        "url": f"data:image/png;base64,{image_base64}"
                                    }
                                })
                                
                                # 从结果中移除图像数据，只保留描述
                                result_json_without_image = {k: v for k, v in result_json.items() if k != 'image_base64'}
                                result_str = json.dumps(result_json_without_image, ensure_ascii=False)
                        except (json.JSONDecodeError, AttributeError, IndexError):
                            # 如果不是 JSON 或解析失败，使用原始字符串
                            pass
                        
                        # 构建工具响应消息
                        tool_messages.append({
                            "role": "tool",
                            "tool_call_id": tool_call.id,
                            "content": result_str
                        })
                    except Exception as e:
                        logger.error(f"工具 {tool_name} 执行失败: {e}")
                        tool_messages.append({
                            "role": "tool",
                            "tool_call_id": tool_call.id,
                            "content": f"Error: {str(e)}"
                        })
                
                # 将工具调用和结果添加到对话历史
                messages.append(message.model_dump())
                messages.extend(tool_messages)
                
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
                
                final_response = self.client.chat.completions.create(
                    model=self.model,
                    messages=messages,
                    temperature=0.1,
                    max_tokens=4000,
                    timeout=timeout
                )
                
                return final_response.choices[0].message.content
            
            # 没有工具调用，直接返回 AI 响应
            return message.content
            
        except Exception as e:
            logger.error(f"Error processing message: {e}")
            # logger.exception("详细错误:")
            return f"处理消息时出错: {e}"

    async def start_interactive_session(self):
        """启动交互式会话"""
        print("=" * 60)
        print("🎮 iGameVis 智能助手")
        print("=" * 60)
        print("我是你的 iGameVis 助手，可以帮你操作 3D 可视化应用程序。")
        print("输入 'quit' 或 'exit' 退出，输入 'help' 查看帮助。")
        print("=" * 60)
        
        while True:
            try:
                user_input = input("\n👤 你: ").strip()
                
                if user_input.lower() in ['quit', 'exit', '退出']:
                    print("\n👋 再见！")
                    break
                
                if not user_input:
                    continue
                
                print("\n🤖 iGameVis助手: 正在处理...")
                
                response = await self.process_user_message(user_input)
                print(f"\n🤖 iGameVis助手: {response}")
                
            except KeyboardInterrupt:
                print("\n\n👋 再见！")
                break
            except Exception as e:
                print(f"\n❌ 错误: {e}")


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
            print(f"❌ 服务器文件不存在: {server_path}")
            return
        
        print("🔌 正在连接到 iGameVis MCP 服务器...")
        
        if not await client.connect_to_server(server_path):
            print("❌ 无法连接到服务器")
            return
        
        print("✅ 连接成功！")
        
        # 启动交互式会话
        await client.start_interactive_session()
        
    except Exception as e:
        print(f"❌ 启动失败: {e}")
        logger.error(f"Startup error: {e}")
    finally:
        await client.cleanup()

if __name__ == "__main__":
    asyncio.run(main())
