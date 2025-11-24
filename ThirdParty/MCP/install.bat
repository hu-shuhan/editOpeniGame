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

REM Show Python path and environment info
echo.
echo [INFO] Python executable path:
python -c "import sys; print(sys.executable)"

REM Check if in conda environment
if defined CONDA_DEFAULT_ENV (
    echo [INFO] Detected Conda environment: %CONDA_DEFAULT_ENV%
    echo [INFO] Virtual environment will be created using this Conda Python as base
) else (
    echo [INFO] Using system Python (no Conda environment detected)
)

REM Check Python version (need 3.10+)
echo.
echo [INFO] Checking Python version (requires 3.10 or higher)...
python -c "import sys; exit(0 if sys.version_info >= (3, 10) else 1)"
if errorlevel 1 (
    echo.
    echo Error: Python 3.10 or higher is required for MCP package
    echo Your current Python version:
    python --version
    echo Please install Python 3.10 or higher
    pause
    exit /b 1
)
echo [OK] Python version is compatible
echo.

REM Check if virtual environment exists
if not exist ".venv" (
    echo [INFO] Creating virtual environment with current Python...
    if defined CONDA_DEFAULT_ENV (
        echo [INFO] Using Conda environment '%CONDA_DEFAULT_ENV%' as base
    )
    python -m venv .venv
    if errorlevel 1 (
        echo Error: Failed to create virtual environment
        pause
        exit /b 1
    )
    echo [OK] Virtual environment created successfully
    echo [INFO] The .venv will inherit packages from current environment
) else (
    echo [OK] Virtual environment already exists
    if defined CONDA_DEFAULT_ENV (
        echo [INFO] Note: Existing .venv may not be based on current Conda environment
        echo [INFO] To recreate with current Conda environment, delete .venv folder and re-run
    )
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
echo [INFO] Now using Python from:
python -c "import sys; print(sys.executable)"
echo.

REM Upgrade pip
echo [INFO] Upgrading pip...
python -m pip install --upgrade pip -i https://pypi.tuna.tsinghua.edu.cn/simple
echo.

REM Install dependencies using requirements.txt
echo [INFO] Installing project dependencies from requirements.txt...
echo [INFO] Note: mcp package requires Python 3.10 or higher
echo.
echo [INFO] Trying official PyPI source first (recommended for mcp package)...
pip install -r requirements.txt
if errorlevel 1 (
    echo.
    echo Warning: Installation failed with official PyPI
    echo Trying with Tsinghua mirror...
    pip install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn
    if errorlevel 1 (
        echo.
        echo Error: Failed to install dependencies
        echo Please check:
        echo 1. Python version is 3.10 or higher
        echo 2. Internet connection is available
        echo 3. pip is up to date
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
echo Run python main.py to start the project
echo ========================================
echo.
pause
