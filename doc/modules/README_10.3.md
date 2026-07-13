# 指标 10.3：物理场特征可视交互模块

## 模块作用

提供**感知驱动**的大规模多尺度物理场特征可视交互：用户在并行坐标、相关矩阵、密度图、探针线等 2D 分析视图中刷选/框选，通过 `Selection` 回调联动 3D 模型高亮与筛选。

> 与 **10.1** 的区别：10.3 强调**交互联动**；10.1 强调分析数据生成。  
> 与 **11.3** 的区别：11.3 是云图/流线/形变等**场渲染输出**；10.3 是**多变量统计视图 ↔ 3D 选择**联动。

## 源码路径

| 路径 | 说明 |
|------|------|
| `iGameCore/Filters/ParallelCoordinates/` | 并行坐标（交互入口之一） |
| `iGameCore/Filters/VariableCorrelation/` | 变量相关性 |
| `iGameCore/Filters/VariableDensity/` | 变量密度 |
| `iGameCore/Filters/PlotLine/` | 探针线 |
| `iGameCore/Core/Common/iGameCtxPresObjData.*` | 图表与网格对象的上下文联动基类 |
| `iGameCore/Filters/Selection/` | 选择回调与刷选回写 |
| `Examples/MultiscaleInteraction/TestMultiscaleInteraction.cpp` | 四视图联动集成示例 |

## 调用方式

### 注册选择联动

```cpp
parallelCoordinatesData->SetDefaultSelectionFunc(name, mesh->GetSelection());
variableCorrelationData->SetDefaultSelectionFunc(name, mesh->GetSelection());
variableDensityData->SetDefaultSelectionFunc(name, mesh->GetSelection());
plotLineData->SetDefaultSelectionFunc(name, mesh->GetSelection());
```

### 在分析视图中筛选并回写 3D 选点

```cpp
auto pointIds = parallelCoordinatesData->FiltInRangeIds(variableMinMaxValues);
// 或 variableCorrelationData->FiltInRangeIds(...)
// 或 variableDensityData->FiltInRangeIds(...)
SelectPoints(mesh, pointIds);
```

### GUI

| Dock 面板 | Widget |
|-----------|--------|
| `dockWidget_ParallelCoordinatesField` | `igQtParallelCoordinatesWidget` |
| `dockWidget_VariableCorrelationField` | `igQtVariableCorrelationWidget` |
| `dockWidget_VariableDensityField` | `igQtVariableDensityWidget` |
| `dockWidget_DataChangeField` | `igQtDataChangeWidget` |

选择操作通过 `Selection` 系列 Filter 与上述面板联动。

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testMultiscaleInteraction` | **推荐验收示例**：四分析视图 + Selection 联动 |
| `testParallelCoordinatesData` | 并行坐标单模块 |
| `testVariableCorrelationData` | 相关性单模块 |
| `testVariableDensityData` | 密度单模块 |
| `testPlotLineData` | 探针线单模块 |
| `testSetSelectionCallBack` | 选择回调 |
