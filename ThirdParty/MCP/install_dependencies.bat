@echo off
echo ====================================================
echo        iGame MCP Python依赖安装脚本
echo ====================================================
echo.

REM 检查Python环境
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到Python！请先安装Python 3.13+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

echo [信息] 检测到Python版本:
python --version
echo.

REM 检查pip
python -m pip --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到pip包管理器！
    pause
    exit /b 1
)

echo [信息] pip版本:
python -m pip --version
echo.

echo [安装] 正在安装MCP项目依赖包...
echo.

REM 安装核心依赖
echo [1/4] 安装 fastmcp...
python -m pip install fastmcp>=0.2.0

echo [2/4] 安装 python-dotenv...
python -m pip install python-dotenv>=1.0.0  

echo [3/4] 安装 openai...
python -m pip install openai>=1.0.0

echo [4/4] 安装 mcp...
python -m pip install mcp>=1.0.0

echo.
echo ====================================================
echo [完成] 所有依赖安装完成！
echo.
echo 下一步:
echo 1. 运行 start_bridge.bat 启动MCP服务器
echo 2. 在iGameVis中使用AI聊天助手
echo ====================================================
pause 