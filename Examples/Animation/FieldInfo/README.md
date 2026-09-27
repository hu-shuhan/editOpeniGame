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

本次只修改字段描述和参数配置，不恢复此前已回退的动画时间/导出改动。

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
