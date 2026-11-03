#include "iGameForceStaticMeshFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDataObject.h"
#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace {
// 深拷贝输入网格，生成一个独立的新网格作为静态缓存块
iGame::DataObject::Pointer CloneMesh(iGame::DataObject::Pointer input) {
    using namespace iGame;
    if (input == nullptr) return nullptr;

    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto pts = Points::New();
        pts->DeepCopy(src->GetPoints());
        return pts;
    };
    auto copyAttrs = [](DataObject* src) -> AttributeSet::Pointer {
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(src->GetAttributeSet());
        return attrs;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            auto dst = SurfaceMesh::New();
            dst->SetPoints(copyPoints(src));
            if (src->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(src->GetFaces());
                dst->SetFaces(faces);
            }
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            auto cells = CellArray::New();
            cells->DeepCopy(src->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(src->GetCellTypes());
            dst->SetCells(cells, types);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            auto dst = VolumeMesh::New();
            dst->SetPoints(copyPoints(src));
            auto vols = CellArray::New();
            vols->DeepCopy(src->GetVolumes());
            dst->SetVolumes(vols);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            auto dst = StructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_POINT_SET: {
            auto src = DynamicCast<PointSet>(input);
            auto dst = PointSet::New();
            dst->SetPoints(copyPoints(src));
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        default:
            break;
    }

    // 兜底：未在 switch 中列举的派生类型（如 Lagrange 单元网格、网格子类），
    // 退化为「点 + 属性」的静态表示，保证缓存流程可用而不是直接失败。
    if (auto ps = DynamicCast<PointSet>(input)) {
        auto dst = PointSet::New();
        dst->SetPoints(copyPoints(ps));
        dst->SetAttributeSet(copyAttrs(ps));
        dst->SetName(ps->GetName());
        return dst;
    }
    return nullptr;
}
} // namespace

IGAME_NAMESPACE_BEGIN

ForceStaticMeshFilter::ForceStaticMeshFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ForceStaticMeshFilter::Execute() { return ExecuteInternal(false, -1, 0.f); }

bool ForceStaticMeshFilter::ExecuteAtTimeStep(int frameIndex, float timeValue) {
    return ExecuteInternal(true, frameIndex, timeValue);
}

bool ForceStaticMeshFilter::ExecuteInternal(bool timeSeriesMode, int frameIndex, float timeValue) {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_StatusMessage = "输入为空，无法建立静态网格缓存。";
        return false;
    }

    m_LastRebuilt = false;
    m_LastRebuildReason = RR_NONE;
    m_StatusMessage.clear();
    std::string sizeChangedMessage; // 规模变化的专用文案（含旧 → 新规模）

    // 关闭静态缓存：不做任何缓存，输出直接跟随输入（几何随输入变化）
    if (!m_StaticCacheEnabled) {
        SetOutput(input);
        m_StatusMessage = "静态缓存已关闭：输出跟随输入网格，几何随时间步变化。";
        return true;
    }

    if (DynamicCast<PointSet>(input) == nullptr && !input->HasSubDataObject()) {
        m_StatusMessage = "当前对象不支持静态网格（需要网格 / 点集）。";
        return false;
    }

    auto sources = CollectSourceBlocks(input);
    if (sources.empty()) {
        m_StatusMessage = "输入没有可缓存的数据块。";
        return false;
    }

    // 记录缓存几何对应的时间步：手动执行不携带时间步上下文
    const int blockTimeStep = timeSeriesMode ? frameIndex : -1;
    const float blockTimeValue = timeSeriesMode ? timeValue : 0.f;
    const bool hasTimeStepContext = (blockTimeStep >= 0);

    // 1) 判定是否需要整体重建
    bool rebuildAll = false;
    RebuildReason rebuildReason = RR_NONE;
    if (m_ForceCacheComputation) {
        rebuildAll = true;
        rebuildReason = RR_FORCED;
    } else if (!m_CacheInitialized || m_Blocks.empty()) {
        rebuildAll = true;
        rebuildReason = RR_FIRST_BUILD;
    } else if (m_Blocks.size() != sources.size()) {
        // 块结构改变（例如时序数据换了块数）：旧缓存无法对应，整体重建
        rebuildAll = true;
        rebuildReason = RR_INPUT_CHANGED;
    } else if (!timeSeriesMode && (m_CachedInput == nullptr || m_CachedInput.get() != input.get())) {
        // 手动执行时切换了输入对象：即使点数/单元数相同也必须重建，
        // 否则会错误复用另一个模型的几何。
        rebuildAll = true;
        rebuildReason = RR_INPUT_CHANGED;
    }

    if (rebuildAll) {
        // 保留旧缓存块对象：类型一致时在它身上原地重建几何，保持输出对象身份，
        // 界面（模型 / 模型树 / 场景引用）无需替换数据对象。
        std::vector<CacheBlock> previousBlocks;
        previousBlocks.swap(m_Blocks);
        m_Blocks.assign(sources.size(), CacheBlock{});
        for (size_t i = 0; i < sources.size(); ++i) {
            if (i < previousBlocks.size()) { m_Blocks[i].Cache = previousBlocks[i].Cache; }
            RebuildBlock(m_Blocks[i], sources[i], blockTimeStep, blockTimeValue);
        }
        m_LastRebuilt = true;
        m_LastRebuildReason = rebuildReason;
        if (hasTimeStepContext) { m_CacheHasTimeStep = true; }
    } else {
        // 2) 逐块复用：规模一致只更新属性；规模变化自动重建该块
        for (size_t i = 0; i < sources.size(); ++i) {
            auto& block = m_Blocks[i];
            const auto& src = sources[i];
            if (block.Cache == nullptr) {
                RebuildBlock(block, src, blockTimeStep, blockTimeValue);
                m_LastRebuilt = true;
                m_LastRebuildReason = RR_FIRST_BUILD;
                if (hasTimeStepContext) { m_CacheHasTimeStep = true; }
                continue;
            }

            const IGsize newPoints = CountPoints(src);
            const IGsize newCells = CountCells(src);
            if (newPoints != block.PointCount || newCells != block.CellCount) {
                char buffer[256];
                std::snprintf(buffer, sizeof(buffer),
                              "检测到几何规模变化（%llu 点 / %llu 单元 → %llu 点 / %llu 单元），"
                              "已自动重新建立几何缓存。",
                              static_cast<unsigned long long>(block.PointCount),
                              static_cast<unsigned long long>(block.CellCount),
                              static_cast<unsigned long long>(newPoints),
                              static_cast<unsigned long long>(newCells));
                if (sizeChangedMessage.empty()) { sizeChangedMessage = buffer; }
                RebuildBlock(block, src, blockTimeStep, blockTimeValue);
                m_LastRebuilt = true;
                m_LastRebuildReason = RR_SIZE_CHANGED;
                if (hasTimeStepContext) { m_CacheHasTimeStep = true; }
            } else {
                UpdateAttributes(src, block.Cache);
            }
        }
    }

    // 3) 组装输出：单块输出缓存网格本身；多块输出统一的容器对象
    DataObject::Pointer out;
    if (m_Blocks.size() == 1) {
        out = m_Blocks[0].Cache;
    } else if (SyncContainerChildren()) {
        out = m_Cache;
    } else {
        out = m_Blocks[0].Cache;
    }
    if (out == nullptr) {
        m_StatusMessage = "静态网格缓存建立失败。";
        return false;
    }
    // 多块：容器的属性集要与首块对齐，否则模型树 / 云图取不到属性
    if (out->HasSubDataObject()) { SyncContainerAttributes(out); }

    // 4) 属性 / 几何更新后，缓存块与其渲染数据都必须标记为已修改
    for (auto& block: m_Blocks) { MarkObjectModified(block.Cache); }
    MarkObjectModified(out);

    m_CachedInput = input;
    m_CacheInitialized = true;

    // 5) 状态文案（规模变化的分支优先，其余按重建原因给出说明）
    if (!sizeChangedMessage.empty()) {
        m_StatusMessage = sizeChangedMessage;
        if (m_Blocks.size() > 1) { m_StatusMessage += "（多块数据：按块重建）"; }
    } else if (m_LastRebuilt) {
        switch (m_LastRebuildReason) {
            case RR_FORCED:
                m_StatusMessage = "已重新建立几何缓存（强制重建）：" + GetCacheDescription() + "。";
                break;
            case RR_INPUT_CHANGED:
                m_StatusMessage = "输入模型已改变，已重新建立几何缓存：" + GetCacheDescription() + "。";
                break;
            case RR_FIRST_BUILD:
            default:
                m_StatusMessage = "已建立几何缓存：" + GetCacheDescription()
                        + "。后续时间步只更新属性，几何保持不变。";
                break;
        }
    } else {
        m_StatusMessage = "已复用几何缓存，仅更新属性数据：" + GetCacheDescription() + "。";
    }

    SetOutput(out);
    return true;
}

void ForceStaticMeshFilter::RebuildBlock(CacheBlock& block, const DataObject::Pointer& src,
                                         int timeStepIndex, float timeValue) {
    DataObject::Pointer rebuilt;
    // 优先原地重建几何：对象身份不变，模型树 / 场景中的模型无需被替换
    if (block.Cache != nullptr && RebuildMeshInPlace(block.Cache, src)) {
        rebuilt = block.Cache;
    } else {
        rebuilt = CloneMesh(src);
    }
    block.Cache = rebuilt;
    block.PointCount = CountPoints(src);
    block.CellCount = CountCells(src);
    block.TimeStepIndex = timeStepIndex;
    block.TimeValue = timeValue;
    if (block.Cache != nullptr) {
        MarkObjectModified(block.Cache);
    }
}

bool ForceStaticMeshFilter::RebuildMeshInPlace(const DataObject::Pointer& cache,
                                               const DataObject::Pointer& src) {
    if (cache == nullptr || src == nullptr) return false;
    // 具体类型必须一致，否则原地更新无法完成（交给上层新建对象）
    if (cache->GetDataObjectType() != src->GetDataObjectType()) return false;

    auto srcPs = DynamicCast<PointSet>(src);
    auto cachePs = DynamicCast<PointSet>(cache);
    if (srcPs == nullptr || cachePs == nullptr) return false;

    // 点坐标：整体替换为源网格的拷贝
    auto points = Points::New();
    points->DeepCopy(srcPs->GetPoints());
    cachePs->SetPoints(points);

    switch (cache->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto c = DynamicCast<SurfaceMesh>(cache);
            auto s = DynamicCast<SurfaceMesh>(src);
            auto faces = CellArray::New();
            if (s->GetFaces()) { faces->DeepCopy(s->GetFaces()); }
            c->SetFaces(faces);
            break;
        }
        case IG_VOLUME_MESH: {
            auto c = DynamicCast<VolumeMesh>(cache);
            auto s = DynamicCast<VolumeMesh>(src);
            auto volumes = CellArray::New();
            if (s->GetVolumes()) { volumes->DeepCopy(s->GetVolumes()); }
            c->SetVolumes(volumes);
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto c = DynamicCast<StructuredMesh>(cache);
            auto s = DynamicCast<StructuredMesh>(src);
            c->SetDimensionSize(s->GetDimensionSize());
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto c = DynamicCast<UnstructuredMesh>(cache);
            auto s = DynamicCast<UnstructuredMesh>(src);
            auto cells = CellArray::New();
            cells->DeepCopy(s->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(s->GetCellTypes());
            c->SetCells(cells, types);
            break;
        }
        default:
            break;
    }

    // 属性同样替换为源网格的当前属性
    UpdateAttributes(src, cache);
    MarkObjectModified(cache);
    return true;
}

bool ForceStaticMeshFilter::SyncContainerChildren() {
    if (m_Blocks.empty()) return false;

    if (m_Cache == nullptr) {
        m_Cache = DrawObject::New();
    }

    // 子块对象在「仅更新属性」时不变，此时不需要动容器结构，避免界面每帧重建模型树
    bool sameChildren = (m_Cache->GetNumberOfSubDataObjects() == static_cast<int>(m_Blocks.size()));
    if (sameChildren) {
        auto it = m_Cache->SubDataObjectIteratorBegin();
        for (const auto& block: m_Blocks) {
            if (it == m_Cache->SubDataObjectIteratorEnd() || it->second != block.Cache) {
                sameChildren = false;
                break;
            }
            ++it;
        }
    }

    if (!sameChildren) {
        m_Cache->ClearSubDataObject();
        for (auto& block: m_Blocks) {
            if (block.Cache != nullptr) { m_Cache->AddSubDataObject(block.Cache); }
        }
    }
    return true;
}

void ForceStaticMeshFilter::SyncContainerAttributes(const DataObject::Pointer& container) {
    if (container == nullptr || !container->HasSubDataObject()) return;

    auto firstSub = container->SubDataObjectIteratorBegin()->second;
    auto subAttr = firstSub ? firstSub->GetAttributeSet() : nullptr;
    auto parentAttr = container->GetAttributeSet();
    if (subAttr == nullptr || parentAttr == nullptr) return;

    // 容器自身只是壳，属性数据都在子块上；这里按同名补齐占位属性，
    // 让模型树 / 云图能按父容器的属性下标寻址（与 PVD 读取时的处理一致）。
    auto subAll = subAttr->GetAllAttributes();
    const IGsize subNum = subAll ? subAll->GetNumberOfElements() : 0;
    for (IGsize i = 0; i < subNum; ++i) {
        auto& attr = subAll->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) continue;

        const std::string name = attr.pointer->GetName();
        if (parentAttr->GetAttributeIndex(name) >= 0) continue;

        const int dim = attr.pointer->GetDimension();
        DoubleArray::Pointer placeholder = DoubleArray::New();
        placeholder->SetName(name);
        placeholder->SetDimension(dim);

        DoubleArray::Pointer range = DoubleArray::New();
        range->SetDimension(2);
        range->Resize(dim + 1);
        for (int j = 0; j < dim + 1; ++j) {
            range->SetElement(j, {DBL_MIN, DBL_MAX});
        }

        switch (attr.type) {
            case IG_SCALAR:
                parentAttr->AddScalar(attr.attachmentType, placeholder, range);
                break;
            case IG_VECTOR:
                parentAttr->AddVector(attr.attachmentType, placeholder, range);
                break;
            default:
                parentAttr->AddAttribute(attr.type, attr.attachmentType, placeholder, range);
                break;
        }
    }

    // 用子块本帧的实际数值刷新容器与子块的值域
    parentAttr->Modified();
    container->ReCollectSubDataObjectDataRange();
    container->UpdateSubDataObjectDataRange();
}

std::vector<DataObject::Pointer> ForceStaticMeshFilter::CollectSourceBlocks(const DataObject::Pointer& input) {
    std::vector<DataObject::Pointer> blocks;
    if (input == nullptr) return blocks;

    // 时序数据（MultiSubFiles）的输入是容器对象，真正的网格挂在子对象上
    if (input->HasSubDataObject()) {
        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it) {
            if (it->second) { blocks.push_back(it->second); }
        }
    }
    if (blocks.empty()) { blocks.push_back(input); }
    return blocks;
}

IGsize ForceStaticMeshFilter::CountPoints(const DataObject::Pointer& obj) {
    auto ps = DynamicCast<PointSet>(obj);
    return ps ? ps->GetNumberOfPoints() : 0;
}

IGsize ForceStaticMeshFilter::CountCells(const DataObject::Pointer& obj) {
    if (obj == nullptr) return 0;

    // 按继承层次从最派生往下判断：StructuredMesh → VolumeMesh → SurfaceMesh → UnstructuredMesh
    if (auto st = DynamicCast<StructuredMesh>(obj)) return st->GetNumberOfCells();
    if (auto vm = DynamicCast<VolumeMesh>(obj)) return vm->GetNumberOfVolumes();
    if (auto sm = DynamicCast<SurfaceMesh>(obj)) return sm->GetNumberOfFaces();
    if (auto um = DynamicCast<UnstructuredMesh>(obj)) return um->GetNumberOfCells();
    auto ca = obj->GetCellArray();
    return ca ? ca->GetNumberOfCells() : 0;
}

DataObject::Pointer ForceStaticMeshFilter::CloneMesh(const DataObject::Pointer& input) {
    return ::CloneMesh(input);
}

bool ForceStaticMeshFilter::UpdateAttributes(const DataObject::Pointer& src, const DataObject::Pointer& cache) {
    if (src == nullptr || cache == nullptr) return false;

    auto srcPs = DynamicCast<PointSet>(src);
    auto cachePs = DynamicCast<PointSet>(cache);
    if (srcPs == nullptr || cachePs == nullptr) return false;

    // 用输入的属性数据替换缓存中的属性，几何（点 / 单元）保持缓存不动
    auto attrs = AttributeSet::New();
    attrs->DeepCopy(srcPs->GetAttributeSet());
    cache->SetAttributeSet(attrs);
    return true;
}

void ForceStaticMeshFilter::MarkObjectModified(const DataObject::Pointer& obj) {
    if (obj == nullptr) return;

    obj->Modified();

    auto attrSet = obj->GetAttributeSet();
    if (attrSet != nullptr) { attrSet->Modified(); }

    // 渲染数据（抽壳网格 / GPU 缓冲）按新属性重建，否则云图仍显示旧值
    if (auto drawObj = DynamicCast<DrawObject>(obj)) {
        if (attrSet != nullptr) { attrSet->ForceReConvertToDrawableData(); }
        drawObj->ForceReConvertToDrawableData();
    }
}

bool ForceStaticMeshFilter::HasCache() const {
    return m_CacheInitialized && !m_Blocks.empty() && m_Blocks.front().Cache != nullptr;
}

int ForceStaticMeshFilter::GetNumberOfCacheBlocks() const {
    return static_cast<int>(m_Blocks.size());
}

int ForceStaticMeshFilter::GetCacheTimeStepIndex() const {
    if (m_Blocks.empty() || !m_CacheHasTimeStep) return -1;
    return m_Blocks.front().TimeStepIndex;
}

float ForceStaticMeshFilter::GetCacheTimeValue() const {
    if (m_Blocks.empty() || !m_CacheHasTimeStep) return 0.f;
    return m_Blocks.front().TimeValue;
}

IGsize ForceStaticMeshFilter::GetCacheNumberOfPoints() const {
    if (m_Blocks.empty()) return 0;
    return m_Blocks.front().PointCount;
}

IGsize ForceStaticMeshFilter::GetCacheNumberOfCells() const {
    if (m_Blocks.empty()) return 0;
    return m_Blocks.front().CellCount;
}

bool ForceStaticMeshFilter::WasCacheRebuilt() const { return m_LastRebuilt; }

ForceStaticMeshFilter::RebuildReason ForceStaticMeshFilter::GetLastRebuildReason() const {
    return static_cast<RebuildReason>(m_LastRebuildReason);
}

const std::string& ForceStaticMeshFilter::GetStatusMessage() const { return m_StatusMessage; }

std::string ForceStaticMeshFilter::GetCacheDescription() const {
    if (!HasCache()) return "尚未建立几何缓存";

    char buffer[256];
    const int frame = GetCacheTimeStepIndex();
    if (frame >= 0) {
        std::snprintf(buffer, sizeof(buffer), "缓存几何来自 t=%g（第 %d 帧）",
                      static_cast<double>(GetCacheTimeValue()), frame + 1);
    } else {
        std::snprintf(buffer, sizeof(buffer), "缓存几何来自首次执行");
    }

    std::string desc(buffer);
    if (m_Blocks.size() > 1) {
        char extra[128];
        std::snprintf(extra, sizeof(extra), "：共 %d 块，首块 %llu 点 / %llu 单元",
                      static_cast<int>(m_Blocks.size()),
                      static_cast<unsigned long long>(GetCacheNumberOfPoints()),
                      static_cast<unsigned long long>(GetCacheNumberOfCells()));
        desc += extra;
    } else {
        char extra[128];
        std::snprintf(extra, sizeof(extra), "：%llu 点 / %llu 单元",
                      static_cast<unsigned long long>(GetCacheNumberOfPoints()),
                      static_cast<unsigned long long>(GetCacheNumberOfCells()));
        desc += extra;
    }
    return desc;
}

void ForceStaticMeshFilter::SetForceCacheComputation(bool on) { m_ForceCacheComputation = on; }

bool ForceStaticMeshFilter::GetForceCacheComputation() const { return m_ForceCacheComputation; }

void ForceStaticMeshFilter::SetStaticCacheEnabled(bool on) { m_StaticCacheEnabled = on; }

bool ForceStaticMeshFilter::GetStaticCacheEnabled() const { return m_StaticCacheEnabled; }

IGAME_NAMESPACE_END
