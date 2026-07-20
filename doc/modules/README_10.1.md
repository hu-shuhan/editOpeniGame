# 指标 10.1：面向 CAE 仿真数据的智能可视分析

## 指标构成

面向 CAE 仿真多维物理场数据，提供由用户交互驱动、由数据特征引导的智能可视分析能力，共含四个子功能：

| # | 子功能 | 状态 |
|---|--------|------|
| 1 | 选点驱动的局部区域图表分析：选择一个点，根据当前属性自动确定 bounding box，对局部区域做图表分析 | ✅ 已实现 |
| 2 | 基于信息熵的区域流线种子点自动计算 | ✅ 已实现 |
| 3 | 智能流线筛选 | ✅ 已实现 |
| 4 | 基于语义分割的 table 属性 | ⏳ 规划中 |

> 本文档记录已完成的子功能 **1**、**2**、**3**。4 待实现后补充。
---

## 子功能 1：选点驱动的局部区域图表分析

### 功能说明

用户在 3D 模型上选择一个点（或单元 / 框选区域），系统按当前属性自动计算该选择的 bounding box，将分析范围收缩到该局部区域，并在此区域上生成图表（探针线 / 径向图、并行坐标、变量相关性、变量密度），辅助发现局部区域内的变量关联与分布规律。

流程：**选点 / 框选 → 自动求 bounding box → 局部区域图表分析**。

### 源码路径

| 路径 | 说明 |
|------|------|
| `Qt/src/IQWidgets/igQtDataChangeWidget.cpp` / `Qt/include/IQWidgets/igQtDataChangeWidget.h` | 局部区域图表分析主面板（选区、径向图、图表绘制） |
| `iGameCore/Filters/PlotLine/iGameGeneratePlotLineDataFilter.*` | 探针线 / 区域数据生成 |
| `iGameCore/Filters/ParallelCoordinates/iGameGenerateParallelCoordinatesData.*` | 并行坐标数据生成 |
| `iGameCore/Filters/VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.*` | 变量相关性矩阵 |
| `iGameCore/Filters/VariableDensity/iGameGenerateVariableDensityDataFilter.*` | 变量密度分布 |
| `iGameCore/Rendering/Core/Interactor/iGameBoxStyle.*` | 框选盒交互，提供 bounding box 极值点 |

### 关键实现

1. **选择 → bounding box**：从选区（`SelectBox` 的 `BoxStyle`）取极值点，构造包围盒：

   ```cpp
   auto boxStyle = DynamicCast<iGame::BoxStyle>(interactor->GetSpecialInteractor("SelectBox"));
   auto minMaxP  = boxStyle->GetBox()->GetExtremePoint();
   auto boundingBox = BoundingBox(minMaxP.first, minMaxP.second);
   SetRadialPoint(boundingBox);   // 将分析范围绑定到该局部区域
   ```

   未显式框选时，退化为整模型包围盒 `m_Mesh->GetBoundingBox()`。

2. **局部区域图表**：在选定区域上按属性生成 `PlotLineData`，并根据变量的色相 / 饱和度生成图层：

   ```cpp
   auto Data = PlotLineData::New(attrs, dataType, MIN_H, MAX_H, MIN_S, MAX_S);
   ```

3. **选区联动**：选区变化通过 `SelectionCallbackEvent(itemType, ids, ope)` 回调，实时刷新被选中变量的图像（`GenerateChoosedVariableImage`）。

### GUI

| Dock 面板 | Widget | 说明 |
|-----------|--------|------|
| `dockWidget_DataChangeField` | `igQtDataChangeWidget` | 选点 / 框选 → 局部区域径向图 / 探针线图表 |
| `dockWidget_ParallelCoordinatesField` | `igQtParallelCoordinatesWidget` | 并行坐标 |
| `dockWidget_VariableCorrelationField` | `igQtVariableCorrelationWidget` | 变量相关性 |
| `dockWidget_VariableDensityField` | `igQtVariableDensityWidget` | 变量密度 |

---

## 子功能 2：基于信息熵的区域流线种子点自动计算

### 功能说明

对区域内的矢量场自动计算流线种子点：将区域划分为均匀盒格，用**方向信息熵**度量每个盒子内流场方向的复杂程度，优先在熵最高（流动方向最杂乱、结构最丰富）的盒子里布种，从而把有限的种子点集中到最值得观察的区域，避免均匀撒点造成的浪费与遗漏。

### 源码路径

| 路径 | 类 / 函数 | 说明 |
|------|-----------|------|
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | `StreamTracer::getEntropySeeding` | 信息熵种子点计算（核心） |
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | `StreamTracer::getModelSelect` / `getAllSubBlockCenters` / `computeSubBlockCenters` | 从选区求焦点 bounding box 并划分子块中心 |
| `iGameCore/Core/Common/iGamePointFinder.h` | `PointFinder`（`GetNumberOfBoxes` / `GetPointsInBox`） | 均匀盒格空间划分 |
| `Qt/src/IQWidgets/igQtStreamTracerWidget.cpp` | `generateStreamline` | GUI 触发种子生成与流线计算 |

### 算法要点

`getEntropySeeding(vectorName, topPercent = 0.1f, ptsPerExtrema = 2)`：

1. **盒格划分**：由 `PointFinder` 将点集划分为 `GetNumberOfBoxes()` 个空间盒子，`GetPointsInBox(i)` 给出盒内点集。
2. **方向熵计算**（并行，逐盒）：对盒内每个点的矢量做方向分箱——采用 Lambert 圆柱等面积投影，方位角 `φ` 24 箱、极角 `cosθ` 15 箱，共 360 个等面积方向箱；统计各箱概率 `p`，计算香农熵 `H = -Σ p·ln(p)`。
3. **区域筛选**：按熵降序排序，取熵最高的前 `topPercent` 个盒子。
4. **极值布种**（并行）：在每个入选盒子内，按矢量模长排序，取 `ptsPerExtrema` 个最小模长点与最大模长点作为种子（覆盖低速滞止区与高速区）。

计算过程使用 `ThreadPool::parallelFor` 并行化。

### 调用方式

```cpp
auto tracer = iGame::StreamTracer::New();
tracer->initStreamTracer(model);        // 绑定模型 / 网格
tracer->AddPtFinder(pointFinder);       // 提供盒格划分
// 信息熵种子点：熵最高 2.5% 的盒子、每盒 8 个极值点
auto seeds = tracer->getEntropySeeding(vectorName, 0.025f, 8);

// 结合均匀子块中心（可选），再计算流线
tracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
tracer->Execute();
auto streamlines = tracer->GetOutput();
```

### GUI

在流线面板 `igQtStreamTracerWidget` 中，种子模式（`control == 6`）即“信息熵种子”：先调用 `getEntropySeeding` 得到高熵区种子，再叠加均匀子块中心 `computeSubBlockCenters`，随后生成流线。

区域来源支持“选区自动包围盒”：`getModelSelect` 依据选中的点 / 单元自动求 bounding box，再在焦点区域内划分子块中心作为布种范围。

### 相关示例

| 示例 | 说明 |
|------|------|
| `Examples/Filter/Vector/TestStreamline.cpp` | 流线与种子点生成 |

---

## 子功能 3：智能流线筛选

### 功能说明

一次布种往往生成成百上千条流线，直接显示会互相遮挡、难以观察。智能流线筛选按**形状特征**对流线聚类，从每一类中挑选代表流线，在大幅减少数量的同时保留流场的结构多样性——既去掉大量近似冗余的流线，又不丢失有代表性的流动模式。筛选结果按所属类别（`ClusterLabel`）着色，便于区分不同流动模式。

### 源码路径

| 路径 | 类 / 函数 | 说明 |
|------|-----------|------|
| `iGameCore/Filters/StreamView/iGameStreamlineSimplifier.*` | `StreamlineSimplifier` | 流线筛选（核心） |
| `Qt/src/IQWidgets/igQtStreamTracerWidget.cpp` | `igQtStreamTracerWidget::Simplifier` | GUI 触发筛选、缓存原始流线、结果着色 |

### 算法流程

`StreamlineSimplifier::Execute()` 分为五步：

1. **流线重建 `ExtractStreamlines`**：依据 `StreamlineId`（cell 属性）与 `Velocity`（point 属性）把 `IG_LINE` 单元还原成一条条流线；剔除点数 `< 3` 的，并按弧长过滤（短于弧长中位数 10% 的直接丢弃）。
2. **曲率直方图 `ComputeHistograms`**：沿流线计算相邻方向向量夹角作为离散曲率，落到 `CurvBins`（默认 40）个区间，得到归一化直方图及其 CDF。
3. **距离矩阵 `BuildDistanceMatrix`**：为每条流线提取 5 维形状特征（弯曲比 bendRatio、最大转角 maxTurn、Top-K 平均转角、中段平均转角、弯曲段占比），归一化后加权 L1 距离（权重 `0.10/0.35/0.25/0.20/0.10`）；再叠加曲率 CDF 距离，合成 `0.95·特征距离 + 0.05·曲率距离`。
4. **层次聚类 `ClusterAverage`**：平均连接（average-linkage）凝聚聚类，合并到 `NumClusters` 个类别。
5. **代表采样 `SampleRepresentatives`**：按 `TotalTarget` 在各类间按配额分配（小类全选、余量补给大类），类内按等间隔（linspace）取代表流线。

`BuildOutputMesh` 重建输出网格，并写入 `Velocity` 与 `ClusterLabel` 属性用于着色。

### 参数

| Setter | 含义 | 默认 |
|--------|------|------|
| `SetCurvBins(int)` | 曲率直方图分箱数 | 40 |
| `SetNumClusters(int)` | 聚类目标类别数 | 20 |
| `SetTotalTarget(int)` | 筛选后保留的流线总数 | 50 |

### 调用方式

```cpp
auto simp = iGame::StreamlineSimplifier::New();
simp->SetInput(streamlineMesh);   // 输入：含 StreamlineId / Velocity 的流线网格
simp->SetCurvBins(40);
simp->SetNumClusters(clusterCount);
simp->SetTotalTarget(keepCount);
if (simp->Execute()) {
    auto simplified = simp->GetOutput();   // 输出：筛选后的代表流线，带 ClusterLabel
}
```

### GUI

在流线面板 `igQtStreamTracerWidget` 中，先在模型树选中已生成的流线对象（名称含 `_StreamLine`），点击 **Cluster** 按钮触发 `Simplifier()`：类别数取自 `clusterSpin`、保留总数取自 `perClusterSpin`。首次筛选会缓存原始流线快照，后续可反复调参而不丢失原始数据；结果按 `ClusterLabel` 云图着色。

---

## 相关示例（图表分析）

| 示例 Target | 说明 |
|-------------|------|
| `testParallelCoordinatesData` | 并行坐标 |
| `testVariableCorrelationData` | 变量相关性 |
| `testVariableDensityData` | 变量密度 |
| `testPlotLineData` | 探针线 / 局部区域数据 |
