# -*- coding: utf-8 -*-
"""
iGameVis Chat Server
专门用于处理 iGameVis 聊天消息的服务器
连接到 iGameVis 的 8080 端口，接收聊天消息并通过 AI 回答
"""

import asyncio
import socket
import struct
import json
import sys
import os
import logging
from datetime import datetime

# 添加当前目录和 Client 目录到路径
current_dir = os.path.dirname(os.path.abspath(__file__))
client_dir = os.path.join(current_dir, 'Client')
sys.path.insert(0, current_dir)
sys.path.insert(0, client_dir)

# 导入 MCP 客户端
try:
    from Client.iGameVis_Client import iGameVisMCPClient
except ImportError as e:
    print(f"错误：无法导入 iGameVisMCPClient: {e}")
    print(f"当前目录: {current_dir}")
    print(f"Client 目录: {client_dir}")
    print(f"sys.path: {sys.path}")
    sys.exit(1)

# 导入配置
import config

# 配置日志
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format=config.LOG_FORMAT
)
logger = logging.getLogger(__name__)


class iGameVisChatServer:
    """iGameVis 聊天服务器 - 连接到 iGameVis 8080 端口并处理聊天"""
    
    def __init__(self, host='localhost', port=8080):
        self.host = host
        self.port = port
        self.socket = None
        self.mcp_client = None
        self.running = False
        
    async def connect_to_igamevis(self):
        """连接到 iGameVis 的聊天端口"""
        try:
            logger.info(f"正在连接到 iGameVis 聊天端口 {self.host}:{self.port}...")
            
            # 创建 socket 连接
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.socket.settimeout(1.0)  # 设置超时以便能响应停止信号
            
            logger.info(f"✅ 已连接到 iGameVis 聊天端口 {self.host}:{self.port}")
            return True
            
        except Exception as e:
            logger.error(f"❌ 连接到 iGameVis 失败: {e}")
            return False
    
    async def initialize_mcp_client(self):
        """初始化 MCP 客户端（用于 AI 回答）"""
        try:
            logger.info("正在初始化 MCP 客户端...")
            
            self.mcp_client = iGameVisMCPClient()
            
            # 连接到 MCP 服务器
            server_path = os.path.join(
                os.path.dirname(__file__),
                "Servers",
                "iGameVis_Server.py"
            )
            
            if not os.path.exists(server_path):
                logger.error(f"❌ MCP 服务器文件不存在: {server_path}")
                return False
            
            if not await self.mcp_client.connect_to_server(server_path):
                logger.error("❌ 无法连接到 MCP 服务器")
                return False
            
            logger.info("✅ MCP 客户端初始化成功")
            return True
            
        except Exception as e:
            logger.error(f"❌ 初始化 MCP 客户端失败: {e}")
            return False
    
    def send_message(self, message_dict):
        """发送消息到 iGameVis"""
        try:
            # 将消息转换为 JSON
            message_json = json.dumps(message_dict, ensure_ascii=False)
            message_bytes = message_json.encode('utf-8')
            
            # 发送消息长度（4字节，小端序）
            length = len(message_bytes)
            self.socket.sendall(struct.pack('<I', length))
            
            # 发送消息内容
            self.socket.sendall(message_bytes)
            
            logger.debug(f"发送消息到 iGameVis: {message_json[:100]}...")
            
        except Exception as e:
            logger.error(f"发送消息失败: {e}")
            raise
    
    def receive_message(self):
        """从 iGameVis 接收消息"""
        try:
            # 接收消息长度（4字节，小端序）
            length_bytes = b''
            while len(length_bytes) < 4:
                try:
                    chunk = self.socket.recv(4 - len(length_bytes))
                    if not chunk:
                        return None  # 连接已关闭
                    length_bytes += chunk
                except socket.timeout:
                    if not self.running:
                        return None
                    continue
            
            length = struct.unpack('<I', length_bytes)[0]
            
            # 接收消息内容
            message_bytes = b''
            while len(message_bytes) < length:
                try:
                    chunk = self.socket.recv(length - len(message_bytes))
                    if not chunk:
                        return None  # 连接已关闭
                    message_bytes += chunk
                except socket.timeout:
                    if not self.running:
                        return None
                    continue
            
            # 解码消息
            message_json = message_bytes.decode('utf-8')
            message_dict = json.loads(message_json)
            
            logger.debug(f"从 iGameVis 接收消息: {message_json[:100]}...")
            
            return message_dict
            
        except socket.timeout:
            return None
        except Exception as e:
            logger.error(f"接收消息失败: {e}")
            return None
    
    async def handle_chat_message(self, message_dict):
        """处理聊天消息"""
        try:
            msg_type = message_dict.get('type', '')
            content = message_dict.get('content', '')
            
            logger.info(f"收到消息类型: {msg_type}")
            
            if msg_type == 'chat' or msg_type == 'message':
                # 用户的聊天消息，需要 AI 回答
                logger.info(f"用户消息: {content}")
                
                # 发送确认消息
                ack_message = {
                    'type': 'ack',
                    'message': '消息已收到，正在处理...',
                    'timestamp': datetime.now().isoformat()
                }
                self.send_message(ack_message)
                
                # 使用 MCP 客户端获取 AI 回答
                logger.info("正在获取 AI 回答...")
                ai_response = await self.mcp_client.process_user_message(content)
                
                logger.info(f"AI 回答: {ai_response[:100]}...")
                
                # 发送 AI 回答
                response_message = {
                    'type': 'response',
                    'content': ai_response,
                    'timestamp': datetime.now().isoformat()
                }
                self.send_message(response_message)
                
            elif msg_type == 'ping':
                # 响应 ping
                pong_message = {
                    'type': 'pong',
                    'timestamp': datetime.now().isoformat()
                }
                self.send_message(pong_message)
                logger.debug("响应 ping")
                
            elif msg_type == 'ack':
                # 确认消息，不需要响应
                logger.debug("收到确认消息")
                
            else:
                logger.warning(f"未知消息类型: {msg_type}")
                
        except Exception as e:
            logger.error(f"处理消息时出错: {e}")
            # 发送错误消息
            try:
                error_message = {
                    'type': 'error',
                    'message': f'处理消息时出错: {str(e)}',
                    'timestamp': datetime.now().isoformat()
                }
                self.send_message(error_message)
            except:
                pass
    
    async def run(self):
        """运行聊天服务器"""
        try:
            # 1. 初始化 MCP 客户端
            if not await self.initialize_mcp_client():
                logger.error("❌ 初始化失败")
                return
            
            # 2. 连接到 iGameVis
            if not await self.connect_to_igamevis():
                logger.error("❌ 连接失败")
                return
            
            logger.info("✅ iGameVis 聊天服务器已启动")
            logger.info("等待接收聊天消息...")
            
            self.running = True
            
            # 3. 消息循环
            while self.running:
                # 接收消息
                message = self.receive_message()
                
                if message is None:
                    if not self.running:
                        break
                    continue
                
                # 处理消息
                await self.handle_chat_message(message)
            
            logger.info("聊天服务器已停止")
            
        except KeyboardInterrupt:
            logger.info("收到中断信号，正在停止...")
        except Exception as e:
            logger.error(f"运行时错误: {e}")
            logger.exception("详细错误:")
        finally:
            await self.cleanup()
    
    async def cleanup(self):
        """清理资源"""
        try:
            self.running = False
            
            if self.socket:
                try:
                    self.socket.close()
                    logger.info("Socket 连接已关闭")
                except:
                    pass
            
            if self.mcp_client:
                try:
                    await self.mcp_client.cleanup()
                    logger.info("MCP 客户端已清理")
                except:
                    pass
                    
        except Exception as e:
            logger.error(f"清理资源时出错: {e}")


async def main():
    """主函数"""
    # 使用 logger 而不是 print，避免编码问题
    logger.info("=" * 60)
    logger.info("iGameVis Chat Server")
    logger.info("=" * 60)
    logger.info("专门用于处理 iGameVis 聊天消息")
    logger.info("连接到 iGameVis 的 8080 端口")
    logger.info("=" * 60)
    
    # 创建并运行聊天服务器
    chat_server = iGameVisChatServer(host='localhost', port=8080)
    await chat_server.run()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("收到中断信号，程序退出")

