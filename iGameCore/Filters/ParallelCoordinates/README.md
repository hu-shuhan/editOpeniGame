# 指标 10.1：智能可视分析模块

## 模块作用

面向 CAE 仿真数据提供智能可视分析能力，通过多变量并行坐标、变量相关性分析、变量密度分布和探针线分析，帮助用户从多维物理场数据中快速发现变量间的关联规律与分布特征，并支持与交互选择的联动分析。

主要能力包括：

- 多变量并行坐标数据生成与可视化
- 变量相关性矩阵分析
- 变量密度分布统计
- 探针线（PlotLine）数据提取
- 与选择回调的联动交互

## 本目录核心实现

| 文件 | 类 | 说明 |
|------|-----|------|
| `iGameGenerateParallelCoordinatesData.h/.cpp` | `GenerateParallelCoordinatesDataFilter` | 从网格属性生成并行坐标分析数据 |

## 关联源码路径

- 变量相关性：[`../VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.h`](../VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.h)
- 变量密度：[`../VariableDensity/iGameGenerateVariableDensityDataFilter.h`](../VariableDensity/iGameGenerateVariableDensityDataFilter.h)
- 探针线：[`../PlotLine/iGameGeneratePlotLineDataFilter.h`](../PlotLine/iGameGeneratePlotLineDataFilter.h)
- 交互选择：[`../Selection/`](../Selection/)（`SetSelectionCallBackFuncFilter` 等）
- 分析数据类型：[`../../Core/Common/iGameParallelCoordinatesData.h`](../../Core/Common/iGameParallelCoordinatesData.h) 等
- Qt 面板：[`../../../Qt/src/IQWidgets/igQtParallelCoordinatesWidget.cpp`](../../../Qt/src/IQWidgets/igQtParallelCoordinatesWidget.cpp)

## 调用方式

### 编程接口：生成并行坐标数据

```cpp
auto filter = iGame::GenerateParallelCoordinatesDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
auto data = iGame::DynamicCast<iGame::ParallelCoordinatesData>(filter->GetOutput(0));
```

### 编程接口：变量相关性分析

```cpp
auto filter = iGame::GenerateVariableCorrelationDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
```

### GUI 调用

Qt 主窗口加载对应 Dock 面板：

- `dockWidget_ParallelCoordinatesField` → `igQtParallelCoordinatesWidget`
- `dockWidget_VariableCorrelationField` → `igQtVariableCorrelationWidget`
- `dockWidget_VariableDensityField` → `igQtVariableDensityWidget`

选择操作通过 `Selection` 系列 Filter 的回调函数与可视化面板联动更新。

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testParallelCoordinatesData` | 并行坐标数据生成 |
| `testVariableCorrelationData` | 变量相关性分析 |
| `testVariableDensityData` | 变量密度分布 |
| `testPlotLineData` | 探针线数据 |
| `testMultiscaleInteraction` | 多尺度交互联动分析 |