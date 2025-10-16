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
        
        # print(f"桥梁服务器初始化完成，监听 {self.host}:{self.port}")

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
                # print("创建共享的MCP客户端实例...")
                self.mcp_client = MCPClient()
                server_script_path = os.path.join(os.path.dirname(__file__), "Servers", "server.py")
                await self.mcp_client.connect_to_server(server_script_path)
                # print("共享MCP客户端创建成功")
            
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
        """处理查询消息，支持文本和JSON格式"""
        if not self.mcp_client or not self.mcp_loop:
            return "MCP客户端未初始化"

        try:
            # 尝试解析为JSON
            message_json = json.loads(message)
            message_type = message_json.get("type", "unknown")

            if message_type == "question" or message_type == "query":
                # 新的用户问题，使用增强的AI处理
                return self.process_user_question(message_json.get('content', ''), client_id)
            elif message_type == "operation_result" or message_type == "execution_result":
                # 特殊处理操作结果，特别是包含图像的结果
                return self.handle_operation_result(message_json, client_id)
            else:
                ai_prompt = f"收到消息: {json.dumps(message_json, ensure_ascii=False)}\n请根据消息内容回答。"
                return self.process_ai_query(ai_prompt)
        except json.JSONDecodeError:
            # 不是JSON，按普通文本处理
            return self.process_user_question(message, client_id)

    def process_user_question(self, user_question, client_id=None):
        """处理用户问题，简化逻辑：判断返回类型并相应处理"""
        try:
            print(f"🤖 [DEBUG] 开始处理用户问题: {user_question}")
            
            # 为客户端创建会话状态
            if client_id and client_id not in self.active_sessions:
                self.active_sessions[client_id] = {
                    "commands": [],
                    "results": [],
                    "stage": "initial"
                }

            # 简洁的AI提示
            ai_prompt = f"用户问题: {user_question}"
            print(f"🤖 [DEBUG] 发送给MCP客户端的提示: {ai_prompt}")

            # 让AI处理
            future = asyncio.run_coroutine_threadsafe(
                self.mcp_client.process_query(ai_prompt),
                self.mcp_loop
            )

            ai_result = future.result(timeout=30)
            print(f"🤖 [DEBUG] MCP客户端返回结果: {ai_result}")
            
            # 检查AI结果是否是command类型JSON
            try:
                result_json = json.loads(ai_result)
                if isinstance(result_json, dict) and result_json.get("type") == "command":
                    # 是command类型 - 发送给主进程执行并等待结果
                    print(f"⚡ [DEBUG] MCP客户端返回command类型，发送给主进程执行")
                    print(f"⚡ [DEBUG] 命令详情: {json.dumps(result_json, ensure_ascii=False, indent=2)}")
                    return ai_result  # 直接返回，格式已经正确
                else:
                    # 不是command类型 - AI已经回答了，发送给主进程显示
                    print(f"💬 [DEBUG] MCP客户端返回非command类型，AI已回答")
                    return ai_result
            except json.JSONDecodeError:
                # 不是JSON - AI已经回答了，发送给主进程显示
                print(f"💬 [DEBUG] MCP客户端返回非JSON格式，AI已回答")
                return ai_result

        except Exception as e:
            error_msg = f"处理用户问题时出错: {str(e)}"
            print(f"❌ [DEBUG] {error_msg}")
            return error_msg

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
                    # 如果检测到图像，处理图像信息但不保存
                    image_result = self.process_images_info(images, "工具执行结果", processed_content)
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
                image_result = self.process_images_info(images, "AI回复", processed_result)
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

    def process_images_info(self, images, context="", text_content=""):
        """
        处理图像信息，不保存到本地，只返回图像数量和基本信息
        Args:
            images: 提取的图像列表
            context: 上下文信息
            text_content: 文本内容
        Returns:
            str: 处理结果消息
        """
        if not images:
            return f"{context}\n{text_content}"
        
        print(f"🖼️ 检测到 {len(images)} 个图像")
        valid_images = 0
        
        for i, image_info in enumerate(images):
            image_data_b64 = image_info.get("data", "")
            image_key = image_info.get("key", f"image_{i}")
            
            # 检查图像数据有效性
            if image_data_b64 and image_data_b64 not in ["no_renderer", "image_null", ""]:
                valid_images += 1
                print(f"✅ 图像 {image_key}: 有效")
            else:
                print(f"❌ 图像 {image_key}: 无效的图像数据")
        
        # 返回简洁的信息
        if valid_images > 0:
            return f"✅ 操作完成，检测到 {valid_images} 个有效图像"
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
            # 如果有图像，收集图像信息（但不保存到本地）
            image_context = ""
            if has_images and images:
                valid_images = 0
                for i, image_info in enumerate(images):
                    image_data_b64 = image_info.get("data", "")
                    image_key = image_info.get("key", f"image_{i}")
                    
                    if image_data_b64 and image_data_b64 not in ["no_renderer", "image_null", ""]:
                        valid_images += 1
                        print(f"✅ 图像 {image_key}: 有效")
                    else:
                        print(f"❌ 图像 {image_key}: 无效的图像数据")
                
                if valid_images > 0:
                    image_context = f"检测到 {valid_images} 个有效图像"
            
            # 构建完整的查询信息
            info_parts = []
            
            # 添加操作状态
            status = "成功" if success else "失败"
            info_parts.append(f"操作: {action} ({status})")
            
            # 添加原始消息内容
            if message:
                info_parts.append(f"详细信息: {message}")
            
            # 添加图像上下文
            if image_context:
                info_parts.append(f"图像处理: {image_context}")
            
            # 构建完整的查询文本
            full_info = "\n".join(info_parts)
            
            # 构建简洁的AI查询
            query = f"操作结果: {full_info}\n\n请用自然语言告诉用户操作结果。"
            
            # 统一调用AI查询，自动处理图像数据
            future = asyncio.run_coroutine_threadsafe(
                self.mcp_client.process_query(query, images if has_images else None),
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
        """处理客户端连接，支持双向JSON消息通信"""
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

                # 处理不同类型的消息
                self.handle_message(client_socket, message, client_id)

        except Exception as e:
            print(f"客户端处理错误: {e}")
        finally:
            # 清理客户端状态
            self.cleanup_client(client_id)
            client_socket.close()
            print(f"客户端 {client_id} 已断开连接")

    def handle_message(self, client_socket, message, client_id):
        """处理不同类型的消息"""
        try:
            print(f"📨 [DEBUG] 收到消息: {message}")
            
            # 尝试解析JSON消息
            try:
                message_json = json.loads(message)
                message_type = message_json.get("type", "")
                print(f"📨 [DEBUG] 解析为JSON消息，类型: {message_type}")
                
                if message_type == "query" or message_type == "question":
                    # 主进程发送的问题
                    print(f"❓ [DEBUG] 收到主进程问题: {message_json.get('content', '')}")
                    self.handle_query_message(client_socket, message_json, client_id)
                    
                elif message_type == "execution_result" or message_type == "operation_result":
                    # 主进程返回的执行结果
                    print(f"✅ [DEBUG] 收到主进程执行结果: {message_json.get('content', '')}")
                    self.handle_execution_result(client_socket, message_json, client_id)
                    
                else:
                    # 其他类型消息，按普通文本处理
                    print(f"❓ [DEBUG] 收到未知类型消息: {message_type}")
                    self.handle_text_message(client_socket, message, client_id)
                    
            except json.JSONDecodeError:
                # 不是JSON，按普通文本消息处理
                print(f"📝 [DEBUG] 不是JSON，按文本消息处理: {message}")
                self.handle_text_message(client_socket, message, client_id)
                
        except Exception as e:
            print(f"❌ [DEBUG] 处理消息时出错: {e}")
            self.send_message_to_client(client_socket, {
                "type": "error",
                "content": f"处理消息时出错: {str(e)}",
                "timestamp": str(time.time())
            })

    def handle_query_message(self, client_socket, message_json, client_id):
        """处理主进程发送的问题"""
        query_content = message_json.get("content", "")
        
        # 调用AI处理问题
        answer = self.process_query_sync(query_content, client_id)
        
        # 检查AI回答是否是命令
        try:
            answer_json = json.loads(answer)
            if answer_json.get("type") == "command":
                # 是命令，发送给主进程执行
                print(f"AI返回命令，发送给主进程执行: {answer_json.get('command', '')}")
                
                # 保存会话状态，等待执行结果
                if client_id not in self.active_sessions:
                    self.active_sessions[client_id] = {}
                self.active_sessions[client_id]["waiting_for_execution"] = True
                self.active_sessions[client_id]["original_query"] = query_content
                
                # 提取命令内容并发送给主进程
                print(f"🔍 [DEBUG] handle_query_message解析到的命令JSON结构: {json.dumps(answer_json, ensure_ascii=False, indent=2)}")
                
                # 直接发送标准格式的命令给主进程
                print(f"📤 [DEBUG] handle_query_message发送标准格式命令给主进程: {json.dumps(answer_json, ensure_ascii=False)}")
                self.send_message_to_client(client_socket, answer_json)
            else:
                # 不是命令，直接回复
                self.send_message_to_client(client_socket, {
                    "type": "reply",
                    "content": answer,
                    "timestamp": str(time.time())
                })
        except json.JSONDecodeError:
            # AI回答不是JSON，直接回复
            self.send_message_to_client(client_socket, {
                "type": "reply", 
                "content": answer,
                "timestamp": str(time.time())
            })

    def handle_execution_result(self, client_socket, message_json, client_id):
        """处理主进程返回的执行结果"""
        execution_result = message_json.get("content", "")
        print(f"🔄 [DEBUG] 处理执行结果: {execution_result}")
        
        # 检查是否有等待中的会话
        if client_id in self.active_sessions and self.active_sessions[client_id].get("waiting_for_execution"):
            original_query = self.active_sessions[client_id].get("original_query", "")
            print(f"🔄 [DEBUG] 找到等待中的会话，原始问题: {original_query}")
            
            # 基于执行结果让AI生成最终回答
            result_prompt = f"用户问题：{original_query}\n命令执行结果：{execution_result}\n请基于执行结果用自然语言回答用户的问题。"
            print(f"🔄 [DEBUG] 生成最终回答的提示: {result_prompt}")
            
            final_answer = self.process_query_sync(result_prompt, client_id)
            print(f"🔄 [DEBUG] AI生成的最终回答: {final_answer}")
            
            # 发送最终回答
            reply_message = {
                "type": "reply",
                "content": final_answer,
                "timestamp": str(time.time())
            }
            print(f"📤 [DEBUG] 发送最终回答: {json.dumps(reply_message, ensure_ascii=False)}")
            self.send_message_to_client(client_socket, reply_message)
            
            # 清理会话状态
            self.active_sessions[client_id]["waiting_for_execution"] = False
            print(f"🔄 [DEBUG] 会话状态已清理")
        else:
            print(f"❌ [DEBUG] 收到执行结果但没有等待中的会话: {client_id}")
            print(f"❌ [DEBUG] 当前会话状态: {self.active_sessions.get(client_id, '无')}")

    def handle_text_message(self, client_socket, message, client_id):
        """处理普通文本消息（兼容旧版本）"""
        print(f"🔍 [DEBUG] 处理文本消息: {message}")
        
        # 调用AI处理
        answer = self.process_query_sync(message, client_id)
        print(f"🔍 [DEBUG] AI返回结果: {answer}")
        
        # 检查是否是命令
        try:
            answer_json = json.loads(answer)
            if answer_json.get("type") == "command":
                # 是命令，发送给主进程执行
                print(f"✅ [DEBUG] 检测到命令类型: {answer_json.get('command', '')}")
                print(f"🔄 [DEBUG] 保存会话状态，等待主进程执行...")
                
                # 保存会话状态
                if client_id not in self.active_sessions:
                    self.active_sessions[client_id] = {}
                self.active_sessions[client_id]["waiting_for_execution"] = True
                self.active_sessions[client_id]["original_query"] = message
                
                # 提取命令内容并发送给主进程
                print(f"🔍 [DEBUG] 解析到的命令JSON结构: {json.dumps(answer_json, ensure_ascii=False, indent=2)}")
                
                # 直接发送标准格式的命令给主进程
                print(f"📤 [DEBUG] handle_text_message发送标准格式命令给主进程: {json.dumps(answer_json, ensure_ascii=False)}")
                self.send_message_to_client(client_socket, answer_json)
            else:
                # 不是命令，直接回复
                print(f"📝 [DEBUG] 不是命令，直接回复: {answer}")
                self.send_message_to_client(client_socket, {
                    "type": "reply",
                    "content": answer,
                    "timestamp": str(time.time())
                })
        except json.JSONDecodeError:
            # 不是JSON，直接回复
            print(f"📝 [DEBUG] 不是JSON格式，直接回复: {answer}")
            self.send_message_to_client(client_socket, {
                "type": "reply",
                "content": answer,
                "timestamp": str(time.time())
            })



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
            print(f"Server started on {self.host}:{self.port}")
            print(f"Model: {config.MODEL}")
            
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