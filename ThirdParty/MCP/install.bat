@echo off
chcp 65001 >nul
echo ========================================
echo MCP Project Installation Script
echo ========================================

REM Check if in MCP directory
if not exist "requirements.txt" (
    echo Error: Please run this script in the MCP project root directory
    pause
    exit /b 1
)

echo Current directory: %CD%
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found, please install Python 3.8+ first
    pause
    exit /b 1
)

echo [OK] Python is installed
python --version
echo.

REM Check if virtual environment exists
if not exist ".venv" (
    echo [INFO] Creating virtual environment...
    python -m venv .venv
    if errorlevel 1 (
        echo Error: Failed to create virtual environment
        pause
        exit /b 1
    )
    echo [OK] Virtual environment created successfully
) else (
    echo [OK] Virtual environment already exists
)

echo.
echo [INFO] Activating virtual environment...
call .venv\Scripts\activate.bat
if errorlevel 1 (
    echo Error: Failed to activate virtual environment
    pause
    exit /b 1
)

echo [OK] Virtual environment activated
echo.

REM Upgrade pip
echo [INFO] Upgrading pip...
python -m pip install --upgrade pip -i https://pypi.tuna.tsinghua.edu.cn/simple
echo.

REM Install dependencies using requirements.txt
echo [INFO] Installing project dependencies from requirements.txt...
pip install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn
if errorlevel 1 (
    echo Warning: Some packages failed to install with Tsinghua mirror
    echo Trying with official PyPI source...
    pip install -r requirements.txt
    if errorlevel 1 (
        echo Error: Failed to install dependencies
        pause
        exit /b 1
    )
)

echo.
echo [OK] All dependencies installed successfully!
echo.
echo [INFO] Installed packages:
pip list

echo.
echo ========================================
echo Installation completed!
echo Usage:
echo 1. Run python main.py to start the project
echo 2. Run python bridge_server.py to start the bridge server
echo ========================================
echo.
pause
