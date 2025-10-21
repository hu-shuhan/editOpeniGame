import asyncio
import sys
import os
from Client.client import main as client_main
from bridge_server import MCPBridge
import config

def print_menu():
    """显示菜单选项"""
    print("=== MCP Project ===")
    print(f"当前模型: {config.MODEL}")
    print("1. 启动桥梁服务器 (连接iGameVis客户端到MCP)")
    print("2. 启动MCP客户端 (交互式对话)")
    print("3. 选择AI模型")
    print("4. 退出")

def show_model_menu():
    """显示模型选择菜单"""
    print("\n=== 选择AI模型 ===")
    for key, (model_key, model_name) in config.MODEL_MENU.items():
        current = " (当前)" if model_key == config.SELECTED_MODEL else ""
        print(f"{key}. {model_name}{current}")
    
    choice = input("\n请选择模型 (1-4): ").strip()
    
    if choice in config.MODEL_MENU:
        model_key, model_name = config.MODEL_MENU[choice]
        
        # 读取当前config.py文件
        config_file = "config.py"
        with open(config_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 替换SELECTED_MODEL的值
        lines = content.split('\n')
        for i, line in enumerate(lines):
            if line.strip().startswith('SELECTED_MODEL = '):
                lines[i] = f'SELECTED_MODEL = "{model_key}"  # 可选: "qwen", "gemini", "openai", "claude"'
                break
        
        # 写回文件
        with open(config_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(lines))
        
        print(f"\n✅ 已切换到: {model_name}")
        print("⚠️  请重新启动程序以使配置生效！")
        print("   (退出程序后重新运行 python main.py)")
        input("\n按回车键继续...")
    else:
        print("无效选择")

def start_bridge_server():
    """启动桥梁服务器"""
    print(f"正在启动MCP桥梁服务器...")
    print(f"使用模型: {config.MODEL}")
    bridge = MCPBridge()
    bridge.start()

async def start_mcp_client():
    """启动MCP客户端"""
    server_path = os.path.abspath("Servers/server.py")
    await client_main(server_path)

def main():
    """主函数"""
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()
        if mode == "bridge":
            start_bridge_server()
            return
        elif mode == "client":
            asyncio.run(start_mcp_client())
            return
    
    # 交互式菜单
    while True:
        print_menu()
        try:
            choice = input("\n请选择 (1-4): ").strip()
            
            if choice == "1":
                start_bridge_server()
                break
            elif choice == "2":
                asyncio.run(start_mcp_client())
                break
            elif choice == "3":
                show_model_menu()
            elif choice == "4":
                print("再见！")
                break
            else:
                print("无效选择，请重试。")
                
        except KeyboardInterrupt:
            print("\n程序已被用户停止")
            break
        except Exception as e:
            print(f"错误: {e}")

if __name__ == "__main__":
    main()
