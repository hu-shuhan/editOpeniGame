# SDK 编译与打包指南

本指南对应 `main` 分支的 [Script/build_and_package.py](Script/build_and_package.py)。
SDK 入口是核心库 `iGameCore`；Qt、CGNS、LibTorch 等模块按需启用。

## 1. 获取代码

```bash
git clone --branch main --recurse-submodules https://github.com/hu-shuhan/editOpeniGame.git
cd editOpeniGame
```

已有仓库可执行 `git submodule update --init --recursive`。CGNS 的子模块路径是
`ThirdParty/cgns`；仅构建桌面 SDK 不需要安装或激活 emsdk 工具链。

## 2. 准备依赖

需要 Python 3、CMake 3.19 或更新版本，以及支持本项目 C++20 代码的编译器。

- Windows：安装 Visual Studio 2022 的 C++ 桌面开发组件和 Windows SDK，从 x64 开发者终端运行。
- Linux：脚本按所选版本使用 `/usr/bin/gcc-11`、`gcc-13`、`gcc-15` 及对应 `g++`。
  先检查需要的编译器是否安装；不要求同时安装三个版本，可用 `--gcc-versions` 筛选。
- 默认 CMake 配置会构建 Examples，Examples 需要 HDF5；可通过 `HDF5_DIR`
  或 `HDF5_HOME` 指定安装路径。只需要核心库时可采用下文的手动构建并关闭 Examples。
- 动画导出示例需要 FFMPEG；Windows 的查找路径包含 `ThirdParty/FFMPEG`，
  Linux 使用系统开发库。FFMPEG 未找到时不会生成动画导出示例。

Linux 上 GLFW 的 X11/Wayland 构建依赖包括下列开发包（以 apt 系发行版为例）：

```bash
sudo apt install build-essential cmake python3 pkg-config \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev \
  libwayland-bin libwayland-dev libxkbcommon-dev libhdf5-dev \
  libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
  libpostproc-dev libswresample-dev libswscale-dev
```

其中 `wayland-scanner` 由 `libwayland-bin` 提供。手动使用 CMake 时可通过
`-DGLFW_BUILD_WAYLAND=OFF` 关闭 Wayland 后端。

## 3. 使用打包脚本

在仓库根目录运行。以下命令显式选择 Release，以匹配 SDK 输出目录及当前脚本配置。

Linux 单版本或多版本：

```bash
python3 Script/build_and_package.py --build-type Release --gcc-versions 11
python3 Script/build_and_package.py --build-type Release --gcc-versions 11 13
```

省略 `--gcc-versions` 时依次构建 GCC 11、13、15。这些数字表示编译器版本，
并非固定的 Ubuntu 版本。Windows 会忽略 GCC 筛选参数并使用 MSVC：

```powershell
python Script/build_and_package.py --build-type Release --generator "Visual Studio 17 2022"
```

| 参数 | 用途 |
| --- | --- |
| `--enable-cgns` | 启用 CGNS，需准备对应子模块及 HDF5 |
| `--enable-libtorch` | 启用 LibTorch，需准备与编译器、CUDA 相匹配的包 |
| `--enable-gpscuda` | 启用 GPS CUDA 模块，需 CUDA 与 CMakeGPS 的相关依赖 |
| `--enable-nastran` | 启用 Nastran 模块 |
| `--enable-qt` | 启用 Qt GUI，需 Qt 5 Core/Gui/Widgets/OpenGL/Charts/Svg |
| `--generator` | 显式选择 CMake 生成器 |
| `--reuse-build` | 复用构建目录；默认会清理该配置的旧构建目录 |
| `--skip-package` | 保留 install 目录，不移动或压缩产物 |

可用 `python Script/build_and_package.py --help` 查看完整参数。
当前脚本为单配置生成器写入 `CMAKE_BUILD_TYPE=Release`，因此需要其他构建类型或
脚本未暴露的 CMake 选项时，应使用手动构建。

## 4. 可选模块与常见问题

### LibTorch

使用 [PyTorch 官方下载入口](https://pytorch.org/get-started/locally/) 获取 LibTorch。
当前 CMake 使用以下 GPU 包目录：

```text
ThirdParty/libtorch/Windows/Release/GPU/
ThirdParty/libtorch/Windows/Debug/GPU/
ThirdParty/libtorch/Linux/Release/GPU/
ThirdParty/libtorch/Linux/Debug/GPU/
```

选中目录下应直接包含 `include`、`lib`、`share`（Windows 包还包含 `bin`），
避免多套一层 `libtorch`。按包要求准备 CUDA；不要仅凭旧指南固定选择 CUDA 版本。
优先使用 Release 配置；Windows 的 Debug/Release 库需与编译配置一致。

### CGNS 与 HDF5

若 CMake 报 `uninstall` 目标重名，检查所检出的 `ThirdParty/cgns/CMakeLists.txt`：
旧版子模块的 `ADD_CUSTOM_TARGET(uninstall ...)` 与 GLFW 冲突时，
可将该目标改名为 `cgns_uninstall`。仅在实际出现重名时修改子模块。
Windows 可设置 `HDF5_HOME` 为 HDF5 安装根目录，Linux 配置优先使用串行 HDF5。

### Qt

只打包核心库时不传 `--enable-qt`。需要 GUI 时，可设置 `CMAKE_PREFIX_PATH`
环境变量指向 Qt 的编译器目录，例如 `C:/Qt/Qt5.14.2/5.14.2/msvc2017_64`。
项目仍含机器相关的 Qt/HDF5 查找路径；配置失败时核对 CMake 输出和实际安装位置。

## 5. 手动构建核心 SDK

以下示例关闭可选模块和 Examples。运行前准备本机编译器环境；多配置生成器应在
build/install 阶段保留 `--config Release`。

```bash
cmake -S . -B build-sdk -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_QT_MODULE=OFF -DEXAMPLE_COMPILE=OFF \
  -DENABLE_CGNS_MODULE=OFF -DENABLE_LIBTORCH_MODULE=OFF \
  -DENABLE_GPSCUDA_MODULE=OFF -DENABLE_NASTRAN_MODULE=OFF
cmake --build build-sdk --config Release --parallel
cmake --install build-sdk --config Release
```

PowerShell 中可将配置命令写在一行，或使用 PowerShell 的续行方式。
手动构建的安装目录为 `build-sdk/install`。

## 6. 产物

脚本配置目录分别为 `cmake-autobuild-release-gcc11`、
`cmake-autobuild-release-gcc13`、`cmake-autobuild-release-gcc15` 或 Windows 的
`cmake-autobuild-msvc`。每个配置独立安装和打包：

```text
<配置目录>/install/          # --skip-package 时保留
<配置目录>/iGameCore/        # 默认打包时由 install 移动而来
<配置目录>/iGameCore.tar.gz
```

使用方可将 CMake 的 `iGameCore_DIR` 指向产物的 `lib/cmake/iGameCore`。
同时部署所启用模块需要的运行时库；SDK 包的系统和编译器配置应与使用方匹配。
