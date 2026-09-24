# 第一批标准 Filter 示例与测试

对应算法集成提交：`e7ec6571`（`Integrate additional standard filters`）。
补齐示例来源：`https://github.com/dayuwan77/igamevis`，版本
`eccac729b57aeacbe9312d7d5189f6990bb4eebd`，沿用源仓库同名文件路径。
保留当前分支较新的 Elevation 示例，以及 `b42e58d2` 引入的 ExtractSubset 示例。

## 对应关系

下表源码路径相对于 `Examples/Filter/`，模型路径相对于 `Examples/Models/`。

| Filter | 构建目标 | 示例源码 | 模型 / 检查内容 |
|---|---|---|---|
| coordinates | testPointCoordinates | PointCoordinates/TestPointCoordinates.cpp | 内部构造数据；空输入、坐标数组、独立输出、重名、重复执行及混合单元连接关系 |
| cell_centers | testCellCenterFilter | MyFilter/TestCellCenterFilter.cpp | CellCenter_hexa_grid.vtk；中心坐标、属性长度和 double 类型 |
| random_vectors | testRandomVectors | AttributeManipulation/TestRandomVectors.cpp | CellCenter_hexa_grid.vtk、CellCenter_surface_mixed.vtk；向量模长、独立输出和单元连接关系 |
| remove_ghost_information | testRemoveGhostInformation | RemoveGhostInformation/TestRemoveGhostInformation.cpp | 内部构造数据；9 项检查，包含点重映射、隐藏单元、属性类型及 64 位精度 |
| elevation | testElevation | Elevation/TestElevation.cpp | 现有 12 项检查；ElevationSlopeTerrain.vtk、ElevationTerraces.vtk |
| mask / mask_points | testMaskPoints | MaskPoints/TestMaskPoints.cpp | 内部构造数据；18 项采样、属性和边界检查 |
| feature_edges | testFeatureEdges | FeatureExtraction/FeatureEdges.cpp | mazewheel.obj；检查边数量、Edge Types 和 Edge Ids |
| feature_edges | testFeatureEdgesVisualization | FeatureExtraction/FeatureEdgesVisualization.cpp | mazewheel.obj；检查输出，默认显示输入表面和特征边 |
| extract_subset | testExtractSubset | TestExtractSubset.cpp | 现有 Structured_Volume_Test.vtk；提取 18 点、4 个六面体，保留中间层的非均匀坐标 |
| outline_corners | testOutlineCorners | FeatureExtraction/OutlineCorners.cpp | 内置包围盒或外部模型；检查 32 点、24 条线段 |
| probe / probe_location | testProbe | Probe/TestProbe.cpp | AIGen_Hex_PipeSegment.vtk；10 个固定种子的查询点，校验有效标记、属性长度和数值有限性 |
| convert_to_vertex | testConvertToVertex | Convert/TestConvertToVertex.cpp | AIGen_Tet_TwistedRod.vtk；每个输入点对应一个顶点单元，坐标保持一致 |

本次新增 10 个示例源码、5 个模型文件，登记全部 12 个目标。
`testRandomVectorsMixedCells` 复用随机向量可执行文件检查三角形与四边形混合输入，
因此 `batch1-filters` 标签共包含 13 项 CTest。

## 构建与运行

配置项目时启用 `EXAMPLE_COMPILE=ON`，然后运行：

```powershell
cmake --build <build-dir> --target testPointCoordinates testCellCenterFilter testRandomVectors testRemoveGhostInformation testElevation testMaskPoints testFeatureEdges testFeatureEdgesVisualization testExtractSubset testOutlineCorners testProbe testConvertToVertex
ctest --test-dir <build-dir>/Examples -L batch1-filters --output-on-failure
```

模型和运行库沿用项目的 Examples 部署规则。VTK 模型由 Git LFS 管理，
新检出仓库时需要取得实际模型内容。

手动运行时先进入 `<build-dir>/Examples`。常用命令：

```powershell
.\testCellCenterFilter.exe
.\testRandomVectors.exe Models/CellCenter_hexa_grid.vtk
.\testFeatureEdges.exe Models/mazewheel.obj
.\testFeatureEdgesVisualization.exe Models/mazewheel.obj
.\testExtractSubset.exe
.\testConvertToVertex.exe
.\testOutlineCorners.exe
```

CellCenter、FeatureEdgesVisualization、ExtractSubset、ConvertToVertex 默认打开渲染窗口；
命令末尾增加 `--no-render` 可只运行输出检查。
CTest 自动传入模型参数和 `--no-render`，并要求程序正常退出；超时或非零退出都视为失败。
自动检查不包含 OpenGL 画面的人工验收。

## 本分支适配与回归修复

- 坐标示例适配本分支的独立输出接口，检查原输入和此前生成的输出不被后续执行修改。
- 混合单元回归暴露了 PointCoordinates 和 RandomVectors 拷贝时的偏移错误：
  本分支 `CellArray::DeepCopy` 会追加偏移，构造时已有的零偏移使可变大小单元发生错位。
  两个过滤器在复制前清空目标 CellArray，保留单元大小和连接关系。
- ExtractSubset 原示例未登记 CMake，且读取、执行失败返回成功；本次补齐注册、错误退出码和输出检查。
- ConvertToVertex 同样补上错误退出码和顶点单元检查。
- Probe 原示例在全部查询无效时仍会输出 PASS；本次检查固定查询点确实有效，且属性维度及数值合法。

提交主题：`test: add examples for first-batch standard filters`。
测试源码中的来源与回归说明保留了查询本次提交的命令。

## 验证记录（2026-09-24）

- Windows x64 / MSVC 19.39 / Release：主程序和全部 12 个示例目标编译通过。
- `batch1-filters`：13/13 通过。
- 模型缺失检查：ExtractSubset、ConvertToVertex 均返回错误码 1。
- 本次使用无窗口检查，未逐项人工验收 OpenGL 显示效果。
