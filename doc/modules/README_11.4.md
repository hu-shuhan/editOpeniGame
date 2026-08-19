# 指标 11.4：CAE 仿真结果高精并行可视化软件

## 指标构成

面向 CAE 仿真结果，构建**高精并行可视化平台**：在统一数据模型与 OpenGL / Meshlet 管线上，集成高阶（等几何）高保真显示、多类型场可视化、多尺度交互、关键特征智能提取与结果评测能力，并形成可开源交付的软件体系。

| # | 子功能（对考核条目） | 状态 | 详细文档 |
|---|----------------------|------|----------|
| 1 | 高精并行可视化内核（对标 VTK：Meshlet GPU 加速、线程池、渲染压力调度） | ✅ 已实现 | **本文详写** |
| 2 | 等几何 / 高阶单元高保真可视化（谱方法专项见「已知缺口」） | ✅ 部分实现（Spline / Lagrange） | [README_7.1.md](README_7.1.md) |
| 3 | 云图 / 自适应矢量场 / 张量场等形式的场可视化 | ✅ 已实现 | [README_11.3.md](README_11.3.md) |
| 4 | 局部聚焦与局部微观 / 全局宏观流线联合显示 | ✅ 已实现 | [GitCode README_10.3.md](https://gitcode.com/yanhekaiyuan/iGameVis-Open/blob/main/doc/modules/README_10.3.md) |
| 5 | 多层次关键特征提取（经典算子、时序涡量与 LibTorch 涡检测） | ✅ 已实现 | 本文详写 + [README_10.2.md](README_10.2.md) |
| 6 | 基于 LLM 的可视化结果智能评测与分析报告生成 | ✅ 已实现（依赖外部 LLM / 报告服务） | 本文详写 + [GitCode README_10.3.md](https://gitcode.com/yanhekaiyuan/iGameVis-Open/blob/main/doc/modules/README_10.3.md) |

> **写法说明**：11.4 是**平台总指标**。子功能 1、5、6 在本文展开；2～4 以交叉引用为主，避免与其他指标文档重复粘贴。
> 与 **11.3** 的区别：11.3 写「场怎么画」；11.4 写「平台如何并行、加速，以及各专项如何拼成整机能力」。  

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

## 子功能 4：局部聚焦与多尺度流线联合显示

平台支持点选或框选局部区域，并根据选区包围框调整相机视角与旋转中心；流线生成时可同时使用局部选区中的高 / 低速度种子点和贯穿全场的种子线，联合展示微观流动结构与宏观流动趋势。

完整的操作步骤、界面截图和测试程序见 **[GitCode README_10.3.md](https://gitcode.com/yanhekaiyuan/iGameVis-Open/blob/main/doc/modules/README_10.3.md)**。

---

## 子功能 5：多层次关键特征智能提取

### 功能说明

分别面向表面网格与体网格执行经典特征提取，并将生成属性接入云图显示；同时支持多块 / 时序数据的逐块逐帧涡量计算，以及启用 LibTorch 后的涡结构智能检测。

| 输入与任务 | 执行内容 | 输出属性 / 结果 |
|------------|----------|-----------------|
| 表面网格 | 梯度、曲率、Laplacian | `gradient`、`curvatures`、`laplacians` |
| 三维体网格 | 经典涡量提取 | `vorticities` |
| 多块 / 时序体网格 | 各子块、各时间帧按需计算涡量 | 每块、每帧的 `vorticities` |
| LibTorch + 三维体速度场 | TorchScript 涡结构预测 | `vortexPredict`；有人工标注时同时计算评测指标 |

> 梯度、曲率和 Laplacian 要求表面网格。若当前数据是体网格，应先执行「数据处理 → 表面提取」，再选中新生成的 `<原名>_surface`；涡量则直接在原三维体网格上计算。

### 验收流程

1. **表面网格经典特征**：选中表面网格及待处理属性，依次执行梯度、曲率和 Laplacian；在模型树中选择新生成的属性，切换云图并检查数值范围和着色结果。
2. **体网格经典涡量**：选中体网格的速度矢量属性，执行「计算涡量」；选择 `vorticities` 的模长或分量，以云图检查涡结构。
3. **多块及时序涡量**：加载 PVD 等多块 / 时序数据并执行涡量计算。系统对当前帧各子块递归计算，并在动画播放时逐帧按需补算；拖动时间轴或播放动画，检查每一帧的 `vorticities` 是否刷新且动画是否连续。
4. **LibTorch 智能检测**：以 `ENABLE_LIBTORCH_MODULE=ON` 编译，准备 TorchScript `.pt` 模型后加载体网格速度场，执行「涡旋预测 (PredictVortex)」；切换到 `vortexPredict` 云图，检查预测标签与特征区域。若数据包含 `PredictedLabel`，同时检查 Accuracy / Precision / Recall 输出。

### 源码路径

| 路径 | 类 / API | 说明 |
|------|----------|------|
| `iGameCore/Filters/FeatureExtraction/iGameGradientFilter.*` | `GradientFilter` | 表面梯度 |
| `iGameCore/Filters/FeatureExtraction/iGameCurvatureFilter.*` | `CurvatureFilter` | 表面曲率 |
| `iGameCore/Filters/FeatureExtraction/iGameLaplacianFilter.*` | `LaplacianFilter` | 表面 Laplacian |
| `iGameCore/Filters/FeatureExtraction/iGameVortexFilter.*` | `VortexFilter` | 体网格涡量与 MultiBlock 递归 |
| `Qt/src/IQWidgets/igQtAnimationWidget.*` | `ensureVortexForCurrentFrame` | 动画播放期逐帧按需计算 |
| `iGameCore/Filters/FeatureExtraction/iGameVortexDetectionFilter.*` | `VortexDetection` | LibTorch 涡结构智能检测 |

完整 API、云图接入及智能检测指标见 **[README_10.2.md](README_10.2.md)**；逐帧涡量与动画操作见 **[README_11.3.md](README_11.3.md)**。

### 测试用例（入口）

| Target | 验证内容 | 条件 |
|--------|----------|------|
| `testGradientExtraction` / `testCurvatureExtraction` / `testLaplacianExtraction` | 表面网格经典特征 | 默认 |
| `testVortexExtraction` | 体网格经典涡量 | 默认 |
| `testTimeVaryingVector` + GUI 动画 | 多块及时序播放 | PVD 测试数据 |
| `testVortexDetection` | 预测标签、特征区域及标注评测 | `ENABLE_LIBTORCH_MODULE=ON` |

---

## 子功能 6：基于 LLM 的分析报告生成

### 功能说明

平台通过 `MeshReportGenerator` 将当前模型转换为适合报告分析的表面数据，完成三角化、简化和临时 VTK 导出，再由 `MeshReportClient` 发送给报告读取程序。报告程序读取预处理模型及属性场，调用已配置的 LLM 生成分析结论，并将报告文件返回客户端保存。

### 操作流程

1. **配置 LLM API 信息**：在外部报告读取程序中配置所用 LLM 的 API 地址、API Key 和模型名称。API 凭据只保存在报告服务端，不写入 iGameVis 客户端或仓库。
2. **启动报告读取程序并加载预切割模型**：启动报告服务，默认监听 `127.0.0.1:8766`；在 iGameVis 中加载已完成预切割 / 预处理的模型，并确认需要分析的属性场可用。若直接传入原始模型，客户端也会在发送前自动执行表面转换、三角化和简化。
3. **生成分析报告**：设置报告保存路径、报告服务地址 / 端口和需要分析的属性场，执行 `MeshReportGenerator::Execute()`。客户端发送 VTK 数据，接收报告程序返回的报告二进制内容，并保存为指定文件（例如 `.docx`）。

外部 Python 环境、大模型服务及 API 密钥配置的通用说明，可参考 **[GitCode README_10.3.md 的“基于 MCP 的文本交互”](https://gitcode.com/yanhekaiyuan/iGameVis-Open/blob/main/doc/modules/README_10.3.md)**；分析报告的模型预处理、TCP 请求和文件保存流程仍以本节说明为准。

### 源码路径

| 路径 | 类 / API | 说明 |
|------|----------|------|
| `iGameCore/Core/Common/MeshReport/iGameMeshReportGenerator.*` | `MeshReportGenerator::Execute` | 三角化、简化、传输、接收与保存报告的完整流程 |
| `iGameCore/Core/Common/MeshReport/iGameMeshReportClient.*` | `connect` / `requestReport` | 报告读取程序 TCP 客户端，默认端口 `8766` |
| `iGameCore/Filters/DataProcessing/iGameMeshTriangulationFilter.*` | `MeshTriangulationFilter` | 发送前网格三角化 |
| `iGameCore/Filters/DataProcessing/iGameMeshSimplificationFilterPro.*` | `MeshSimplificationFilterPro` | 发送前模型简化 |

### 调用方式

```cpp
auto obj = iGame::FileIO::ReadFile("./Models/precut_model.vtk");

auto report = iGame::MeshReportGenerator::New(
    "./output/analysis_report.docx", "127.0.0.1", 8766);
report->SetInput(obj);
report->SetSpecifiedFields({"pressure", "velocity"}); // 空列表表示分析全部属性场
report->SetSimplificationRatio(0.1f);

if (!report->Execute()) {
    std::cerr << report->GetErrorMessage() << std::endl;
}
```

### 验收要点

- 报告读取程序能够使用已配置的 LLM API 正常启动，客户端可连接到配置的主机和端口。
- 预切割模型及指定属性场能够被读取；服务端收到的 VTK 数据非空。
- 报告生成后保存路径中出现可正常打开的分析报告，内容包含所选属性场的分析结果。
- 当前仓库提供 iGameVis 侧的模型预处理和 TCP 客户端；LLM 配置、提示词和报告模板由外部报告读取程序负责。

---

## 相关示例汇总

| Target | 对应子功能 | 条件 |
|--------|------------|------|
| `testMeshletRendering` | 1 并行加速 | 默认 |
| `testSetRenderingPressure` | 1 渲染压力 | 默认 |
| `testSplineReaderCPU` / `testSplineReaderGPU` / `testConvertToLagrangeUnstructuredMesh` | 2 高保真 | GPU 样条需 GPS CUDA |
| `testSetScalarField` / `testVector*` / `testTensorView` / … | 3 场可视化 | 见 11.3 |
| GUI 包围框视角 / `testStreamline` / `testMultiscaleInteraction` | 4 局部聚焦与多尺度流线 | 默认 |
| `testGradientExtraction` / … / `testVortexDetection` | 5 经典与智能特征提取 | 涡预测需 LibTorch |
| `MeshReportGenerator::Execute` | 6 LLM 分析报告 | 需外部报告读取服务 |

---

## 关联指标

| 指标 | 文档 | 与 11.4 关系 |
|------|------|--------------|
| 7.1 | [README_7.1.md](README_7.1.md) | 高阶 / 等几何高保真 |
| 10.1 | [README_10.1.md](README_10.1.md) | 分析数据生成（熵种子、流线筛选等） |
| 10.2 | [README_10.2.md](README_10.2.md) | 关键特征提取与评测 |
| 11.3 | [README_11.3.md](README_11.3.md) | 场可视化输出 |

模块索引：[README.md](README.md)
