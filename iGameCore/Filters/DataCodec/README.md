# iGame 框架调用 DataCodec 指南

## 1. 文档范围

本文说明 iGame 框架如何直接调用 DataCodec 公共入口，包括：

- iGame `DataObject` 如何变成 DataCodec 编码输入
- DataCodec 包如何还原成 iGame `DataObject`
- LeafPackage 与 FramePackage 使用哪些 adapter
- 属性目标如何产生
- options 如何展开为实际参数组
- iGame 文件系统和线程池如何注入 DataCodec
- 每个公开参数的含义、默认值和产生方式

本文使用以下公共入口：

```cpp
::datacodec::Encode(const ::datacodec::EncodeRequest&)
::datacodec::DecodePackage(const ::datacodec::DecodePackageRequest&)
```

示例文件：

- `Examples/Filter/Compression/TestDataCodecEncode.cpp`
- `Examples/Filter/Compression/TestDataCodecDecode.cpp`

这两个示例不经过 `IGDCWriter` 和 `IGDCReader`。

## 2. 调用层次

```text
iGame DataObject
    -> iGame encode adapter
    -> datacodec::Encode
    -> IByteRangeOutput

IByteRangeReader
    -> datacodec::DecodePackage
    -> iGame decode adapter 或 frame assembly
    -> iGame DataObject
```

| 层次 | 责任 |
|---|---|
| DataCodec Entry | 校验 request、解析配置、选择 Leaf/Frame workflow |
| DataCodec Adapter API | 定义几何、拓扑、属性的输入输出合同 |
| iGame Adapter | 在 `DataObject` 和 DataCodec Adapter API 之间转换 |
| ByteRange API | 为 DataCodec 提供随机访问输入输出 |
| ExecutionResources | 为 DataCodec 提供并行任务执行器 |
| IGDC IO | 产品级文件读写、缓存、时序和 UI 集成 |

## 3. 最小编码调用

```cpp
auto object = iGame::FileIO::ReadFile(sourcePath);
iGame::iGameEncodeAdapter adapter(object);
iGame::iGameFileByteRangeOutput output(encodedPath);

auto configuration = ::datacodec::MakeEncodeConfigurationParams({
    .tier = ::datacodec::DataCodecEncodeTier::Balanced,
});

auto result = ::datacodec::Encode({
    .input = ::datacodec::EncodeInput::LeafAdapter(&adapter),
    .output = ::datacodec::EncodeOutput::ByteRange(output),
    .attributeSelection =
        ::datacodec::AttributeSelectionMode::AllAvailable,
    .configuration = std::move(configuration),
    .executionResources = iGame::MakeDataCodecExecutionResources(),
});
```

调用过程：

1. `FileIO::ReadFile` 产生 iGame `DataObject`
2. `iGameEncodeAdapter` 借用 `DataObject` 中的几何、拓扑和属性
3. `MakeEncodeConfigurationParams` 把用户 options 展开为完整配置
4. `iGameFileByteRangeOutput` 把文件路径适配成随机访问输出
5. `MakeDataCodecExecutionResources` 把 iGame 线程池适配成 DataCodec task runner
6. `Encode` 同步完成编码并返回结果

`object`、`adapter` 和 `output` 必须存活到 `Encode` 返回。

## 4. LeafPackage 与 FramePackage 编码

普通叶对象使用 `iGameEncodeAdapter`：

```cpp
auto input = ::datacodec::EncodeInput::LeafAdapter(&leafAdapter);
```

包含子对象的多块对象使用 `iGameBlockTreeAdapter`：

```cpp
iGame::iGameBlockTreeAdapter blockTreeAdapter(rootObject);
auto input = ::datacodec::EncodeInput::BlockTreeAdapter(&blockTreeAdapter);
```

`EncodePackageKind::Auto` 会根据 adapter 类型选择包类型：

- `IEncodeAdapter` 对应 LeafPackage
- `IBlockTreeAdapter` 对应 FramePackage

多时间帧序列使用 frame-sequence workflow。它需要帧提供器、输出路径规划和跨帧 reference session，不属于单次 `Encode` 示例范围。

## 5. 最小解码调用

```cpp
auto inputReader =
    std::make_shared<iGame::iGameFileByteRangeReader>(encodedPath);
iGame::iGameDecodeAdapter leafAdapter;
iGame::iGameFramePackageDecodeAssembly frameAssembly;

auto configuration = ::datacodec::MakeDecodeConfigurationParams({
    .tier = ::datacodec::DataCodecDecodeTier::Balanced,
});

auto result = ::datacodec::DecodePackage({
    .inputReader = inputReader,
    .leafAdapter = &leafAdapter,
    .frameAssembly = &frameAssembly,
    .attributeSelection =
        ::datacodec::AttributeSelectionMode::AllAvailable,
    .configuration = configuration.PackageConfiguration(),
    .executionResources = iGame::MakeDataCodecExecutionResources(),
});

auto object = result.decodedFramePackage
    ? frameAssembly.Output()
    : leafAdapter.TakeDataObject();
```

`DecodePackage` 自动检查包类型：

- LeafPackage 写入 `leafAdapter`
- FramePackage 通过 `frameAssembly` 为每个 leaf 创建 adapter 并组装根对象

`inputReader`、`leafAdapter` 和 `frameAssembly` 必须存活到 `DecodePackage` 返回。

完整解码示例会把取得的 `DataObject` 加入 iGame 场景并启动交互窗口：

```cpp
auto scene = iGame::Scene::New();
scene->AddModel(object);

auto window = iGame::RenderWindow::New();
window->SetSize(1920, 1080);
window->SetScene(scene);

auto interactor = iGame::Interactor::New();
interactor->Initialize(scene);
interactor->CreateDefaultStyle();
window->SetInteractor(interactor);
window->Show();
```

窗口代码属于 iGame 展示层，DataCodec 解码结果仍通过 adapter 取得。

## 6. iGame Adapter 的来源和职责

| Adapter | 构造输入 | DataCodec 看到的内容 | 输出或所有权 |
|---|---|---|---|
| `iGameEncodeAdapter` | 一个叶 `DataObject` | geometry、topology、point/cell attributes、cell type mapping | 借用源对象数据 |
| `iGameBlockTreeAdapter` | 多块根 `DataObject` | branch records、leaf records、逐 leaf encode adapter | 持有根对象智能指针 |
| `iGameDecodeAdapter` | 空对象或已有目标对象 | geometry/topology/attribute 写入接口 | 产生一个 `DataObject::Pointer` |
| `iGameFramePackageDecodeAssembly` | 默认构造 | branch/leaf 组装接口 | 产生多块根 `DataObject::Pointer` |
| `iGameFileByteRangeReader` | 文件路径 | `IByteRangeReader` | 持有映射或文件读取状态 |
| `iGameFileByteRangeOutput` | 文件路径 | `IByteRangeOutput` | 持有输出流和逻辑大小 |
| `DataCodecThreadPoolTaskRunner` | iGame 全局线程池 | `IParallelTaskRunner` | task runner 为进程级共享对象 |

## 7. EncodeRequest 字段

| 字段 | 来源 | 含义 |
|---|---|---|
| `input` | encode adapter | Leaf 或 block tree 输入以及 frame 元数据 |
| `output` | Memory 或 ByteRange 工厂 | 编码包的目标位置和包类型 |
| `attributeSelection` | 调用方 | 不编码、编码全部、编码显式目标 |
| `attributeTargets` | 调用方或属性 catalog | `Explicit` 模式使用的目标列表 |
| `configuration` | encode configuration factory | codec、pipeline、execution 和配置来源 |
| `runRecordSink` | 可选调用方 sink | 运行消息、阶段时间、资源记录和进度 |
| `executionResources` | iGame 或其他宿主 | 并行任务执行器 |

### EncodeInput 字段

`EncodeInput::LeafAdapter` 会填写：

- `adapter`
- `leafPath`
- `objectName`
- `meshType`
- `frameIndex`

名称和 mesh type 为空时，Leaf executor 会从 adapter 取得。

`EncodeInput::BlockTreeAdapter` 会填写：

- `adapter`
- `rootName`
- `frameIndex`
- `frameCount`
- `timeValue`

### EncodeOutput

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `packageKind` | `Auto` | 根据 adapter 选择包类型，也可显式要求 LeafPackage 或 FramePackage |
| `target` | Memory | 内存输出或调用方提供的 `IByteRangeOutput`，应通过下面两个工厂创建 |

内存输出：

```cpp
auto output = ::datacodec::EncodeOutput::Memory();
```

结果位于 `EncodeResult::encodedBytes`。

文件或其他随机访问输出：

```cpp
auto output = ::datacodec::EncodeOutput::ByteRange(byteRangeOutput);
```

结果字节数位于 `EncodeResult::encodedByteCount`。

## 8. DecodePackageRequest 字段

| 字段 | 来源 | 含义 |
|---|---|---|
| `inputReader` | 文件、内存、网络 adapter | 编码包随机访问输入 |
| `framePackageMetadata` | 可选预检查结果 | 已解析的 FramePackage metadata，避免重复解析 |
| `leafAdapter` | 宿主框架 | LeafPackage 输出目标 |
| `frameAssembly` | 宿主框架 | FramePackage 输出目标 |
| `requestedFrameIndex` | 可选调用方选择 | 从 frame package 中选择目标帧 |
| `attributeSelection` | 调用方 | 不解码、解码全部、解码显式目标 |
| `attributeTargets` | catalog 或 session | `Explicit` 模式使用的目标列表 |
| `topologyReferenceKey` | reference session | 外部拓扑 reference 身份 |
| `topologyOwnerFrameIndex` | reference session | 拓扑 reference 所属帧 |
| `configuration` | decode configuration | package decode 实际消费的参数 |
| `runRecordSink` | 可选调用方 sink | 运行记录和进度 |
| `session` | 可选复用 session | 跨请求复用 reference 和 workspace |
| `executionResources` | iGame 或其他宿主 | 并行任务执行器 |
| `stopToken` | 调用方 | 协作式取消 |

`framePackageMetadata`、`topologyReferenceKey`、`topologyOwnerFrameIndex` 和 `session` 属于播放、时序或缓存路径。一次性文件解码可以保持默认值。

### 入口结果

`EncodeResult`：

| 字段 | 含义 |
|---|---|
| `success` | 整个编码调用是否成功 |
| `hasEncodedOutput` | 是否已经形成完整编码包 |
| `encodedBytes` | `EncodeOutput::Memory` 返回的包字节 |
| `encodedByteCount` | 完整编码包字节数 |
| `leafCount` | 包含的 leaf 数量 |
| `packageKind` | 实际生成的 LeafPackage 或 FramePackage |
| `messages` | 错误、警告和运行诊断 |

`DecodePackageResult`：

| 字段 | 含义 |
|---|---|
| `success` | 整个解码调用是否成功 |
| `cancelled` | 是否响应 `stopToken` 取消 |
| `decodedFramePackage` | 输入是否按 FramePackage 完成解码 |
| `inputBytes` | 本次包输入的逻辑字节数 |
| `messages` | 错误、警告和运行诊断 |

## 9. 属性选择

```cpp
enum class AttributeSelectionMode : std::uint8_t {
    None,
    AllAvailable,
    Explicit,
};
```

| 模式 | 行为 |
|---|---|
| `None` | 只处理几何和拓扑 |
| `AllAvailable` | 编码 Entry 从 adapter 枚举全部属性，解码 workflow 根据包 metadata 处理全部属性 |
| `Explicit` | 使用 `attributeTargets` |

非 `Explicit` 模式携带 `attributeTargets` 会返回 request contract 错误。

`AttributeTarget`：

```cpp
struct AttributeTarget {
    std::uint32_t frameIndex;
    BlockPath blockPath;
    std::size_t attrIndex;
};
```

`attrIndex` 是 leaf 内扁平属性索引：

1. point 属性按原始顺序排列
2. cell 属性排列在 point 属性之后
3. 第一个 cell 属性索引等于 point 属性数量

显式编码目标可以从 `CollectDataCodecEncodeAttributeDescriptors` 取得。显式解码目标可以从 `DataCodecDataObjectDecodeSession::AvailableAttributes` 取得。

## 10. 参数生成顺序

编码：

```text
DataCodecEncodeOptions
    -> MakeEncodeConfigurationParams
    -> DataCodecEncodeConfigurationParams
    -> EncodeRequest::configuration
```

解码：

```text
DataCodecDecodeOptions
    -> MakeDecodeConfigurationParams
    -> DataCodecDecodeConfigurationParams
    -> PackageConfiguration
    -> DecodePackageRequest::configuration
```

配置生效顺序：

1. 参数结构体默认值
2. 性能档位 factory
3. compression enhancement
4. runtime profile 约束
5. options 中的显式覆盖
6. 调用方对 configuration 的高级修改
7. Encode Entry 或 Decode leaf executor 再次应用 runtime profile 硬约束

Wasm runtime profile 的内存和并行限制属于硬约束。调用方高级修改无法突破这些限制。

## 11. DataCodecEncodeOptions

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `tier` | `Balanced` | 选择时间、综合或内存优先参数组 |
| `enableCompressionEnhancement` | `false` | 启用更偏向压缩率的 reference 和候选策略 |
| `packageZstdLevel` | 未指定 | 覆盖最终 package field Zstd level |
| `temporalKeyFrameInterval` | 未指定 | 同时覆盖属性和几何关键帧间隔，大于 0 时生效 |

### Encode tier

| Tier | 主要目标 | 典型策略 |
|---|---|---|
| `TimePriority` | 缩短编码时间 | 更多内存路径、更多属性 lane、较低 Zstd level |
| `Balanced` | 综合时间、内存和体积 | Managed storage、中等窗口、Zstd level 3 |
| `MemoryPriority` | 限制活跃内存 | Managed storage、较少 lane、较小窗口 |

## 12. DataCodecDecodeOptions

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `tier` | `Balanced` | 选择快速、综合或低内存解码参数组 |
| `validationProfile` | `Required` | 选择必需校验或审计校验 |
| `enableDecodedResultCache` | 未指定 | 覆盖档位的完整帧缓存开关 |
| `decodedResultCacheFrameLimit` | 未指定 | 覆盖完整帧缓存条目上限 |
| `enableFullInputPrefetch` | 未指定 | 覆盖完整输入预取策略 |

### Decode tier

| Tier | 主要目标 | 典型策略 |
|---|---|---|
| `Fast` | 最大化解码吞吐 | Memory cache、大窗口、更多属性 lane、输入预取 |
| `Balanced` | 综合吞吐和驻留量 | Managed cache、中等窗口和并发度 |
| `LowMemory` | 限制内存驻留 | Managed cache、小窗口、单 lane 或低并发 |

### Validation profile

| Profile | 含义 |
|---|---|
| `Required` | 执行格式安全、范围和必要语义校验 |
| `Audit` | 增加更严格的结构和结果一致性校验 |

## 13. Runtime profile

| Profile | 使用环境 | 作用 |
|---|---|---|
| `Native` | 桌面和服务器原生进程 | 使用 Native 档位资源预算 |
| `Wasm4GiB` | 4 GiB 地址空间 Wasm | 强制 Managed storage、小窗口和低并发 |
| `Wasm16GiB` | 扩展内存 Wasm | 使用受限的大窗口和 Managed storage |

Runtime profile 同时记录在 `configuration.source.runtimeProfile` 中。Entry 根据它重新应用硬约束。

## 14. Configuration 参数组

### Encode configuration

```cpp
struct DataCodecEncodeConfigurationParams {
    EncodeCodecControlParams controlParams;
    EncodePipelineControlParams pipelineControl;
    EncodeExecutionOptions execution;
    DataCodecEncodeConfigurationSource source;
};
```

| 参数组 | 作用 |
|---|---|
| `controlParams` | 数值压缩、空间分块、reference、校验和资源预算 |
| `pipelineControl` | 点/单元重排和最终 package field 编码 |
| `execution` | stage 并行开关 |
| `source` | 记录配置档位和运行环境来源 |

### Decode configuration

完整 decode configuration 由 factory 产生：

```cpp
struct DataCodecDecodeConfigurationParams {
    DecodeControlParams controlParams;
    DecodeExecutionOptions execution;
    DecodedFrameCachePolicy decodedFrameCachePolicy;
    EncodedInputCachePolicy encodedInputCachePolicy;
    DataCodecDecodeConfigurationSource source;
};
```

`DecodePackage` 只消费以下 package 配置：

```cpp
struct DataCodecDecodePackageConfigurationParams {
    DecodeControlParams controlParams;
    DecodeExecutionOptions execution;
    DataCodecDecodeConfigurationSource source;
};
```

| 参数组 | 作用 |
|---|---|
| `controlParams` | 解码校验和资源预算 |
| `execution` | stage 并行、输入预取和 topology observer |
| `decodedFrameCachePolicy` | session 级完整解码帧缓存策略 |
| `encodedInputCachePolicy` | session 级编码输入缓存策略 |
| `source` | 记录配置档位、校验档位和运行环境来源 |

`configuration.PackageConfiguration()` 复制 `controlParams`、`execution` 和 `source`。两个 cache policy 留给 DataObject bridge、playback 和其他 session 调用方。

## 15. CodecControlParams

| 字段 | 产生方式 | 含义 |
|---|---|---|
| `geomControl` | factory | 几何 NumericArray 精度和区域控制 |
| `spatialBlockPolicy` | 默认值或高级覆盖 | point/cell 的稳定空间块元素数 |
| `topologyReference` | factory | 拓扑跨帧指纹复用开关 |
| `geometryReference` | factory | 几何时域 reference 策略 |
| `attrControl` | 调用方按属性名填写 | 逐属性 NumericArray 控制覆盖 |
| `defaultAttrControl` | factory | 没有逐属性覆盖时使用的策略 |
| `attrReference` | factory | 属性帧内和时域 reference 策略 |
| `validation` | encode/decode factory | 解码校验模式 |
| `resourceBudget` | tier 和 runtime profile | 存储、窗口、并发和临时资源预算 |

`spatialBlockPolicy.pointElementCount` 和 `cellElementCount` 的结构体默认值均为 `262144`。它们决定 Ordinary 和 Reference 共享的语义分块边界。

`DecodeControlParams` 只包含 `validation` 和 `resourceBudget`，不携带编码精度、reference 或 remap 参数。

`CodecValidationPolicy`：

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `decodeMode` | `Required` | 执行必要校验，`Strict` 启用审计级校验 |
| `validateTopologyReferences` | `false` | 额外验证拓扑 reference 关系 |
| `validateFloatingPointValues` | `false` | 额外验证浮点结果 |

`DataCodecDecodeValidationProfile::Audit` 会把 `decodeMode` 设为 `Strict`，并启用另外两个校验开关。

## 16. NumericArrayControlParams

```cpp
struct NumericArrayControlParams {
    NumericArrayRegionControlParams regionControl;
    std::vector<RegionRun> regionRuns;
};
```

| 字段 | 结构体默认值 | 含义 |
|---|---:|---|
| `regionControl.defaultPrecision` | 无压缩器 | 未命中其他区域时使用的压缩器配置 |
| `regionControl.regions` | 空 | 可选的多区域精度配置 |
| `regionControl.runPolicy` | 见下表 | 区域 run 归一化和碎片合并阈值 |
| `regionRuns` | 空 | 元素范围到 region id 的映射 |

`CompressorConfig::options` 直接传给 SZ3。具体 option key 由当前 SZ3 接线和 factory 决定。普通调用优先使用 `MakeEncodeConfigurationParams` 产生的默认配置。

`NumericArrayRegionPrecision`：

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `name` | 空 | 区域精度名称，用于标识和诊断 |
| `hasCompressor` | `false` | 该精度项是否携带有效压缩器配置 |
| `compressor` | 空 options | 该精度项使用的 SZ3 参数 |

`NumericArrayRegionRunNormalizePolicy`：

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `maxRegionCount` | `8` | `regions` 允许配置的最大区域数量 |
| `maxRunsPerRegion` | `8` | 每个精度层归一化后允许保留的最大 run 数 |
| `minCoreRunLength` | `4096` | run 被视为核心区间的最小元素数 |
| `coreMinRatio` | `0.10` | run 被视为核心区间的最小区域内占比，与长度条件满足一个即可 |
| `minLongestRunRatio` | `0.20` | 没有核心区间时，最长 run 可作为核心区间的最低占比 |
| `maxFragmentRunLength` | `512` | 非核心碎片 run 允许的最大元素数 |
| `maxFragmentElementRatio` | `0.05` | 所有碎片元素占该区域元素的最大比例 |
| `maxCoalesceGap` | `256` | 为减少 run 数量可合并的相邻 run 最大间隙 |
| `maxExpansionRatio` | `0.02` | 合并间隙新增精细化元素占完整 block 的最大比例 |
| `maxRefinedElementRatio` | `8.0` | 所有精度层累计精细化元素占完整 block 的最大比例 |

`RegionRun`：

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `begin` | `0` | 元素范围起点 |
| `count` | `0` | 元素数量，实际提交的 run 必须大于 `0` |
| `regionId` | `1` | 从 `1` 开始索引 `regions`，`0` 保留给默认区域 |

逐属性覆盖：

```cpp
configuration.controlParams.attrControl[attributeName] = customControl;
configuration.source.customControlParams = true;
```

## 17. Reference 参数

### AttrReferenceControlParams

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `enabled` | `true` | 属性 frame 内和 frame 间 reference 总开关 |
| `intraField` | Affine Auto | 同一帧不同属性之间的 reference |
| `temporalField` | Predictor Auto | 不同帧同名属性之间的 reference |

### IntraFieldReferenceControlParams

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `codec` | `Affine` | Wavelet、Affine、Predictor 或关闭 |
| `selectionMode` | `Auto` | 自动比较 Ordinary/Reference 或强制 Reference |
| `autoSelectionStrategy` | `Exact` | 完整候选比较或 bounded probe |
| `sampleCount` | `256` | 每个字段的粗筛采样数 |
| `minimumSampleScore` | `0.5` | Predictor/Wavelet 粗筛阈值 |
| `affine.precheckRSquared` | `0.92` | Affine 字段级预检查阈值 |
| `affine.blockRSquared` | `0.95` | Affine block 级接受阈值 |

### TemporalFieldReferenceControlParams

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `codec` | `Predictor` | Wavelet、Predictor 或关闭 |
| `selectionMode` | `Auto` | 自动选择或强制 reference |
| `keyFrameInterval` | `8` | GOP 关键帧间隔，0 表示只有首帧是关键帧 |
| `forcePredFrames` | `false` | 调试时强制首帧后全部规划为预测帧 |
| `predictor.enableLocalWindowSearch` | `false` | 启用局部 offset 搜索 |
| `predictor.windowRadius` | `8` | 局部搜索半径 |
| `predictor.searchStrategy` | `ExhaustiveL2` | offset 候选搜索策略 |

### Geometry 与 Topology

| 参数 | 默认值 | 含义 |
|---|---:|---|
| `geometryReference.enabled` | `true` | 几何时域 reference 总开关 |
| `geometryReference.temporalField` | Predictor Auto | 几何时域 reference 策略 |
| `topologyReference.enabled` | `true` | 拓扑指纹复用开关 |

## 18. EncodePipelineControlParams

| 字段 | 结构体默认值 | 含义 |
|---|---:|---|
| `pointOrder` | `Morton` | 点及 point 属性是否按 Morton 顺序重排 |
| `cellOrder` | `Morton` | cell、topology 和 cell 属性是否按 Morton 顺序重排 |
| `packageFields.mode` | `Zstd` | 最终 package field 使用 Raw 或 Zstd |
| `packageFields.zstdLevel` | `3` | 最终 package field Zstd level |
| `packageFields.workerCount` | `4` | 最终 package field Zstd worker 数量 |

性能档位会覆盖这些结构体默认值。

## 19. Execution 参数

### EncodeExecutionOptions

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `enableParallelStages` | `true` | 允许独立 stage 和属性任务并行 |

### DecodeExecutionOptions

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `enableParallelStages` | `true` | 允许 geometry、topology、attribute 并行 |
| `enableFullInputPrefetch` | `false` | 在完整帧解码前预取整个输入 |
| `topologyOutputMode` | `CommitToAdapter` | 提交拓扑到 adapter 或只发送 observer |
| `topologyBlockObserver` | 空 | 可选拓扑块观察者 |

### ExecutionResources

```cpp
struct DataCodecExecutionResources {
    IParallelTaskRunner* parallelTaskRunner;
};
```

iGame 使用：

```cpp
.executionResources = iGame::MakeDataCodecExecutionResources()
```

未提供 task runner 时，Entry 使用 `InlineParallelTaskRunner` 串行执行。该回退保证默认 request 可运行。

## 20. ResourceBudgetControlParams

以下默认值是结构体默认值。性能档位和 runtime profile 会生成实际运行值。

### 全局驻留与窗口

| 字段 | 默认值 | 单位 | 含义 |
|---|---:|---|---|
| `residentLimitMiB` | `0` | MiB | Memory Store 和长期驻留缓存总上限，0 表示不限制 |
| `accessWindowMiB` | `64` | MiB | 单次窗口化读写粒度 |
| `activeWindowMiB` | `256` | MiB | 同时活跃的共享窗口总预算 |

### Scratch 和 staging

| 字段 | 默认值 | 单位 | 含义 |
|---|---:|---|---|
| `scratchRetainedBlockCount` | `16` | 块 | Scratch Pool 留存块数量上限 |
| `scratchRetainedBlockMiB` | `64` | MiB | Scratch Pool 单块留存上限 |
| `scratchRetainedTotalMiB` | `1024` | MiB | Scratch Pool 总留存上限 |
| `attributeScratchQuotaMiB` | `256` | MiB | 属性自有 scratch 活跃分配额度，0 表示不单独限制 |
| `attributeMemoryStagingLimitMiB` | `0` | MiB | 属性 Memory staging 上限，0 表示禁止 Memory staging |
| `attributeManagedStagingLogicalLimitMiB` | `0` | MiB | Managed staging 在途逻辑数据上限，0 表示不单独限制 |

### 解码 storage mode

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `attributeDecodePayloadMode` | `Managed` | 属性 raw payload 使用 Managed、Memory 或 OneShotZstd |
| `attributeDecodeCacheMode` | `Managed` | 属性 decoded cache 存储模式 |
| `geometryDecodeCacheMode` | `Managed` | geometry decoded cache 存储模式 |
| `geometryDecodeReferenceCacheMode` | `Managed` | geometry reference cache 存储模式 |
| `topologyDecodeInputMode` | `Managed` | topology encoded input staging 模式 |
| `topologyDecodeCacheMode` | `Managed` | topology decoded cache 存储模式 |
| `topologyDecodeReferenceCacheMode` | `Managed` | topology reference cache 存储模式 |

`Managed` 允许 ByteStore 根据预算选择受管存储。`Memory` 要求内存存储并受对应 Memory limit 限制。

### 编码 storage mode

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `geometryEncodeTransferCacheMode` | `Managed` | geometry 编码 transfer cache |
| `geometryEncodeStagingMode` | `Managed` | geometry 编码 staging |
| `attributeEncodeTransferCacheMode` | `Managed` | attribute 编码 transfer cache |
| `attributeEncodeStagingMode` | `Managed` | attribute 编码 staging |
| `topologyEncodeTransferCacheMode` | `Managed` | topology 最终 transfer cache |
| `remapEncodeStorageMode` | `Managed` | remap 中间结果 |
| `packageFieldStagingMode` | `Managed` | 最终 package field 压缩 staging |
| `attributeEncodeReferenceCacheMode` | `Managed` | attribute 关键帧 reference cache |
| `geometryEncodeReferenceCacheMode` | `Managed` | geometry 关键帧 reference cache |

### Reference 与 Memory 限额

| 字段 | 默认值 | 单位 | `0` 的含义 |
|---|---:|---|---|
| `encodeReferenceResidentLimitMiB` | `0` | MiB | 编码 reference cache 不限制驻留字节 |
| `attributeDecodeMemoryPayloadLimitMiB` | `0` | MiB | 禁止属性 raw payload 内存 staging |
| `attributeDecodeMemoryCacheLimitMiB` | `0` | MiB | 禁止属性 decoded memory cache |
| `geometryDecodeMemoryCacheLimitMiB` | `0` | MiB | 禁止 geometry decoded memory cache |
| `geometryDecodeMemoryReferenceLimitMiB` | `0` | MiB | 禁止 geometry reference memory cache |
| `topologyDecodeMemoryInputLimitMiB` | `0` | MiB | 禁止 topology memory input |
| `topologyDecodeMemoryCacheLimitMiB` | `0` | MiB | 禁止 topology decoded memory cache |
| `topologyDecodeMemoryReferenceLimitMiB` | `0` | MiB | 禁止 topology reference memory cache |
| `decodeReferenceResidentLimitMiB` | `4096` | MiB | 解码 reference frame cache 不限制驻留字节 |
| `decodeReferenceFrameLimit` | `4` | 帧 | 解码 reference frame cache 不限制条目数 |

### 并发 lane

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `attributePressioLanes` | `1` | 同时进入 pressio 的属性任务数 |
| `attributeReferenceLanes` | `1` | 同时运行 reference-heavy 属性任务数 |
| `attributeDecodeLanes` | `4` | 同时运行属性解码任务数 |
| `attributeCommitLanes` | `2` | 同时把属性 cache 写入 adapter 的任务数 |
| `topologyBlockLanes` | `4` | 同时运行拓扑块编解码的任务数 |

### Topology 和 remap

| 字段 | 默认值 | 单位 | 含义 |
|---|---:|---|---|
| `topologyStreamBufferMiB` | `1` | MiB | 每个 topology stream 的缓冲估算 |
| `topologyBufferBudgetMiB` | `4` | MiB | 所有 topology stream buffer 活跃总量 |
| `remapMortonLeafMiB` | `256` | MiB | Morton leaf 排序块预算 |
| `remapMortonRunBufferMiB` | `8` | MiB | Morton run 读取缓冲预算 |
| `remapScratchQuotaMiB` | `256` | MiB | Remap 临时数组活跃总量 |

数值归一化规则：

- `accessWindowMiB`、`activeWindowMiB`、`topologyBufferBudgetMiB`、`remapMortonLeafMiB`、`remapMortonRunBufferMiB` 和 `remapScratchQuotaMiB` 的 `0` 会按 `1 MiB` 使用
- 所有 lane 数量的 `0` 会按 `1` 使用
- scratch block count、block MiB 或 total MiB 形成的有效容量为 `0` 时不保留 scratch block
- 字段注释标明“不限制”的 `0` 会转换为无上限
- 字段注释标明“不允许”的 `0` 会禁止对应 Memory 路径

## 21. Decode cache 参数

完整 decode configuration 还包含两种 session 级缓存策略。`PackageConfiguration()` 不携带它们。

### DecodedFrameCachePolicy

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `enabled` | `true` | 是否使用 DataCodec 默认完整帧 LRU |
| `residentFrameLimit` | `3` | 完整帧条目上限，0 表示不限制 |
| `residentLimitBytes` | `0` | 完整帧驻留字节上限，0 表示不限制 |
| `prefetchFrameCount` | `1` | 播放预取候选数量，不参与 LRU 淘汰 |

### EncodedInputCachePolicy

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `enabled` | `false` | 是否缓存原始编码包字节 |
| `residentInputLimit` | `3` | 输入条目上限，0 表示不限制 |
| `residentLimitBytes` | `0` | 输入驻留字节上限，0 表示不限制 |

Native 文件输入默认依赖内存映射和操作系统页缓存。Wasm 或网络输入可以启用 encoded input cache。

## 22. ConfigurationSource

Encode source：

- `performanceTier`
- `runtimeProfile`
- `compressionEnhancementEnabled`
- `customControlParams`

Decode source：

- `performanceTier`
- `runtimeProfile`
- `validationProfile`
- `customControlParams`

`source` 用于运行约束重应用、遥测和诊断。实际算法参数位于 `controlParams`、`pipelineControl` 和 `execution`。

手动修改高级参数时应设置：

```cpp
configuration.source.customControlParams = true;
```

## 23. 调用方参数与包内元数据

以下对象属于调用方控制参数：

- `DataCodecEncodeOptions`
- `DataCodecDecodeOptions`
- `CodecControlParams`
- `DecodeControlParams`
- `EncodePipelineControlParams`
- `EncodeExecutionOptions`
- `DecodeExecutionOptions`
- cache policies
- `AttributeSelectionMode`
- `AttributeTarget`

以下对象由编码器根据 adapter 和控制参数产生：

- `CodecStorageParams`
- `GeometryStorageParams`
- `TopoStorageParams`
- `AttrStorageParams`
- `NumericArrayStorageParams`
- topology block layout
- numeric-array block layout

这些 storage params 会写入包，并在解码时作为布局和 codec metadata 读取。调用方无需创建它们。

## 24. 常见配置

### TimePriority 编码

```cpp
auto configuration = ::datacodec::MakeEncodeConfigurationParams({
    .tier = ::datacodec::DataCodecEncodeTier::TimePriority,
});
```

### MemoryPriority 编码

```cpp
auto configuration = ::datacodec::MakeEncodeConfigurationParams({
    .tier = ::datacodec::DataCodecEncodeTier::MemoryPriority,
});
```

### LowMemory 解码

```cpp
auto configuration = ::datacodec::MakeDecodeConfigurationParams({
    .tier = ::datacodec::DataCodecDecodeTier::LowMemory,
});
```

### 显式属性解码

```cpp
auto result = ::datacodec::DecodePackage({
    .inputReader = inputReader,
    .leafAdapter = &leafAdapter,
    .frameAssembly = &frameAssembly,
    .attributeSelection = ::datacodec::AttributeSelectionMode::Explicit,
    .attributeTargets = targets,
    .configuration = configuration.PackageConfiguration(),
    .executionResources = iGame::MakeDataCodecExecutionResources(),
});
```

### Raw package field 调试

```cpp
auto configuration = ::datacodec::MakeEncodeConfigurationParams({
    .tier = ::datacodec::DataCodecEncodeTier::Balanced,
});
configuration.pipelineControl.packageFields.mode =
    ::datacodec::PackageFieldEncodingMode::Raw;
configuration.source.customControlParams = true;
```

### 自定义访问窗口

```cpp
auto configuration = ::datacodec::MakeDecodeConfigurationParams({
    .tier = ::datacodec::DataCodecDecodeTier::Balanced,
});
configuration.controlParams.resourceBudget.SetCacheWindowMiB(128u, 512u);
configuration.source.customControlParams = true;
```

## 25. 错误和取消

编码和解码结果均包含 `messages`：

```cpp
for (const auto& message : result.messages) {
    std::cerr << message.text << '\n';
}
```

`success == false` 表示 request contract、格式、资源、codec 或 commit 失败。

解码调用可以传入 `std::stop_token`。取消后的结果满足：

- `success == false`
- `cancelled == true`
- adapter 执行 Abort 或 failure cleanup

## 26. 生命周期摘要

| 对象 | 生命周期要求 |
|---|---|
| encode adapter | 存活到 `Encode` 返回 |
| block tree adapter | 存活到 `Encode` 返回 |
| byte range output | 存活到 `Encode` 返回 |
| byte range reader | 由 `shared_ptr` 保持，session 路径可能继续持有 |
| leaf decode adapter | 存活到 `DecodePackage` 返回 |
| frame assembly | 存活到 `DecodePackage` 返回 |
| task runner | 覆盖整个编解码调用 |
| configuration | request 按值保存，调用开始后不依赖原对象 |

## 27. 入口选择建议

| 场景 | 建议入口 |
|---|---|
| 学习 DataCodec 核心调用 | `Encode` / `DecodePackage` |
| 单次 iGame DataObject 解码 | `DecodeDataCodecDataObject` |
| 增量属性请求 | `DataCodecDataObjectDecodeSession` |
| 多帧播放与预取 | `PlaybackSession` 和 iGame playback bridge |
| 产品文件 IO | `IGDCWriter` / `IGDCReader` |

核心示例使用第一行入口，完整展示 adapter、参数、资源和结果路径。
