# 指标 10.1：智能可视分析模块

## 模块作用

面向 CAE 仿真多维物理场数据，提供统计可视分析能力：从网格标量/矢量属性生成并行坐标、变量相关性、密度分布和探针线数据，辅助发现变量关联与分布规律。

> 与 **10.3** 的区别：10.1 侧重**分析数据生成与统计**；10.3 侧重**刷选交互与 3D 模型联动**。

## 源码路径

| 路径 | 说明 |
|------|------|
| `iGameCore/Filters/ParallelCoordinates/iGameGenerateParallelCoordinatesData.*` | 并行坐标数据生成 |
| `iGameCore/Filters/VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.*` | 变量相关性矩阵 |
| `iGameCore/Filters/VariableDensity/iGameGenerateVariableDensityDataFilter.*` | 变量密度分布 |
| `iGameCore/Filters/PlotLine/iGameGeneratePlotLineDataFilter.*` | 探针线数据 |
| `iGameCore/Core/Common/iGameParallelCoordinatesData.h` 等 | 分析数据对象 |

## 调用方式

### 并行坐标

```cpp
auto filter = iGame::GenerateParallelCoordinatesDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
auto data = iGame::DynamicCast<iGame::ParallelCoordinatesData>(filter->GetOutput(0));
```

### 变量相关性

```cpp
auto filter = iGame::GenerateVariableCorrelationDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
```

### 变量密度

```cpp
auto filter = iGame::GenerateVariableDensityDataFilter::New(IG_POINT, boxNum);
filter->SetInput(0, mesh);
filter->Execute();
```

### GUI

| Dock 面板 | Widget |
|-----------|--------|
| `dockWidget_ParallelCoordinatesField` | `igQtParallelCoordinatesWidget` |
| `dockWidget_VariableCorrelationField` | `igQtVariableCorrelationWidget` |
| `dockWidget_VariableDensityField` | `igQtVariableDensityWidget` |
| `dockWidget_DataChangeField` | `igQtDataChangeWidget`（探针线/数据变化） |

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testParallelCoordinatesData` | 并行坐标 |
| `testVariableCorrelationData` | 变量相关性 |
| `testVariableDensityData` | 变量密度 |
| `testPlotLineData` | 探针线 |

## 可选能力

- `VortexDetectionFilter`（LibTorch 涡旋检测）属于 **10.2**，需 `ENABLE_LIBTORCH_MODULE=ON`。
