# 指标 10.3：物理场特征可视交互模块

## 模块作用

提供感知驱动的大规模多尺度物理场特征可视交互能力，支持流线追踪、自适应矢量场/张量场渲染、结构形变显示和等值面提取，并与选择操作和多变量分析联动，实现物理场特征的交互式探索。

主要能力包括：

- 流线生成与追踪（种子点配置、流线简化）
- 自适应矢量场绘制（全单元 / 范围内 / 间隔采样）
- 张量场椭球/立方体渲染
- 结构应力形变计算与显示
- 等值面/等值线提取

## 本目录核心实现

| 文件 | 类 | 说明 |
|------|-----|------|
| `iGameStreamBase.h/.cpp` | `StreamBase` | 流线生成基类，配置种子与追踪参数 |
| `iGameStreamTracer.h/.cpp` | `StreamTracer` | 流线追踪算法 |
| `iGameStreamlineSimplifier.h/.cpp` | `StreamlineSimplifier` | 流线简化 |

## 关联源码路径

- 矢量场绘制：[`../VectorView/iGameVectorBase.h`](../VectorView/iGameVectorBase.h)（`DrawVector`、`DrawType::AllCell | CellInRange | EveryNth`）
- 张量场绘制：[`../TensorView/iGameTensorFilter.h`](../TensorView/iGameTensorFilter.h)
- 结构形变：[`../Deformation/iGameStressDeformationFilter.h`](../Deformation/iGameStressDeformationFilter.h)
- 等值面提取：[`../Contour/iGameContourFilter.h`](../Contour/iGameContourFilter.h)
- Qt 面板：[`../../../Qt/src/IQWidgets/igQtStreamTracerWidget.cpp`](../../../Qt/src/IQWidgets/igQtStreamTracerWidget.cpp)、`igQtVectorWidget`、`igQtTensorWidget`、`igQtDeformationWidget`

## 调用方式

### 编程接口：流线追踪

```cpp
auto streamBase = iGame::StreamBase::New();
auto streamtracer = streamBase->streamFilter;
streamtracer->initStreamTracer(dataObj);
// 配置种子点与追踪参数
streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
streamtracer->Execute();
streamBase->SetUpdate(true);
scene->AddModel(streamBase);
```

### 编程接口：自适应矢量场

```cpp
iGame::iGameVectorBase vectorView;
vectorView.SetDrawMode(iGame::iGameVectorBase::AllCell);   // 或 CellInRange / EveryNth
vectorView.DrawVector("Velocity", dataObj);
```

### 编程接口：结构形变

```cpp
auto deformFilter = iGame::StressDeformationFilter::New();
deformFilter->SetInput(drawObj);
deformFilter->Execute();
```

### GUI 调用

Qt 主窗口 Dock 面板：

- `dockWidget_FlowField` → `igQtStreamTracerWidget`（流线）
- `dockWidget_VectorField` → `igQtVectorWidget`（矢量场）
- `dockWidget_TensorField` → `igQtTensorWidget`（张量场）
- 形变面板 → `igQtDeformationWidget`（结构形变）
- `dockWidget_ContourExtract` → `igQtContourExtractWidget`（等值面）

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testStreamline` | 流线追踪 |
| `testVector` / `testVectorAllCell` / `testVectorCellInRange` / `testVectorEveryNth` | 自适应矢量场 |
| `testTensorView` | 张量场可视化 |
| `testDeformation` | 结构形变 |
| `testContourLine` | 等值面提取 |
| `testMultiscaleInteraction` | 多尺度交互联动 |