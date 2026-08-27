# iGameVis 环境搭建指南（Qt + 基础环境）

本文档面向**尚未配置开发环境**的 Windows 机器，说明如何从零搭建 **仅开启 Qt 模块** 时的编译与运行环境。

> 后续文档会补充 CGNS、Nastran、LibTorch、CUDA 等可选模块的环境说明。

---

## 1. 构建目标说明

本指南对应的 CMake 开关配置如下（仅 Qt 开启，其余关闭）：

| 开关 | 值 | 说明 |
|------|----|------|
| `ENABLE_QT_MODULE` | **ON** | 编译 GUI / Qt 前端 |
| `ENABLE_CGNS_MODULE` | OFF | 不依赖 HDF5 / CGNS |
| `ENABLE_NASTRAN_MODULE` | OFF | 不依赖 Nastran 相关 Python 库 |
| `ENABLE_LIBTORCH_MODULE` | OFF | 不依赖 LibTorch |
| `ENABLE_GPSCUDA_MODULE` | OFF | 不依赖 CUDA |

在此配置下：

- **iGameCore** 与 **ThirdParty** 目录内的依赖会随工程一起编译，无需单独安装 Eigen、GLFW、zlib 等。
- **AbqSDK**、**FFMPEG** 为可选依赖，找不到也不会阻止编译。
- 必须自行安装的是：**Visual Studio、CMake、Qt 5**。

---

## 2. 基础环境

### 2.1 操作系统

- **Windows 10 / 11，64 位**
- 项目路径、数据文件路径**不要包含中文**（见项目 README 说明）。

### 2.2 Visual Studio

安装 **Visual Studio 2022**（Community 版即可），勾选工作负载：

- **使用 C++ 的桌面开发**

其中需包含：

- MSVC v143 编译器工具集
- Windows 10/11 SDK
- CMake 工具（可选，也可单独安装 CMake）

> 项目当前使用 **Qt 5.15.2 MSVC2019 64-bit** 工具链。该 Kit 与 VS 2019 / VS 2022 均可配合使用，推荐使用 VS 2022。

### 2.3 CMake

- 版本要求：**≥ 3.19**（见根目录 `CMakeLists.txt`）
- 可从 [cmake.org](https://cmake.org/download/) 安装，或使用 VS 自带 CMake。
- 安装后确认命令可用：

```powershell
cmake --version
```

### 2.4 Git

用于拉取源码：

git clone https://github.com/mky8812/editOpeniGame.git


若仓库使用了 ThirdParty 子模块，按需加 `--recurse-submodules`。

### 2.5 显卡驱动（运行阶段）

程序默认请求 **OpenGL 4.6 Core Profile**（见 `main.cpp`）。若运行后 3D 视图黑屏，可能是驱动或显卡不支持 4.6，可参考 `iGameVisNoticeToUsers.md` 中 OpenGL 3.3 降级说明。

---

## 3. Qt 环境

### 3.1 版本与工具链

| 项目 | 要求 |
|------|------|
| Qt 版本 | **5.15.2** |
| 编译器 Kit | **MSVC 2019 64-bit**（目录名一般为 `msvc2019_64`） |
| C++ 标准 | C++20（由 CMake 统一设置） |

### 3.2 必须安装的 Qt 组件

在 Qt Maintenance Tool / 在线安装器中，除 `Qt 5.15.2 → MSVC 2019 64-bit` 外，还需勾选：

- **Qt Charts**
- **Qt SVG**

CMake 中会查找以下模块（见 `Qt/CMakeLists.txt`）：

```
Core  Gui  Widgets  OpenGL  Charts  Svg
```

缺少 **Charts** 或 **Svg** 时，`find_package(Qt5 ...)` 会失败。

### 3.3 安装步骤（Qt Online Installer）

1. 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)。
2. 登录 Qt 账号（开源协议选 LGPL / 商业按实际情况）。
3. 选择安装路径，例如：`C:\Qt`
4. 在组件列表中勾选：
   - `Qt → Qt 5.15.2 → MSVC 2019 64-bit`
   - `Qt → Qt 5.15.2 → Qt Charts`
   - `Qt → Qt 5.15.2 → Qt SVG`
5. 等待安装完成。

安装完成后的典型目录结构：

```
C:\Qt\
└── 5.15.2\
    └── msvc2019_64\
        ├── bin\          ← qmake.exe、windeployqt.exe
        ├── lib\
        │   └── cmake\Qt5\  ← CMake 查找入口
        └── include\
```

### 3.4 配置 Qt 路径（二选一）

工程通过 `Qt/CMakeLists.txt` 定位 Qt，优先级如下。

#### 方式 A：设置环境变量 `QT_HOME`（推荐）

`QT_HOME` 应指向 **包含 `5.15.2` 版本目录的 Qt 根路径**。

示例：若 Qt 装在 `C:\Qt\5.15.2\msvc2019_64\...`，则：

```
QT_HOME = C:\Qt
```

CMake 会解析为：

```
C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
```

**Windows 设置方法：**

1. `Win + R` → 输入 `sysdm.cpl` →
2. **高级** → **环境变量**
3. 在「用户变量」或「系统变量」中新建：
   - 变量名：`QT_HOME`
   - 变量值：`C:\Qt`
4. 确认保存后，**重新打开**终端 / IDE。


#### 方式 B：直接修改 CMakeLists.txt

若未设置 `QT_HOME`，需编辑 `Qt/CMakeLists.txt` 中的默认路径：

```cmake
set(Qt5_DIR "C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5")
```

将路径改为你本机实际的 `.../lib/cmake/Qt5` 目录。

> 修改 Qt 路径后，必须删除旧的 CMake 缓存（删除整个 `build` 目录，或重新 `-B` 一个新目录）。

### 3.5 验证 Qt 安装

```powershell
C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe -v
```

应看到类似输出：

```
QMake version 3.1
Using Qt version 5.15.2 in C:/Qt/5.15.2/msvc2019_64/lib
```

---

## 4. 编译步骤


### 4.1 配置（仅 Qt 模块）

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DENABLE_QT_MODULE=ON `
  -DENABLE_CGNS_MODULE=OFF `
  -DENABLE_NASTRAN_MODULE=OFF `
  -DENABLE_LIBTORCH_MODULE=OFF `
  -DENABLE_GPSCUDA_MODULE=OFF
```

每个人的编译环境不同，建议另外于根目录新建一个CMakeUserPresets.json，参照CMakePresets.json中的构建、编译预设，书写适用于自己的预设配置，后续只需指定预设名，使用`cmake --preset [预设名字]`即可快速重复构建、编译。



### 4.2 编译

Release 版本（推荐日常使用）：

```powershell
cmake --build build --config Release --parallel
```

Debug 版本：

```powershell
cmake --build build --config Debug --parallel
```

### 4.3 输出位置

使用 Visual Studio 生成器时，可执行文件通常在：

```
build\Release\iGameVis.exe
```

或：

```
build\x64\Release\iGameVis.exe
```

具体以编译日志为准。

### 4.4 Qt 运行时依赖部署

Windows 下，编译成功后 CMake 会自动调用 `windeployqt`，将所需 Qt DLL 复制到 exe 同目录（见根 `CMakeLists.txt` 的 `POST_BUILD` 规则）。

若手动部署，可执行：

```powershell
C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe build\Release\iGameVis.exe
```

---

## 5. 运行

```powershell
.\build\Release\iGameVis.exe
```

或在 Visual Studio 中打开 `build\iGameVis.sln`，将 `iGameVis` 设为启动项目后 F5 运行。

---

## 6. 使用 IDE 打开（可选）

### Visual Studio

1. 先按 4.1 完成 `cmake -B build ...` 配置。
2. 打开 `build\iGameVis.sln`。
3. 选择配置 `Release` / `Debug`，平台 `x64`，编译并运行。

### CLion / VS Code

- 配置 CMake Profile，将上述 `-DENABLE_*` 参数写入 CMake options。
- 确保 IDE 能读取到 `QT_HOME` 环境变量。

---

## 7. 常见问题

### 7.1 `Could not find Qt5` / `Qt5_DIR-NOTFOUND`

**原因：** Qt 未安装、路径错误，或未安装 Charts/Svg 组件。

**处理：**

1. 确认 `C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5\Qt5Config.cmake` 存在。
2. 检查 `QT_HOME` 或 `Qt/CMakeLists.txt` 中的 `Qt5_DIR`。
3. 删除 `build` 目录后重新 cmake。

### 7.2 编译器与 Qt Kit 不匹配

**现象：** 链接错误、LNK20xx、运行时缺少 `VCRUNTIME140.dll` 等。

**处理：**

- Qt 必须使用 **msvc2019_64** 对应安装包。
- 使用 VS 2022 编译即可，无需单独安装 VS 2019；若仍异常，在 VS Installer 中补充 **MSVC v142 - VS 2019 C++ x64/x86 生成工具**。

### 7.3 Anaconda / Miniconda 干扰

根 `CMakeLists.txt` 会在检测到 `CONDA_PREFIX` 时尝试忽略 Conda 路径。建议在配置和编译前：

```powershell
conda deactivate
```

或在干净终端中操作，避免 CMake 选错编译器。

### 7.4 修改 Qt 路径后仍使用旧配置

CMake 会缓存旧路径。请删除 `build` 目录，或至少删除其中的 `CMakeCache.txt`，然后重新配置。

### 7.5 3D 视图黑屏

默认需要 OpenGL 4.6。若显卡或驱动不支持，参考 `iGameVisNoticeToUsers.md` 将渲染降级到 OpenGL 3.3，并同步修改 `main.cpp` 中的 `format.setVersion(...)`。

### 7.6 路径含中文导致异常

数据导入、工程路径请使用英文路径，避免中文目录名。

---

## 8. 环境检查清单

在交给他人搭建环境前，可按此清单自检：

- [ ] Windows 10/11 x64
- [ ] Visual Studio 2022 +「使用 C++ 的桌面开发」
- [ ] CMake ≥ 3.19
- [ ] Qt 5.15.2 MSVC2019 64-bit 已安装
- [ ] Qt Charts、Qt SVG 已安装
- [ ] `QT_HOME` 或 `Qt5_DIR` 已正确配置
- [ ] 源码路径无中文
- [ ] CMake 配置时仅开启 `ENABLE_QT_MODULE`
- [ ] Release 编译成功
- [ ] `iGameVis.exe` 可启动，界面正常显示

---

## 9. 参考文件

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 模块开关、C++20、windeployqt |
| `Qt/CMakeLists.txt` | Qt 版本、组件、`Qt5_DIR` |
| `main.cpp` | OpenGL 版本、Qt 应用入口 |
| `README.md` | 项目概览与简要构建说明 |
| `iGameVisNoticeToUsers.md` | 使用手册、OpenGL 降级说明 |

---

## 10. 下一步

完成本文档环境后，若需要启用其他模块，还需额外安装：

- **CGNS 模块**：HDF5 + CGNS（`ENABLE_CGNS_MODULE=ON`）
- **Nastran 模块**：Python / pyNastran 相关（`ENABLE_NASTRAN_MODULE=ON`）
- **LibTorch 模块**：ThirdParty 内 libtorch + 可选 CUDA（`ENABLE_LIBTORCH_MODULE=ON`）
- **GPS CUDA 模块**：CUDA Toolkit（`ENABLE_GPSCUDA_MODULE=ON`）

上述内容将在后续文档中单独说明。
