import socket
import struct
import threading
import os
import asyncio
import json
import re
import time
import concurrent.futures
import signal
import sys
import base64
from Client.client import MCPClient
import config

# 图像处理相关导入
try:
    import matplotlib
    matplotlib.use('Agg')  # 使用非GUI后端，避免线程问题
    import matplotlib.pyplot as plt
    import matplotlib.image as mpimg
    from PIL import Image
    import io
    IMAGING_AVAILABLE = True
    print("✅ matplotlib和PIL已安装，图像保存功能已启用")
except ImportError:
    IMAGING_AVAILABLE = False
    print("⚠️ matplotlib/PIL 未安装，图像显示功能将受限")
    # 即使没有matplotlib，我们仍然尝试导入base64来验证数据
    try:
        from PIL import Image
        import io
        PIL_ONLY = True
        print("✅ 检测到PIL，可以保存图像文件")
    except ImportError:
        PIL_ONLY = False
        print("❌ PIL也不可用，无法处理图像")

# 全局变量用于进程管理
global_bridge_server = None
global_shutdown_event = threading.Event()

def cleanup_and_exit():
    """清理资源并退出"""
    global global_bridge_server
    
    print("正在清理资源...")
    
    # 设置关闭事件
    global_shutdown_event.set()
    
    # 清理桥梁服务器
    if global_bridge_server:
        try:
            global_bridge_server.cleanup()
        except:
            pass
    
    print("资源清理完成，退出程序")
    os._exit(0)

def signal_handler(signum, frame):
    """信号处理器"""
    print(f"接收到信号 {signum}，开始清理...")
    cleanup_and_exit()

def setup_signal_handlers():
    """设置信号处理器"""
    if sys.platform == "win32":
        # Windows信号处理
        try:
            import ctypes
            from ctypes import wintypes
            
            kernel32 = ctypes.windll.kernel32
            
            @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.DWORD)
            def console_ctrl_handler(ctrl_type):
                if ctrl_type in (0, 1, 2):  # CTRL_C, CTRL_BREAK, CTRL_CLOSE
                    cleanup_and_exit()
                    return True
                return False
            
            kernel32.SetConsoleCtrlHandler(console_ctrl_handler, True)
        except:
            pass
    else:
        # Linux信号处理
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)

class MCPBridge:
    """
    MCP桥梁服务器 - 连接iGameVis客户端和现有的MCP系统
    """
    def __init__(self, host=None, port=None):
        global global_bridge_server
        global_bridge_server = self
        
        self.host = host or config.SOCKET_HOST
        self.port = port or config.SOCKET_PORT
        
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # 共享的MCP客户端实例
        self.mcp_client = None
        self.mcp_loop = None
        self.mcp_thread = None
        
        # 多工具调用支持
        self.active_sessions = {}  # 存储活跃的会话状态
        self.command_queue = {}    # 每个客户端的命令队列
        
        print(f"桥梁服务器初始化完成，监听 {self.host}:{self.port}")

    def cleanup(self):
        """清理桥梁服务器资源"""
        print("正在清理桥梁服务器...")
        
        # 清理共享的MCP客户端
        if self.mcp_loop and self.mcp_client:
            try:
                # 停止事件循环
                self.mcp_loop.call_soon_threadsafe(self.mcp_loop.stop)
                if self.mcp_thread and self.mcp_thread.is_alive():
                    self.mcp_thread.join(timeout=5)
                print("共享MCP客户端已清理")
            except Exception as e:
                print(f"清理共享MCP客户端时出错: {e}")
        
        # 关闭socket
        try:
            self.sock.close()
        except:
            pass

    def parse_json_response(self, text):
        """
        解析响应，统一JSON格式
        Args:
            text (str): 响应文本
        Returns:
            dict: 解析后的响应对象
        """
        # 尝试直接解析为JSON
        try:
            json_obj = json.loads(text)
            if isinstance(json_obj, dict) and 'type' in json_obj:
                return json_obj
        except json.JSONDecodeError:
            pass
        # 如果解析失败，作为普通回复处理
        default_response = {
            "type": "reply",
            "content": text,
            "timestamp": str(time.time())
        }
        return default_response

    def send_message_to_client(self, client_socket, json_response):
        """
        发送JSON响应到客户端
        Args:
            client_socket: 客户端套接字
            json_response: JSON响应对象或响应列表
        """
        try:
            # 如果是列表，发送多个消息
            if isinstance(json_response, list):
                for response in json_response:
                    message_str = json.dumps(response, ensure_ascii=False)
                    message_bytes = message_str.encode('utf-8')
                    client_socket.send(struct.pack('i', len(message_bytes)))
                    client_socket.send(message_bytes)
                    time.sleep(0.1)  # 短暂延迟确保消息顺序
            else:
                # 单个消息
                if isinstance(json_response, dict):
                    message_str = json.dumps(json_response, ensure_ascii=False)
                else:
                    message_str = str(json_response)
                message_bytes = message_str.encode('utf-8')
                client_socket.send(struct.pack('i', len(message_bytes)))
                client_socket.send(message_bytes)
        except Exception as e:
            print(f"发送消息到客户端时出错: {e}")

    def init_mcp_client(self):
        """在独立线程中初始化MCP客户端"""
        def mcp_thread_func():
            # 创建新的事件循环
            self.mcp_loop = asyncio.new_event_loop()
            asyncio.set_event_loop(self.mcp_loop)
            
            async def setup_mcp():
                print("创建共享的MCP客户端实例...")
                self.mcp_client = MCPClient()
                server_script_path = os.path.join(os.path.dirname(__file__), "Servers", "server.py")
                await self.mcp_client.connect_to_server(server_script_path)
                print("共享MCP客户端创建成功")
            
            # 初始化MCP客户端
            self.mcp_loop.run_until_complete(setup_mcp())
            
            # 保持事件循环运行
            self.mcp_loop.run_forever()
        
        # 启动MCP线程
        self.mcp_thread = threading.Thread(target=mcp_thread_func, daemon=True)
        self.mcp_thread.start()
        
        # 等待MCP客户端初始化完成
        while self.mcp_client is None:
            time.sleep(0.1)
        
        print("MCP客户端线程启动完成")
    
    def process_query_sync(self, message, client_id=None):
        """解析JSON消息，支持多工具调用的智能处理"""
        if not self.mcp_client or not self.mcp_loop:
            return "MCP客户端未初始化"

        # 解析JSON消息，根据type判断类型
        message_json = json.loads(message)
        message_type = message_json.get("type", "unknown")

        if message_type == "question":
            # 新的用户问题，使用增强的AI处理
            return self.process_user_question(message_json.get('content', ''), client_id)
        elif message_type == "operation_result":
            # 特殊处理操作结果，特别是包含图像的结果
            return self.handle_operation_result(message_json, client_id)
        else:
            ai_prompt = f"收到消息: {json.dumps(message_json, ensure_ascii=False)}\n请根据消息内容回答。"
            return self.process_ai_query(ai_prompt)

    def process_user_question(self, user_question, client_id=None):
        """处理用户问题，支持智能多工具调用"""
        try:
            # 为客户端创建会话状态
            if client_id and client_id not in self.active_sessions:
                self.active_sessions[client_id] = {
                    "commands": [],
                    "results": [],
                    "stage": "initial"
                }

            # 使用增强的AI提示，让AI能够规划多步骤操作
            ai_prompt = f"""
用户问题: {user_question}

你是一个智能的3D模型处理助手。请分析用户的需求，制定合适的执行计划。

如果用户的需求需要多个步骤才能完成，你可以：
1. 首先调用必要的工具收集信息
2. 基于结果决定下一步操作
3. 最终向用户提供完整的回答

请根据用户问题选择合适的工具并执行操作。
"""

            # 让AI处理
            future = asyncio.run_coroutine_threadsafe(
                self.mcp_client.process_query(ai_prompt),
                self.mcp_loop
            )

            ai_result = future.result(timeout=30)
            return self.handle_tool_result(ai_result, client_id)

        except Exception as e:
            return f"处理用户问题时出错: {str(e)}"

    def process_ai_query(self, ai_prompt):
        """处理普通AI查询"""
        try:
            future = asyncio.run_coroutine_threadsafe(
                self.mcp_client.process_query(ai_prompt),
                self.mcp_loop
            )
            
            ai_result = future.result(timeout=30)
            return self.handle_tool_result(ai_result)

        except Exception as e:
            return f"处理AI查询时出错: {str(e)}"

    def handle_tool_result(self, tool_result, client_id=None):
        """处理工具调用的结果，支持批量命令"""
        try:
            result_json = json.loads(tool_result)
            result_type = result_json.get("type", "")

            if result_type == "command":
                # 检查是否是单个命令还是批量命令
                return self.prepare_command_execution(result_json, client_id)
            elif result_type == "commands":
                # 批量命令处理
                return self.prepare_batch_commands(result_json, client_id)
            elif result_type == "reply":
                # 检查回复内容中是否包含图像
                content = result_json.get("content", "")
                has_images, images, processed_content = self.detect_and_extract_images(content)
                
                if has_images and images:
                    # 如果检测到图像，直接在桥梁服务器中处理
                    image_result = self.process_and_display_images(images, "工具执行结果", processed_content)
                    return image_result
                else:
                    # 没有图像时的正常处理
                    final_prompt = f"基于以下工具执行结果，用自然语言回答用户: {processed_content}"
                    future = asyncio.run_coroutine_threadsafe(
                        self.mcp_client.process_query(final_prompt),
                        self.mcp_loop
                    )
                    return future.result(timeout=30)
            else:
                return tool_result
        except:
            # 不是JSON，说明AI直接回答了
            # 检查是否包含图像
            has_images, images, processed_result = self.detect_and_extract_images(tool_result)
            if has_images and images:
                image_result = self.process_and_display_images(images, "AI回复", processed_result)
                return image_result
            return tool_result

    def prepare_command_execution(self, command_json, client_id=None):
        """准备单个命令执行"""
        if client_id:
            # 如果有客户端ID，将命令添加到队列
            if client_id not in self.command_queue:
                self.command_queue[client_id] = []
            self.command_queue[client_id].append(command_json)
            
            # 标记需要执行命令
            command_json["requires_execution"] = True
            
        return json.dumps(command_json, ensure_ascii=False)

    def prepare_batch_commands(self, commands_json, client_id=None):
        """准备批量命令执行"""
        commands = commands_json.get("commands", [])
        
        if client_id:
            # 将所有命令添加到队列
            if client_id not in self.command_queue:
                self.command_queue[client_id] = []
            self.command_queue[client_id].extend(commands)
        
        # 返回第一个命令开始执行
        if commands:
            first_command = commands[0]
            first_command["requires_execution"] = True
            first_command["batch_total"] = len(commands)
            first_command["batch_index"] = 0
            return json.dumps(first_command, ensure_ascii=False)
        
        return json.dumps({"type": "reply", "content": "没有需要执行的命令"}, ensure_ascii=False)

    def try_save_images(self, images):
        """
        尝试保存图像到文件，即使matplotlib不可用
        
        Args:
            images: 图像信息列表
        Returns:
            list: 成功保存的文件路径列表
        """
        saved_files = []
        
        for i, image_info in enumerate(images):
            try:
                image_data_b64 = image_info.get("data", "")
                image_key = image_info.get("key", f"image_{i}")
                
                # 检查图像数据有效性
                if not image_data_b64 or image_data_b64 in ["no_renderer", "image_null", ""]:
                    print(f"❌ 图像 {image_key}: 无效的图像数据")
                    continue
                
                # 尝试用基础base64库解码（不需要PIL）
                try:
                    image_bytes = base64.b64decode(image_data_b64)
                    print(f"✅ 图像 {image_key}: Base64解码成功，数据大小 {len(image_bytes)} 字节")
                    
                    # 保存原始字节数据
                    raw_filename = f"image_{i}_{image_key}_raw.bin"
                    with open(raw_filename, 'wb') as f:
                        f.write(image_bytes)
                    print(f"✅ 保存原始数据到: {raw_filename}")
                    saved_files.append(raw_filename)
                    
                    # 尝试检测图像格式
                    if image_bytes.startswith(b'\xff\xd8\xff'):
                        ext = '.jpg'
                    elif image_bytes.startswith(b'\x89PNG'):
                        ext = '.png'
                    elif image_bytes.startswith(b'BM'):
                        ext = '.bmp'
                    else:
                        print(f"⚠️ 图像 {image_key}: 未知格式，尝试PNG")
                        ext = '.png'
                    
                    # 保存为图像文件
                    img_filename = f"image_{i}_{image_key}{ext}"
                    with open(img_filename, 'wb') as f:
                        f.write(image_bytes)
                    print(f"✅ 保存图像文件到: {img_filename}")
                    saved_files.append(img_filename)
                    
                    # 如果有PIL，尝试验证图像
                    if 'PIL_ONLY' in globals() and PIL_ONLY:
                        try:
                            img = Image.open(io.BytesIO(image_bytes))
                            print(f"✅ PIL验证成功: {img.format} {img.size} {img.mode}")
                        except Exception as pil_e:
                            print(f"⚠️ PIL验证失败: {pil_e}")
                    
                except Exception as decode_e:
                    print(f"❌ 图像 {image_key}: Base64解码失败 - {decode_e}")
                    continue
                    
            except Exception as e:
                print(f"❌ 处理图像 {i} 时出错: {e}")
                continue
        
        return saved_files

    def detect_and_extract_images(self, data):
        """
        通用地检测JSON数据中的图像并提取
        Args:
            data: 可能包含图像的数据（字符串、字典或列表）
        Returns:
            tuple: (是否包含图像, 提取的图像列表, 处理后的数据)
        """
        images = []
        has_images = False
        
        def extract_from_dict(obj, path=""):
            nonlocal has_images
            if isinstance(obj, dict):
                for key, value in obj.items():
                    current_path = f"{path}.{key}" if path else key
                    if isinstance(value, str) and self.is_base64_image(value):
                        images.append({
                            "path": current_path,
                            "data": value,
                            "key": key
                        })
                        has_images = True
                        # 替换原数据中的图像为占位符
                        obj[key] = f"[图像数据已提取: {len(value)} 字符]"
                    elif isinstance(value, (dict, list)):
                        extract_from_dict(value, current_path)
            elif isinstance(obj, list):
                for i, item in enumerate(obj):
                    current_path = f"{path}[{i}]" if path else f"[{i}]"
                    if isinstance(item, str) and self.is_base64_image(item):
                        images.append({
                            "path": current_path,
                            "data": item,
                            "key": f"item_{i}"
                        })
                        has_images = True
                        # 替换原数据中的图像为占位符
                        obj[i] = f"[图像数据已提取: {len(item)} 字符]"
                    elif isinstance(item, (dict, list)):
                        extract_from_dict(item, current_path)
        
        # 处理不同类型的输入数据
        if isinstance(data, str):
            try:
                parsed_data = json.loads(data)
                extract_from_dict(parsed_data)
                # 避免重复序列化，直接返回解析后的数据
                return has_images, images, data
            except json.JSONDecodeError:
                # 如果不是JSON，检查是否整个字符串就是base64图像
                if self.is_base64_image(data):
                    images.append({
                        "path": "root",
                        "data": data,
                        "key": "image"
                    })
                    return True, images, "[图像数据已提取]"
                return False, [], data
        elif isinstance(data, (dict, list)):
            extract_from_dict(data)
            return has_images, images, data
        else:
            return False, [], data

    def is_base64_image(self, data):
        """
        检测字符串是否为base64编码的图像
        """
        if not isinstance(data, str) or len(data) < 100:
            return False
        
        # 检查是否符合base64的基本特征
        import re
        if not re.match(r'^[A-Za-z0-9+/]*={0,2}$', data):
            return False
        
        # 长度检查：base64图像通常比较长
        if len(data) < 1000:
            return False
            
        # 尝试解码前几个字节检查是否为图像文件头
        try:
            import base64
            decoded = base64.b64decode(data[:100])
            # 检查常见图像文件头
            if (decoded.startswith(b'\x89PNG') or  # PNG
                decoded.startswith(b'\xFF\xD8\xFF') or  # JPEG
                decoded.startswith(b'GIF8') or  # GIF
                decoded.startswith(b'BM')):  # BMP
                return True
        except:
            pass
        
        return False

    def process_and_display_images(self, images, context="", text_content=""):
        """
        处理并显示图像，并生成模型总结
        Args:
            images: 提取的图像列表
            context: 上下文信息
            text_content: 文本内容
        Returns:
            str: 处理结果消息
        """
        if not images:
            return f"{context}\n{text_content}"
        
        if not IMAGING_AVAILABLE:
            print(f"⚠️ 检测到 {len(images)} 个图像，matplotlib不可用，尝试保存图像文件...")
            # 尝试验证和保存图像数据
            saved_files = self.try_save_images(images)
            if saved_files:
                files_list = "\n".join([f"- {f}" for f in saved_files])
                return f"{context}\n{text_content}\n\n✅ 检测到 {len(images)} 个图像，已保存到文件：\n{files_list}\n\n请手动打开查看图像是否正确。"
            else:
                return f"{context}\n{text_content}\n\n❌ 检测到 {len(images)} 个图像，但无法处理（缺少PIL库）"
        
        print(f"🖼️ 检测到 {len(images)} 个图像，正在处理和显示...")
        displayed_images = []
        failed_images = []
        
        for i, image_info in enumerate(images):
            try:
                image_data_b64 = image_info.get("data", "")
                image_key = image_info.get("key", f"image_{i}")
                image_path_info = image_info.get("path", "unknown")
                
                # 检查图像数据有效性
                if not image_data_b64 or image_data_b64 in ["no_renderer", "image_null", ""]:
                    failed_images.append(f"{image_key}: 无效的图像数据")
                    continue
                
                # 解码Base64图像
                image_data = base64.b64decode(image_data_b64)
                image = Image.open(io.BytesIO(image_data))
                
                # 使用matplotlib保存图像
                plt.figure(figsize=(10, 8))
                plt.imshow(image)
                plt.title(f"Image: {image_key}", fontsize=14)  # 使用英文避免字体问题
                plt.axis('off')
                plt.tight_layout()
                
                # 保存图像文件
                save_filename = f"model_image_{i}_{image_key}.png"
                plt.savefig(save_filename, dpi=150, bbox_inches='tight')
                plt.close()  # 关闭figure释放内存
                print(f"✅ 图像已保存到: {save_filename}")
                
                displayed_images.append({
                    "key": image_key,
                    "size": f"{image.width}x{image.height}",
                    "source_path": image_path_info
                })
                
            except Exception as e:
                failed_images.append(f"{image_key}: {str(e)}")
        
        # 如果检测到模型信息和图像，优先显示AI分析结果
        if context and "操作:" in context and text_content and displayed_images:
            try:
                # 提取纯文本的模型信息
                model_info_text = ""
                try:
                    # 尝试解析JSON获取description字段
                    text_json = json.loads(text_content)
                    if isinstance(text_json, dict) and "description" in text_json:
                        model_info_text = text_json["description"]
                    else:
                        model_info_text = text_content
                except:
                    # 如果不是JSON，直接使用原文本
                    model_info_text = text_content
                
                print(f"📊 正在分析模型: {model_info_text[:100]}...")
                
                # 调用模型分析工具
                future = asyncio.run_coroutine_threadsafe(
                    self.mcp_client.call_tool("analyze_model_summary", {
                        "model_info": model_info_text,
                        "has_image": True,
                        "image_description": f"模型的3D可视化图像已保存到文件"
                    }),
                    self.mcp_loop
                )
                
                analysis_result = future.result(timeout=15)
                
                # 解析分析结果，只返回AI分析内容
                try:
                    analysis_json = json.loads(analysis_result)
                    if analysis_json.get("type") == "reply":
                        analysis_content = analysis_json.get("content", "")
                        if analysis_content:
                            return f"🤖 {analysis_content}"
                except:
                    # 如果不是JSON格式，直接返回
                    if analysis_result and "分析模型信息时发生错误" not in analysis_result:
                        return f"🤖 {analysis_result}"
                        
            except Exception as e:
                print(f"生成模型总结时出错: {e}")
                # 如果分析失败，回退到简洁的基础信息
                pass
        
        # 回退方案：返回简洁的基础信息
        if displayed_images:
            return f"✅ 模型信息获取成功，图像已保存到 {len(displayed_images)} 个文件"
        else:
            return f"{context}\n{text_content}" if context and text_content else "操作完成"

    def handle_operation_result(self, operation_result_json, client_id=None):
        """处理操作结果，自动检测和处理图像，支持批量命令继续执行"""
        try:
            content = operation_result_json.get("content", {})
            action = content.get("action", "")
            success = content.get("success", False)
            message = content.get("message", "")

            # 检测消息中是否包含图像
            has_images, images, processed_message = self.detect_and_extract_images(message)
            
            # 检查是否有未完成的批量命令
            next_command = self.get_next_command(client_id, action, success)
            
            if next_command:
                # 如果有下一个命令，先处理当前结果，然后执行下一个命令
                if has_images and images:
                    # 保存图像处理结果，但不阻塞下一个命令
                    threading.Thread(target=self.process_and_display_images, 
                                   args=(images, f"操作: {action}", processed_message)).start()
                
                # 返回下一个命令
                return next_command
            
            # 统一通过AI处理所有工具结果
            return self.generate_ai_summary_for_tool_result(action, success, processed_message, has_images, images)
        except Exception as e:
            return f"处理操作结果时出错: {str(e)}"

    def generate_ai_summary_for_tool_result(self, action, success, message, has_images=False, images=None):
        """
        为所有工具结果生成AI总结
        
        Args:
            action: 执行的动作
            success: 是否成功
            message: 原始消息
            has_images: 是否包含图像
            images: 图像数据列表
        Returns:
            str: AI生成的总结
        """
        try:
            # 如果有图像，先处理图像（但不显示原始数据）
            image_context = ""
            if has_images and images:
                # 后台处理图像，但收集上下文信息
                processed_images = []
                for i, image_info in enumerate(images):
                    try:
                        image_data_b64 = image_info.get("data", "")
                        image_key = image_info.get("key", f"image_{i}")
                        
                        if image_data_b64 and image_data_b64 not in ["no_renderer", "image_null", ""]:
                            # 解码并保存图像
                            image_data = base64.b64decode(image_data_b64)
                            image = Image.open(io.BytesIO(image_data))
                            
                            # 保存图像文件
                            save_filename = f"model_image_{i}_{image_key}.png"
                            if IMAGING_AVAILABLE:
                                plt.figure(figsize=(10, 8))
                                plt.imshow(image)
                                plt.title(f"Image: {image_key}", fontsize=14)
                                plt.axis('off')
                                plt.tight_layout()
                                plt.savefig(save_filename, dpi=150, bbox_inches='tight')
                                plt.close()
                            else:
                                # 直接保存原始图像
                                image.save(save_filename)
                            
                            processed_images.append({
                                "filename": save_filename,
                                "size": f"{image.width}x{image.height}",
                                "key": image_key
                            })
                            print(f"✅ 图像已保存到: {save_filename}")
                    except Exception as e:
                        print(f"❌ 处理图像失败: {e}")
                
                if processed_images:
                    image_context = f"已保存 {len(processed_images)} 个图像文件: " + ", ".join([img["filename"] for img in processed_images])
            
            # 构建AI提示，根据不同工具类型优化
            prompt = self.build_smart_prompt(action, success, message, image_context)
            
            # 调用AI生成总结
            future = asyncio.run_coroutine_threadsafe(
                self.mcp_client.process_query(prompt),
                self.mcp_loop
            )
            result = future.result(timeout=30)
            
            # 解析AI结果
            try:
                result_json = json.loads(result)
                if result_json.get("type") == "reply":
                    return result_json.get("content", result)
            except:
                pass
            
            return result
            
        except Exception as e:
            print(f"生成AI总结时出错: {e}")
            # 回退到简单格式
            status = "✅ 成功" if success else "❌ 失败"
            return f"{status} {action}: {message}"

    def build_smart_prompt(self, action, success, message, image_context=""):
        """
        根据工具类型构建智能提示
        """
        status = "成功" if success else "失败"
        
        # 根据不同的操作类型定制提示
        if action == "get_model_info":
            if success:
                prompt = f"""
                用户刚刚获取了3D模型信息，操作成功。原始数据：{message}
                {f'图像信息：{image_context}' if image_context else ''}
                
                请分析这个3D模型，用自然、专业的语言告诉用户：
                1. 这是什么类型的模型
                2. 模型的规模和复杂度
                3. 模型的几何特征
                4. 可能的应用场景
                
                请直接给出分析结果，不要提及技术细节或原始数据。
                """
            else:
                prompt = f"获取模型信息失败：{message}。请用友好的语言告诉用户发生了什么，并建议解决方案。"
        
        elif action == "open_file":
            if success:
                prompt = f"用户成功打开了文件。结果：{message}。请用简洁友好的语言确认文件已打开，并提示用户可以进行下一步操作。"
            else:
                prompt = f"文件打开失败：{message}。请分析可能的原因并给出解决建议。"
        
        elif action in ["camera_control", "change_camera_type"]:
            if success:
                prompt = f"视角调整成功。请简洁地确认操作完成。"
            else:
                prompt = f"视角调整失败：{message}。请说明问题并建议解决方案。"
        
        elif action in ["save_file_as", "save_screenshot"]:
            if success:
                prompt = f"文件保存成功。结果：{message}。请确认保存完成并告知用户文件位置。"
            else:
                prompt = f"文件保存失败：{message}。请分析原因并提供解决建议。"
        
        elif action == "get_model_eight_views":
            if success:
                prompt = f"""
                用户成功获取了模型的八个视角图像。
                {f'图像信息：{image_context}' if image_context else ''}
                
                请告诉用户：
                1. 已成功获取了模型从8个不同角度的视图
                2. 这些视角包括前后、上下、左右的各种组合
                3. 这些多角度图像有助于全面了解模型的3D结构
                4. 用户可以查看保存的图像文件来观察模型的不同侧面
                
                请用简洁友好的语言确认操作完成。
                """
            else:
                prompt = f"获取八视角图像失败：{message}。请分析原因并提供解决建议。"
        
        else:
            # 通用处理
            prompt = f"""
            操作 '{action}' {status}。
            结果：{message}
            {f'图像信息：{image_context}' if image_context else ''}
            
            请用自然、友好的语言向用户报告这个结果，重点说明：
            1. 操作是否成功
            2. 用户得到了什么
            3. 下一步可以做什么（如果适用）
            
            避免显示技术细节和原始数据。
            """
        
        return prompt

    def get_next_command(self, client_id, completed_action, success):
        """获取下一个要执行的命令"""
        if not client_id or client_id not in self.command_queue:
            return None
        
        queue = self.command_queue[client_id]
        if not queue:
            return None
        
        # 移除已完成的命令（队列中的第一个）
        if queue:
            completed_command = queue.pop(0)
            
            # 记录到会话状态
            if client_id in self.active_sessions:
                self.active_sessions[client_id]["results"].append({
                    "action": completed_action,
                    "success": success
                })
        
        # 检查是否还有待执行的命令
        if queue:
            next_command = queue[0]
            next_command["requires_execution"] = True
            return json.dumps(next_command, ensure_ascii=False)
        
        # 所有命令执行完毕，清理会话状态
        if client_id in self.command_queue:
            del self.command_queue[client_id]
        
        return None

    def receive_message(self, client_socket):
        """接收消息"""
        try:
            length_data = client_socket.recv(4)
            if not length_data:
                return None
            length = struct.unpack('i', length_data)[0]

            message_bytes = client_socket.recv(length)
            return message_bytes.decode('utf-8')
        except:
            return None

    def handle_client(self, client_socket, addr):
        """处理客户端连接，支持多工具调用"""
        client_id = f"{addr[0]}:{addr[1]}"
        print(f"客户端 {client_id} 已连接")
        
        try:
            while True:
                # 接收消息
                message = self.receive_message(client_socket)
                if not message:
                    break

                # 检查关闭命令
                if message.strip().upper() in ["SHUTDOWN_SERVER", "CLOSE_SERVER", "EXIT_SERVER"]:
                    self.send_message_to_client(client_socket, {
                        "type": "reply",
                        "content": "服务器关闭中...",
                        "timestamp": str(time.time())
                    })
                    cleanup_and_exit()
                    break

                # 处理消息，传入客户端ID以支持会话状态
                answer = self.process_query_sync(message, client_id)

                # 检查是否是命令并开始执行流程
                self.execute_command_flow(client_socket, answer, client_id)

        except Exception as e:
            print(f"客户端处理错误: {e}")
        finally:
            # 清理客户端状态
            self.cleanup_client(client_id)
            client_socket.close()
            print(f"客户端 {client_id} 已断开连接")

    def execute_command_flow(self, client_socket, answer, client_id):
        """执行命令流程，支持多命令串联"""
        try:
            answer_json = json.loads(answer)
            answer_type = answer_json.get("type", "")
            
            if answer_type == "command":
                # 发送命令并等待结果
                self.send_message_to_client(client_socket, answer_json)
                
                # 等待主进程执行结果
                result = self.receive_message(client_socket)
                if result:
                    # 处理执行结果，可能触发下一个命令
                    next_response = self.process_query_sync(result, client_id)
                    
                    # 递归处理，支持命令链
                    if self.is_command_response(next_response):
                        self.execute_command_flow(client_socket, next_response, client_id)
                    else:
                        # 最终回复
                        self.send_final_reply(client_socket, next_response)
            else:
                # 非命令回复，直接发送
                self.send_final_reply(client_socket, answer)
                
        except json.JSONDecodeError:
            # 不是JSON，作为普通回复处理
            self.send_final_reply(client_socket, answer)

    def is_command_response(self, response):
        """检查响应是否是命令"""
        try:
            response_json = json.loads(response)
            return response_json.get("type") == "command"
        except:
            return False

    def send_final_reply(self, client_socket, content):
        """发送最终回复给客户端"""
        try:
            if isinstance(content, str):
                # 如果是字符串，尝试解析为JSON
                try:
                    content_json = json.loads(content)
                    self.send_message_to_client(client_socket, content_json)
                except:
                    # 不是JSON，包装为reply消息
                    self.send_message_to_client(client_socket, {
                        "type": "reply",
                        "content": content,
                        "timestamp": str(time.time())
                    })
            else:
                self.send_message_to_client(client_socket, content)
        except Exception as e:
            print(f"发送最终回复时出错: {e}")

    def cleanup_client(self, client_id):
        """清理客户端相关状态"""
        if client_id in self.active_sessions:
            del self.active_sessions[client_id]
        if client_id in self.command_queue:
            del self.command_queue[client_id]

    def start(self):
        """启动桥梁服务器"""
        try:
            # 设置信号处理器
            setup_signal_handlers()
            
            # 首先初始化MCP客户端
            self.init_mcp_client()
            
            self.sock.bind((self.host, self.port))
            self.sock.listen(config.MAX_CONNECTIONS)
            print(f"=== MCP Bridge Server ===")
            print(f"桥梁服务器已启动，监听 {self.host}:{self.port}")
            print(f"使用模型: {config.MODEL}")
            print(f"💡 支持关闭命令: SHUTDOWN_SERVER, CLOSE_SERVER, EXIT_SERVER")
            print("等待客户端连接...\n")
            
            while not global_shutdown_event.is_set():
                try:
                    # 设置超时，避免无限阻塞
                    self.sock.settimeout(1.0)
                    client_socket, addr = self.sock.accept()
                    
                    # 为每个客户端创建新线程
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, addr)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    if not global_shutdown_event.is_set():
                        print(f"接受连接时出错: {e}")
                    break
                
        except KeyboardInterrupt:
            print("\n桥梁服务器正在关闭...")
        except Exception as e:
            print(f"桥梁服务器错误: {e}")
        finally:
            self.cleanup()

if __name__ == "__main__":
    bridge = MCPBridge()
    bridge.start() 