# GPU 体绘制（Compute Shader）参考资料与落地清单

> 面向 iGameVis（C++17 + Qt5 + OpenGL，`IGAME_OPENGL_VERSION_460` 路径）整理。
> 目的：在动手用 compute shader 重写体绘制之前，把**算法路线、必读文献、参考实现、测试数据、本仓库缺口**一次对齐。

---

## 1. 先定路线：数据形态决定算法族

体绘制的实现方式几乎完全由"数据是规则体还是非结构网格"决定，需要先做决策，再选资料。

| 数据形态 | 推荐算法族 | 计算特征 | 成熟度 | 与本仓库的契合点 |
|---|---|---|---|---|
| 规则网格 / 结构网格（CT、CFD 结果重采样、标量场） | 3D Texture + Compute Ray Casting（砖块 + min/max 金字塔 + 空域跳过 + 早停） | 每像素一条光线，workgroup 制导，高度并行 | 最高，VTK/OSPRay 等都有成熟实现 | `StructuredMesh`（i/j/k + `GetPointIndex`）可直接映射为 3D 纹理；缺 `GLTexture3d` |
| 非结构四面体 / 多面体（VTK/CGNS 原始网格） | ① GPU 光线-单元求交（BVH 加速）② Cell Projection + compute 排序合成 | ① 需要单元定位加速结构 ② 每 cell 投影 + 逐像素排序 | 中，工程量大 | `VolumeMesh`/`UnstructuredMesh` 已有；现有链接表方案已在此路线上 |
| 混合 / 超显存数据 | Brick 稀疏缓存 + 分级 LOD + out-of-core | 需要砖块池、异步上传、LOD 生成 | 高但工程复杂 | 现有 `GLBuffer`/`GLTextureBuffer` 可扩展 |

**对本项目的关键判断**：如果目标是"把 CAE 结果的物理量（应力/温度/压力）画成体"，而数据只有非结构网格，那么**先做"非结构 → 规则体"的重采样**（仓库已有 `iGameConvertToVolumeMeshFilter` 可参考），可以把整条 compute 管线收敛到最成熟的规则体路线上，最快拿到可用结果；非结构直接光线求交可以作为第二阶段的目标。

---

## 2. 光学模型与合成公式（动手前必须统一）

所有实现细节都可以妥协，唯独合成公式必须统一，否则调参时会出现"越调越偏"的现象。

- 发射-吸收模型、α 合成、梯度光照的数学基础：
  - Max, N. *Optical Models for Direct Volume Rendering*. IEEE TVCG, 1995.
  - Drebin, Carpenter, Hanrahan. *Volume Rendering*. SIGGRAPH 1988.（传输函数 / 概率分类的源头）
- 前向合成（front-to-back，推荐，支持早停）：

  ```
  C += (1 - A) * a * c;
  A += (1 - A) * a;
  if (A > 0.99) break;   // 早停
  ```

- 中间累加缓冲建议 `RGBA16F`，最后再做色调映射；用 `RGBA8` 在 α 接近 1 时会出现明显阶梯。

---

## 3. 必读文献清单

### 3.1 教材（先读一本就够）

| 材料 | 说明 |
|---|---|
| *Real-Time Volume Graphics*（Engel, Hadwiger, Kniss, Rezk-Salama, Weiskopf；A K Peters, 2006） | 事实标准教材，覆盖光学模型、切片与光线投射、传输函数、光照、GPU 实现。中文资料里的"体绘制"术语体系基本来自这里 |
| *OpenGL Programming Guide*（第 8 版）"Memory" 与 "Compute Shaders" 两章 | 覆盖图像读写（image load/store）、SSBO、原子操作、workgroup 语义，是写 compute 体绘制的语法基础 |
| *High Performance Visualization*（Bethel, Childs, Hansen 编, 2012） | 面向超大规模科学可视化的工程视角：out-of-core、并行、in-situ |

### 3.2 GPU 光线投射框架（最核心的一组）

| 文献 | 解决的问题 |
|---|---|
| Levoy, M. *Display of Surfaces from Volume Data*. IEEE CG&A, 1988. | 光线投射的起点 |
| Levoy, M. *Efficient Ray Tracing of Volume Data*. ACM TOG, 1990. | 分级八叉树、自适应采样、提前终止——空域跳过的理论根源 |
| Roettger, Guthe, Weiskopf, Ertl, Strasser. *Smart Hardware-Accelerated Volume Rendering*. VisSym 2003. | 3D 纹理 + 代理几何的 ray casting 框架与质量/性能策略 |
| Stegmaier, Strengert, Klein, Ertl. *A Simple and Flexible Volume Rendering Framework for Graphics-Hardware-Based Raycasting*. Volume Graphics 2005. | 工程上最接近"可直接照做"的框架：采样步长、传输函数、光照分层组织 |
| Krüger, Westermann. *Acceleration Techniques for GPU-Based Volume Rendering*. IEEE VIS 2003. | GPU 上的空域跳过与早停，性能提升的主来源 |
| Engel, Kraus, Ertl. *High-Quality Pre-Integrated Volume Rendering*. EGGH/HWWS 2001. | 预积分传输函数，消除大步长造成的色偏与锯齿 |
| Hadwiger 等. *Advanced Illumination Techniques for GPU Volume Ray Casting*. SIGGRAPH Asia 2008 课程. | 光照、阴影、AO 的系统整理 |

### 3.3 非结构网格（四面体）体绘制

| 文献 | 说明 |
|---|---|
| Shirley, Tuchman. *A Polygonal Approximation to Direct Scalar Volume Rendering*. 1990. | Projected Tetrahedra 原始论文 |
| Max, Hanrahan, Crawfis. *Area and Volume Coherence for Efficient Visualization of 3D Scalar Functions*. 1990. | 四面体内线性插值 + 投影一致性 |
| Garrity. *Raytracing Irregular Volume Data*. 1990. | 非结构体数据的光线投射 |
| Weiler, Kraus, Merz, Ertl. *Hardware-Based View-Independent Cell Projection*. IEEE TVCG 2003. | 现代 GPU 上把 cell projection 做高质量 |
| Marroquim, Maximo, Farias, Esperança. *Volume and Isosurface Rendering with GPU-Accelerated Cell Projection*. CGF 2008. | 给出完整 GPU 实现路径，适合复现 |
| Muigg, Hadwiger, Doleisch, Grüner. *Interactive Volume Visualization of General Polyhedral Grids*. IEEE TVCG 2011. | 任意多面体网格的 GPU 光线投射 |
| Garth, Childs. *Fast, Memory-Efficient Cell Location in Unstructured Grids*. IEEE TVCG 2010. | 非结构光线投射的关键组件：BVH + 稀疏索引做单元定位 |

### 3.4 稀疏结构与大数据工程化

| 材料 | 说明 |
|---|---|
| GVDB：*Raytracing Sparse Voxel Database Structures on the GPU*（HPG 2016），仓库 `nvidia/gvdb-voxels` | 稀疏砖块 + GPU 光线遍历 |
| Museth. *VDB: High-Resolution Sparse Volumes with Dynamic Topology*. ACM TOG 2013；NanoVDB（SIGGRAPH 2021 Talks） | GPU 友好的稀疏体结构，OpenVDB/ASWF 维护 |
| Laine, Karras. *Efficient Sparse Voxel Octrees*. I3D 2010. | SVO 遍历 |
| Fogal, Krüger. *Tuvok: An Architecture for Large Scale Volume Rendering*. VMV 2010. | out-of-core + 多 GPU 的完整架构 |
| Hillaire, S. *Physically Based and Unified Volumetric Rendering in Frostbite*. SIGGRAPH 2015（Advances in Real-Time Rendering 课程） | compute + froxel 3D 网格：光注入、时域复用、各向异性累加，架构可迁移 |

### 3.5 现代光线传输与去噪（质量上限路线）

- Kroes, Post, Botha. *Exposure Render: An Interactive Photo-Realistic Volume Rendering Framework*. PLoS ONE, 2012.
- Novák 等. *Monte Carlo Methods for Volumetric Light Transport Simulation*. Eurographics STAR, 2018.
- 若走随机采样路线，去噪（SVGF / 双边滤波 / OptiX Denoiser 思路）是必需配套。

---

## 4. OpenGL Compute Shader 工程资料

### 4.1 规范与官方文档

- Khronos OpenGL Wiki（`registry.khronos.org/OpenGL/wiki`）中这几页要通读：
  - Compute Shader（workgroup 语义、内存模型、shared memory）
  - Image Load Store（`image3D`、`glBindImageTexture`、格式限定符）
  - Shader Storage Buffer Object（std430 布局、动态长度数组）
  - Atomic Counter、Memory Model（barrier 与可见性规则）
- 扩展规范：`GL_ARB_compute_shader`、`GL_ARB_shader_image_load_store`、`GL_ARB_shader_storage_buffer_object`、`GL_ARB_shader_ballot`、`GL_KHR_shader_subgroup`（做 warp 内前缀和、排序、加速结构遍历时非常关键）。
- 本仓库已自带 `iGameCore/Rendering/Shaders/GLSLangTool/`（三平台 `glslangValidator`），新增 `.comp` 建议纳入离线校验流程。

### 4.2 调试与性能工具

| 工具 | 用途 |
|---|---|
| RenderDoc | 通用 OpenGL 抓帧，可直接单步 compute、查看 image/SSBO 内容——排查"光线结果为空/全黑"类问题最高效 |
| NVIDIA Nsight Graphics | 着色器占用率、访存瓶颈、warp 分歧分析 |
| AMD Radeon GPU Profiler / Intel GPA | 多厂商覆盖 |

### 4.3 compute 在体绘制中的典型分工

1. 每像素光线生成与包围盒求交（初始化首尾 t）
2. min/max 金字塔生成（归约）与空域跳过
3. 采样、梯度求值、光照
4. 前向合成与早停
5. 传输函数预积分表生成（小纹理，一次算好）
6. 直方图统计 / 传输函数自动设计
7. 大数据的砖块 LOD 生成、压缩体解码
8. 非结构路线的逐像素片段排序（基数/双调排序替掉现在的插入排序）
9. 降噪与后处理

---

## 5. 可直接读代码的开源实现

| 项目 | 参考价值 |
|---|---|
| **VTK**：`vtkOpenGLGPUVolumeRayCastMapper` + `vtkVolumeShaderComposer` | 最贴近"CAE 可视化产品"的参考：传输函数（1D/2D）、裁剪、光照、混合策略、多 pass 组织。你们已经在读 VTK 格式，术语和数据结构可以直接对齐 |
| **ParaView** 文档与实现 | 功能与交互基线（体绘制参数面板、颜色/不透明度映射、性能策略），也是对标用户体验的参照 |
| **Intel OSPRay / OSPRay Studio** | 基于砖块与稀疏结构的 CPU/GPU 光线追踪框架，工程架构清晰 |
| **NVIDIA OptiX SDK**（`optixVolumeViewer` 等示例）+ OptiX Advanced Samples（NanoVDB、路径追踪、降噪） | 若未来允许 CUDA/OptiX 路线 |
| **SCI Utah：Tuvok / ImageVis3D** | out-of-core 大体积渲染的完整开源实现 |
| **Voreen** | 开源体渲染框架，含 STL 内部结构与网络式管线组织 |
| **OpenVDB / NanoVDB**（ASWF） | 稀疏体数据结构与 GPU 读取路径 |
| **NVIDIA GVDB-Voxels** | 稀疏砖块 + GPU 光线遍历 |
| **Google Filament**（froxel volumetrics）、**Wicked Engine** | compute 驱动体积渲染的干净工程实现，适合借鉴 pass 划分与资源管理 |
| **Khronos ANARI**（+ VisRTX / OSPRay 后端） | 另一条路：不自研渲染器，用标准 API 接入现成 GPU 体渲染后端 |

---

## 6. 测试数据与性能基准

| 来源 | 说明 |
|---|---|
| Open SciVis Datasets（`klacansky.com/open-scivis-datasets`） | 数十个公开体数据，标注分辨率与体积（CT、流场、燃烧等），适合做固定性能基准 |
| The Volume Library（Ulm 大学，早期 volvis.org 数据集的后续维护版本） | 经典体数据（Tooth、Bonsai 等） |
| ParaView 自带示例数据、Visible Human 数据 | 快速验证与截图对比 |
| 本项目 CAE 数据 | `Tet_Plane.vtk` 等经由重采样转为规则体，作为真实业务基准 |
| 程序化生成（柏林噪声等） | 扫分辨率 256³ / 512³ / 1024³，观察带宽瓶颈与显存占用曲线 |

**基准指标建议**：① 固定相机下的帧时间；② 采样步长—质量曲线；③ 显存占用；④ 首帧加载/上传耗时；⑤ 交互（旋转/缩放）期间的帧率降级策略。

---

## 7. 本仓库现状与缺口

### 7.1 现有实现（体绘制的当前基线）

- 渲染路径：`Scene::VolumeRenderingPass()`（`iGameCore/Rendering/Core/iGameScene.cpp:1467`）
  1. 清空 OIT 原子计数器与 head pointer 纹理
  2. `Model::DrawWithVolume()`（`iGameModel.cpp:647`）绘制几何，片段写入逐像素链接表
  3. 全屏 pass 取链接表、逐像素**插入排序**、再按片段间插值重建
  4. Blit 回主 framebuffer
- 着色器：`Shaders/GLSL/VolumeRenderingLink.frag`、`VolumeRenderingSort.frag`（`#version 420 core`）
- 性能约束（代码中已写明）：离屏 FBO 固定宽 `VOLUME_RENDERING_FRAMEBUFFER_WIDTH = 1024`（`iGameRenderingMacro.h`）；`MAX_FRAGMENTS = 512`；`STEP_BETWEEN_FRAGMENTS = 100`；`Examples/Rendering/SetVolumeRendering.cpp` 注释明确指出"只适合小规模数据与小窗口"。

### 7.2 已有可复用设施

| 设施 | 位置 | 用途 |
|---|---|---|
| compute 调度与 GL 4.6 分支 | `IGAME_OPENGL_VERSION_460`；`glDispatchCompute` 见 `iGameModel.cpp:1241`、`:1364` | compute 管线入口模板 |
| 现成 compute 着色器 | `Shaders/GLSL/MeshletCull.comp`、`DepthReduce.comp`（`#version 450`） | 数据驱动的 compute 写法、SSBO/image 绑定范式 |
| GL 封装 | `Rendering/OpenGL/`：`GLBuffer`（含 `GL_SHADER_STORAGE_BUFFER`）、`GLTexture2d`（`BindImage`）、`GLTextureBuffer`、`GLFrameBuffer`、`GLShader` | 扩展 3D 纹理的接入点 |
| 间接绘制 | `GLIndirectCommand.h` | 体绘制中"自适应 workgroup / 多段 dispatch"场景可用 |
| 着色器注册 | `iGameShaderManager.cpp`：`ShaderType` 枚举 + `./Resources/Shaders/*` 注册 | 新增体绘制 compute 着色器的注册位置 |
| 数据侧 | `iGameStructuredMesh`（i/j/k、`GetPointIndex`）、`iGameVolumeMesh`、`iGameUnstructuredMesh` | 规则体映射 3D 纹理；非结构网格走单元投影/求交 |
| 映射器 | `DrawObject::GetColorMapper()` 的 `SetOpacityMappingEnabled` | 可演进为传输函数 1D/2D 纹理来源 |

### 7.3 需要补齐的基础设施（按优先级）

1. **`GLTexture3d` / 3D image 支持**：当前仓库无 `GL_TEXTURE_3D`、`sampler3D`、`image3D` 的任何使用，这是规则体路线的第一块砖。
2. **传输函数纹理**：1D（颜色）+ 2D 预积分表；并把不透明度映射从"线性跟随物理量"升级为可编辑的曲线/控制点。
3. **min/max 金字塔生成（compute 归约）** 与 **空域跳过**：性能提升的主要来源。
4. **砖块化与 LOD**：为超显存数据和交互期降采样做准备。
5. **统一的体渲染合成 pass 接口**：让规则体与非结构两条路线输出同一套（颜色、不透明度、深度）中间结果，便于复用现有 resolve/blit 流程。

### 7.4 易踩的坑

- 合成顺序混用（front-to-back 与 back-to-front 混写）会导致"看起来对、调参全乱"。
- 不做预积分传输函数，大步长下会出现明显色偏；这是"看起来糊"的最常见原因之一。
- 3D 纹理的 z 向连续性与采样的空间局部性对 cache 影响极大；砖块化时考虑 Z-order/Morton 排布。
- 梯度用中心差分时，纹理边界必须 clamp，否则外壳出现黑边。
- `imageLoad`/`imageStore` 之间必须正确设置 `glMemoryBarrier` 的 barrier bit，依赖关系要画清楚。
- 不要沿用 `MAX_FRAMEBUFFER_WIDTH = 8192` 为 head pointer 分配纹理；新管线应按视口实际尺寸分配，并支持半分辨率交互。
- 64 位原子操作（`GL_ARB_gpu_shader_int64`）支持不普遍，砖块分配/压缩体解码时优先用 32 位方案。
- 每帧 `glGetError()` 轮询（本项目 `GLCheckError()`）在热路径上有开销，新管线只在调试开关下启用。

---

## 8. 建议的落地节奏

| 阶段 | 目标 | 产出与验收 |
|---|---|---|
| 0（约 1 周） | 最小闭环：`GLTexture3d` + 1D 传输函数 + 全屏 compute ray marching（小结构体） | 与 VTK/OSPRay 参考图逐像素比对，误差可解释 |
| 1（1–2 周） | 空域跳过（min/max 金字塔）+ 早停 + 自适应步长 | 与现有 1024 限制基线对比质量/帧率；给出采样步长—质量曲线 |
| 2（2–4 周） | 大数据：砖块稀疏缓存 + 异步上传（PBO / persistent map）+ LOD + out-of-core | 512³/1024³ 数据的显存占用与帧时间数据 |
| 3（持续） | 质量：预积分传输函数、梯度光照、AO/阴影，必要时蒙特卡洛 + 去噪 | 视觉质量对标截图 + 性能预算 |
| 4 | 非结构网格（四面体光线求交或 cell projection），与规则体路线统一接口 | 直接吃原始 VTK/CGNS 网格，无需重采样 |

---

## 9. 两周阅读顺序（推荐）

| 时间 | 内容 |
|---|---|
| 第 1–2 天 | *Real-Time Volume Graphics* 光学模型章节 + Max 1995 → 统一合成公式 |
| 第 3–4 天 | Roettger 2003 / Stegmaier 2005 + GPU Gems 第 39 章（Volume Rendering Techniques） |
| 第 5–6 天 | Krüger & Westermann 2003 → 空域跳过与早停；随后通读 VTK GPU volume mapper 的着色器组织 |
| 第 7 天 | Khronos Wiki 的 compute / image load-store / SSBO 三页 + 精读本仓库 `MeshletCull.comp` 作为模板 |
| 第 8–10 天 | 阶段 0 最小闭环实现 |
| 第 11–14 天 | 预积分传输函数 + min/max 金字塔，配合 RenderDoc/Nsight 做第一次性能剖析 |

---

## 10. 参考链接（入口页）

- Khronos OpenGL Wiki：`https://registry.khronos.org/OpenGL/wiki/`
- NVIDIA GPU Gems 系列：`https://developer.nvidia.com/gpugems`
- VTK：`https://vtk.org/` ・ParaView：`https://www.paraview.org/`
- OSPRay：`https://www.ospray.org/`
- OpenVDB / NanoVDB：`https://github.com/AcademySoftwareFoundation/openvdb`
- Open SciVis Datasets：`https://klacansky.com/open-scivis-datasets/`
- Tuvok / ImageVis3D：`https://github.com/SCIInstitute/Tuvok`、`https://github.com/SCIInstitute/ImageVis3D`
- Filament：`https://github.com/google/filament` ・Wicked Engine：`https://github.com/turanszkij/WickedEngine`
- GVDB-Voxels：`https://github.com/NVIDIA/gvdb-voxels`
- ANARI：`https://github.com/KhronosGroup/ANARI-SDK`
- RenderDoc：`https://renderdoc.org/`

