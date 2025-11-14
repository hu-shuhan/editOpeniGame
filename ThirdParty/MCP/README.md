# iGame MCP AI Assistant

This is the MCP (Model Context Protocol) AI assistant component for the iGameVis project, providing intelligent chat and tool invocation functionality.

## 📦 Installation

### Prerequisites

- **Python 3.10 or higher** is required (Python 3.13 recommended)
- Conda or any Python virtual environment manager

### Installation Steps

1. **Activate your Python environment** (Conda, venv, etc.)

```bash
# Example with Conda
conda activate your_env

# Or create a new environment
conda create -n mcpenv python=3.13
conda activate mcpenv
```

2. **Run the installation script**

```bash
install.bat
```

The script will:
- Detect your current Python environment
- Create a local `.venv` virtual environment based on your activated Python
- Install all required dependencies including `mcp`, `openai`, `python-dotenv`, etc.

## 📁 Project Structure

```
ThirdParty/MCP/
├── config.py              # Configuration file (API keys, model selection)
├── iGameVis_Chat.py       # Socket bridge server (connects iGameVis to MCP)
├── main.py                # Main entry point for testing
├── install.bat            # Dependency installation script
├── requirements.txt       # Python package dependencies
├── Client/
│   └── iGameVis_Client.py # MCP client implementation
└── Servers/
    └── iGameVis_Server.py # MCP tool server (provides tools/functions)
```

## 🔧 Key Files

### `config.py`
Configuration file for the MCP system:
- **Model Selection**: Choose between Qwen, Gemini, OpenAI, Claude
- **API Keys**: Configure your AI model API keys
- **Socket Settings**: Configure host and port for iGameVis communication

```python
SELECTED_MODEL = "qwen"  # Options: "qwen", "gemini", "openai", "claude"
SOCKET_HOST = "127.0.0.1"
SOCKET_PORT = 8080
```

### `iGameVis_Chat.py`
Socket bridge server that:
- Accepts connections from iGameVis C++ application
- Forwards chat messages to the MCP client
- Returns AI responses back to iGameVis
- Handles binary protocol communication

### `Client/iGameVis_Client.py`
MCP client that:
- Connects to MCP server
- Communicates with AI models (Qwen, Gemini, etc.)
- Processes user messages and tool calls
- Manages chat history

### `Servers/iGameVis_Server.py`
MCP tool server providing various tools:
- **Desktop file operations**: List, search, count files
- **Mesh file analysis**: Analyze VTK, OBJ, STL files
- **Scalar field inspection**: Extract scalar data from mesh files
- **File system utilities**: Find files by extension

### `main.py`
Simple test entry point for running the MCP client in standalone mode (without socket bridge).

## 🚀 Usage

### For iGameVis Integration

1. **Configure API keys** in `config.py`
2. **Start the bridge server**:
   ```bash
   python iGameVis_Chat.py
   ```
3. **Connect from iGameVis**: Use the AI Chat Widget in the iGameVis application

### For Testing (Standalone)

```bash
python main.py
```

This runs the MCP client directly in the terminal for testing purposes.

## ⚙️ Supported AI Models

- ✅ **Alibaba Qwen** (通义千问) - Default model
- ✅ **Google Gemini** - Requires API key
- ⚠️ **OpenAI GPT** - Requires API key
- ⚠️ **Anthropic Claude** - Requires API key

Configure your preferred model and API key in `config.py`.

## 🚨 Important Notes

- **Python Version**: Must be 3.10 or higher (3.13 recommended for best compatibility)
- **API Keys**: Keep your API keys secure, never commit them to version control
- **Port**: Default socket port is 8080, ensure it's not blocked by firewall
- **Virtual Environment**: The `install.bat` script creates a `.venv` folder in the MCP directory

## 🐛 Troubleshooting

1. **Installation fails**: 
   - Check Python version: `python --version` (must be ≥3.10)
   - Try running `install.bat` again
   - Install manually: `pip install -r requirements.txt`

2. **Connection errors**:
   - Ensure `iGameVis_Chat.py` is running before connecting from iGameVis
   - Check if port 8080 is available

3. **API errors**:
   - Verify API key is correctly configured in `config.py`
   - Check internet connection
   - Try switching to a different AI model

---

*iGame MCP AI Assistant - Making your CAD work smarter*
