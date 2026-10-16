# 标准过滤器迁移记录

迁移日期：2026-10-13。目标分支：`multifilter`。

源代码：`E:\igamevis\lianshou\editOpeniGame-main`，对应仓库 https://github.com/dayuwan77/igamevis 。以该本地目录中的实现为准。

## 菜单范围

只迁移“算法处理 → 标准过滤器”的入口和接线，不迁移开发菜单或测试菜单的重复入口。

35 个新增入口：

```text
rename_arrays, append_arc_length, surface_vectors, perlin_noise,
outline, reverse_sense, warp_by_scalar, warp_by_vector,
texture_map_to_plane, texture_map_to_cylinder, texture_map_to_sphere,
decimate_polyline, histogram, compute_quartiles, integrate_variables,
append_attributes, compute_connected_surface_properties, generate_surface_tangents,
connectivity, extract_selection, tube, ribbon, smooth, subdivide,
linear_extrusion, tensor_principal_invariants, yield_criteria, group_datasets,
extract_block, extract_time_steps, temporal_shift_scale, extract_enclosed_points,
resample_with_dataset, median, temporal_statistics
```

标准菜单中的 `append_reduce` 改为 `append_datasets（附加数据集）`；QAction 标识改为 `action_filter_append_datasets`，data 改为 `append_datasets`。该入口按需求仍处于算法未接入状态。

## 完整功能

以下 10 项在源仓库中均存在实际算法实现，已迁移并接到标准菜单。

| Filter | 实现位置（相对于 iGameCore/Filters） | 参数界面 | 测试 |
| --- | --- | --- | --- |
| validate_cells | ValidateCells/iGameValidateCellsFilter | 直接执行，显示 ValidityState 和无效单元 | 源示例 + 自动混合单元校验 |
| extract_cells_by_type | ExtractCellsByType/iGameExtractCellsByTypeFilter | igQtExtractCellsByTypeWidget，C++ 创建 UI | 源无渲染回归 |
| extract_location | DataProcessing/ExtractLocation | igQtExtractLocationWidget，C++ 创建 UI | 源位置查询及拖点轴约束回归 |
| outline | Outline/iGameOutlineFilter | 无参数，独立包围盒结果 | 源 --check 回归 + 标准菜单执行 |
| reverse_sense | ReverseSense/iGameReverseSenseFilter | 通用参数面板 | 源自检和渲染示例 |
| warp_by_scalar | WarpByScalar/iGameWarpByScalarFilter | 通用参数面板 | 源示例 + 自动位移数值校验 |
| append_attributes | AppendAttributes/iGameAppendAttributesFilter | 输入模型选择窗口 | 源示例 + 自动属性与失败边界校验 |
| tube | Tube/iGameTube | 通用参数面板 | 源示例 + 自动开口/封端拓扑校验 |
| subdivide | Subdivide/iGameSubdivide | 通用参数面板 | 源示例 + 自动共享边及多代 CellData 继承回归 |
| linear_extrusion | Modeling/iGameLinearExtrusionFilter | igQtLinearExtrusionWidget + LinearExtrusionWidget.ui | 源几何/属性/失败边界回归 + 标准菜单 Apply |

迁移资产包括 21 个算法/支持文件、3 组 Qt widget 头文件和实现、1 个 Designer UI、12 个源测试程序、14 个 Examples/Models 模型，以及 tube_complex 和 subdivide_cell_data 中的 17 个辅助模型/说明/生成文件。

另补充 TestMigratedStandardFilters.cpp，扩展已有 StandardFiltersMenuValidation.cpp。

为完整保留 Extract Location 的交互，迁移 Interactor/SingleDragStyle 中的轴约束、单点拾取和坐标更新逻辑。为 Reverse Sense 的标准参数界面，迁移通用参数面板的可选纵向布局功能，原有面板默认保持两列布局。

算法文件已逐字节核对源仓库一致；专用 widget/UI 保留源实现，Linear Extrusion 额外用 QPointer 保护结果删除回调，避免结果对象晚于面板销毁时访问失效面板。7 个原本为 Git LFS 指针的模型均已恢复为真实数据，其中 2 个来自本地缓存，5 个从源仓库下载并校验 SHA-256。

## 构建与验证

构建目录：`out/build/codex-multifilter`。新增接线放在 igQtMainWindowMigratedFilters.cpp 中，由现有 MainWindow 标准菜单分派。

自动算法回归命令：

```powershell
ctest --test-dir out/build/codex-multifilter/Examples -L migrated-standard-filters --output-on-failure
```

标准菜单回归命令：

```powershell
ctest --test-dir out/build/codex-multifilter/Examples -R '^testStandardFiltersMenu$' --output-on-failure
```

`iGameVis` 和全部 12 个源示例、新增组合回归程序、标准菜单测试均已构建成功。

最终验证命令：

```powershell
ctest --test-dir out/build/codex-multifilter/Examples -R '^testStandardFiltersMenu$|^testMigrated' --output-on-failure
```

结果：9/9 通过。包含 7 组算法回归、标准菜单回归，以及组合回归程序的原有 smoke 注册。标准菜单测试验证了全部新增入口和更名标识、已有面板复用与重复 Apply、Outline 的结果几何、Linear Extrusion 的实际 Apply。

已有菜单测试按旧版 QDockWidget 查找面板，已适配当前分支的左侧 Tab/QWidget 结构。验证时发现旧构建缓存的 MSVC 依赖前缀乱码，导致旧对象文件未随头文件变化重编；完整重编 Qt 模块及核心库后，重复符号和运行期堆损坏均消失。

未进行可见窗口中的人工视觉验收；原有渲染示例保留，供人工验证。第一项中其余新增入口按要求只创建菜单，未扩大到完整算法迁移。

根目录 CMakeLists.txt 中用户原有未提交的构建选项修改已保留。本次集成提交主题：`feat: integrate additional standard filters`；未推送。
