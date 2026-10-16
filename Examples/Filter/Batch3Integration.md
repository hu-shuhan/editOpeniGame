# 第三批过滤器集成

范围为用户确认的 **17 组、20 个菜单入口**。不包含 `append_reduce`。
原清单中的 `TestAppendReduce.cpp` 等路径存在错位，以下按示例实际调用的类重新对应。

来源：[dayuwan77/igamevis](https://github.com/dayuwan77/igamevis)，提交
`fdafcbb9f54870dd0b51894a65423efa0fe9483c`。
目标分支为 `iGameVis-multiFilter`，集成前已合并 `origin/main` 的 `ac81d7ca`，
合并提交为 `f94a2903`。仅迁入表中算法及必要依赖，未整体覆盖源仓库的主窗口。

示例路径相对于 `Examples/Filter`，模型路径相对于 `Examples/Models`。

| Filter 组 | 示例文件 / CMake 目标 | 模型 |
| --- | --- | --- |
| angular_periodic | `Periodic/TestAngularPeriodic.cpp` / `testAngularPeriodic`；`Periodic/TestAngularPeriodicSelfCheck.cpp` / `testAngularPeriodicSelfCheck` | `AIGen_Surface_RingSector.obj`；另有内存构造的混合、多面体及旋转轴用例 |
| append_location_attributes | `TestAppendLocationAttribute.cpp` / `testAppendLocationAttributeFilter` | `Test_Append_Location_Attribute.vtk` |
| boundary_mesh_quality | `TestBoundaryMeshQuality.cpp` / `testBoundaryMeshQuality` | `Boundary_Mesh_Quality_Test.vtk`（源仓库现已提供） |
| clean_to_grid / clean_poly_data / clean_cells_to_grid | `TestCleanToGridFilter.cpp` / `testCleanToGridFilter` | `Convert_Quad_Bicycle.vtk` |
| count_cell_faces | `FeatureExtraction/CountCellFaces.cpp` / `testCountCellFaces`；`FeatureExtraction/CountCellFacesOutput.cpp` / `testCountCellFacesOutput` | `CountCellFaces_MixedCells.vtk`、`CountCellFaces_QuadTensor.vtk` |
| feature_edges_region_ids | `FeatureExtraction/FeatureEdgeRegion.cpp` / `testFeatureEdgeRegion` | `FeatureRegion_MountingPlate.vtk` |
| generate_ids | `TestThresholdAndGenerateIds.cpp` / `testThresholdAndGenerateIds` | `GenerateIdsTestData.vtk`、`GenerateIdsMixedCells.vtk` |
| mesh_quality | `MeshQuality/TestMeshQuality.cpp` / `testMeshQuality` | `MeshQuality_Complex.vtk` |
| point_line_interpolator | `PointLineInterpolator/TestPointLineInterpolator.cpp` / `testPointLineInterpolator` | `PointLineInterpolatorFilter_Test.vtk` |
| point_set_to_octree_image | `Convert/TestPointSetToOctree.cpp` / `testPointSetToOctree`；`Convert/TestResampleOctreeChecks.cpp` / `testResampleOctreeChecks` | `OctreePoints.vtk`；复用 `ResampleCubeVector.vtk` |
| point_volume_interpolator | `Interpolation/TestPointVolumeInterpolator.cpp` / `testPointVolumeInterpolator`；`Interpolation/TestPointVolumeInterpolatorSelfCheck.cpp` / `testPointVolumeInterpolatorSelfCheck` | `AIGen_Points_ScatterCloud.vtk`、`AIGen_Points_VertexCloud.vtk` |
| probe / probe_location | 复用 `Probe/TestProbe.cpp` / `testProbe`，保留目标仓库更严格的断言 | 复用 `AIGen_Hex_PipeSegment.vtk` |
| shrink | `Shrink/TestShrink.cpp` / `testShrink`；`Shrink/TestShrinkModel.cpp` / `testShrinkModel` | `Shrink_Cube.vtk`、`Shrink_TwoTets.vtk` |
| surface_normals | `SurfaceNormals/TestSurfaceNormalsFilter.cpp` / `testSurfaceNormalsFilter` | `SurfaceNormalsFilter_pyramid_roof.vtk` |
| threshold | `TestThresholdAndGenerateIds.cpp` / `testThresholdAndGenerateIds` | `ThresholdScalarField.vtk`、`ThresholdVolumeData.vtk` |
| Volume Mesh Simplification | `TestVolumeMeshSimplification.cpp` / `testVolumeMeshSimplification` | `VolumeSimplification_FlangedTube.vtk` |
| Mesh Tetrahedralize | `TestMeshTetrahedralize.cpp` / `testMeshTetrahedralize` | `VolumeSimplification_FlangedTube.vtk` |

## 菜单使用

从菜单“标准过滤器”中选择对应项，在参数面板点击“应用”，结果作为独立模型加入模型树。
四面体化和体网格简化沿用“Data Processing（数据处理）”分类。
面板显示并保留打开时的输入；应用后切换当前模型，不会使再次应用变成对上一次结果叠加处理。
要更换输入，选择另一个模型后重新打开菜单。

- `append_location_attributes` 添加点坐标 `LocationAttribute` 和单元中心 `CellCenter`。
- 三个清理入口使用源仓库的 `CleanToGridFilter`。`clean_poly_data` 先提取表面、清理后输出表面；`clean_cells_to_grid` 默认关闭合并点，可调整清理开关。
- 特征区域和法向量入口支持先从体网格提取表面。
- 体网格简化先执行四面体化，再执行边坍缩；启用保留边界时，目标数量是期望值，约束可能阻止继续简化。
- 八叉树图像按源实现输出规则 `StructuredMesh`，`octree` 单元属性记录八分区占用位；不是独立的树形数据对象。
- 点体积插值使用全部点属性，提供采样范围、分辨率、插值核、邻域和空值参数。
- `probe` 和 `probe_location` 继续使用此前已经接入的球形采样和单点采样入口。

## 验证

配置时启用 `EXAMPLE_COMPILE=ON` 和 `ENABLE_QT_MODULE=ON`。
构建上述目标、`testThirdBatchData`、`testThirdBatchMenu` 及 `iGameVis` 后运行：

```text
ctest --test-dir <build>/Examples -L batch3-filters --output-on-failure
ctest --test-dir <build>/Examples -R "^testProbe$" --output-on-failure
```

第三批标签包含 20 个源示例和 2 个集成回归测试。
CTest 设置 `IGAME_EXAMPLE_NO_RENDER=1` 跳过可视化窗口，保留算法执行和数值断言。
直接运行可视化示例则仍会打开窗口；模型和渲染资源由 CMake 复制到示例运行目录。
菜单测试使用 Qt offscreen 和 CPU 场景，验证实际 action、参数应用、模型插入、别名和重复操作；它不代替真实 GPU 的视觉检查。

`ThirdBatchDataValidation.cpp` 覆盖混合单元拷贝：修复 `CellArray::DeepCopy` 重复起始偏移造成的拓扑损坏，
并校验位置、单元中心、ID、面积、收缩及单位法向量。
它还使用 `Batch3MixedLinesAndFaces.vtk` 验证 VTK 读取器保留混合面的首偏移，
并让 `LINES` 和 `POLYGONS` 各自持有独立拓扑；收缩测试验证 64 位 ID 精度和点属性范围。

2026-09-28 验证结果：Windows / MSVC 19.39 / Qt 5.14.2 / Release，`iGameVis` 和全部测试目标编译成功。
使用全新目录 `out/build/batch3-integration` 执行三批回归：**51/51 通过**，
其中第一批 13 项、第二批 16 项、第三批 22 项，总耗时约 80 秒。
验证配置关闭了 GPSCUDA、LibTorch、CGNS、Nastran 和 AbqSDK 可选模块。
