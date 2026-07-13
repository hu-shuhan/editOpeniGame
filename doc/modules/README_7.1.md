# 指标 7.1：高阶可视化模块

## 模块作用

面向高阶 CAE 仿真数据，提供 Lagrange 高阶单元网格转换与 NURBS/Spline 几何的高保真读取和可视化能力。

主要能力：

- 线性网格到 Lagrange 高阶非结构化网格的转换
- Lagrange / Quadratic 高阶单元定义与细分绘制
- NURBS/Spline 曲面与体数据的 CPU/GPU 读取与可视化

## 源码路径

| 路径 | 说明 |
|------|------|
| `iGameCore/Filters/Convert/iGameConvertToLagrangeUnstructuredMeshFilter.*` | 线性网格转 Lagrange 高阶网格 |
| `iGameCore/Core/CellModel/iGameLagrange*.h` | Lagrange 高阶单元模型 |
| `iGameCore/Core/CellModel/Quadratic/` | 二阶单元模型 |
| `iGameCore/Core/DataModel/iGameLagrangeUnstructuredMesh.*` | 高阶网格细分与可绘制数据转换 |
| `iGameCore/Core/DataModel/iGameSplineGeometry.h` | 样条几何数据对象 |
| `iGameCore/IO/Spline XML/iGameSplineReaderCPU.h` | CPU 样条读取 |
| `iGameCore/IO/Spline XML/iGameSplineReaderGPU.h` | GPU 样条读取（需 `ENABLE_GPSCUDA_MODULE`） |

## 调用方式

### 高阶网格转换

```cpp
auto filter = iGame::ConvertToLagrangeUnstructuredMeshFilter::New();
filter->SetInput(mesh);
filter->Execute();
auto output = filter->GetOutput(0);
```

### 样条数据读取

```cpp
auto reader = iGame::SplineReaderCPU::New();
reader->SetFilePath(filePath);
reader->Execute();
auto obj = reader->GetOutput();
```

### GUI

Qt 通过 `igQtFileLoader::OpenSplineFile()` 加载 `.xml` 样条文件，按用户选项选择 CPU/GPU 曲面或体读取器。

## 相关示例

| 示例 Target | 说明 | 条件 |
|-------------|------|------|
| `testConvertToLagrangeUnstructuredMesh` | 线性网格转 Lagrange 高阶网格 | 默认 |
| `testSplineReaderCPU` | CPU 样条读取与可视化 | 默认 |
| `testSplineReaderGPU` | GPU 样条读取 | `ENABLE_GPSCUDA_MODULE=ON` |

## 已知限制

- VTK 高阶单元类型在 IO 层已有解析，但用户文档注明 **VTK 高阶网格端到端可视化尚未完全适配**（见 `iGameVisNoticeToUsers.md`）。
