# 指标 11.4：CAE 仿真结果高精并行可视化软件

## 指标构成

面向 CAE 仿真结果，构建**高精并行可视化平台**：在统一数据模型与 OpenGL / Meshlet 管线上，集成高阶（等几何）高保真显示、多类型场可视化、多尺度交互、关键特征智能提取与结果评测能力，并形成可开源交付的软件体系。

| # | 子功能（对考核条目） | 状态 | 详细文档 |
|---|----------------------|------|----------|
| 1 | 高精并行可视化内核（对标 VTK：Meshlet GPU 加速、线程池、渲染压力调度） | ✅ 已实现 | **本文详写** |
| 2 | 等几何 / 高阶单元高保真可视化（谱方法专项见「已知缺口」） | ✅ 部分实现（Spline / Lagrange） | [README_7.1.md](README_7.1.md) |
| 3 | 云图 / 自适应矢量场 / 张量场等形式的场可视化 | ✅ 已实现 | [README_11.3.md](README_11.3.md) |
| 4 | 大规模多尺度物理场特征可视交互 | ✅ 已实现 | [README_10.3.md](README_10.3.md) |
| 5 | 多层次关键特征智能提取 | ✅ 已实现 | [README_10.2.md](README_10.2.md) |
| 6 | 融合专家知识 / 标注的可视化结果智能评测 | ✅ 部分实现（涡预测 Precision/Recall；网格质量度量待强化） | 本文概述 + [README_10.2.md](README_10.2.md) |

> **写法说明**：11.4 是**平台总指标**。子功能 1、6 在本文展开；2～5 以交叉引用为主，避免与 7.1 / 10.x / 11.3 重复粘贴。  
> 与 **11.3** 的区别：11.3 写「场怎么画」；11.4 写「平台如何并行、加速，以及各专项如何拼成整机能力」。  
> 与 **11.2** 的区别：11.2 侧重多格式 IO；11.4 侧重渲染并行与整机集成。

![架构图](../../Resources/Images/架构图.png)

---

## 平台总览

`iGameCore` + `Qt` 构成 iGameVis 可视化软件主体：

| 层级 | 路径 | 职责 |
|------|------|------|
| 数据与并行 | `iGameCore/Core/` | `DataObject` / `DrawObject`、`ThreadPool` |
| 算法过滤 | `iGameCore/Filters/` | 特征提取、场可视化、形变、多尺度分析 |
| 读写 | `iGameCore/IO/` | VTK / CGNS / PVD / Spline 等 |
| 渲染 | `iGameCore/Rendering/` | Scene、OpenGL、Meshlet、交互器 |
| GUI | `Qt/` | 主窗口、Dock、模型树、过滤器菜单 |

典型 GUI 数据流：

```text
main.cpp → igQtMainWindow → igQtFileLoader::OpenFile()
         → FileIO::ReadFile() → Scene::AddModel()
         → Dock / 菜单调用 Filters / DrawObject / Meshlet
```

---

## 子功能 1：高精并行可视化内核

### 功能说明

在统一 `DrawObject` 管线上提供三类并行 / 加速能力，支撑大规模 CAE 网格高帧率交互：

1. **Meshlet GPU 加速**：将网格划分为 Meshlet，经 mesh shader / 计算着色器做视锥裁剪与绘制调度。  
2. **CPU 线程池**：`ThreadPool::parallelFor` / `Commit` 用于 Filter、编解码、时序帧加载等。  
3. **渲染压力调度**：`Scene` 按目标帧率 / GPU 占用限制跳帧，保证交互流畅。

### 源码路径

| 路径 | 类 / API | 说明 |
|------|----------|------|
| `iGameCore/Rendering/Core/Meshleter/` | `Meshleter` / `SurfaceMeshMeshleter` | Meshlet 构建与绘制 |
| `iGameCore/Rendering/Shaders/GLSL/MeshShaders/` | Mesh / Task / MeshletCull | GPU 着色器 |
| `iGameCore/Core/DataModel/iGameDrawObject.*` | `SetAccelerationOption` / `SetRenderWithMeshlet` | 打开加速 |
| `iGameCore/Core/Common/iGameThreadPool.h` | `parallelFor` / `Commit` | CPU 并行 |
| `iGameCore/Rendering/Core/iGameScene.*` | `SetTargetFps` / `SetGpuUsageLimit` / `ShouldRenderThisCall` | 帧率与压力控制 |
| `Qt/src/IQComponents/igQtModelTreeWidget.*` | 模型树菜单 | 可切换加速选项 |

### 调用方式

**Meshlet 加速**（`Examples/Rendering/MeshletRendering.cpp`）：

```cpp
auto scene = iGame::Scene::New();
auto dataObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);   // 启用 Meshlet 加速路径
scene->AddModel(dataObj);
scene->ResetCameraView();
```

**渲染压力 / 目标帧率**（`Examples/Rendering/SetRenderingPressure.cpp`）：

```cpp
auto scene = iGame::Scene::New();
auto dataObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
scene->AddModel(dataObj);
scene->ResetCameraView();
// scene->SetGpuUsageLimit(0.1f);  // 可选：限制 GPU 占用比例
scene->SetTargetFps(30);           // 目标帧率；内部按 ShouldRenderThisCall 跳帧
```

**CPU 并行**（Filter / IO 内部广泛使用）：

```cpp
iGame::ThreadPool::parallelFor(0, count, [&](int i) {
    // 并行处理第 i 个单元 / 点
});
```

### GUI

| 入口 | 说明 |
|------|------|
| 模型树右键 / 加速相关菜单 | `SetAccelerationOption`、Meshlet 开关 |
| 场景交互 | 大模型下由 `SetTargetFps` / GPU 限制保持流畅 |

### 测试用例

| Target | 源文件 | 默认数据 | 说明 |
|--------|--------|----------|------|
| `testMeshletRendering` | `Examples/Rendering/MeshletRendering.cpp` | `./Models/Tet_Plane.vtk` | Meshlet GPU 加速 |
| `testSetRenderingPressure` | `Examples/Rendering/SetRenderingPressure.cpp` | `./Models/Tet_Plane.vtk` | 目标帧率 / 渲染压力 |

编译 Examples 需 `EXAMPLE_COMPILE=ON`（或按仓库当前 Examples 独立构建方式）。

---

## 子功能 2：等几何 / 高阶单元高保真可视化

### 功能说明

支持 Lagrange 高阶单元转换与 NURBS/Spline 几何的 CPU / GPU 读取与可视化，面向等几何分析（IGA）等新型仿真结果的高保真显示。

细节、API、GUI 与示例见 **[README_7.1.md](README_7.1.md)**。

### 测试用例（入口）

| Target | 条件 |
|--------|------|
| `testConvertToLagrangeUnstructuredMesh` | 默认 |
| `testSplineReaderCPU` | 默认 |
| `testSplineReaderGPU` | `ENABLE_GPSCUDA_MODULE=ON` |

### 已知缺口

- **谱方法（spectral）专用可视化模块**：当前仓库无独立「谱方法」场可视化专项；高保真能力以 **Spline / Lagrange 高阶单元** 为主承载。  
- VTK 高阶网格端到端可视化仍有适配限制（见用户手册 / 7.1「已知限制」）。

---

## 子功能 3：云图 / 自适应矢量场 / 张量场等场可视化

### 功能说明

平台集成标量云图、自适应采样矢量 Glyph、张量 Glyph、结构形变、时序流线与动画导出等输出能力。

细节、截图、调用方式与测试用例见 **[README_11.3.md](README_11.3.md)**（英文：[README_11.3.en.md](README_11.3.en.md)）。

### 测试用例（入口）

| Target | 说明 |
|--------|------|
| `testSetScalarField` | 云图 |
| `testVector` / `testVectorEveryNth` / … | 自适应矢量场 |
| `testTensorView` | 张量场 |
| `testDeformation` / `testDeformationCode` | 结构形变 |
| `testStreamline` / `testTimeVaryingVector` | 流线 / 时序 |
| `testAnimation` / `testSaveAnimation` | 动画播放 / 导出 |

---

## 子功能 4：大规模多尺度物理场特征可视交互

### 功能说明

通过并行坐标、相关矩阵、密度图、探针线等 2D 分析视图刷选，经 `Selection` 回调与 3D 模型高亮 / 筛选联动，形成「多尺度」感知驱动交互。

细节见 **[README_10.3.md](README_10.3.md)**。

### 测试用例（入口）

| Target | 源文件 |
|--------|--------|
| `testMultiscaleInteraction` | `Examples/MultiscaleInteraction/TestMultiscaleInteraction.cpp` |
| `testParallelCoordinatesData` 等 | 各单视图示例 |

---

## 子功能 5：多层次关键特征智能提取

### 功能说明

提供经典物理特征（梯度 / 曲率 / Laplacian / 涡量）与基于神经网络的涡结构检测；结果写入 `AttributeSet`，可接 11.3 云图与 10.3 选区分析。

细节与精度指标（Precision / Recall ≥ 90%）见 **[README_10.2.md](README_10.2.md)**。

### 测试用例（入口）

| Target | 条件 |
|--------|------|
| `testGradientExtraction` / `testCurvatureExtraction` / `testLaplacianExtraction` / `testVortexExtraction` | 默认 |
| `testVortexDetection` | `ENABLE_LIBTORCH_MODULE=ON` |

---

## 子功能 6：可视化结果智能评测

### 功能说明

当前与「专家知识 / 标注」结合最完整的路径是：**涡结构预测结果 vs 人工标注属性**，计算 Accuracy / Precision / Recall（实现于 10.2 `VortexDetection::EvaluatePredictMetrics`）。

网格质量度量（`Filters/MeshMetrics/`）可作为几何质量侧辅助评测，但 GUI / Examples 接入仍偏弱，现场演示建议以涡预测指标为主。

### 源码路径

| 路径 | API | 说明 |
|------|-----|------|
| `iGameCore/Filters/FeatureExtraction/`（涡预测） | `EvaluatePredictMetrics` / `GetPrecision` / `GetRecall` | 标注对比评测 |
| `iGameCore/Filters/MeshMetrics/` | `SurfaceMeshMetricsFilter` / `VolumeMeshMetricsFilter` | 网格质量（待强化接入） |

### 调用方式（摘自 10.2 路径）

```cpp
// 在完成涡预测且存在 PredictedLabel 标注属性后：
filter->EvaluatePredictMetrics(/* ... */);
double precision = filter->GetPrecision();
double recall    = filter->GetRecall();
```

### 测试用例

| Target | 说明 |
|--------|------|
| `testVortexDetection` | 涡预测 + 指标计算（需 LibTorch） |

### 已知缺口

- 「融合专家知识」的**独立可视化质量评测组件**（规则库 / 专家打分面板）尚未成独立产品模块。  
- 主窗口 Precision/Recall 浮层曾预留，当前可能为注释状态；数值仍可通过 API / 控制台读取。

---

## 开源与交付物映射

考核交付物与本仓库文档的对应关系（证书编号请按实际补全）：

| 交付物 | 说明 | 登记 |
|--------|------|------|
| 开源软件代码库 | 本仓库 `iGameCore` + `Qt` + `Examples` + `doc/` | 仓库地址 / License：________ |
| 软件著作权 × 5 | 按软著证书归档 | 证书编号：________ |
| 软件测试报告 × 5 | 建议覆盖：Meshlet、场可视化(11.3)、特征提取(10.2)、多尺度交互(10.3)、高阶/Spline(7.1) | 报告编号：________ |
| 硕 / 博士培养 × 2 | 学位证书归档 | 姓名 / 学位：________ |

运行与验收操作说明可参考：[Examples/HOW_TO_RUN.md](../../Examples/HOW_TO_RUN.md)（若已纳入仓库）。

---

## 相关示例汇总

| Target | 对应子功能 | 条件 |
|--------|------------|------|
| `testMeshletRendering` | 1 并行加速 | 默认 |
| `testSetRenderingPressure` | 1 渲染压力 | 默认 |
| `testSplineReaderCPU` / `testSplineReaderGPU` / `testConvertToLagrangeUnstructuredMesh` | 2 高保真 | GPU 样条需 GPS CUDA |
| `testSetScalarField` / `testVector*` / `testTensorView` / … | 3 场可视化 | 见 11.3 |
| `testMultiscaleInteraction` | 4 多尺度交互 | 默认 |
| `testGradientExtraction` / … / `testVortexDetection` | 5–6 特征与评测 | 涡预测需 LibTorch |

---

## 验收自检清单

| 子功能 | 建议现场验证 |
|--------|----------------|
| 1 并行内核 | 打开 Meshlet：`testMeshletRendering`；设置目标帧率：`testSetRenderingPressure`；大网格交互不卡死 |
| 2 高保真 | 加载 Spline XML 或 Lagrange 转换结果，几何与属性显示正确 |
| 3 场可视化 | 按 11.3 清单：云图 / 矢量 / 张量 / 形变 / 流线 / 动画至少各演示一项 |
| 4 多尺度交互 | `testMultiscaleInteraction`：2D 刷选 ↔ 3D 高亮联动 |
| 5 特征提取 | 梯度 / 曲率 / 涡预测等结果进入属性树并可云图显示 |
| 6 智能评测 | `testVortexDetection` 输出 Precision / Recall；说明与标注对比流程 |
| 开源交付 | 可指出代码路径、编译方式；软著 / 测试报告 / 培养材料备查 |

---

## 关联指标

| 指标 | 文档 | 与 11.4 关系 |
|------|------|--------------|
| 7.1 | [README_7.1.md](README_7.1.md) | 高阶 / 等几何高保真 |
| 10.1 | [README_10.1.md](README_10.1.md) | 分析数据生成（熵种子、流线筛选等） |
| 10.2 | [README_10.2.md](README_10.2.md) | 关键特征提取与评测 |
| 10.3 | [README_10.3.md](README_10.3.md) | 多尺度可视交互 |
| 11.2 | [README_11.2.md](README_11.2.md) | 多格式 IO |
| 11.3 | [README_11.3.md](README_11.3.md) | 场可视化输出 |

模块索引：[README.md](README.md)
