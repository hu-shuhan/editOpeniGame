# Examples 与指标模块对照

完整指标文档见 [doc/modules/README.md](../doc/modules/README.md)。

| 指标 | 示例入口 |
|------|----------|
| 7.1 高阶可视化 | `Convert/TestConvertToLagrangeUnstructuredMesh.cpp`，`IO/SplineReaderCPU.cpp` |
| 10.1 智能可视分析 | `Filter/VisualizationData/TestParallelCoordinatesData.cpp` 等 |
| 10.2 特征提取 | `Filter/FeatureExtraction/*Extraction.cpp` |
| 10.3 物理场特征交互 | `MultiscaleInteraction/TestMultiscaleInteraction.cpp` |
| 11.2 vtk/CGNS | `IO/CGNSReader.cpp`（需 `ENABLE_CGNS_MODULE`） |
| 11.3 场可视化 | `Rendering/SetScalarField.cpp`，`Filter/Vector/TestStreamline.cpp`，`Animation/SaveAnimation.cpp` 等 |
| 11.4 并行可视化 | `Rendering/MeshletRendering.cpp`，`Rendering/SetRenderingPressure.cpp` |

编译示例需 `EXAMPLE_COMPILE=ON`。
