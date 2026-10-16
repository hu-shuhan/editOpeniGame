# 动画 Filter 字段描述

修复日期：2026-09-28；修复和测试同次提交，提交主题：`修复动画 Filter 字段传递与播放颜色映射`。

查询该提交：`git log --diff-filter=A --format="%h %s" -- Examples/Animation/FieldInfo/Validation.cpp`。

原问题：参数窗口直接读取原始模型。即使前面添加了“单元数据转点数据”，后面的等值体仍因没有点字段而无法配置；保存参数时也使用原始模型校验。

现在的操作：添加“单元数据转点数据”→ 添加“等值体”→ 打开等值体参数，选择转换后的点字段并填写上下限 → 播放。配置阶段不运行转换，不修改源数据，真实算法仍逐帧执行。

## 接口约定

`igQtAnimationFieldInfo` 保存字段名称、Point/Cell 归属、分量数和属性类型。`igQtAnimationDataInfo` 保存网格类型、几何能力和各子块描述，不持有数组或网格，也不扫描数值范围。

`igQtAnimationFilterDescriptor` 增加两个可选回调：

```cpp
bool parameterSchemaFromInfo(const igQtAnimationDataInfo& input,
                             igQtAnimationFilterParameterSchema& schema,
                             QString& error);

bool describeOutput(const igQtAnimationDataInfo& input,
                    const QVariantMap& parameters,
                    igQtAnimationDataInfo& output,
                    QString& error);
```

- `parameterSchemaFromInfo` 根据上游描述提供选项；此时当前节点的参数可以尚未填写。
- `describeOutput` 校验本节点参数及输入字段后，描述实际算法将输出的字段。可以随参数改变描述。
- 回调不得调用 `execute`、读取帧文件、修改真实数据或执行 OpenGL 操作。
- 旧的真实数据 `supports/validateParameters/execute` 保留，用于播放时验证实际帧。描述不是每个时间步都必然成功的保证。
- 未实现描述接口的 Filter 返回明确错误，不猜测其输出、不自动执行算法。旧注册/执行接口仍可使用。

`igQtDescribeAnimationPipelineInput` 从源描述开始推导指定行之前的所有节点。参数窗口打开和保存都使用这份上游描述；没有假造一个带点字段的 DataObject。

## 内置规则与界面

- 转换点数据：保留点字段，将单元字段改为同名同分量的点字段；转换后点字段名称冲突时拒绝配置。
- 等值面/等值体：要求选定点字段及分量有效；按实际算法保留 Point/Cell 属性，输出网格类型为 UnstructuredMesh。空输出及数值范围仍由运行时处理。
- 多块：逐块推导，参数下拉框只列出所有直接子块共有且类型、分量一致的点字段。嵌套块尚不受现有等值提取适配器支持，会明确报错。
- 精确转换后范围无法只靠描述得到，因此等值/上下限没有虚构默认值，需要填写。
- 添加、删除、移动、保存参数及重新初始化模型时更新下游配置状态。参数失效显示“需配置”或“上游未就绪”。
- 原有字段消失时保留原选择并要求修正，不悄悄切换成另一个字段。流程改变后，旧窗口不能保存到移动后的行。

## 分步验证

1. 先实现描述接口和内置规则，通过 `Animation.FieldInfo.Descriptions` 后接入界面。
2. 接入参数窗口和 Apply 校验，通过 `Animation.FieldInfo.ParameterDialog`。使用真实 PVD/VTU，在配置完成前检查源字段仍为 Cell；之后播放两帧验证等值体字段范围与几何边界随帧变化。
3. 运行 `Animation.FieldInfo.LegacyPlayback`，复用原有点数据动画兼容性测试；完整构建 `iGameVis`。

覆盖：标量/向量、源数组和范围不被修改、同名冲突、多块字段缺失、未知适配器不执行、无效分量/阈值、删除/重排恢复、旧窗口失效、源字段改名，以及两帧实际执行。

```powershell
cmake -S . -B out/build/x64-release-cgns -DIGAME_BUILD_ANIMATION_INFO_TESTS=ON
cmake --build out/build/x64-release-cgns --target iGameVis testAnimationFieldInfo testAnimationFieldInfoLegacy --parallel 6
ctest --test-dir out/build/x64-release-cgns -R '^Animation.FieldInfo.' --output-on-failure --stop-on-failure
```

Windows 需初始化 MSVC，并将 Qt、HDF5、FFmpeg 运行库目录加入 PATH。集成测试使用真实 OpenGL 4.6 上下文。

构建及测试日志位于 `out/build/x64-release-cgns/field-info-final-build.log` 和 `field-info-final-regression.log`。

上述首次提交修改字段描述和参数配置。后续最终输出缓存改造见下文。

## 真实 Density 数据验收

数据目录：`D:/vtk_files/vtk_files`。打开其中的 `Result.pvd`，它引用 100 个 VTU 帧，时间索引为 0～99；不要将同目录的 VTK 和 VTU 混合导入。每帧包含 157,034 个点和 682,709 个单元，`Density` 是单元标量，原始点字段为空。

界面步骤：进入动画输出，使用原始时间模式；依次添加“单元数据转点数据”和“等值体（IsoVolume）”；打开等值体参数，确认点字段包含 `Density`，分量选 0，下限填 `0.2`、上限填 `1.0`，应用后播放。字段可选项应在首次播放前就出现，源数据仍保持单元字段。

第一帧 Density 全为约 0.3，因此先用 0.2～1.0 验收。若改为 0.5～1.0，第一帧会产生空结果；现有运行时会将空等值体报告为失败。

设置上述 Windows 运行库环境后，可执行：

```powershell
& ./out/build/x64-release-cgns/testAnimationFieldInfo.exe dataset 'D:/vtk_files/vtk_files'
```

该测试抽取第 1、50、100 帧，经实际文件读取、字段描述传递、点属性转换和等值体提取，并检查输出非空、输出标量位于设定区间。2026-09-28 三帧全部通过；输出单元数分别为 682,709、303,616、302,109。此项为实际数据算法测试，不代表已完成全部 100 帧的界面播放验收。日志：`out/build/x64-release-cgns/field-info-real-dataset.log`。

## 播放颜色映射回归

修复日期：2026-09-28；与上述字段描述修复同次提交。仅有一个 Density 字段时，播放代码先直接设置字段索引，后续 `ViewCloudPicture` 将其判为未变化，跳过启用颜色及子对象绑定；之前的几何验证无法发现这个显示问题。

现在通过 `ViewCloudPicture` 应用 Filter 指定的字段和分量，向输出子块共享源模型的颜色映射，并保留插值播放的分量选择。`Animation.FieldInfo.ParameterDialog` 检查两帧及插值帧的实际绘制对象着色开关、字段/分量和共享颜色映射，再比较同一帧的着色截图与单色截图，确保颜色生效。截图为构建目录下的 `animation-color-first.png`、`animation-color-second.png` 和 `animation-color-interpolated.png`。

## Pipeline 最终输出缓存（待提交）

2026-09-28 按用户要求，将动画窗口的缓存移到整个 Pipeline 之后：

```text
请求输出帧 → 查最终输出缓存
  命中 → 应用当前显示设置 → 显示/导出
  未命中 → 独立读取源帧或插值 → 执行 Pipeline → 缓存成功结果 → 显示/导出
```

- 空 Pipeline 同样执行统一入口，其输入就是最终输出。所有帧使用独立对象，下一帧和原地修改 Filter 不会覆盖已有缓存或原始模型。
- 设置 N 表示最多缓存 N 个最终输出，包含缓存中的当前帧；不再隐含加一。0 立即清空缓存并关闭复用，缩容立即按 LRU 淘汰。当前显示对象、源模型以及渲染资源本身仍占内存，容量不是整个应用的内存上限。
- 不提前读帧，不缓存中间 Filter 结果。动画使用仅复制时间、文件路径、帧类型的私有 `StreamingData` 读取上下文，不查询或修改源模型的旧缓存；命中最终输出时不读文件、不插值、不执行数据 Filter。初版关闭共享源缓存的做法已由下述隔离修复替换。
- 缓存键区分源帧、原始/插值模式、插值比例和输出帧号。修改 Filter 参数、增删/重排节点、切换源模型、切换时间模式或应用新的插值序列时清空旧缓存。涡量/差值计算设置改变也会失效。
- 相机、颜色映射、范围锁定、显示样式仍在显示阶段应用。范围刷新只处理当前最终输出，不通过全源帧扫描绕过缓存；释放缓存及旧显示对象时保持 OpenGL 上下文有效。
- 原始时间、插值播放和保存动画共享 `renderAnimationOutputFrame` 所用的输出路径。导出遍历当前时间模式的输出序列；例如两个源帧配置三个插值帧，导出入口获取三帧。失败输出不缓存，也不将上一帧作为成功结果继续导出。
- PNG 逐帧写文件；MP4/GIF 仍使用现有的整段图像缓冲后编码。该编码缓冲与模型输出缓存是两件事，本次没有实现流式视频编码。

扩展 Filter 时，`execute` 应根据输入数据、参数和时间信息确定数值输出；不要依赖每次显示都执行的副作用，也不要根据 `exporting` 改变数值结果，因为播放和导出会复用同一结果。

代码入口：`igQtAnimationOutputCache` 管理成功结果的 LRU；`igQtLoadAnimationFrame` 创建独立输入并处理插值；`igQtAnimationWidget::displayAnimationFrame` 统一求值与显示。插值检查相邻块、字段布局、点数及单元连接关系，多文件块按清单顺序挂载。

新增 `Animation.FieldInfo.OutputCache`，检查读取/算法调用次数、空 Pipeline、容量、失效和失败重试。参数窗口集成测试补充实际结果对象复用、修改参数、修改时间序列、切换源模型、关闭缓存、原始数据不变，以及移走源文件后仍能使用缓存导出。原有顶点/表面/混合数据回归检查当前最终显示对象。

真实 Density 数据缓存验收（从构建目录运行，以便加载着色器资源）：

```powershell
Set-Location out/build/x64-release-cgns
./testAnimationFieldInfo.exe dataset-cache 'D:/vtk_files/vtk_files'
```

该验收抽测第 1、50、100 帧，验证等值体、着色、播放/导出复用同一结果，以及源缓存为空、源 Density 仍为单元属性。日志为 `output-cache-dataset.log`。自动测试验证导出取帧与渲染路径，没有自动操作保存对话框生成 MP4/GIF。

## 切帧重置颜色映射（2026-09-28，待提交）

主窗口在 `AnimationDataChanged` 后调用通用属性树重建，末尾的 `viewAttribute(-1)` 关闭了刚应用的颜色映射。回归最初仅覆盖动画控件，未连接模型树回调，因此未发现。现在动画使用保留显示选择的树刷新入口；更新树时屏蔽选择信号，保留当前着色字段。非动画的通用重建仍保留原行为。

同一 Pipeline 后续帧继承用户选择的字段、分量及单色显示；修改 Pipeline 后重新使用其默认显示字段。按用户要求移除了新增的“手动固定”及全部公共标量面板改动，沿用已有的“只扩不缩”：动画初次输出或 Pipeline 改动后，为默认按帧调整、未锁定的着色字段启用该模式。后续切帧继承累积范围；用户明确选择其他原有范围模式时，不在每帧强制改回。模型树信号屏蔽仅用于动画刷新，其他模块的旧入口保留原信号行为。

`Animation.FieldInfo.ParameterDialog` 已接入真实模型树和标量面板回调；修复前复现 `animation extraction did not enable scalar coloring`。修复后检查普通帧、插值帧、缓存命中、关闭缓存、导出取帧的着色，以及调色板、另选字段/分量、关闭着色和恢复每帧范围。补充验证面板仍仅有原来的三种模式，累积范围扩为 0～4 后，回放较小范围的缓存帧或插值帧仍保持 0～4。四组动画回归全部通过。

## 动画缓存与旧缓存隔离（2026-10-07）

修复提交主题：`fix(animation): isolate output frame reads from legacy cache`。查询提交：`git log --format="%h %s" --grep="isolate output frame reads from legacy cache" -- Examples/Animation/FieldInfo/Validation.cpp`。

原问题：动画初始化、读帧和缓存容量调整直接调用源时间序列的 `DisableCache()`，清掉其他功能保留的源帧。新增回归先在旧缓存存入两帧，初始化动画时即复现 `animation changed legacy cache capacity or entries`。

现在仅调整动画模块：每次最终输出缓存未命中时，复制时间序列元数据到默认不开启缓存的私有读取上下文，沿用原读取器读取独立数据。复制完整序列以保留 ODB 字段帧索引；文件路径数组也独立复制。插值和动画预处理使用该上下文，最终输出继续持有它，避免播放/导出缓存命中时重新接回旧缓存。源模型的缓存开关、容量、对象和 LRU 顺序保持原样。动画缓存设置只调整最终输出缓存。

不修改 `StreamingData`、普通读写器、主窗口菜单、标量面板、属性差值或涡量算法。已有的模型对象替换、范围统计和属性树生命周期问题不在本次修复范围内。其他功能若主动开启其旧缓存，允许与最终输出缓存同时存在，两者的内存占用分别计算。

`Animation.FieldInfo.ParameterDialog` 补充：旧缓存中的数值故意改为磁盘之外的值，验证动画仍从文件读取且数组独立；验证插值、失败读帧、缓存缩容/关闭、Pipeline 变更、导出、模型切换和析构不破坏旧缓存。三帧、容量二的用例检查 LRU 淘汰顺序未被动画读取改变；清除旧缓存也不影响已取得的动画结果。原有最终输出缓存、字段描述和播放回归继续保留。
