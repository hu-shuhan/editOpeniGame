# Nastran Reader 使用说明

## 概述

NastranReader 通过集成 Python 的 pyNastran 库来读取 Nastran BDF（几何）和 OP2（结果）文件。使用 pybind11 实现 C++ 和 Python 的交互。

## 功能特性

1. 读取 BDF 文件中的几何数据（节点、单元）
2. 读取 OP2 文件中的结果数据（位移、应力、应变等）
3. 自动转换为 iGame 的 UnstructuredMesh 数据结构
4. 支持多种单元类型（CQUAD4, CTRIA3, CHEXA, CTETRA 等）
5. 自动管理 Python 解释器生命周期

## 依赖要求

### Python 环境
- Python 3.6 或更高版本
- numpy
- scipy  
- cpylog
- pyNastran

### C++ 依赖
- pybind11
- C++11 或更高版本

## CMake 配置

在配置项目时，可以通过以下方式指定 Python 路径：

```bash
cmake -DPython3_ROOT_DIR="C:/Python39" ..
```

CMake 会自动：
1. 查找 Python 解释器
2. 检查必需的 Python 模块
3. 如果缺少模块，自动通过 pip 安装
4. 配置 pybind11

## 使用示例

### C++ 代码示例

```cpp
#include "iGameNastranReader.h"

using namespace iGame;

// 只读取几何数据
NastranReader::Pointer reader = NastranReader::New();
reader->SetBDFFileName("model.bdf");
reader->Execute();

UnstructuredMesh::Pointer mesh = reader->GetUnstructuredMeshOutput();

// 读取几何和结果数据
NastranReader::Pointer reader2 = NastranReader::New();
reader2->SetBDFFileName("model.bdf");
reader2->SetOP2FileName("model.op2");
reader2->Execute();

UnstructuredMesh::Pointer mesh2 = reader2->GetUnstructuredMeshOutput();
// mesh2 包含位移等结果数据
```

## 数据转换说明

### 几何数据
- **节点**: 转换为 Points，坐标为 float[3]
- **单元**: 转换为 CellArray，自动识别单元类型并映射到 VTK 单元类型
- **节点ID**: 存储在 PointData 中，名称为 "NodeIds"
- **单元ID**: 存储在 CellData 中，名称为 "ElementIds"

### 结果数据
- **位移**: 转换为 PointData 数组，名称为 "Displacement"，包含 [tx, ty, tz]
- **应力**: 转换为 CellData 数组（开发中）

## Python 脚本位置

Python 包装器脚本 `nastran_reader_wrapper.py` 位于：
- 开发时: `ThirdParty/Python/pyNastran/`
- 安装后: `<install_prefix>/Resources/Python/pyNastran/`

## 故障排除

### 问题：找不到 Python 模块

**解决方案**：
```bash
# 手动安装依赖
pip install numpy scipy cpylog pyNastran
```

### 问题：pybind11 未找到

**解决方案**：
```bash
# 安装 pybind11
pip install pybind11

# 或者在 CMake 中指定路径
cmake -Dpybind11_DIR="path/to/pybind11" ..
```

### 问题：Python 解释器路径错误

**解决方案**：
```bash
# 指定正确的 Python 路径
cmake -DPython3_ROOT_DIR="C:/Python39" ..
# 或
cmake -DPython3_EXECUTABLE="C:/Python39/python.exe" ..
```

### 问题：运行时找不到 Python 脚本

**解决方案**：
确保 Python 脚本已正确安装到 Resources 目录，或手动将脚本目录添加到 Python 路径：

```cpp
// 在程序启动时
py::module_ sys = py::module_::import("sys");
sys.attr("path").attr("insert")(0, "path/to/scripts");
```

## 技术细节

### Python 解释器管理
- 使用单例模式，整个应用程序生命周期只初始化一次
- 自动管理 GIL (Global Interpreter Lock)
- 线程安全（通过 pybind11 的 RAII 机制）

### 内存管理
- numpy 数组数据通过 memcpy 复制到 C++ 侧
- 不直接共享内存（避免生命周期问题）
- Python 对象在函数返回后自动释放

### 性能考虑
- 大型模型可能需要较长时间进行数据转换
- 考虑使用多线程加载（需要额外的 GIL 管理）
- numpy 数组复制是当前的性能瓶颈

## 许可证

遵循项目主许可证。pyNastran 采用 BSD 3-Clause 许可证。


