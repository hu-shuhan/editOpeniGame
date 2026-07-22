# Examples 模块索引

**使用与验收请先看：[HOW_TO_RUN.md](./HOW_TO_RUN.md)**（编译、运行、验收用例、一览表、模块依赖、FAQ）。

### IDE 操作入口（详细步骤在 HOW_TO_RUN）

| IDE | 文档章节 | 要点 |
|-----|----------|------|
| **CLion** | [§1.1 CLion 上手](./HOW_TO_RUN.md#11-clion-上手推荐先这样试) | 另开窗口打开 `Examples`；Release；设 `iGameCore_DIR`；Working directory = 构建目录 |
| **Visual Studio** | [§1.2 Visual Studio 上手](./HOW_TO_RUN.md#12-visual-studio-上手cmake-打开文件夹) | 「打开文件夹」选 `Examples`；Release；设 `iGameCore_DIR`；工作目录用构建根目录（有 `Models/`），不是 `Release\` 子目录 |

共同前提：主工程先 **Release + Install**，存在 `…/install/lib/cmake/iGameCore/`。根工程保持 `EXAMPLE_COMPILE=OFF`。

建议先验收：`testLosslessEncode`（需参数 `.\Models\StreamTest.vtk`）、`testSetViewStyle`、`testCGNS`（见 HOW_TO_RUN 第 4 节）。

---

张量场可视化模块：`Filter/Tensor/TestTensorView.cpp`

时变流场可视化模块：`Filter/Vector/`

动画输出可视化功能模块：`Animation/SaveAnimation.cpp`

并行可视化于 GPU 调度模块：`Rendering/MeshletRendering.cpp`

多尺度物理场特征可视交互模块：`Filter/Vector/TestStreamline.cpp`

体绘制与面绘制可视化模块：`IO/SplineReaderCPU.cpp`、`IO/SplineReaderGPU.cpp`

GPU 调度与渲染优化功能模块：`Rendering/SetRenderingPressure.cpp`（交互时提供简化网格）
