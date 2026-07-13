# 指标 7.1：高阶可视化模块

## 模块作用

面向高阶 CAE 仿真数据，提供 Lagrange 高阶单元网格转换与 NURBS/Spline 几何的高保真读取和可视化能力，支持从线性网格升级为高阶非结构化网格，以及样条曲面和体数据的精确绘制。

主要能力包括：

- 线性网格到 Lagrange 高阶非结构化网格的转换
- Lagrange 高阶单元类型（三角形、四面体等）的定义与存储
- NURBS/Spline 几何数据的 CPU/GPU 读取与可视化

## 本目录核心实现

| 文件 | 类 | 说明 |
|------|-----|------|
| `iGameConvertToLagrangeUnstructuredMeshFilter.h/.cpp` | `ConvertToLagrangeUnstructuredMeshFilter` | 将线性非结构化网格转换为 Lagrange 高阶网格 |

## 关联源码路径

- 高阶单元模型：[`../../Core/CellModel/iGameLagrangeTriangle.h`](../../Core/CellModel/iGameLagrangeTriangle.h) 等
- 高阶网格数据：[`../../Core/DataModel/iGameLagrangeUnstructuredMesh.h`](../../Core/DataModel/iGameLagrangeUnstructuredMesh.h)
- 样条几何：[`../../Core/DataModel/iGameSplineGeometry.h`](../../Core/DataModel/iGameSplineGeometry.h)
- 样条 XML 读取：[`../../IO/Spline XML/iGameSplineReaderCPU.h`](../../IO/Spline%20XML/iGameSplineReaderCPU.h)、[`iGameSplineReaderGPU.h`](../../IO/Spline%20XML/iGameSplineReaderGPU.h)

## 调用方式

### 编程接口：高阶网格转换

```cpp
auto filter = iGame::ConvertToLagrangeUnstructuredMeshFilter::New();
filter->SetInput(mesh);
filter->Execute();
auto output = filter->GetOutput(0);
```

### 编程接口：样条数据读取

```cpp
auto reader = iGame::SplineReaderCPU::New();
reader->SetFilePath(filePath);
reader->Execute();
auto obj = reader->GetOutput();
```

### GUI 调用

Qt 前端通过 `igQtFileLoader::OpenSplineFile()` 加载 `.xml` 样条文件，根据用户选项选择 CPU/GPU 曲面或体数据读取器（`SplineReaderCPU` / `SplineReaderGPU`）。

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testConvertToLagrangeUnstructuredMesh` | 线性网格转 Lagrange 高阶网格 |
| `testSplineReaderCPU` | CPU 样条数据读取与可视化 |