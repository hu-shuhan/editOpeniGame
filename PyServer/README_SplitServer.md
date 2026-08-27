# P3SAM 分割服务器 (SplitServer)

P3-SAM 3D 网格零件分割服务器。接收 OBJ 网格，返回带 `part_id` 单元（cell）数据的 VTK 文件。

---

## 运行要求

| 项目 | 要求 |
|------|-------------|
| 操作系统 | Windows 10/11 或 Linux |
| Python | 3.10 |
| CUDA | 12.x（必须与 PyTorch 构建版本匹配）|
| GPU | NVIDIA，建议显存 >= 8 GB |

> **Windows 重要提示：** 建议将整个项目放在**纯英文且不含空格的路径**下。
> 如果路径包含中文（例如“服务器”），`torch_scatter` / `chamfer3D` 等 CUDA 扩展在编译时可能因为路径编码问题失败。

> 如果 cmd 提示 `'conda' 不是内部或外部命令`，请使用 `<Miniconda安装目录>\Scripts\conda.exe`，或先执行 `<Miniconda安装目录>\Scripts\activate.bat p3sam`。

---

## 模型权重

服务器默认以**离线模式**运行，不发起任何网络请求。首次使用时，先以 `--allow-online` 运行一次，让服务器自动下载所需权重。

### 必需文件

| 文件 | 来源 | 用途 |
|------|--------|---------|
| `weights/p3sam/p3sam.safetensors` | HuggingFace 上的 `tencent/Hunyuan3D-Part` | P3SAM 模型权重 |
| `sonata.pth` | HuggingFace 上的 `facebook/sonata` | Sonata 主干权重，代码运行必需，首次 `--allow-online` 自动下载 |

> 注意：`sonata.pth` 不是 `p3sam.safetensors` 的一部分。缺少它时即使有 `p3sam.safetensors` 也会在启动时报 `OfflineModeIsEnabled` 或找不到 `facebook/sonata`。

`config/sonata.json`（Sonata 架构配置）已随仓库分发，无需下载。

### 首次设置：自动下载

首次运行时传 `--allow-online` 让服务器自动下载权重：

```bash
python p3sam_server.py --ckpt_path ./weights/p3sam/p3sam.safetensors --allow-online
```

权重会被缓存到本地。之后所有运行都不需要 `--allow-online`。

> **Windows 盘符问题：** 代码中使用 `/root/sonata` 作为 Sonata 缓存目录。Windows 下这个路径会解析到**当前盘符的根目录**。
> 例如从 `E:` 盘启动时缓存到 `E:\root\sonata`，从 `C:` 盘启动时则会去找 `C:\root\sonata`。
> 因此请记住 Sonata 实际缓存到哪个盘，之后启动服务器时也**从同一个盘符启动**。

### 备选方案：手动下载

如果机器没有外网，先在其它机器上下载权重。

**新版 `hf` 命令（`huggingface-cli` 已废弃）：**

```bash
hf download tencent/Hunyuan3D-Part p3sam/p3sam.safetensors --local-dir ./weights
```

下载后文件位于：

```text
./weights/p3sam/p3sam.safetensors
```

---

## 安装

### 1. 创建 Conda 环境

```bash
conda create -n p3sam python=3.10
conda activate p3sam
```

如果 `conda` 不在 PATH：

```bat
<Miniconda安装目录>\Scripts\conda.exe create -n p3sam python=3.10 -y
<Miniconda安装目录>\Scripts\activate.bat p3sam
```

### 2. 安装 PyTorch（以 CUDA 12.1 为例）

```bash
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
```

### 3. 安装 Python 依赖

```bash
pip install trimesh meshio numpy scipy scikit-learn numba tqdm fpsample
pip install spconv-cu121
pip install diffusers einops omegaconf addict timm torchdiffeq
pip install pyyaml huggingface_hub
```

> `spconv` 版本必须与你的 CUDA 版本匹配。
> 完整列表：https://github.com/traveller59/spconv

> **注意：`torch_scatter` 不要直接 `pip install torch_scatter`。**
> 该命令会因 pip 构建隔离导致源码包看不到已安装的 torch，报 `ModuleNotFoundError: No module named 'torch'`。
> 请按下面的 Windows 手动编译流程安装。

#### Windows 编译 torch_scatter

```bat
cd /d %TEMP%
if exist torch_scatter_src rmdir /s /q torch_scatter_src
mkdir torch_scatter_src
cd torch_scatter_src

<Miniconda安装目录>\envs\p3sam\python.exe -m pip download torch-scatter --no-deps --no-binary :all: --no-build-isolation
tar -xf torch_scatter-2.1.2.tar.gz
cd torch_scatter-2.1.2

call "<VisualStudio安装路径>\VC\Auxiliary\Build\vcvars64.bat"
set DISTUTILS_USE_SDK=1
set CUDA_HOME=<CUDA安装路径>
set CUDA_PATH=<CUDA安装路径>
<Miniconda安装目录>\envs\p3sam\python.exe setup.py install
```

关键点：

- 必须加载 MSVC x64 环境（`vcvars64.bat`）。
- 必须设置 `DISTUTILS_USE_SDK=1`。
- `CUDA_HOME` / `CUDA_PATH` 必须指向**实际安装的 CUDA 目录**，且 CUDA 大版本要与 PyTorch 匹配（例如 PyTorch cu121 配 CUDA 12.x）。
- **在 `call vcvars64.bat` 之后不要再执行 `set PATH=...;%PATH%` 去覆盖 PATH**，否则会把 MSVC 的 `cl.exe` 路径冲掉。
- 如果路径含中文导致编译失败，先把整个源码目录复制到纯英文路径（如 `%TEMP%\torch_scatter_src`）再编译。

验证安装：

```bash
python -c "import torch_scatter; print('torch_scatter OK')"
```

### 4. 编译 chamfer3D CUDA 扩展

**Windows**（需要 Visual Studio Build Tools + CUDA Toolkit）：

如果项目路径是纯英文，可以直接：

```bat
cd utils\chamfer3D
python setup.py install
```

如果项目路径含中文（例如“服务器”），**必须先复制到纯英文临时目录再编译**，否则 `nvcc`/`ninja` 会报找不到文件或路径乱码：

```bat
cd /d %TEMP%
if exist chamfer3D_build rmdir /s /q chamfer3D_build
xcopy /e /i <项目路径>\utils\chamfer3D chamfer3D_build
cd chamfer3D_build

call "<VisualStudio安装路径>\VC\Auxiliary\Build\vcvars64.bat"
set DISTUTILS_USE_SDK=1
set CUDA_HOME=<CUDA安装路径>
set CUDA_PATH=<CUDA安装路径>
python setup.py install
```

**Linux**（需要 GCC + CUDA Toolkit）：

```bash
cd utils/chamfer3D
python setup.py install
```

成功后 `chamfer_3D` 会在 conda 环境中注册。

---

## 启动服务器

```bash
python p3sam_server.py --ckpt_path <path/to/weights> [options]
```

推荐使用降低显存占用的参数：

```bat
python p3sam_server.py --ckpt_path weights\p3sam\p3sam.safetensors --host 127.0.0.1 --port 8765 --point_num 5000 --prompt_num 128
```

如果 `python` 不是 p3sam 环境，使用完整路径：

```bat
<Miniconda安装目录>\envs\p3sam\python.exe p3sam_server.py --ckpt_path weights\p3sam\p3sam.safetensors --host 127.0.0.1 --port 8765 --point_num 5000 --prompt_num 128
```

### 参数

| 参数 | 默认值 | 说明 |
|----------|---------|-------------|
| `--ckpt_path` | `None` | 模型权重路径（.safetensors 或 .pt）。**必填。** |
| `--host` | `0.0.0.0` | 监听地址。`0.0.0.0` 接受所有来源连接。 |
| `--port` | `8765` | 监听端口。 |
| `--point_num` | `10000` | 点云采样数量。影响精度与速度。 |
| `--prompt_num` | `250` | prompt 点数。 |
| `--seed` | `42` | 随机种子，用于可复现结果。 |
| `--allow-online` | 关闭 | 允许 HuggingFace 下载。首次运行时使用，自动拉取权重。 |

### 示例

```bash
python p3sam_server.py --ckpt_path ./weights/p3sam/p3sam.safetensors --port 8765 --point_num 10000 --prompt_num 250
```

看到 `P3-SAM Server listening on 0.0.0.0:8765` 即就绪。按 `Ctrl+C` 停止。

> **启动失败排查：**
> - 报 `OfflineModeIsEnabled`：说明 Sonata 权重未缓存或当前处于离线模式。首次运行加 `--allow-online`。
> - 报找不到 `C:\root\sonata` / `E:\root\sonata`：说明当前盘符和 Sonata 缓存盘符不一致，切换到缓存所在盘启动。
> - 报 `ModuleNotFoundError: No module named 'torch_scatter'`：说明第 3 步手动编译没有完成，重新编译安装。

---

## 测试

用随附的 `test_client.py` 发送 OBJ 文件、接收 VTK 结果：

```bash
python test_client.py <input.obj> <output.vtk> [host] [port]
```

**示例：**

```bash
python test_client.py car_simple.obj result.vtk 127.0.0.1 8765
```

成功后 `result.vtk` 包含 `part_id` 单元（cell）数据字段，每个值是对应面的从 0 开始的零件编号。

---

## 通信协议

简单二进制 TCP 流，**小端序**。

**请求（客户端 -> 服务器）：**

| 字段 | 类型 | 说明 |
|-------|------|-------------|
| `post_process` | `uint8` (1 字节) | 是否启用后处理：1=开，0=关 |
| `obj_size` | `uint32` (4 字节) | OBJ 数据字节长度 |
| `obj_data` | `bytes` (obj_size 字节) | 原始 OBJ 文件内容 |

**成功响应（服务器 -> 客户端）：**

| 字段 | 类型 | 说明 |
|-------|------|-------------|
| `success` | `uint8` (1 字节) | 值 `1` |
| `vtk_size` | `uint32` (4 字节) | VTK 数据字节长度 |
| `vtk_data` | `bytes` (vtk_size 字节) | VTK 文件内容（含 `part_id` 单元数据）|

**失败响应（服务器 -> 客户端）：**

| 字段 | 类型 | 说明 |
|-------|------|-------------|
| `success` | `uint8` (1 字节) | 值 `0` |
| `msg_size` | `uint32` (4 字节) | 错误信息字节长度 |
| `error_msg` | `bytes` (msg_size 字节) | UTF-8 编码的错误描述 |

---

## FAQ

**Q: 找不到 `chamfer_3D` 模块**
A: 先编译扩展：`cd utils/chamfer3D && python setup.py install`。如果项目路径含中文，先复制到纯英文路径再编译。

**Q: `pip install torch_scatter` 报错找不到 torch / 构建失败**
A: 不要直接 `pip install torch_scatter`。按“安装 → 第 3 步”中的 Windows 编译流程，下载源码后使用 MSVC + `DISTUTILS_USE_SDK=1` + 正确 `CUDA_HOME` 手动编译安装。

**Q: 编译时提示找不到 nvcc / CUDA 版本不对**
A: 确认 `CUDA_HOME` 指向实际安装目录；README 示例是 C 盘，实际环境可能装在 D 盘或其他位置。同时 CUDA 大版本要与 PyTorch 一致（cu121 配 CUDA 12.x）。

**Q: 编译 chamfer3D 时 nvcc 报找不到输出文件 / 路径乱码**
A: 如果项目路径含中文（如“服务器”），nvcc 可能无法正确处理输出路径。把 `utils/chamfer3D` 整个复制到纯 ASCII 路径（如 `%TEMP%\chamfer3D_build`）再 `python setup.py install`。

**Q: 首次启动提示 OfflineModeIsEnabled / 找不到 facebook/sonata**
A: 除了 `p3sam.safetensors`，代码还会从 HuggingFace 加载 Sonata 主干 `sonata.pth`。首次运行必须加 `--allow-online` 让它自动下载；之后可去掉该参数离线运行。

**Q: 启动后提示找不到 `/root/sonata` 或 `C:\root\sonata`**
A: 这是 Windows 盘符解析问题。请从 Sonata 实际缓存所在盘启动服务器（例如缓存是 `E:\root\sonata`，就先 `cd /d E:\` 再启动）。

**Q: `huggingface-cli` 提示已废弃**
A: 新版使用：

```bash
hf download tencent/Hunyuan3D-Part p3sam/p3sam.safetensors --local-dir ./weights
```

**Q: 6GB 显存不够**
A: 用 `--point_num 5000 --prompt_num 128` 可明显降低显存占用。

**Q: CUDA 显存不足**
A: 减小 `--point_num`（如 `5000`）或 `--prompt_num`（如 `128`）。

**Q: 连接超时**
A: 检查服务器防火墙是否放行端口，或用 `--host 127.0.0.1` 限制为本机连接。
