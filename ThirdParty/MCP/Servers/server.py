import os
from pathlib import Path
import json
import base64
from datetime import datetime

from dotenv import load_dotenv
from mcp.server.fastmcp import FastMCP

import logging

# 关闭MCP调试日志
logging.getLogger('mcp').setLevel(logging.WARNING)
logging.getLogger('mcp.server').setLevel(logging.WARNING)

# 加载环境变量
load_dotenv()

# 创建 MCP Server
mcp = FastMCP("CAD辅助工具")

def create_json_response(response_type: str, content, timestamp: str = None) -> dict:
    """Create a standardized JSON response."""
    from datetime import datetime
    return {
        "type": response_type,
        "content": content,
        "timestamp": timestamp or datetime.now().isoformat()
    }

def create_json_reply(message: str) -> str:
    """Create a JSON reply message."""
    response = create_json_response("reply", message)
    return json.dumps(response, ensure_ascii=False, indent=2)

def create_json_command(action: str, data: dict = None, message: str = None) -> str:
    """Create a JSON command message."""
    command_content = {
        "action": action,
        "data": data or {},
        "message": message or ""
    }
    response = create_json_response("command", command_content)
    return json.dumps(response, ensure_ascii=False, indent=2)

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

@mcp.tool()
def get_user_desktop_path() -> str:
    """Get the desktop path for the current user."""
    try:
        desktop_path = get_desktop_path()
        message = f"桌面路径: {str(desktop_path)}"
        return create_json_reply(message)
    except Exception as e:
        return create_json_reply(f"获取桌面路径时发生错误: {e}")

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
            return create_json_reply(f"No files with extensions {extensions} found on desktop.")

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

        return create_json_reply("\n".join(output))
    except Exception as e:
        return create_json_reply(f"Error accessing desktop: {e}")

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
            return create_json_reply(f"Directory '{directory_path}' does not exist.")
        
        if not search_path.is_dir():
            return create_json_reply(f"'{directory_path}' is not a directory.")
        
        if extensions is None:
            matching_files = [f for f in search_path.iterdir() if f.is_file()]
        else:
            search_patterns = [f'*.{ext.lower().strip(".")}' for ext in extensions]
            matching_files = []
            for pattern in search_patterns:
                matching_files.extend(list(search_path.glob(pattern)))
        
        if not matching_files:
            if extensions:
                return create_json_reply(f"No files with extensions {extensions} found in '{directory_path}'.")
            else:
                return create_json_reply(f"No files found in '{directory_path}'.")
        
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
        
        return create_json_reply("\n".join(output))
        
    except Exception as e:
        return create_json_reply(f"Error accessing directory '{directory_path}': {e}")

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
            return create_json_reply(f"File '{filename}' not found on desktop.")
        elif len(matching_files) == 1:
            return create_json_reply(str(matching_files[0]).replace("\\", "/"))
        else:
            file_list = '\n'.join([f"- {file.name}: {str(file).replace('\\', '/')}" for file in matching_files])
            return create_json_reply(f"Multiple files matching '{filename}' found:\n{file_list}")
            
    except Exception as e:
        return create_json_reply(f"Error searching for file: {e}")

@mcp.tool()
def open_file_with_path(file_path: str) -> str:
    """Open a file in the C++ application by providing its full path.
    Args:
        file_path (str): Full path to the file to open
    Returns:
        str: JSON command to open the file or error message
    """
    try:
        file_path_obj = Path(file_path)
        if not file_path_obj.exists():
            return create_json_reply(f"Error: File '{file_path}' does not exist.")
        
        if not file_path_obj.is_file():
            return create_json_reply(f"Error: '{file_path}' is not a file.")
        
        return create_json_command("open_file", {
            "file_path": str(file_path_obj.absolute()).replace("\\", "/"),
            "file_name": file_path_obj.name,
            "file_extension": file_path_obj.suffix.lower()
        })
        
    except Exception as e:
        return create_json_reply(f"Error opening file: {e}")

@mcp.tool()
def open_desktop_file(filename: str) -> str:
    """Find and open a specific file from the desktop in the C++ application.
    Args:
        filename (str): EXACT name of the file to find and open
    Returns:
        str: JSON command to open the file or error message
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
            return create_json_reply(f"Error: File '{filename}' not found on desktop.")
        elif len(matching_files) == 1:
            return open_file_with_path(str(matching_files[0]).replace("\\", "/"))
        else:
            file_list = '\n'.join([f"- {file.name}: {str(file).replace('\\', '/')}" for file in matching_files])
            return create_json_reply(f"Multiple files matching '{filename}' found:\n{file_list}\n\nPlease use 'open_file_with_path' with the specific file path.")
            
    except Exception as e:
        return create_json_reply(f"Error searching for file: {e}")

@mcp.tool()
def open_file(file_path_or_name: str) -> str:
    """Open a file in the C++ application. This is the primary tool for opening files.
    Args:
        file_path_or_name (str): EXACT filename or full file path
    Returns:
        str: JSON command to open the file or error message
    """
    try:
        file_path_obj = Path(file_path_or_name)
        if file_path_obj.exists() and file_path_obj.is_file():
            return open_file_with_path(str(file_path_obj).replace("\\", "/"))
        
        return open_desktop_file(file_path_or_name)
        
    except Exception as e:
        return create_json_reply(f"Error opening file: {e}")

@mcp.tool()
def get_file_info(file_path: str) -> str:
    """Get information about a file.
    Args:
        file_path (str): Full path to the file
    Returns:
        str: File information including name, size, and path
    """
    try:
        file_path_obj = Path(file_path)
        if not file_path_obj.exists():
            return create_json_reply(f"Error: File '{file_path}' does not exist.")
        
        file_size = file_path_obj.stat().st_size
        file_size_mb = file_size / (1024 * 1024)
        
        info = f"""
文件信息:
- 文件名: {file_path_obj.name}
- 格式: {file_path_obj.suffix.lower()}
- 大小: {round(file_size_mb, 2)} MB
- 路径: {str(file_path_obj.absolute()).replace("\\", "/")}
"""
        
        return create_json_reply(info)
        
    except Exception as e:
        return create_json_reply(f"Error getting file info: {e}")

@mcp.tool()
def get_model_info() -> str:
    """获取当前模型信息"""
    return create_json_command("get_model_info", {}, "")

@mcp.tool()
def analyze_model_summary(model_info: str, has_image: bool = False, image_description: str = "") -> str:
    """
    基于模型信息和图像生成模型总结描述
    Args:
        model_info (str): 模型的详细信息（文件名、类型、数据统计等）
        has_image (bool): 是否有模型图像
        image_description (str): 图像的描述信息（如果有的话）
    Returns:
        str: 模型的总结性描述
    """
    try:
        # 解析模型信息，提取关键数据
        lines = model_info.split('\n')
        
        # 提取关键信息
        file_name = ""
        file_path = ""
        mesh_type = ""
        stats_info = []
        memory_info = ""
        bbox_info = []
        
        for line in lines:
            line = line.strip()
            if line.startswith("文件名:"):
                file_name = line.replace("文件名:", "").strip()
            elif line.startswith("路径:"):
                file_path = line.replace("路径:", "").strip()
            elif line.startswith("类型:"):
                mesh_type = line.replace("类型:", "").strip()
            elif line.startswith("内存:"):
                memory_info = line.replace("内存:", "").strip()
            elif "# of" in line or "# of Dimension" in line:
                stats_info.append(line.strip())
            elif any(axis in line for axis in ["X:", "Y:", "Z:"]):
                bbox_info.append(line.strip())
        
        # 生成智能总结
        summary_parts = []
        
        # 1. 基本识别
        if file_name and file_name != "(n/a)":
            summary_parts.append(f"这是一个名为 '{file_name}' 的3D模型。")
        else:
            summary_parts.append("这是一个3D模型。")
        
        # 2. 类型分析
        type_descriptions = {
            "Surface Mesh": "表面网格模型，主要用于显示和渲染物体的外表面",
            "Volume Mesh": "体积网格模型，常用于有限元分析和体积计算",
            "Structured Mesh": "结构化网格模型，具有规则的网格拓扑结构",
            "Unstructured Mesh": "非结构化网格模型，网格单元分布灵活",
            "Multiblock Mesh": "多块网格模型，由多个子网格块组成的复合模型"
        }
        
        if mesh_type in type_descriptions:
            summary_parts.append(type_descriptions[mesh_type] + "。")
        elif mesh_type:
            summary_parts.append(f"模型类型为{mesh_type}。")
        
        # 3. 规模分析
        scale_info = []
        for stat in stats_info:
            if "Faces" in stat:
                face_count = int(''.join(filter(str.isdigit, stat)))
                if face_count > 1000000:
                    scale_info.append("高精度模型（超过100万个面）")
                elif face_count > 100000:
                    scale_info.append("中等精度模型（10万-100万个面）")
                elif face_count > 10000:
                    scale_info.append("一般精度模型（1万-10万个面）")
                else:
                    scale_info.append("低精度模型（少于1万个面）")
            elif "Points" in stat:
                point_count = int(''.join(filter(str.isdigit, stat)))
                scale_info.append(f"包含{format_number(point_count)}个顶点")
            elif "Volumes" in stat or "Cells" in stat:
                cell_count = int(''.join(filter(str.isdigit, stat)))
                scale_info.append(f"包含{format_number(cell_count)}个体积单元")
        
        if scale_info:
            summary_parts.append("从数据规模来看，" + "，".join(scale_info) + "。")
        
        # 4. 内存占用分析
        if memory_info:
            if "GB" in memory_info:
                summary_parts.append("这是一个大型模型，内存占用较高。")
            elif "MB" in memory_info:
                mb_value = float(''.join(filter(str.isdigit, memory_info.split("MB")[0])))
                if mb_value > 100:
                    summary_parts.append("这是一个中等规模的模型。")
                else:
                    summary_parts.append("这是一个轻量级模型。")
        
        # 5. 几何特征分析
        if bbox_info:
            # 分析边界框，判断模型的几何特征
            dimensions = []
            for bbox_line in bbox_info:
                if "delta:" in bbox_line:
                    delta_value = float(bbox_line.split("delta:")[-1].strip())
                    dimensions.append(delta_value)
            
            if len(dimensions) == 3:
                max_dim = max(dimensions)
                min_dim = min(dimensions)
                ratio = max_dim / min_dim if min_dim > 0 else 1
                
                if ratio > 10:
                    summary_parts.append("模型在某个方向上显著拉长，可能是管道、梁或类似的细长结构。")
                elif ratio > 3:
                    summary_parts.append("模型具有明显的主方向，可能是板状或条状结构。")
                else:
                    summary_parts.append("模型在各个方向上比较均匀，接近立方体或球形结构。")
        
        # 6. 图像信息整合
        # 如果有图像，不再添加额外的图像描述文字
        
        # 7. 应用场景推测
        application_hints = []
        if "Volume Mesh" in mesh_type:
            application_hints.append("适用于结构分析、热传导分析等工程仿真")
        elif "Surface Mesh" in mesh_type:
            application_hints.append("适用于渲染显示、表面处理等应用")
        
        if file_name:
            name_lower = file_name.lower()
            if any(word in name_lower for word in ["engine", "motor", "机械"]):
                application_hints.append("可能是机械工程相关的模型")
            elif any(word in name_lower for word in ["building", "house", "建筑"]):
                application_hints.append("可能是建筑或结构工程相关的模型")
            elif any(word in name_lower for word in ["bio", "medical", "生物", "医学"]):
                application_hints.append("可能是生物医学相关的模型")
        
        if application_hints:
            summary_parts.append("根据特征分析，" + "，".join(application_hints) + "。")
        
        # 组合最终总结
        final_summary = " ".join(summary_parts)
        
        return create_json_reply(final_summary)
        
    except Exception as e:
        return create_json_reply(f"分析模型信息时发生错误: {str(e)}")

def format_number(num):
    """格式化大数字显示"""
    if num >= 1000000:
        return f"{num/1000000:.1f}百万"
    elif num >= 1000:
        return f"{num/1000:.1f}千"
    else:
        return str(num)

def create_json_commands(commands: list) -> str:
    """Create a JSON message containing multiple commands."""
    response = create_json_response("commands", {"commands": commands})
    return json.dumps(response, ensure_ascii=False, indent=2)

@mcp.tool()
def open_file_and_get_info(file_path_or_name: str) -> str:
    """
    打开文件并获取模型信息 - 多步骤操作示例
    Args:
        file_path_or_name (str): 文件名或完整路径
    Returns:
        str: 包含多个命令的JSON响应
    """
    try:
        # 构建命令序列
        commands = []
        
        # 第一步：打开文件
        file_path_obj = Path(file_path_or_name)
        if file_path_obj.exists() and file_path_obj.is_file():
            commands.append({
                "type": "command",
                "content": {
                    "action": "open_file",
                    "data": {
                        "file_path": str(file_path_obj.absolute()).replace("\\", "/"),
                        "file_name": file_path_obj.name,
                        "file_extension": file_path_obj.suffix.lower()
                    },
                    "message": f"打开文件: {file_path_obj.name}"
                }
            })
        else:
            # 尝试从桌面查找
            desktop_path = get_desktop_path()
            desktop_file = desktop_path / file_path_or_name
            if desktop_file.exists():
                commands.append({
                    "type": "command", 
                    "content": {
                        "action": "open_file",
                        "data": {
                            "file_path": str(desktop_file.absolute()).replace("\\", "/"),
                            "file_name": desktop_file.name,
                            "file_extension": desktop_file.suffix.lower()
                        },
                        "message": f"从桌面打开文件: {desktop_file.name}"
                    }
                })
            else:
                return create_json_reply(f"文件未找到: {file_path_or_name}")
        
        # 第二步：获取模型信息
        commands.append({
            "type": "command",
            "content": {
                "action": "get_model_info",
                "data": {},
                "message": "获取模型信息"
            }
        })
        
        return create_json_commands(commands)
        
    except Exception as e:
        return create_json_reply(f"创建命令序列时出错: {str(e)}")



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
        str: JSON command to control camera
    """
    valid_control_types = ["view", "position", "target", "zoom", "rotate"]

    if control_type not in valid_control_types:
        return create_json_reply(f"Error: Invalid control type '{control_type}'. Valid types are: {', '.join(valid_control_types)}")

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
            return create_json_reply(f"Error: Invalid view type '{view_type}'. Valid formats:\n" + "\n".join(valid_examples))
        data["view_type"] = normalized_view_type

    elif control_type == "position":
        data.update({"x": x, "y": y, "z": z})

    elif control_type == "target":
        data.update({"x": x, "y": y, "z": z})

    elif control_type == "zoom":
        if factor <= 0:
            return create_json_reply("Error: Zoom factor must be greater than 0")
        data["factor"] = factor

    elif control_type == "rotate":
        data.update({"angle_x": angle_x, "angle_y": angle_y, "angle_z": angle_z})

    return create_json_command("camera_control", data)

# ============================================================================
# 文件操作工具
# ============================================================================

@mcp.tool()
def save_file_as(file_path: str = "", file_name: str = "") -> str:
    """另存为文件到指定路径
    Args:
        file_path (str): 保存文件的完整路径
        file_name (str): 文件名（可选，如果file_path已包含文件名）
    """
    data = {}
    if file_path:
        data["file_path"] = file_path
    if file_name:
        data["file_name"] = file_name
    return create_json_command("save_file_as", data)

@mcp.tool()
def save_screenshot(file_path: str, width: int = 1920, height: int = 1080) -> str:
    """保存屏幕截图
    Args:
        file_path (str): 保存截图的完整路径
        width (int): 截图宽度，默认1920
        height (int): 截图高度，默认1080
    """
    data = {
        "file_path": file_path,
        "width": width,
        "height": height
    }
    return create_json_command("save_screenshot", data)

# ============================================================================
# 视图和显示控制工具
# ============================================================================

@mcp.tool()
def change_background_color(r: int, g: int, b: int) -> str:
    """更改背景颜色
    Args:
        r (int): 红色分量 (0-255)
        g (int): 绿色分量 (0-255)
        b (int): 蓝色分量 (0-255)
    """
    data = {
        "r": max(0, min(255, r)),
        "g": max(0, min(255, g)),
        "b": max(0, min(255, b))
    }
    return create_json_command("change_background_color", data)

@mcp.tool()
def toggle_colorbar() -> str:
    """切换颜色条的显示/隐藏"""
    return create_json_command("toggle_colorbar", {})

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
        return create_json_reply(f"Error: Invalid camera type '{camera_type}'. Valid types are: {', '.join(valid_types)}")

    data = {"camera_type": camera_type.lower()}
    return create_json_command("change_camera_type", data)

# ============================================================================
# 模型操作工具
# ============================================================================

@mcp.tool()
def delete_current_model() -> str:
    """删除当前选中的模型"""
    return create_json_command("delete_current_model", {})

@mcp.tool()
def show_model_tree() -> str:
    """显示模型树窗口"""
    return create_json_command("show_model_tree", {})

@mcp.tool()
def show_scalar_field() -> str:
    """显示标量场窗口"""
    return create_json_command("show_scalar_field", {})

@mcp.tool()
def show_vector_field() -> str:
    """显示矢量场窗口"""
    return create_json_command("show_vector_field", {})

@mcp.tool()
def show_tensor_field() -> str:
    """显示张量场窗口"""
    return create_json_command("show_tensor_field", {})

# ============================================================================
# 交互模式工具
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
        return create_json_reply(f"Error: Invalid interaction mode '{mode}'. Valid modes are: {', '.join(valid_modes)}")

    data = {"mode": mode.lower()}
    return create_json_command("change_interaction_mode", data)

# ============================================================================
# 算法处理工具
# ============================================================================

@mcp.tool()
def get_model_eight_views(image_size: dict = None, quality: str = "high") -> str:
    """
    获取当前模型的八个视角图像
    
    Args:
        image_size (dict): 图像尺寸，格式: {"width": 800, "height": 600}
        quality (str): 图像质量，"high" 或 "normal"
    
    Returns:
        str: JSON格式的命令
    """
    # 设置默认参数
    if image_size is None:
        image_size = {"width": 800, "height": 600}
    
    # 验证参数
    if not isinstance(image_size, dict) or "width" not in image_size or "height" not in image_size:
        return create_json_reply("错误：image_size 必须包含 width 和 height")
    
    if quality not in ["high", "normal"]:
        return create_json_reply("错误：quality 必须是 'high' 或 'normal'")
    
    return create_json_command(
        "get_model_eight_views",
        {
            "image_size": image_size,
            "quality": quality
        }
    )

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
        return create_json_reply(f"Error: Invalid filter type '{filter_type}'. Valid types are: {', '.join(valid_filters)}")

    data = {"filter_type": filter_type.lower()}
    # 添加参数
    for key, value in parameters.items():
        data[key] = value

    return create_json_command("apply_mesh_filter", data)



if __name__ == "__main__":
    # Initialize and run the server
    mcp.run()
