# DataCodec 开发者调用指南

## 1. 文档说明

本文档面向需要调用 DataCodec 的开发者（iGame 应用层、界面层、浏览器工程，以及接入 DataCodec 的第三方系统），说明一次编解码调用需要构造哪些内容、实现哪些接口、填充哪些参数、调用哪些函数、按什么流程走通。

- 功能特性与设计背景：`Feature_Guide.md`
- 浏览器示例工程优化说明：`Wasm_Example_Optimization_Guide.md`
- 参考示例：`Examples/Filter/Codec/TestDataCodecEncode.cpp`、`Examples/Filter/Codec/TestDataCodecDecode.cpp`（iGame 对象调用路径）、`Examples/Filter/Codec/VTK/`（第三方系统接入路径）

调用 DataCodec 有两条路径：

- **现成适配器路径（推荐，零实现成本）**：直接使用 iGame 提供的适配器与 IO 实现，只需按本文第 3、4 章的步骤拼装参数；
- **自定义接入路径**：实现第 5 章的公开接口，把第三方数据对象桥接进 DataCodec，用于非 iGame 数据体系。

两条路径共用同一对入口函数与同一套参数语义。

---

## 2. 调用模型总览

### 2.1 入口与流程骨架

DataCodec 对外只有两个入口，均为单次同步调用：

- 编码入口：`datacodec::Encode(const EncodeRequest&)`，返回 `EncodeResult`；
- 解码入口：`datacodec::DecodePackage(const DecodePackageRequest&)`，返回 `DecodePackageResult`。

一次编码调用构造「输入适配器 + 输出目标 + 配置 + 执行资源」四类内容；一次解码调用构造「输入读取器 + 结果接收器 + 配置 + 执行资源」四类内容。DataCodec 在调用内部完成校验、并行调度、压缩与组装，调用方不接触内部流水线。

### 2.2 编码侧角色

| 角色 | 类型 | 职责 | 必填 |
| --- | --- | --- | --- |
| 输入适配器 | `IEncodeAdapter` 或 `IBlockTreeAdapter` | 把源数据对象桥接为 DataCodec 可读的借用视图（几何、拓扑、属性） | 是（二选一） |
| 输入描述 | `EncodeInput` | 携带适配器指针、对象名、块路径、帧序号等身份元数据 | 是 |
| 输出目标 | `EncodeOutput` | 指定包形态（Leaf/Frame）与结果去向（内存 / 随机访问输出） | 是 |
| 配置参数 | `DataCodecEncodeConfigurationParams` | 性能档位、压缩增强、ZSTD 强度、参考帧间隔、语言 | 是（可用默认） |
| 执行资源 | `DataCodecExecutionResources` | 并行任务执行器 | 可选（缺省内联执行） |
| 观测接收 | `DataCodecOutputSinks`、`IRunRecordSink` | 状态、进度、报告文件、运行记录 | 可选 |

### 2.3 解码侧角色

| 角色 | 类型 | 职责 | 必填 |
| --- | --- | --- | --- |
| 输入读取器 | `IByteRangeReader` | 提供压缩字节的随机范围读取（文件、内存、网络来源） | 是 |
| 结果接收器 | `IDecodeAdapter` 或 `IFramePackageDecodeAssembly` | 接收解码分片并在目标对象上组装结果 | 是（按包形态二选一） |
| 请求参数 | `DecodePackageRequest` | 携带读取器、接收器、属性选择、帧请求、配置等 | 是 |
| 配置参数 | `DataCodecDecodePackageConfigurationParams` | 性能档位、校验级别、缓存与预读策略、语言 | 是（可用默认） |
| 执行资源 | `DataCodecExecutionResources` | 并行任务执行器 | 可选（缺省内联执行） |

### 2.4 包形态

压缩结果分为两种包形态，决定编码输入与解码接收器的类型：

| 包形态 | 适用数据 | 编码输入 | 解码接收器 |
| --- | --- | --- | --- |
| `LeafPackage` | 单个数据块（单块单帧） | `IEncodeAdapter` | `IDecodeAdapter` |
| `FramePackage` | 多块数据树、多帧时间序列 | `IBlockTreeAdapter` | `IFramePackageDecodeAssembly`（建议用 `IDecodedFrameAssembly`） |

包形态由调用方显式指定（`EncodeOutput::Memory(kind)` / `EncodeOutput::ByteRange(sink, kind)` 的 `kind` 参数，或直接设置 `EncodeOutput::packageKind`），也可以使用 `Auto` 由系统按输入适配器类型推断：输入是块树适配器则为 `FramePackage`，否则为 `LeafPackage`。注意形态与适配器类型必须匹配：`LeafPackage` 配叶子适配器、`FramePackage` 配块树适配器，不匹配在入口即被拒绝。

---

## 3. 编码调用指南

### 3.1 第一步：确定包形态

- 数据只有一个网格对象（点集、表面网格、体网格、非结构化网格、结构化网格、多面体网格）→ `LeafPackage`；
- 数据是包含子对象的多块树，或需要作为时间序列的一帧编码 → `FramePackage`（iGame 对象可通过 `DataObject::HasSubDataObject()` 判断是否多块）。

### 3.2 准备输入适配器

**叶子适配器（LeafPackage）**：实现 `IEncodeAdapter`，把单个网格对象暴露为只读视图。接口内容见 5.1。iGame 现成实现：

- `iGame::iGameEncodeAdapter`：构造时传入 `DataObject::Pointer`，支持点集、表面/体/非结构化/结构化网格与高阶网格；
- 构造前可用 `iGame::CanCreateiGameEncodeAdapter(object)` 预检对象类型是否被支持。

**块树适配器（FramePackage）**：实现 `IBlockTreeAdapter`，描述数据树的叶子块（块路径 + 名称）并为每个叶子创建叶子适配器。iGame 现成实现：

- `iGame::iGameBlockTreeAdapter`：构造时传入根 `DataObject`，自动遍历子对象树收集叶子与分支；
- 可通过 `GetLeafRecords()` 检查是否存在可编码叶子（空树应提前拦截）。

### 3.3 构造 EncodeInput

用工厂函数构造，两个工厂对应两种输入形态：

- `EncodeInput::LeafAdapter(adapter, path, name, type, inputFrameIndex)`：`path` 为块路径（`BlockPath`，单块通常可空），`name`/`type` 为对象名与网格类型描述（用于报告），`inputFrameIndex` 为帧序号；
- `EncodeInput::BlockTreeAdapter(adapter, name, inputFrameIndex, inputFrameCount, inputTimeValue)`：`name` 为根名称，后三项描述时间序列的帧身份（帧序号、总帧数、时间值），多帧编码时逐帧调用并递增帧序号。

### 3.4 生成配置参数

调用工厂 `datacodec::MakeEncodeConfigurationParams(options, runtimeProfile)`，`options` 为 `DataCodecEncodeOptions`：

| 选项 | 取值 | 说明 |
| --- | --- | --- |
| `tier` | `TimePriority` / `Balanced` / `MemoryPriority` | 编码性能档位，联动并发、压缩强度、缓存位置与驻留预算 |
| `enableCompressionEnhancement` | bool | 压缩率增强（更积极的空间重排与参考搜索），不改变资源预算 |
| `packageZstdLevel` | 可选 1–22 | 覆盖最终打包的 ZSTD 强度 |
| `temporalKeyFrameInterval` | 可选帧数 | 覆盖时序关键帧间隔 |
| `language` | 中文 / 英文 | 消息语言 |

`runtimeProfile` 取值 `Native` / `Wasm4GiB` / `Wasm16GiB`，默认 `Native`；浏览器环境必须传入对应规格，DataCodec 会据此套用硬性资源约束并做容量校验。工厂返回的配置记录来源（`source.runtimeProfile`），便于复现实验结果。完全不指定选项时也可直接省略（`EncodeRequest::configuration` 默认即标准档 `MakeDefaultEncodeConfigurationParams()`）。

### 3.5 选择输出目标

- `EncodeOutput::Memory(kind)`：完整编码包返回在 `EncodeResult::encodedBytes`，适合小文件或直接转交给内存读取器；
- `EncodeOutput::ByteRange(sink, kind)`：写入调用方提供的 `IByteRangeOutput`（如 iGame 的 `iGameFileByteRangeOutput`），按范围随机写并 `Finalize`，适合大文件直接落盘。

### 3.6 属性选择

`EncodeRequest::attributeSelection` 决定哪些属性参与编码：

- `AllAvailable`（默认）：编码全部点属性与单元属性；
- `None`：只编码几何与拓扑，不编码任何属性；
- `Explicit`：只编码 `attributeTargets` 列出的属性。目标由 `AttributeTarget{frameIndex, blockPath, attrIndex}` 描述，属性索引为扁平索引：点属性在前（0 起），单元属性紧随其后；该模式之外不得携带目标列表（入口校验），列表为空表示不编码属性（等价于 `None` 的显式写法）。

### 3.7 执行资源与观测（可选）

- `executionResources.parallelTaskRunner`：`IParallelTaskRunner` 实现（iGame 提供 `iGame::DataCodecTaskRunner()` / `MakeDataCodecExecutionResources()`）。不提供时使用内联执行器，功能不变、无并行；
- `outputSinks`：注入界面 / 控制台 / 进度 / 报告文件四类接收器之一或组合；
- `runRecordSink`：注入运行记录接收器（生命周期、阶段耗时、资源用量）。

### 3.8 调用与结果处理

调用 `datacodec::Encode({...})`，随后处理 `EncodeResult`：

- `success`：整体是否成功；`hasEncodedOutput`：是否实际产生了编码输出；
- `packageKind`：实际包形态（Auto 推断结果在此可见）；`leafCount`：编码的叶子块数；
- `encodedBytes` / `encodedByteCount`：内存输出路径的字节内容与总量；
- `messages`：本次调用的消息记录（失败时包含错误原因，应逐一输出供排障）。

### 3.9 编码流程小结

1. 加载或构造源数据对象；
2. 按数据形态选择包形态（单块 / 多块 / 多帧）；
3. 创建叶子适配器或块树适配器（iGame 对象直接用现成实现）；
4. 用工厂构造 `EncodeInput`（块路径、对象名、帧身份）；
5. 用 `MakeEncodeConfigurationParams` 生成配置；
6. 选择 `EncodeOutput`（内存或随机访问输出）；
7. 设置属性选择（全量 / 无 / 显式目标）；
8. 调用 `Encode`，检查 `success` 与 `hasEncodedOutput`，失败时输出 `messages`。

---

## 4. 解码调用指南

### 4.1 构造输入读取器

实现 `IByteRangeReader`（见 5.6），iGame 现成实现：

- `iGame::iGameFileByteRangeReader`：内存映射文件读取，提供连续区间视图与平台预取（Wasm 下退化为文件流）；
- `datacodec::MemoryByteRangeReader`：内存字节读取（可接受 `std::span` / `std::vector` / `shared_ptr<const vector>`，全量字节可被共享复用）。

构造后建议核对 `ByteSize()` 非零，空文件提前拦截。

### 4.2 构造结果接收器

按包形态准备：

- `LeafPackage`：实现 `IDecodeAdapter`（见 5.4），解码分片直接在其内部组装目标对象。iGame 现成实现：`iGame::iGameDecodeAdapter`，解码完成用 `TakeDataObject()` 取回 `DataObject`；
- `FramePackage`：实现 `IFramePackageDecodeAssembly`（见 5.5）。iGame 现成实现：`iGame::iGameFramePackageDecodeAssembly`（实现 `IDecodedFrameAssembly`），解码完成用 `Output()` 取回组装好的数据树根对象。

### 4.3 生成配置参数

调用工厂 `datacodec::MakeDecodeConfigurationParams(options, runtimeProfile)`，`options` 为 `DataCodecDecodeOptions`：

| 选项 | 取值 | 说明 |
| --- | --- | --- |
| `tier` | `Fast` / `Balanced` / `LowMemory` | 解码性能档位，联动并发、缓存驻留与窗口 |
| `validationProfile` | `Required` / `Audit` | 结果校验级别（必需级 / 审计级） |
| `enableDecodedResultCache` | 可选 bool | 覆盖完整帧缓存默认策略 |
| `decodedResultCacheFrameLimit` | 可选帧数 | 覆盖完整帧缓存容量 |
| `enableFullInputPrefetch` | 可选 bool | 覆盖完整输入预读策略 |
| `logging` | 文件 / 控制台开关 | 解码日志输出 |
| `language` | 中文 / 英文 | 消息语言 |

工厂返回 `DataCodecDecodeConfigurationParams`，其中包含会话级缓存策略（完整帧缓存、编码输入缓存）与 `PackageConfiguration()` 提取方法。单次包解码只消费包级配置，示例中通过 `configuration.PackageConfiguration()` 传给请求。完全不指定选项时也可直接省略（`DecodePackageRequest::configuration` 默认即标准档 `MakeDefaultDecodePackageConfigurationParams()`）。

### 4.4 填充 DecodePackageRequest

| 字段 | 说明 |
| --- | --- |
| `inputReader` | 输入读取器（必须） |
| `leafAdapter` | LeafPackage 接收器（按包形态提供） |
| `frameAssembly` | FramePackage 接收器（按包形态提供） |
| `framePackageMetadata` | 可选：预先读取的帧包元数据（`FramePackage`），省去重复解析 |
| `requestedFrameIndex` | 可选：多帧数据随机访问目标帧 |
| `attributeSelection` / `attributeTargets` | 与编码侧同语义：全量 / 无 / 显式目标（目标带帧、块路径、属性索引） |
| `topologyReferenceKey` / `topologyOwnerFrameIndex` | 拓扑复用：引用已有拓扑的标识与所属帧（对应 Feature_Guide 的拓扑指纹复用） |
| `configuration` | 包级解码配置（`DataCodecDecodePackageConfigurationParams`） |
| `outputSinks` / `runRecordSink` | 可选观测接收器 |
| `session` | 可选：`DecodeSession*`，多帧播放场景的会话级缓存与帧复用 |
| `executionResources` | 可选并行执行器 |
| `stopToken` | 可选：任务取消令牌（`std::stop_token`） |

### 4.5 调用与结果处理

调用 `datacodec::DecodePackage({...})`，随后处理 `DecodePackageResult`：

- `success` / `cancelled`：成功 / 被取消（取消不视为失败）；
- `decodedFramePackage`：按此标志选择结果来源——为真时从 `frameAssembly` 取组装结果（`Output()`），为假时从 `leafAdapter` 取目标对象（`TakeDataObject()` / `TakeOutput()`）；
- `inputBytes`：本次实际消费的输入字节数；`messages`：消息记录（失败原因）。

### 4.6 解码流程小结

1. 构造输入读取器（文件 / 内存），核对字节数非空；
2. 构造结果接收器（叶子适配器或帧组装器）；
3. 用 `MakeDecodeConfigurationParams` 生成配置并提取包级配置；
4. 填充 `DecodePackageRequest`（读取器、接收器、属性选择、可选帧请求）；
5. 调用 `DecodePackage`，检查 `success` / `cancelled`；
6. 按 `decodedFramePackage` 从对应接收器取回结果对象。

---

## 5. 需要实现的接口

自定义接入（第三方系统）时才需要实现本节接口；iGame 对象调用直接使用现成适配器，可跳过本章。

### 5.1 IEncodeAdapter（编码侧叶子适配器）

继承四个子接口：几何输入、拓扑输入、多面体输入、属性输入，外加单元类型映射与生命周期。按职责分组：

**几何（`IEncodeGeometryInput`）**

| 方法 | 必填 | 说明 |
| --- | --- | --- |
| `GetNumberOfPoints` | 是 | 点数 |
| `GetPoint(index, double[3])` | 是 | 逐点读取回退路径 |
| `GetPointScalarType` | 否 | 回退路径的坐标标量类型（默认 Float64） |
| `TryGetPointsF32` / `TryGetPointsF64` | 否 | 连续点数组零拷贝快路径，能提供则提供，不能返回空 |

**拓扑（`IEncodeTopologyInput`）**

| 方法 | 必填 | 说明 |
| --- | --- | --- |
| `DescribeTopology(TopologyInputDescriptor&)` | 是 | 声明各类数据的来源（连续数组 / 按需读取 / 无）、单元尺寸来源（固定元数 / 偏移表）、是否结构化 / 多面体 |
| `GetNumberOfCells` | 是 | 单元数 |
| `GetCellIdBufferPtr` / `GetCellIdBufferSize` | 是 | 连接表连续缓冲区（拓扑为连续数组时必须） |
| `GetCellIdOffsetPtr` | 是 | 偏移表缓冲区；可变单元网格提供，固定元数网格返回空 |
| `IsFixedCellSize` / `GetFixedCellSize` | 是 | 如实声明是否为固定元数网格及元数值，供编码器决定是否跳过偏移表 |
| `GetCellTypesPtr` / `ReadCellType` | 按需 | 逐单元类型流（连续或按需，默认无） |
| `GetCellPolynomialOrdersPtr` / `ReadCellPolynomialOrder` | 按需 | 高阶单元的逐单元多项式阶（默认无） |
| `GetStructuredAxisSize` | 按需 | 结构化网格轴尺寸（默认不支持） |

除表格标注「按需」（带默认实现）的方法外，拓扑接口均为纯虚方法，必须全部实现；不支持的来源在 `DescribeTopology` 中如实声明即可。

**单元类型映射（`ICellTypeMapping`）**：提供原生单元类型 ↔ DataCodec 规范单元类型的双向映射。`GetCellTypeMappingMode()` 声明映射模式；`ResolveCellType` 解析原生类型的规范信息（家族、固定元数）；`EncodeCellTypeFamilyLocal` / `DecodeCellTypeFamilyLocal` 负责家族-局部码的编解；高阶单元另需 `ResolveCellSizeFromPolynomialOrder` 与 `EncodeCellPolynomialOrderLocal` / `DecodeCellPolynomialOrderLocal`（按阶数解析元数、编解阶数局部码）。接口默认返回 false（不支持），但一旦适配器提供了单元类型流（`GetCellTypesPtr` 非空），映射必须真实可用，否则对应单元形态在编码中被拒绝。编码与解码两侧必须对称（同一张映射表），保证单元类型原样往返。

**属性（`IEncodeAttributeInput`）**

| 方法 | 必填 | 说明 |
| --- | --- | --- |
| `GetNumberOfPointAttrs` / `GetNumberOfCellAttrs` | 是 | 点 / 单元属性个数 |
| `GetPointAttr` / `GetCellAttr(index)` | 是 | 返回 `IEncodeAttrView`（见 5.2） |

**多面体（`IEncodePolyhedronInput`）**：`IsPolyhedronMesh`、面顶点表（`GetFaceIdBufferPtr` / `GetFaceIdOffsetPtr` 及数量）、单元到面引用表（`GetCellFaceBufferPtr` / `GetCellFaceOffsetPtr`）。其中 `GetCellFaceBufferSize` 为纯虚方法必须实现（非多面体网格返回 0），其余方法保持默认实现（表示非多面体）即可。

**生命周期与报告**：`GetMeshType`（网格类型）、`GetName`（名称，用于报告）、`ResetInput`（释放本次输入与转换视图缓存）、`Abort`（失败路径释放临时状态）、`ReleaseConvertedInputs`（释放转换产生的临时数据）；`GetSourceByteSizeHint` / `GetSourceLocationHint` / `GetEncodeStatusInfo` 可选。

### 5.2 IEncodeAttrView（属性视图）

单个属性的只读视图：

- `GetName` / `GetDataType` / `IsDataTypeSupported`：名称、底层标量类型、类型能否按原样进入 DataCodec；
- `GetRole` / `GetAttachType`：语义角色（标量 / 向量 / 法线 / 纹理坐标 / 张量 / 颜色）、附着位置（点 / 单元）；
- `GetComponentCount` / `GetElementCount`：分量数与元素数；
- `TryGetRawPtr`：连续内存零拷贝快路径（可选，不能提供返回空）；
- `GetTuple(index, double*)`：逐元素读取回退路径（必须可用）。

### 5.3 IBlockTreeAdapter（编码侧块树适配器）

- `EnumerateLeafPaths(visitor)`：枚举全部叶子块路径；
- `GetLeaf(path)`：为指定块路径创建叶子适配器；
- `GetRootName`：根名称（可选）；
- `GetLeafRecords` / `GetBranchRecords`：叶子 / 分支记录（路径 + 名称），默认实现基于枚举推导。

### 5.4 IDecodeAdapter（解码侧叶子接收器）

解码流水线按「开始 → 分片写入 → 结束」的阶段序列调用，适配器在目标原生对象上组装结果。除能力声明与单元类型映射（均有默认实现）外，本接口方法均为纯虚，必须全部实现；不支持的形态实现为「返回错误 / 返回空结果」即可，解码流水线会按返回值处理：

**几何**：`SetMeshType(type)`（解码流水线首先调用，创建或切换目标原生对象）、`BeginPoints(count, dimension)`、`WritePointsRange(offset, count, data)`、`EndPoints()`。点坐标以 float 分片交付，`dimension` 必须为 3。

**拓扑**：`BeginTopology(cellCount, connectivityCount, hasOffsets)`、`WriteConnectivityRange` / `WriteOffsetsRange` / `WriteCellTypesRange` / `WriteCellPolynomialOrdersRange`、`EndTopology()`（结束阶段完成原生单元数组组装）；结构化网格走 `SetStructuredAxisSize`。

**多面体（可选）**：`SupportsPolyhedronTopology()` 声明支持后，`BeginPolyhedronTopology` / `WritePolyhedronCellBatch` / `EndPolyhedronTopology` 按块接收多面体拓扑视图。不支持的形态在接口处返回错误即可（Feature_Guide 所述渐进式接入）。

**属性**：`BeginAttribute(attrIndex, meta)`（按元数据创建数组）、`WriteAttributeRange(attrIndex, offset, count, data, byteSize)`（按范围写入原始字节）、`EndAttribute(attrIndex)`（挂接并恢复语义角色）。

**能力声明**：`SupportsConcurrentAttributeRangeWrites()`（不同属性的 range 是否可并发写入）、`SupportsAttributeDecodeStore()` / `CreateAttributeDecodeStore`（是否允许 DataCodec 把解码缓存直接放入原生属性存储）。声明决定解码流水线的并行与写入策略。

**收尾**：`Commit()`（校验并提交结果）、`TakeOutput` 类方法（调用方取回对象）、`ResetOutput()` / `Abort()`（清理半成品，失败路径保证恰好调用一次）。

### 5.5 IFramePackageDecodeAssembly（解码侧帧组装器）

按帧包结构逐段接收：`BeginFramePackage(framePackage)` → `AddBranch(branch)`（分支节点）→ 对每个叶子 `CreateLeafAdapter(leaf, leafPackage)`（创建叶子解码器）→ `CommitLeaf(leaf, adapter)`（叶子解码完成提交）→ `EndFramePackage()`；失败路径 `AbortFramePackage()`。

若同时实现 `IDecodedFrameAssembly`（继承上述接口并增加两项），可获得帧级能力：

- `CreateSupplementAdapter(path)`：为已恢复帧补充读取未加载属性；
- `Payload()`：把组装结果封装为 `IDecodedFramePayload` 供完整帧缓存（按帧索引 LRU 缓存、复用）使用。配套的 `IDecodedFrameAssemblyFactory` 以 `CacheIdentity()` 标识帧组装结果的稳定形式，用于跨调用缓存隔离。

### 5.6 IByteRangeReader / IByteRangeOutput（字节范围 IO）

**读取器**：

- `ByteSize()`、`ReadAt(offset, span, error)`：必填，按范围读取；
- `ContiguousRange(offset, byteSize)`：可选，提供连续只读区间（生命周期由读取器保证，空 span 表示不支持）；
- `PrepareContiguousRange`：可选，带错误语义的连续区间协商（Ready / Unavailable / Error）；
- `RetainAllBytes()`：可选，返回共享所有权的完整字节（内存读取器可复用既有字节）；
- `PrefetchRange(offset, byteSize)`：可选，提前准备映射页面（平台预取）。

**输出**：`WriteAt(offset, span, error)` 按范围随机写，`Finalize(logicalSize, error)` 收尾（落盘、截断到逻辑长度）。

### 5.7 可选接口

- `IParallelTaskRunner` / `IParallelTaskGroup`：向 DataCodec 提供并行执行能力（`CreateGroup(stopToken)`、`Submit`、`Wait`、`Concurrency`）。不提供时以内联执行器兜底；
- `IDataCodecUiSink` / `IDataCodecConsoleSink` / `IDataCodecProgressSink` / `IDataCodecReportFileSink`：四类输出接收器，注入 `outputSinks` 后接收状态、进度与报告文件；
- `IRunRecordSink`：运行记录接收器（运行生命周期、阶段耗时、资源用量、重排顺序等，可按位掩码订阅）。

---

## 6. 关键参数速查

### 6.1 编码请求（EncodeRequest）

| 字段 | 说明 |
| --- | --- |
| `input` | 输入适配器与身份元数据（工厂构造） |
| `output` | 包形态与输出目标（Memory / ByteRange） |
| `attributeSelection` | 属性选择模式（默认 AllAvailable） |
| `attributeTargets` | 显式属性目标（Explicit 模式） |
| `configuration` | 编码配置（默认档 Balanced） |
| `outputSinks` / `runRecordSink` | 观测接收（可选） |
| `executionResources` | 并行执行器（可选） |

### 6.2 解码请求（DecodePackageRequest）

| 字段 | 说明 |
| --- | --- |
| `inputReader` | 输入读取器（必须） |
| `leafAdapter` / `frameAssembly` | 按包形态提供的结果接收器 |
| `framePackageMetadata` | 预读的帧包元数据（可选） |
| `requestedFrameIndex` | 多帧随机访问目标帧（可选） |
| `attributeSelection` / `attributeTargets` | 属性选择（默认全量） |
| `topologyReferenceKey` / `topologyOwnerFrameIndex` | 拓扑复用定位（可选） |
| `configuration` | 包级解码配置 |
| `session` | 多帧会话缓存（可选） |
| `stopToken` | 取消令牌（可选） |
| `executionResources` | 并行执行器（可选） |

### 6.3 结果字段

- `EncodeResult`：`success`、`hasEncodedOutput`、`encodedBytes`、`encodedByteCount`、`leafCount`、`packageKind`、`messages`；
- `DecodePackageResult`：`success`、`cancelled`、`decodedFramePackage`、`inputBytes`、`messages`。

---

## 7. 注意事项

- **契约前置校验**：入口对「包形态 × 适配器类型」、「显式属性模式 × 目标列表」做一致性校验，不匹配直接失败，无需等待压缩流程；
- **32 位拓扑上限**：点索引与连接表使用 32 位无符号索引，超出容量的网格在适配器初始化阶段即应拒绝（第三方适配器建议主动核对）；
- **点坐标交付形态**：解码接口以 float 分片交付点坐标；编码侧坐标可为 float 或 double（快路径按实际存储类型提供，回退路径声明标量类型）；
- **能力声明决定协作方式**：解码适配器务必如实声明并发写入与直存支持，声明会改变解码流水线的并行策略；不支持的多面体 / 结构化 / 多项式阶形态在接口处返回错误即可，不必一次覆盖全部能力；
- **属性索引扁平化**：显式属性目标使用扁平索引，点属性在前、单元属性在后，编码与解码两侧一致；
- **执行资源缺省**：不提供并行执行器时以内联方式执行，功能与结果不变，适合单线程环境（如部分 Wasm 构建）；
- **多帧场景**：单次 `DecodePackage` 解码一个文件包；多帧播放、预取与随机访问需要帧会话（`DecodeSession`）、帧缓存与帧组装器的组合使用，浏览器侧的实现参考 `Examples/Wasm` 与 `Wasm_Example_Optimization_Guide.md`；
- **语言一致性**：配置中的 `language` 决定消息语言；中英文统一切换。
