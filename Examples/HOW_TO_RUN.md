# Examples 测试用例使用说明

本文档面向需要上手 / 验收 iGameVis `Examples` 用例的同学。  
`Examples` 里绝大多数是**独立可执行演示程序**（弹 OpenGL 窗口），不是 `ctest` 自动单测。

验收时请以本文为准；用例有增删时同步改本文。

---

## 目录

1. [前置条件（新机器也看这里）](#1-前置条件新机器也看这里)
2. [共同第一步：编译并 Install 主库](#2-共同第一步编译并-install-主库)
3. [方式一：CLion 跑 Examples](#3-方式一clion-跑-examples)
4. [方式二：Visual Studio 跑 Examples](#4-方式二visual-studio-跑-examples)
5. [方式三：命令行跑 Examples](#5-方式三命令行跑-examples)
6. [建议先跑通的 3 个验收用例](#6-建议先跑通的-3-个验收用例)
7. [用例一览（按模块）](#7-用例一览按模块)
8. [模块依赖](#8-模块依赖编译--运行前核对)
9. [常见问题](#9-常见问题)
10. [交给外部负责人时怎么用本文](#10-交给外部负责人时怎么用本文)

三种跑法共用同一套主库 install；Examples 构建目录建议分开，避免互相覆盖：

| 方式 | Examples 构建目录（建议） |
|------|---------------------------|
| CLion | `cmake-build-examples-clion` |
| Visual Studio | `cmake-build-examples-vs` |
| 命令行 | `cmake-build-examples` |

---

## 1. 前置条件（新机器也看这里）

### 1.1 软件环境

| 组件 | 说明 |
|------|------|
| Git | 拉取仓库 |
| CMake | ≥ 3.19（CLion 预设建议 ≥ 3.23） |
| Visual Studio 2022 | 勾选「使用 C++ 的桌面开发」（MSVC + Windows SDK） |
| Ninja | CLion / 命令行常用；也可只用 VS 生成器 |
| 显卡驱动 | 需支持 OpenGL 4.6 |
| HDF5 1.14.x（可选） | 跑 `testCGNS` 等时需要，例如 `C:\Program Files\HDF_Group\HDF5\1.14.6` |
| CLion（可选） | 方式一使用 |

设置环境变量（跑 CGNS 时）：

```text
HDF5_HOME=C:\Program Files\HDF_Group\HDF5\1.14.6
```

### 1.2 工程约定

主工程默认**不会**编译 Examples：

```cmake
# 仓库根 CMakeLists.txt
EXAMPLE_COMPILE = OFF   # FORCE
```

推荐：**主工程只负责 Release + Install Core**，再**单独**打开 / 配置 `Examples/`。

路径下文以仓库根 `D:\RealStudy\editOpeniGame` 为例，请按本机修改。

---

## 2. 共同第一步：编译并 Install 主库

无论用 CLion、VS 还是命令行跑 Examples，都必须先有：

```text
<主工程构建目录>/install/lib/cmake/iGameCore/
```

### 2.1 命令行安装主库（任选，三种方式都可先做这一步）

**必须**在带 MSVC 的终端里操作，否则会出现 `No CMAKE_C_COMPILER / CMAKE_CXX_COMPILER could be found`：

- 开始菜单打开 **「x64 Native Tools Command Prompt for VS 2022」**（cmd），或  
- **「Developer PowerShell for VS 2022」**

不要用普通「命令提示符」/ 未加载 `vcvars` 的 PowerShell。

#### cmd（Native Tools）— 推荐一行写完，不要用 `` ` ``

cmd 里 `` ` `` **不是**续行符，会变成多余参数并出现  
`Ignoring extra path from command line: "`"`。

```bat
cd /d D:\RealStudy\editOpeniGame

set HDF5_HOME=C:\Program Files\HDF_Group\HDF5\1.14.6

cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_CGNS_MODULE=ON -DENABLE_QT_MODULE=OFF

cmake --build cmake-build-release
cmake --install cmake-build-release
```

#### PowerShell（Developer PowerShell）— 可用 `` ` `` 续行

```powershell
cd D:\RealStudy\editOpeniGame

$env:HDF5_HOME = "C:\Program Files\HDF_Group\HDF5\1.14.6"

# 需要 CGNS 验收时打开 ENABLE_CGNS_MODULE；不需要 Qt 时可关以加快编译
cmake -S . -B cmake-build-release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DENABLE_CGNS_MODULE=ON `
  -DENABLE_QT_MODULE=OFF

cmake --build cmake-build-release
cmake --install cmake-build-release
```

#### 未安装 Ninja 时改用 VS 生成器

```bat
cmake -S . -B cmake-build-release -G "Visual Studio 17 2022" -A x64 -DENABLE_CGNS_MODULE=ON -DENABLE_QT_MODULE=OFF
cmake --build cmake-build-release --config Release
cmake --install cmake-build-release --config Release
```

#### 确认 Install 成功

`cmake --install` 必须完整跑完（中途报错会导致缺包配置）。确认存在：

```text
D:\RealStudy\editOpeniGame\cmake-build-release\install\lib\cmake\iGameCore\iGameCoreConfig.cmake
```

同目录还应有 `iGameCoreConfigVersion.cmake`、`iGameCoreTargets.cmake` 等。

也可用 CLion / VS 打开**仓库根目录**，Release 配置下 Build + Install，效果相同。

### 2.2 改过 Core 之后

根工程重新 **Build + Install**，再到 Examples 侧 **Rebuild** 对应 `test*` 目标。

### 2.3 关于 Examples 的 Install

**Examples 没有 install 步骤**（`Examples/CMakeLists.txt` 无 `install()`）。  
只需对主库执行 `cmake --install`；Examples 配置 + 编译后，在构建目录直接运行 exe 即可。

---

## 3. 方式一：CLion 跑 Examples

主工程继续用原来的窗口（`editOpeniGame` + `cmake-build-release`）。  
**Examples 请另开一个 CLion 窗口**，只打开 `Examples` 目录（不要和根 `CMakeLists.txt` 混在同一个 CMake 配置里）。

### 3.1 打开 Examples

1. **File → Open** → 选仓库下的 `Examples` 文件夹 → **New Window**。  
2. CLion 会读同目录 `CMakePresets.json`。在 CMake 工具窗口 / Settings → Build → CMake 里选用预设：  
   **`examples-release`**（Ninja，构建目录 `../cmake-build-examples-clion`）。  
3. 若预设未带上 `iGameCore_DIR`，在该 Profile 的 CMake options 里加上：

```text
-DiGameCore_DIR=D:/RealStudy/editOpeniGame/cmake-build-release/install/lib/cmake/iGameCore
```

4. **Reload CMake** → **Build**（可先只编 `testLosslessEncode`、`testSetViewStyle`、`testCGNS`）。

### 3.2 运行配置（工作目录必须对）

可执行文件在：

```text
D:\RealStudy\editOpeniGame\cmake-build-examples-clion\testXxx.exe
```

（Ninja 单配置，一般**没有** `Release\` 子目录。）

相对路径 `./Models/...`、`./Resources/...` 要求：

**Working directory = 构建目录本身**

```text
D:\RealStudy\editOpeniGame\cmake-build-examples-clion
```

| 目标 | Program arguments | 说明 |
|------|-------------------|------|
| `testLosslessEncode` | `.\Models\StreamTest.vtk` | 看终端 `PASS`/`FAIL` |
| `testSetViewStyle` | （空） | 弹窗 |
| `testCGNS` | （空） | 弹窗；主库需开 CGNS |

### 3.3 运行时 PATH（缺 DLL 时）

在 Run Configuration 的 Environment 中追加，或系统 PATH：

```text
D:\RealStudy\editOpeniGame\cmake-build-release\install\bin
C:\Program Files\HDF_Group\HDF5\1.14.6\bin
```

### 3.4 小结（CLion）

```text
根工程 Install → 新窗口打开 Examples → 预设 examples-release
→ Build testXxx → Working directory = cmake-build-examples-clion → Run
```

---

## 4. 方式二：Visual Studio 跑 Examples

同样：**主工程先 Install**，再单独用 VS 打开 `Examples`（不要用根目录工程硬开 `EXAMPLE_COMPILE`）。

以 VS 2022 +「打开文件夹」CMake 模式为例。

### 4.1 打开 Examples

1. **文件 → 打开 → 文件夹…**，选择：

```text
D:\RealStudy\editOpeniGame\Examples
```

2. 生成器可用 **Visual Studio 17 2022**（或 Ninja）。建议构建目录：

```text
D:\RealStudy\editOpeniGame\cmake-build-examples-vs
```

（勿与 CLion 的 `cmake-build-examples-clion` 抢同一目录。）

3. **项目 → CMake 设置**（或仓库内已有 `Examples/CMakeSettings.json`）中确认：

```text
iGameCore_DIR=D:/RealStudy/editOpeniGame/cmake-build-release/install/lib/cmake/iGameCore
```

`CMakeSettings.json` 已提供 **x64-Release** 示例（构建目录 `../cmake-build-examples-vs`）。主工程 install 路径不同则改该变量。

4. 配置类型选 **x64-Release** → 等待 CMake 成功 → **生成 → 全部生成**（或只生成需要的 `test*`）。

### 4.2 调试 / 运行（工作目录）

VS 多配置下，exe 通常在：

```text
cmake-build-examples-vs\Release\testXxx.exe
```

而 CMake 拷贝的 `Models/`、`Resources/` 在**构建根目录**：

```text
cmake-build-examples-vs\
```

因此：

1. 解决方案资源管理器 → 选中目标（如 `testLosslessEncode`）→ **调试属性** / **启动项设置**。  
2. **当前工作目录（Working Directory）** 设为构建根：

```text
D:\RealStudy\editOpeniGame\cmake-build-examples-vs
```

**不要**设成 `...\Release`（除非你已把 `Models`、`Resources` 拷进或 junction 到 `Release`）。

3. 程序参数：

| 目标 | 命令参数 | 说明 |
|------|----------|------|
| `testLosslessEncode` | `.\Models\StreamTest.vtk` | 看 `PASS`/`FAIL` |
| `testSetViewStyle` | （空） | 弹窗 |
| `testCGNS` | （空） | 弹窗；主库需 `ENABLE_CGNS_MODULE=ON` |

4. 用 **本地 Windows 调试器** 启动。

缺 `hdf5.dll` 等时：把 `%HDF5_HOME%\bin` 与主工程 `install\bin` 加入 PATH，或复制到 exe 旁。

### 4.3 小结（Visual Studio）

```text
根工程 Install → VS 打开 Examples 文件夹 → x64-Release + iGameCore_DIR
→ 生成 testXxx → Working Directory = cmake-build-examples-vs → 调试/运行
```

---

## 5. 方式三：命令行跑 Examples

适合无 IDE、脚本验收或 CI 本地复现。  
需已安装 VS 2022，并在 **x64 Native Tools / Developer PowerShell** 中操作（否则缺 `cl`）。

前提：第 2 节主库 **Build + Install 已成功**，且存在  
`cmake-build-release\install\lib\cmake\iGameCore\iGameCoreConfig.cmake`。

Examples **不要**执行 `cmake --install`（无安装规则）；配好、编好、进构建目录跑即可。

### 5.1 配置并编译 Examples

#### cmd（一行写完）

```bat
cd /d D:\RealStudy\editOpeniGame

set HDF5_HOME=C:\Program Files\HDF_Group\HDF5\1.14.6

cmake -S Examples -B cmake-build-examples -G Ninja -DCMAKE_BUILD_TYPE=Release -DiGameCore_DIR=D:/RealStudy/editOpeniGame/cmake-build-release/install/lib/cmake/iGameCore

cmake --build cmake-build-examples --target testLosslessEncode testSetViewStyle testCGNS
```

#### PowerShell

```powershell
cd D:\RealStudy\editOpeniGame

$env:HDF5_HOME = "C:\Program Files\HDF_Group\HDF5\1.14.6"

cmake -S Examples -B cmake-build-examples -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DiGameCore_DIR=D:/RealStudy/editOpeniGame/cmake-build-release/install/lib/cmake/iGameCore

cmake --build cmake-build-examples --target testLosslessEncode testSetViewStyle testCGNS
```

编全部目标时去掉 `--target ...`：

```bat
cmake --build cmake-build-examples
```

#### 路径注意（`iGameCore_DIR`）

| 正确 | 错误 |
|------|------|
| `.../cmake-build-release/install/lib/cmake/iGameCore` | `.../cmake-build-release install/...`（空格代替了 `/`） |
| 目录里含 `iGameCoreConfig.cmake` | 指向构建树里的 `cmake-build-release/iGameCore` 后仍缺 install 头文件/库布局 |

独立配置 Examples 时**务必**传 `-DiGameCore_DIR=...`。  
`Examples/CMakeLists.txt` 在未设置时默认是  
`${CMAKE_BINARY_DIR}/install/lib/cmake/iGameCore`  
（即 `cmake-build-examples/install/...`），**不是**主库的 install 路径。

配置成功后，CMake 会把 `Examples/Models`（以及 `Resources`）拷到构建目录。

### 5.2 设置 PATH 并运行

**必须在构建目录下运行**（该目录下应有 `Models/`）。

#### cmd

```bat
cd /d D:\RealStudy\editOpeniGame\cmake-build-examples

set PATH=D:\RealStudy\editOpeniGame\cmake-build-release\install\bin;C:\Program Files\HDF_Group\HDF5\1.14.6\bin;%PATH%

testLosslessEncode.exe .\Models\StreamTest.vtk
testSetViewStyle.exe
testCGNS.exe
```

#### PowerShell

```powershell
$env:PATH = "D:\RealStudy\editOpeniGame\cmake-build-release\install\bin;" +
            "C:\Program Files\HDF_Group\HDF5\1.14.6\bin;" +
            $env:PATH

cd D:\RealStudy\editOpeniGame\cmake-build-examples

.\testLosslessEncode.exe .\Models\StreamTest.vtk
.\testSetViewStyle.exe
.\testCGNS.exe
```

说明：

- 弹窗类：出现渲染窗口即基本成功，关闭窗口退出。  
- 命令行类（如 `testLosslessEncode`）：看终端 `PASS` / `FAIL` / `SKIP`。  
- 若用 VS 生成器且 exe 在 `Release\` 子目录，仍请在**构建根**（含 `Models/`）下启动：

```bat
cd /d D:\RealStudy\editOpeniGame\cmake-build-examples
Release\testSetViewStyle.exe
```

### 5.3 小结（命令行）

```text
x64 开发者终端
  → 主库：cmake 配置 → build → install（确认 iGameCoreConfig.cmake）
  → Examples：cmake -S Examples -B cmake-build-examples -DiGameCore_DIR=...
  → cmake --build cmake-build-examples
  → cd 构建目录 + PATH → 运行 exe
  （Examples 无需 install）
```

---

## 6. 建议先跑通的 3 个验收用例

| # | 可执行文件 | 类型 | 命令 / 说明 | 预期 |
|---|------------|------|-------------|------|
| 1 | `testSetViewStyle` | 弹窗 | `.\testSetViewStyle.exe` | 读 `./Models/Tet_Plane.vtk`，窗口可见点/线/面组合显示 |
| 2 | `testCGNS` | 弹窗 | `.\testCGNS.exe` | 读 `./Models/F6-coarse-vol-v2.cgns`；**主库需 `ENABLE_CGNS_MODULE=ON`** |
| 3 | `testLosslessEncode` | 命令行 | `.\testLosslessEncode.exe .\Models\StreamTest.vtk` | 终端打印 field 统计与 `Result: PASS`（或合理的 `SKIP`） |

`testLosslessEncode` 用法：

```text
testLosslessEncode.exe source.vtk|source.cgns [更多文件...]
testLosslessEncode.exe -h
```

缺数据时：把文件放到 `Examples/Models/` 后重新配置/构建，或改对应 `.cpp` 里的 `fileName`。

---

## 7. 用例一览（按模块）

下列「默认数据」来自源码中的相对路径；仓库 `Examples/Models/` 中**不一定**全部自带，缺文件时需自行补齐。

### Rendering（窗口 / 显示）

| 目标 | 源文件 | 默认数据 | 说明 |
|------|--------|----------|------|
| `testSetRenderWindow` | `Rendering/RenderWindow/SetRenderWindow.cpp` | `Tet_Plane.vtk` | 单窗口 |
| `testSetMultiRenderWindow` | `Rendering/RenderWindow/SetMultiWindow.cpp` | `Tet_Plane.vtk` + `StreamTest.vtk` | 多窗口 |
| `testMeshletRendering` | `Rendering/MeshletRendering.cpp` | `Tet_Plane.vtk` | Meshlet / GPU 调度相关 |
| `testResetCameraView` | `Rendering/ResetCameraView.cpp` | `Tet_Plane.vtk` | 重置相机 |
| `testSetCameraView` | `Rendering/SetCameraView.cpp` | `Tet_Plane.vtk` | 设置相机 |
| `testSetLineWidth` | `Rendering/SetLineWidth.cpp` | `Tet_Plane.vtk` | 线宽 |
| `testSetOrthographicProjection` | `Rendering/SetOrthographicProjection.cpp` | `Tet_Plane.vtk` | 正交投影 |
| `testSetPointSize` | `Rendering/SetPointSize.cpp` | `Tet_Plane.vtk` | 点大小 |
| `testSetTransparency` | `Rendering/SetTransparency.cpp` | `Tet_Plane.vtk` | 透明度 |
| `testSetViewStyle` | `Rendering/SetViewStyle.cpp` | `Tet_Plane.vtk` | 点/线/面显示模式 |
| `testSetScalarField` | `Rendering/SetScalarField.cpp` | `Tet_Plane.vtk` | 标量场 |
| `testSetVolumeRendering` | `Rendering/SetVolumeRendering.cpp` | `Tet_Plane.vtk` | 体绘制相关 |
| `testSetRenderingPressure` | `Rendering/SetRenderingPressure.cpp` | `Tet_Plane.vtk` | 交互压力 / 简化渲染 |

### Filter / Vector & Tensor

| 目标 | 源文件 | 默认数据 |
|------|--------|----------|
| `testVector` | `Filter/Vector/TestVector.cpp` | `StreamTest.vtk` |
| `testTimeVaryingVector` | `Filter/Vector/TestTimeVaryingVector.cpp` | `redsea/1.pvd`（需自备） |
| `testVectorAllCell` | `Filter/Vector/TestVectorAllCell.cpp` | `StreamTest.vtk` |
| `testVectorCellInRange` | `Filter/Vector/TestVectorCellInRange.cpp` | 大 CGNS（需自备） |
| `testVectorEveryNth` | `Filter/Vector/TestVectorEveryNth.cpp` | `StreamTest.vtk` |
| `testVectorSubData` | `Filter/Vector/TestVectorSubData.cpp` | `CAD11/_frames.pvd`（需自备） |
| `testStreamline` | `Filter/Vector/TestStreamline.cpp` | `kit.vtk` |
| `testTensorView` | `Filter/Tensor/TestTensorView.cpp` | `Quad_Plane_Tensor.vtk` |

### Filter / Selection

多数使用 `Tet_Plane.vtk`，在窗口中做点选 / 框选 / 回调演示：  
`testSetSelectionCallBack`、`testSetClearSelectionCallBackFunc`、`testSetPointsSelect`、`testSetCellsSelect`、`testGetClosestPointsInLine`、`testGetClosestCellsInLine`、`testGetPointsInFrustum`、`testGetCellsInFrustum`。

### Filter / Compression

| 目标 | 源文件 | 说明 |
|------|--------|------|
| `testEncoder` | `Filter/Compression/TestEncoder.cpp` | `Quad_Plane_Tensor.vtk` → `comp.igc` |
| `testDecoder` | `Filter/Compression/TestDecoder.cpp` | 读 `comp.igc` |
| `testSimplification` | `Filter/Compression/TestSimplification.cpp` | `mazewheel.obj` |
| `testLosslessEncode` | `Filter/Compression/TestLosslessEncode.cpp` | **命令行**；参数为源文件路径 |

### Filter / Convert、Clip、Slice 等

| 目标 | 默认数据（常见） |
|------|------------------|
| `testConvertToSurfaceMesh` / `PointCloud` / `VolumeMesh` / `PolyhedralCells` | `StreamTest.vtk` |
| `testConvertToLagrangeUnstructuredMesh` | `f6_res_ele.vtk` |
| `testClip` / `testSlice` | `Tet_Plane.vtk` |
| `testContourLine` | `Tet_Plane.vtk` |
| `testDeformation` / `testDeformationCode` | `sukong_Step-1_2.vtu`（需自备） |

### IO

| 目标 | 默认数据 | 额外要求 |
|------|----------|----------|
| `testCGNS` | `F6-coarse-vol-v2.cgns` | 主库开启 CGNS；本用例定义 `CGNS_ENABLE` |
| `testNastranReader` | `ogs.bdf` + `ogs.op2` | Nastran 模块 |
| `testCASReader` | `room.cas` | Fluent CAS |
| `testODBReader` | `CP10_L6_DP1_new.odb` | AbqSDK |
| `testSplineReaderCPU` | `quarter_circle.xml` | — |
| `testSplineReaderGPU` | `Bridge.xml` | GPS CUDA |

### Animation / ColorBar / 其它

| 目标 | 说明 |
|------|------|
| `testAnimation` / `testSaveAnimation` | 常依赖 `CAD11/_frames.pvd`；SaveAnimation 需 FFMPEG |
| `testChangeColorRange` / `testChangeColorBar` | `Tet_Plane.vtk` |
| `testChangeRange` | 属性范围；`CAD11/_frames.pvd` |
| `testMultiscaleInteraction` | `Tet_Plane.vtk` |
| `testVortexExtraction` 等 FeatureExtraction | 部分数据不在默认 `Models/`，需自备 |
| `testVortexDetection` | 需 LibTorch |

模块索引也可参考同目录 `readme.md`（较简略）。

---

## 8. 模块依赖（编译 / 运行前核对）

| 用例 | 主工程建议选项 / 依赖 |
|------|------------------------|
| `testCGNS` | `-DENABLE_CGNS_MODULE=ON`，并配置好 `HDF5_HOME` |
| `testNastranReader` | `-DENABLE_NASTRAN_MODULE=ON` |
| `testODBReader` | AbqSDK |
| `testSplineReaderGPU` | `-DENABLE_GPSCUDA_MODULE=ON` |
| `testSaveAnimation` | FFMPEG |
| `testVortexDetection` | `-DENABLE_LIBTORCH_MODULE=ON` |

未开启对应模块时：可能编不过，或运行读文件失败。

---

## 9. 常见问题

| 现象 | 处理 |
|------|------|
| `No CMAKE_C_COMPILER` / `CXX_COMPILER` | 换到 **x64 Native Tools / Developer PowerShell**，不要用普通 cmd |
| `Ignoring extra path: "`"` | 在 **cmd** 里不要用 PowerShell 的 `` ` `` 续行；改为一行命令，或改用 Developer PowerShell |
| `LNK1181: 无法打开输入文件 shlwapi.lib` | 当前终端未加载 Windows SDK 的 `LIB`。请在 **x64 Native Tools** 里重新 `cmake --build`（配置过也不够，**编译链接也要在开发者终端**） |
| `LNK2019: uncompress` / 缺 zlib | 主库 Config 需能解析到 `zs.lib`（`zlibstatic` 的输出名）。更新 `iGameCoreConfig` 后重新 `cmake --install` 主库，再 **重新配置** Examples |
| `find_package(iGameCore)` 失败 | ① 对主库执行 `cmake --install cmake-build-release` 且必须成功；② 确认存在 `...\install\lib\cmake\iGameCore\iGameCoreConfig.cmake`；③ Examples 配置时带上正确的 `-DiGameCore_DIR=.../cmake-build-release/install/lib/cmake/iGameCore`（`release` 与 `install` 之间是 `/`，不是空格） |
| `cmake --install` 中途失败 | 装不全 `iGameCoreConfig.cmake`；修完安装规则后需重新 **配置主库** 再 `cmake --install` |
| 对 Examples 执行 `cmake --install` | 无作用；Examples 无 install 规则，在构建目录直接跑 exe |
| 运行报读文件失败 / 空白 | 确认工作目录下有 `Models/`；CLion/VS 的 Working directory 指构建根，不是源码目录 |
| `testCGNS` 打不开 | 主库是否 `ENABLE_CGNS_MODULE=ON`；HDF5 环境是否正确；PATH 是否含 `install\bin` 与 HDF5 `bin` |
| 弹窗用例在 `Scene::New` 瞬间崩溃（exit `0xC0000374`） | Examples 必须带上与 Core 相同的宏：`IGAME_PLATFORM_WINDOWS`、`IGAME_OPENGL_VERSION_460`（见 `Examples/CMakeLists.txt` 里 `add_compile_definitions`）。缺宏时 `Scene` 内存布局与 `iGameCore.lib` 不一致。改完后需 **重新配置并重编** Examples |
| 主工程里找不到 Examples 目标 | 默认 `EXAMPLE_COMPILE=OFF`；请按本文第 3/4/5 节独立编 Examples |
| 只有 Debug 构建 | 跟主工程一起编时，源码要求 `CMAKE_BUILD_TYPE STREQUAL "Release"` 才会加入 Examples |
| VS 下读不到模型 | Working Directory 设成 `cmake-build-examples-vs`，不要设成 `Release\` |

---

## 10. 交给外部负责人时怎么用本文

1. 把仓库分支 / tag 发给对方，明确入口文件：`Examples/HOW_TO_RUN.md`。  
2. 请对方先完成第 2 节主库 Install，再任选第 3 / 4 / 5 节一种方式。  
3. 先跑第 6 节「3 个验收用例」。  
4. 其余用例按第 7 节表格按需查阅，不必一次全部跑完。  
5. 若对方环境路径不同，只需改 `iGameCore_DIR`、构建目录与运行目录，无需改业务逻辑。
