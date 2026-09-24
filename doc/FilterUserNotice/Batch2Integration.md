# 第二批 Filter 集成

源仓库：[dayuwan77/igamevis](https://github.com/dayuwan77/igamevis)，
来源提交：`eccac729b57aeacbe9312d7d5189f6990bb4eebd`（main）。
选取指定的算法、参数面板、示例和模型，接入 `iGameVis-multiFilter` 的现有菜单。

## 入口与配套资源

统一入口：菜单栏 → 算法处理 / Filters → 标准过滤器（Standard Filters）
→ Alphabetical（按名称），也可从对应功能分类进入。
加载模型并在模型树中选中它，再选择下列 action。参数面板执行后会在模型树中添加独立结果。
表中的模型均位于 `Examples/Models/`，示例源文件位于 `Examples/Filter/`。

| Action | 示例可执行目标 | 主要模型 |
| --- | --- | --- |
| `cell_size` | `testCellSizeExtraction` | `CellSize_MixedTypes.vtk`、`CellSize_TetraCube.vtk` |
| `count_cell_vertices` | `testCountCellVertices` | `CountCellVertices_mixed_cells.vtk`、`CountCellVertices_empty.vtk` |
| `extract_edges` | `testExtractEdges` | `ExtractEdges_hexa_grid.vtk`、`ExtractEdges_multicomp_cell_data.vtk`、`ExtractEdges_tri_cell_data.vtu` |
| `feature_edges_region_ids` | `testFeatureEdgeRegion` | `FeatureRegion_MountingPlate.vtk` |
| `global_point_and_cell_ids` | `testGenerateGlobalIds` | `GlobalIdsTestModel.vtk` |
| `point_and_cell_ids` | `testPointAndCellIds` | `ClipTest_Plane_UnstructuredGrid.vtk` |
| `process_ids` | `testGenerateProcessIds` | `GenerateProcessIds_SteppedPipe.vtk`、`GenerateProcessIds_VenturiTube.vtk` |
| `reflect` / `axis_aligned_reflection` | `testAxisAlignedReflection` | `Quad_Bicycle.vtk`、`ClipTest_Plane_UnstructuredGrid.vtk` |
| `extract_component` | `testExtractComponent` | `ExtractComponent_FlowPipe.vtk`、`ExtractComponent_BendPipe.vtk` |
| `merge_vector_components` | `testMergeVectorComponents` | `MergeVectorComponents_Quad_Plane.vtk`、`MergeVectorComponents_Tri_Plane.vtk` |
| `resample_to_image` | `testResampleToImage` | `ResampleCube.vtk`、`ResampleCubeVector.vtk` |
| `resample_to_line` | `testResampleToLine` | `Resampletoline_test.vtk`、`ResampletolineTest_Plane_UnstructuredGrid.vtk` |
| `triangle_strips` | `testTriangleStrip`、`testTriangleStripWidget` | `TriangleStripTestModel.vtk`、`SurfaceNormalsFilter_test.vtk` |
| `point_set_to_octree_image` | `testPointSetToOctree` | `OctreePoints.vtk` |

`feature_edges_region_ids` 使用 SurfaceMesh 输入；若导入的是非结构网格，先使用“表面提取”。
`reflect` 与 `axis_aligned_reflection` 共用同一反射参数面板。
全局 ID 的进程偏移由面板参数指定；本次没有引入 MPI 运行时。

## 构建与验证

沿用项目的 CMake 构建；启用 `ENABLE_QT_MODULE=ON` 和 `EXAMPLE_COMPILE=ON`。
所有新增核心和 Qt 文件由既有 `CONFIGURE_DEPENDS` 规则发现，示例已登记到 `Examples/CMakeLists.txt`。
模型会复制到 Examples 构建目录的 `Models/`。交互示例从该目录运行，无需修改模型绝对路径。

```powershell
cmake --build <build-dir> --target iGameVis testCellSizeExtraction testCountCellVertices testExtractEdges testFeatureEdgeRegion testGenerateGlobalIds testPointAndCellIds testGenerateProcessIds testAxisAlignedReflection testExtractComponent testMergeVectorComponents testResampleToImage testResampleToLine testTriangleStrip testTriangleStripWidget testPointSetToOctree testResampleOctreeChecks testBatch2GeometryValidation testStandardFiltersMenu
ctest --test-dir <build-dir>/Examples -L batch2-filters --output-on-failure
```

CTest 自动给可视化示例传入 `--no-render`，Qt 检查使用 `offscreen`，并要求正常退出。
直接运行示例仍保留其默认的可视化模式。
`testResampleOctreeChecks` 覆盖插值、有效点、离散数组和八叉树统计；
`testBatch2GeometryValidation` 覆盖单元尺寸、混合单元拓扑、线性场采样、区域边界和图像无效单元表面；
`testStandardFiltersMenu` 检查全部入口、反射别名、参数面板复用和重复执行的模型树更新。
菜单检查使用 CPU 场景验证信号连接，不替代有 OpenGL 上下文的人工显示检查。

### 本次验证记录（2026-09-24）

- Windows x64 / MSVC 19.39 / Qt 5.14.2 / Release，构建目录 `out/build/filter-integration`。
- `iGameVis` 和本页全部示例、检查目标编译通过。
- `batch2-filters`：18/18 通过；既有 `testModelGeometryParallelValidation`：1/1 通过。
- 此次构建启用 Qt 和 Examples，关闭 CUDA、LibTorch、CGNS、Nastran 和 Abaqus 可选模块。
- 尚未进行逐项 OpenGL 画面的人工验收；Qt 菜单与面板连接由 offscreen 自动检查覆盖。

## 本分支适配

- 沿用现有标准菜单 action，新增参数面板按需创建、复用；结果信号接入模型树。
- 导入边导出和沿线采样交互依赖，补上 `LineSelection` 及交互器入口。
- 图像重采样的 `vtkGhostType` 隐藏单元参与结构网格表面提取，保留目标分支已有的其他网格提取逻辑。
- 补齐 CellSize 非结构网格输出的点坐标；在 CellSize 和 MergeVectorComponents 深拷贝前清空单元偏移，保持混合类型单元连接关系。
- 特征区域过滤器校验边 ID 数组，移除重复深拷贝，失败时清空旧输出。
- 示例增加无窗口检查方式，统一模型相对路径，并补齐 Qt 静态链接配置。

各项原始使用说明随代码一并保留；其中旧菜单截图与旧入口名称以本页所列入口为准。
