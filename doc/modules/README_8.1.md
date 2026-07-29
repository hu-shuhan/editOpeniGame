# 指标 8.1：CAE 仿真数据轻量可视化与高性能渲染

## 指标构成

面向大规模 CAE 仿真网格及结果数据，提供基于 Meshlet 和网格简化的轻量可视化与高性能渲染能力。

| 项目 | 考核要求 |
|------|----------|
| 功能 | 支持 CAE 仿真数据的轻量可视化与高性能渲染 |
| 技术路线 | Meshlet GPU 渲染与网格简化 |
| 规模 | C/S 架构下处理 10 亿级网格数据和结果数据 |
| 成果 | 发表 2 篇高水平论文、申请 1 项发明专利、培养硕/博士研究生 2 名 |
| 考核方式 | 有资质第三方机构评测 |
| 交付物 | 开放链接库、论文发表/录用证明、发明专利受理证书、硕/博士研究生学位证书 |

> 本文档记录当前仓库可追溯的渲染与简化实现。10 亿级 C/S 性能、论文、专利和人才培养须由独立部署记录、测试报告和证明材料验收，不能仅由源码说明证明。

---

## 子功能 1：Meshlet GPU 加速渲染

### 功能说明

针对三角表面网格，系统使用 meshoptimizer 构建 Meshlet，将局部顶点、三角形索引和包围体上传至 OpenGL 缓冲区；渲染阶段通过计算着色器进行可见性剔除并调度绘制，降低不可见几何的处理开销。

当前 Meshlet 配置的每个簇最多包含 64 个顶点和 124 个三角形。

### 源码路径

| 路径 | 类 / 文件 | 说明 |
|------|-----------|------|
| `iGameCore/Rendering/Core/Meshleter/iGameMeshleter.*` | `Meshleter` | Meshlet 数据结构、GPU 缓冲和同步 |
| `iGameCore/Rendering/Core/Meshleter/iGameSurfaceMeshMeshleter.*` | `SurfaceMeshMeshleter` | 三角表面网格 Meshlet 构建、优化和包围体计算 |
| `iGameCore/Rendering/Shaders/GLSL/MeshletCull.comp` | 计算着色器 | Meshlet 可见性剔除 |
| `iGameCore/Rendering/Core/iGameModel.cpp` | 模型绘制路径 | Meshlet 渲染调度 |
| `Examples/Rendering/MeshletRendering.cpp` | `testMeshletRendering` | Meshlet 渲染示例 |

### 调用方式

对应示例 `Examples/Rendering/MeshletRendering.cpp`：

```cpp
auto dataObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);

if (drawObj != nullptr) {
    drawObj->SetAccelerationOption(true);
    scene->AddModel(dataObj);
}
```

`SetAccelerationOption(true)` 启用模型的加速渲染路径。输入应为当前实现支持的三角表面网格；非三角表面或不满足构建条件的对象会回退为普通绘制。

### 测试用例

| Target | 源文件 | 默认数据 | 说明 |
|--------|--------|----------|------|
| `testMeshletRendering` | `Examples/Rendering/MeshletRendering.cpp` | `./Models/Tet_Plane.vtk` | Meshlet 构建与 GPU 绘制 |

---

## 子功能 2：网格简化

### 功能说明

`MeshSimplificationFilter` 面向三角表面网格提供基于二次误差度量的简化能力。调用方可设置目标简化率、边界保持和参与误差计算的属性，以减少渲染几何规模。

### 源码路径

| 路径 | 类 / 文件 | 说明 |
|------|-----------|------|
| `iGameCore/Filters/DataProcessing/iGameMeshSimplificationFilter.*` | `MeshSimplificationFilter` | 网格简化 Filter |
| `iGameCore/Filters/DataProcessing/Simplification/` | 简化辅助算法 | 属性保持与误差计算支撑 |
| `iGameCore/Core/DataModel/iGameDrawObject.cpp` | 交互压力路径 | 绘制压力下的简化调用 |
| `Examples/Filter/Compression/TestSimplification.cpp` | `testSimplification` | 网格简化示例 |

### 调用方式

```cpp
auto filter = iGame::MeshSimplificationFilter::New();
filter->SetTargetReduction(0.5);
filter->SetPreserveBoundary(true);
filter->SetInput(surfaceMesh);
filter->Execute();

auto simplified = filter->GetOutput();
```

`SetTargetReduction(0.5)` 表示示例中的目标面数减少比例为 50%。实际简化结果还受输入拓扑、边界和属性保持约束影响。

### 测试用例

| Target | 源文件 | 默认数据 | 说明 |
|--------|--------|----------|------|
| `testSimplification` | `Examples/Filter/Compression/TestSimplification.cpp` | `./Models/mazewheel.obj` | 三角表面网格简化与显示 |

---

## 开放链接库与验收边界

根工程在 Release 构建并执行安装后，可输出头文件、库文件、运行时依赖和 CMake 包配置，供外部程序集成。发布包应包含与测试版本一致的 `include/`、`lib/`、`bin/`、`Resources/` 及版本说明。

| 验收项 | 建议验证内容 |
|--------|--------------|
| Meshlet 渲染 | Meshlet 目标可构建、加载三角网格后正常显示、切换加速选项无异常 |
| 网格简化 | 记录简化前后的点/面数量、边界保持状态和关键属性范围 |
| 轻量可视化 | 比较普通绘制与加速/简化绘制的首帧、交互帧率和显存/内存占用 |
| 10 亿级 C/S | 固定客户端/服务端硬件、网络、数据规模、传输、解码、首帧和交互日志，由第三方复测 |
| 成果证明 | 论文、专利和学位证明与软件测试材料独立归档 |

当前仓库包含本地渲染、Meshlet 和网格简化实现，但未包含可直接证明 10 亿级 C/S 分布式处理能力的服务端调度实现或第三方性能报告。
