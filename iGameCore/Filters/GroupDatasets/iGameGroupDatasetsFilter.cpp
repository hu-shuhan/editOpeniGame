#include "iGameGroupDatasetsFilter.h"

#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

namespace {

// 递归克隆的深度上限：防御异常/成环的子对象图（正常装配体的层级远小于此）。
constexpr int kCloneMaxDepth = 64;

// 把名字列表拼成"甲、乙、丙"（用于提示信息）。
std::string JoinNames(const std::vector<std::string>& names) {
    std::string result;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) { result += "、"; }
        result += names[i];
    }
    return result;
}

// 生成"源配色器"的独立拷贝（伪彩隔离用）：
// 克隆带着源的色带/范围/标量模式出生，但此后各持一份、互不影响。
// 不改动框架原有的伪彩逻辑 —— 只是把 AddSubDataObject 默认写入的"父容器配色器"
// 替换回克隆自己的一份。
ScalarsToColors::Pointer MakeIndependentMapper(const DataObject::Pointer& source) {
    if (!source) { return nullptr; }
    auto srcMapper = source->GetColorMapper();
    if (!srcMapper) { return nullptr; }
    auto mapper = ScalarsToColors::New();
    if (!mapper->DeepCopy(srcMapper)) { return nullptr; }
    return mapper;
}

// 深拷贝时间序列（动画帧）：复制帧清单（时间值 + 元数据 + 类型），不复制运行期缓存
// （缓存是读盘/插值优化，按需重建；Qt 动画模块的 independentFrames 同此做法）。
StreamingData::Pointer DeepCopyStreamingData(StreamingData::Pointer source) {
    if (!source) { return nullptr; }
    auto framesCopy = StreamingData::New();
    for (auto& frame: source->GetArrays()) {
        auto metadataCopy = StringArray::New();
        if (auto metadata = frame.GetMetaData()) {
            for (IGsize i = 0; i < metadata->GetNumberOfElements(); ++i) {
                metadataCopy->AddElement(metadata->GetElement(i));
            }
        }
        framesCopy->AddTimeStep(frame.GetTimeValue(), metadataCopy, frame.GetFrameType());
    }
    return framesCopy;
}

// 深拷贝一个数据数组对象（New + DeepCopy + 保留名字）；空指针或拷贝失败返回 nullptr。
// 骨架与 iGameCountCellFacesFilter 的 CopyArray 一致（该模式已在多处生产代码验证）。
template <class T>
typename T::Pointer DeepCopyTyped(typename T::Pointer source) {
    if (source.IsNull()) { return nullptr; }
    auto copy = T::New();
    if (!copy->DeepCopy(source)) { return nullptr; }
    copy->SetName(source->GetName());
    return copy;
}

// 按运行期类型分派地深拷贝一个 ArrayObject（属性数组用）。
// 未知类型返回 nullptr，调用方按"该输入不可克隆"跳过。
ArrayObject::Pointer DeepCopyArrayObject(const ArrayObject::Pointer& source) {
    if (source.IsNull()) { return nullptr; }
    switch (source->GetArrayType()) {
#define IG_DEEP_COPY_ARRAY_CASE(Type) \
    case IG_##Type: return DeepCopyTyped<Type>(DynamicCast<Type>(source));
        IG_DEEP_COPY_ARRAY_CASE(FloatArray)
        IG_DEEP_COPY_ARRAY_CASE(DoubleArray)
        IG_DEEP_COPY_ARRAY_CASE(IntArray)
        IG_DEEP_COPY_ARRAY_CASE(UnsignedIntArray)
        IG_DEEP_COPY_ARRAY_CASE(CharArray)
        IG_DEEP_COPY_ARRAY_CASE(UnsignedCharArray)
        IG_DEEP_COPY_ARRAY_CASE(ShortArray)
        IG_DEEP_COPY_ARRAY_CASE(UnsignedShortArray)
        IG_DEEP_COPY_ARRAY_CASE(LongLongArray)
        IG_DEEP_COPY_ARRAY_CASE(UnsignedLongLongArray)
#undef IG_DEEP_COPY_ARRAY_CASE
        default: return nullptr;
    }
    return nullptr;
}

// 深拷贝整张属性表（新旧模型属性互不影响）。
//
// ⚠️ 不能直接用框架的 AttributeSet::DeepCopy / Attribute::DeepCopy：
//    它们内部只处理 FloatArray / DoubleArray，整数属性（如 block_id）会被静默丢掉。
//    这里逐条按运行期类型深拷贝，并复制 dataRange 与范围锁定元数据。
//
// 下标策略：拷贝与源表【1:1 对位】—— 已删除/空项用已删除占位保留，
// 这样 DataObject 的活动标量下标（m_AttributeIndex）在克隆上继续指向同一属性。
// blockMappingIndex（源表 block mapping 下标）对应的项跳过不放进表中，
// 由调用方用 mappingCopy 在目标表上重建（见 CopyCommonFields）。
AttributeSet::Pointer DeepCopyAttributeSet(AttributeSet* src, int blockMappingIndex,
                                           IntArray::Pointer& mappingCopy) {
    if (!src) { return nullptr; }
    auto copy = AttributeSet::New();
    for (IGsize i = 0; i < static_cast<IGsize>(src->GetNumberOfAttributes()); ++i) {
        const auto& attr = src->GetAttribute(i);

        if (attr.isDeleted || !attr.pointer) {
            // 占位：保持与源表 1:1 的下标对应
            auto placeholder = AttributeSet::Attribute::None();
            placeholder.Delete();
            copy->GetAllAttributes()->AddElement(placeholder);
            continue;
        }

        auto arrayCopy = DeepCopyArrayObject(attr.pointer);
        if (!arrayCopy) { return nullptr; } // 不支持的数组类型 → 整个输入跳过

        if (static_cast<int>(i) == blockMappingIndex) {
            // 块映射项：只取出深拷贝的数组，不直接放进表（索引语义由 SetBlockMapping 重建）。
            mappingCopy = DynamicCast<IntArray>(arrayCopy);
            continue;
        }

        DoubleArray::Pointer rangeCopy;
        if (attr.dataRange) {
            rangeCopy = DeepCopyTyped<DoubleArray>(attr.dataRange);
            if (!rangeCopy) { return nullptr; }
        }

        const IGsize added = copy->AddAttribute(attr.type, attr.attachmentType, arrayCopy, rangeCopy);
        auto& addedAttr = copy->GetAttribute(added);
        addedAttr.rangeLocked = attr.rangeLocked;
        addedAttr.rangeMode = attr.rangeMode;
        addedAttr.rangeLockedDimension = attr.rangeLockedDimension;
        addedAttr.runningMin = attr.runningMin;
        addedAttr.runningMax = attr.runningMax;
        addedAttr.runningRangeValid = attr.runningRangeValid;
    }
    return copy;
}

// 复制与结构无关的公共字段：名字、属性表（深拷贝）、活动标量下标、时间序列、块映射、显示标量。
// 返回 false = 该输入无法完整克隆（调用方按"跳过"处理）。
//
// ⚠️ 显示标量必须在【挂子对象之前】复制 —— SetVisibility / SetViewStyle / SetPointSize /
//    SetLineWidth / SetTransparency 都会向子对象传播，若容器已有子对象再调用，
//    会把各子对象自己的独立显示状态覆盖掉。
bool CopyCommonFields(const DataObject::Pointer& src, const DataObject::Pointer& dst) {
    dst->SetName(src->GetName());

    // 属性表：深拷贝（负责人拍板：新旧模型属性不能一起变）。
    IntArray::Pointer mappingCopy;
    auto attributesCopy = DeepCopyAttributeSet(src->GetAttributeSet(),
                                               src->GetBlockMappingAttrIndex(), mappingCopy);
    if (!attributesCopy) { return false; }
    dst->SetAttributeSet(attributesCopy);
    // 块映射在克隆自己的表里重建：HasBlockMapping()/GetBlockMapping() 与源行为一致，
    // 且指向的是拷贝出的数组（改克隆不影响源）。
    if (mappingCopy) { dst->SetBlockMapping(mappingCopy); }

    // 着色状态（活动标量下标/维度/着色开关）【不复制】——
    // m_AttributeIndex 的语义是"已执行过 ViewCloudPicture 的记录"，不是普通输入字段：
    // 直拷会伪造该记录，并绕过 ViewCloudPicture 内的 dataRange 共享、壳传播与
    // m_ColorWithCell 推导，产生不自洽状态（用户点击时还可能命中其早退检查而整个返回，
    // 该早退是"子块点属性不渲染"阻塞 bug 的 root cause，见代码审查记录）。
    // 克隆以未着色状态出生（-1/false，与"新加载的模型未点属性前"一致），
    // 由用户操作经 ViewCloudPicture 全链路建立着色。
    // 时间序列（动画帧）：深拷贝帧清单（时间值/元数据/类型各持一份，互不影响）；
    // 运行期缓存不复制（按需重建）——与 Qt 动画模块 independentFrames 的既定做法一致。
    if (auto srcFrames = src->PeekTimeFrames()) {
        if (auto framesCopy = DeepCopyStreamingData(srcFrames)) {
            dst->SetTimeFrames(framesCopy);
        }
    }

    // 显示标量：克隆带着源对象当前的观感出生；此后彼此独立。
    // （配色器同理由"源拷贝"出生，但它必须在挂组之后设置 —— AddSubDataObject 会覆盖，
    //   见 Execute 与容器挂载处。）
    auto srcDraw = DynamicCast<DrawObject>(src);
    auto dstDraw = DynamicCast<DrawObject>(dst);
    if (srcDraw && dstDraw) {
        dstDraw->SetVisibility(srcDraw->GetVisibility());
        dstDraw->SetViewStyle(static_cast<IGenum>(srcDraw->GetViewStyle()));
        dstDraw->SetPointSize(static_cast<float>(srcDraw->GetPointSize()));
        dstDraw->SetLineWidth(static_cast<float>(srcDraw->GetLineWidth()));
        const float transparency = srcDraw->GetTransparency();
        if (transparency >= 0.0f && transparency <= 1.0f) { // SetTransparency 越界会抛异常
            dstDraw->SetTransparency(transparency);
        }
        dstDraw->SetAccelerationOption(srcDraw->GetAccelerationOption());
    }
    return true;
}

// 为 src 建一个【同类型深拷贝克隆】：几何、属性表、显示状态全部独立，源对象零修改。
// 返回 nullptr 表示该输入无法克隆（类型暂不支持 / 嵌套过深 / 深拷贝失败 / 子块全部不支持），
// 已把名字记入 skippedNames。
DataObject::Pointer CreateDeepClone(const DataObject::Pointer& src,
                                    int depth,
                                    std::vector<std::string>& skippedNames) {
    if (!src) { return nullptr; }
    if (depth > kCloneMaxDepth) {
        skippedNames.push_back(src->GetName());
        return nullptr;
    }

    // 容器（多块组 / 组合对象）：统一建 DrawObject 壳并递归克隆子树（克隆里建克隆）。
    // 注意：不复用子对象的克隆指针 —— 直接挂现有子对象会劫持它们的父指针。
    if (src->HasSubDataObject()) {
        auto container = DrawObject::New();
        if (!CopyCommonFields(src, container)) { // 先复制显示标量（此时无子对象，不会误传播）
            skippedNames.push_back(src->GetName());
            return nullptr;
        }
        bool anyChildCloned = false;
        for (auto it = src->SubDataObjectIteratorBegin(); it != src->SubDataObjectIteratorEnd(); ++it) {
            if (auto childClone = CreateDeepClone(it->second, depth + 1, skippedNames)) {
                container->AddSubDataObject(childClone);
                // 挂载会把容器的配色器写给子克隆 —— 立刻换回"子对象自己源配色器的独立拷贝"。
                // 必须连壳/简化网格一起换（SetColorMapperForRenderables）：壳在建壳瞬间
                // 绑定的是当时本体的配色器（=容器的默认 mapper），而 SetColorMapper 是浅
                // setter 不会覆盖它 —— 否则子块伪彩会一直用容器的默认色带/范围。
                if (auto mapper = MakeIndependentMapper(it->second)) {
                    if (auto childDraw = DynamicCast<DrawObject>(childClone)) {
                        childDraw->SetColorMapperForRenderables(mapper);
                    } else {
                        childClone->SetColorMapper(mapper);
                    }
                }
                anyChildCloned = true;
            }
        }
        if (!anyChildCloned) {
            // 子块全部不支持：空容器渲染不出任何东西，按"跳过"处理。
            skippedNames.push_back(src->GetName());
            return nullptr;
        }
        return container;
    }

    DataObject::Pointer clone = nullptr;
    switch (src->GetDataObjectType()) {
        case IG_POINT_SET: {
            auto srcPointSet = DynamicCast<PointSet>(src);
            if (!srcPointSet) { break; }
            auto pointsCopy = DeepCopyTyped<Points>(srcPointSet->GetPoints());
            if (!pointsCopy) { break; }
            auto pointSetClone = PointSet::New();
            pointSetClone->SetPoints(pointsCopy);
            clone = pointSetClone;
            break;
        }
        case IG_SURFACE_MESH: {
            auto srcSurface = DynamicCast<SurfaceMesh>(src);
            if (!srcSurface) { break; }
            auto pointsCopy = DeepCopyTyped<Points>(srcSurface->GetPoints());
            if (!pointsCopy) { break; }
            auto surfaceClone = SurfaceMesh::New();
            surfaceClone->SetPoints(pointsCopy);
            if (auto* faces = srcSurface->GetFaces()) {
                auto facesCopy = DeepCopyTyped<CellArray>(CellArray::Pointer(faces));
                if (!facesCopy) { break; }
                surfaceClone->SetFaces(facesCopy);
            }
            if (auto* edges = srcSurface->GetEdges()) {
                auto edgesCopy = DeepCopyTyped<CellArray>(CellArray::Pointer(edges));
                if (!edgesCopy) { break; }
                surfaceClone->SetEdges(edgesCopy);
            }
            clone = surfaceClone;
            break;
        }
        case IG_VOLUME_MESH: {
            auto srcVolume = DynamicCast<VolumeMesh>(src);
            if (!srcVolume) { break; }
            auto pointsCopy = DeepCopyTyped<Points>(srcVolume->GetPoints());
            if (!pointsCopy) { break; }
            auto volumeClone = VolumeMesh::New();
            volumeClone->SetPoints(pointsCopy);
            // VolumeMesh 继承自 SurfaceMesh：面/边数组若存在也一并深拷贝。
            if (auto* faces = srcVolume->GetFaces()) {
                auto facesCopy = DeepCopyTyped<CellArray>(CellArray::Pointer(faces));
                if (!facesCopy) { break; }
                volumeClone->SetFaces(facesCopy);
            }
            if (auto* edges = srcVolume->GetEdges()) {
                auto edgesCopy = DeepCopyTyped<CellArray>(CellArray::Pointer(edges));
                if (!edgesCopy) { break; }
                volumeClone->SetEdges(edgesCopy);
            }
            if (auto* volumes = srcVolume->GetVolumes()) {
                auto volumesCopy = DeepCopyTyped<CellArray>(CellArray::Pointer(volumes));
                if (!volumesCopy) { break; }
                volumeClone->SetVolumes(volumesCopy);
            }
            clone = volumeClone;
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto srcUnstructured = DynamicCast<UnstructuredMesh>(src);
            if (!srcUnstructured) { break; }
            auto pointsCopy = DeepCopyTyped<Points>(srcUnstructured->GetPoints());
            if (!pointsCopy) { break; }
            auto cellsCopy = DeepCopyTyped<CellArray>(srcUnstructured->GetCells());
            if (srcUnstructured->GetCells() && !cellsCopy) { break; }
            auto typesCopy = DeepCopyTyped<UnsignedIntArray>(
                    UnsignedIntArray::Pointer(srcUnstructured->GetCellTypes()));
            if (srcUnstructured->GetCellTypes() && !typesCopy) { break; }
            auto meshClone = UnstructuredMesh::New();
            meshClone->SetPoints(pointsCopy);
            meshClone->SetCells(cellsCopy, typesCopy);
            clone = meshClone;
            break;
        }
        default:
            // StructuredMesh / Lagrange / Spline / 编码类等：暂不支持（见头文件说明）。
            break;
    }

    if (!clone) {
        skippedNames.push_back(src->GetName());
        return nullptr;
    }

    if (!CopyCommonFields(src, clone)) {
        skippedNames.push_back(src->GetName());
        return nullptr;
    }
    return clone;
}

} // namespace

GroupDatasetsFilter::GroupDatasetsFilter() {
    // 输入个数由 SetInputs() 动态决定，这里先置 0；
    // 输出恒为 1 个（多块组本身）。基类 SetOutput 有边界检查，
    // 若不预留输出槽，SetOutput(0, ...) 会静默失败。
    this->SetNumberOfInputs(0);
    this->SetNumberOfOutputs(1);
}

void GroupDatasetsFilter::SetInputs(const std::vector<DataObject::Pointer>& objs) {
    // 过滤空指针，并按对象指针去重 —— 同一个对象被重复选中（如多行选中同一对象）时只合一次。
    // 旧实现依赖底层 SubDataObjectsHelper 的"同一对象只保留一份"兜底；
    // 克隆方案下若不去重，同一源对象会被建出多个克隆，兜底不再生效。
    std::vector<DataObject::Pointer> valid;
    valid.reserve(objs.size());
    for (const auto& obj: objs) {
        if (!obj) { continue; }
        bool duplicated = false;
        for (const auto& kept: valid) {
            if (kept.GetPointer() == obj.GetPointer()) {
                duplicated = true;
                break;
            }
        }
        if (!duplicated) { valid.push_back(obj); }
    }

    this->SetNumberOfInputs(static_cast<int>(valid.size()));
    for (int i = 0; i < static_cast<int>(valid.size()); ++i) {
        this->SetInput(i, valid[i]);
    }
}

bool GroupDatasetsFilter::Execute() {
    m_Message.clear();
    m_GroupedCount = 0;
    m_BlockNames.clear();
    m_SkippedNames.clear();

    const int inputCount = this->GetNumberOfInputs();
    if (inputCount <= 0) {
        m_Message = "没有输入对象：请先在模型树中选中至少一个对象（可按住 Ctrl / Shift 多选）。";
        return false;
    }

    // 多块复合容器必须用 DrawObject 创建：
    // 该对象最终会交给 Scene 渲染，Scene 会把它当作 DrawObject 使用，
    // 若用裸 DataObject::New()，后续 DynamicCast<DrawObject> 会失败。
    auto group = DrawObject::New();
    if (!group) {
        m_Message = "创建多块组失败（内存分配失败）。";
        return false;
    }
    if (!m_OutputName.empty()) { group->SetName(m_OutputName); }

    for (int i = 0; i < inputCount; ++i) {
        auto obj = this->GetInput(i);
        if (!obj) { continue; } // 防御：基类已做边界检查，这里再挡一次空指针

        // 容器组合：为每个输入建【同类型深拷贝克隆】再挂入组 ——
        // 几何/属性表/显隐样式全部各持一份（新旧对象互不影响），源对象零修改。
        // （旧实现直接挂原对象：AddSubDataObject 会劫持其父指针，
        //   删除组后源对象父指针悬空 —— 评审 P1，导出路径曾实测崩溃。）
        auto clone = CreateDeepClone(obj, 1, m_SkippedNames);
        if (!clone) { continue; } // 不支持/克隆失败的类型已记入 m_SkippedNames

        // 克隆在"装配完成"状态下再挂组：AddSubDataObject 会 SetParent + 触发转可绘制数据，
        // 并把【组的配色器】写给克隆。随后立刻换回克隆自己的一份（源配色器的独立拷贝）——
        // 伪彩与源、与组都不共享；框架原有伪彩逻辑不动。
        // 必须用 SetColorMapperForRenderables：AddSubDataObject 先写 mapper、后建壳
        // （见 iGameDataObject.cpp），壳会绑定到"组的默认 mapper"，仅换本体的浅 setter
        // 不会覆盖壳 —— 这是"子块伪彩颜色与源不一致/发白"的根因。
        group->AddSubDataObject(clone);
        if (auto mapper = MakeIndependentMapper(obj)) {
            if (auto cloneDraw = DynamicCast<DrawObject>(clone)) {
                cloneDraw->SetColorMapperForRenderables(mapper);
            } else {
                clone->SetColorMapper(mapper);
            }
        }

        m_BlockNames.push_back(clone->GetName());
        ++m_GroupedCount;
    }

    if (m_GroupedCount == 0) {
        if (!m_SkippedNames.empty()) {
            m_Message = "所选对象无法合组（类型暂不支持或克隆失败），未生成多块组：" +
                        JoinNames(m_SkippedNames);
        } else {
            m_Message = "选中的对象全部不可用，未生成多块组。";
        }
        return false;
    }
    if (!m_SkippedNames.empty()) {
        // 部分成功：给出可选提示（UI 可改用 GetSkippedNames() 逐项展示）。
        m_Message = "以下对象未合入多块组（类型暂不支持或克隆失败）：" + JoinNames(m_SkippedNames);
    }

    this->SetOutput(0, group);
    return true;
}

IGAME_NAMESPACE_END
