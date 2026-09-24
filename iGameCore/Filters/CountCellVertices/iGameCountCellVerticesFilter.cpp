#include "iGameCountCellVerticesFilter.h"

// —— 下面这些头文件提供我们用到的数据类型 ——
#include "iGameAttributeSet.h"      // 属性集：管理网格上的点属性/单元属性数组
#include "iGameCellArray.h"         // 单元数组：存储每个 cell 由哪些点组成（连接关系）
#include "iGameCellType.h"          // 单元类型枚举（IG_LINE / IG_TRIANGLE / ...）
#include "iGameFlatArray.h"         // 一维数组基类（DoubleArray / UnsignedIntArray 等）
#include "iGameSurfaceMesh.h"       // 表面网格类型（三角形/四边形面）
#include "iGameUnstructuredMesh.h"  // 非结构网格类型（最通用，任意混合单元）
#include "iGameVolumeMesh.h"        // 体网格类型（四面体/六面体等）

#include <exception>
#include <set>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

/// 结果数组名（输出网格的 Cell Data）
constexpr const char* kArrayName = "cell_vertex_count";

/**
 * 深拷贝一个数组（元素 + 名字 + 维度），返回同类型的新数组。
 */
template <typename T>
typename T::Pointer DeepCopyArray(typename T::Pointer src) {
    if (src == nullptr) { return nullptr; }
    auto dst = T::New();
    dst->DeepCopy(src);
    return dst;
}

/**
 * 按数组实际类型深拷贝一个属性数组（覆盖框架全部 FlatArray 类型）。
 * 注：框架的 AttributeSet::Attribute::DeepCopy 只支持 Float/Double，
 * 这里补齐整数等其余类型，避免属性被静默丢弃。
 */
ArrayObject::Pointer CopyAttribute(ArrayObject::Pointer source) {
    if (source == nullptr) { return nullptr; }
    ArrayObject::Pointer copy;
    switch (source->GetArrayType()) {
#define COPY_ATTRIBUTE(Type) \
    case IG_##Type: copy = DeepCopyArray<Type>(DynamicCast<Type>(source)); break;
        COPY_ATTRIBUTE(FloatArray)
        COPY_ATTRIBUTE(DoubleArray)
        COPY_ATTRIBUTE(IntArray)
        COPY_ATTRIBUTE(UnsignedIntArray)
        COPY_ATTRIBUTE(CharArray)
        COPY_ATTRIBUTE(UnsignedCharArray)
        COPY_ATTRIBUTE(ShortArray)
        COPY_ATTRIBUTE(UnsignedShortArray)
        COPY_ATTRIBUTE(LongLongArray)
        COPY_ATTRIBUTE(UnsignedLongLongArray)
#undef COPY_ATTRIBUTE
        default: break;
    }
    if (copy == nullptr) {
        // 未识别的数组类型：兜底转成 double 保留数据，绝不静默丢弃属性
        const int dim = source->GetDimension();
        if (dim <= 0) { return nullptr; }
        auto fallback = DoubleArray::New();
        fallback->SetName(source->GetName());
        fallback->SetDimension(dim);
        const IGsize values = source->GetNumberOfValues();
        // FlatArray::Resize 收"元素个数"（内部再乘维度），必须传元素数，
        // 传标量数会让多分量数组长度被放大 dim 倍。
        fallback->Resize(source->GetNumberOfElements());
        for (IGsize i = 0; i < values; ++i) { fallback->SetValue(i, source->GetValue(i)); }
        copy = fallback;
    }
    return copy;
}

/**
 * 深拷贝属性集：新建 AttributeSet，逐个深拷贝所有属性（含 dataRange），
 * 并跳过旧的 cell_vertex_count 结果数组（重复执行不会累积同名数组）。
 */
AttributeSet::Pointer DeepCopyAttributes(AttributeSet::Pointer src) {
    auto dst = AttributeSet::New();
    if (src == nullptr) { return dst; }
    auto all = src->GetAllAttributes();
    if (all == nullptr) { return dst; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType == IG_CELL && attr.pointer->GetName() == kArrayName) {
            continue;  // 旧结果数组不复制，下面会写入最新结果
        }
        auto copy = CopyAttribute(attr.pointer);
        if (copy == nullptr) { continue; }  // 不支持的类型保守跳过
        DoubleArray::Pointer copyRange = nullptr;
        if (attr.dataRange != nullptr) {
            copyRange = DoubleArray::New();
            copyRange->DeepCopy(attr.dataRange);
        }
        if (copyRange != nullptr) {
            dst->AddAttribute(attr.type, attr.attachmentType, copy, copyRange);
        } else {
            dst->AddAttribute(attr.type, attr.attachmentType, copy);
        }
    }
    return dst;
}

/**
 * 没有单元类型数组的数据类型（SurfaceMesh / VolumeMesh）按"单元点数"推断类型：
 * 面：3→三角形、4→四边形、其他→多边形；体：4→四面体、5→金字塔、6→三棱柱、8→六面体。
 * 注意这里只用于给输出网格标注类型，顶点数统计本身与类型无关。
 */
UnsignedIntArray::Pointer BuildCellTypesFromPointCount(CellArray::Pointer cells, bool surface) {
    if (cells == nullptr) { return nullptr; }
    auto types = UnsignedIntArray::New();
    const IGsize numCells = cells->GetNumberOfCells();
    igIndex ids[IGAME_CELL_MAX_SIZE] = {0};
    for (IGsize i = 0; i < numCells; ++i) {
        const int vcnt = cells->GetCellIds(i, ids);
        const IGenum type = surface ? SurfaceMesh::GetFaceTypeWithPointNum(vcnt)
                                    : VolumeMesh::GetVolumeTypeWithPointNum(vcnt);
        types->AddValue(type);
    }
    return types;
}

/**
 * 统计一个单元的"顶点数"（按 cellType 分派）。
 *
 * 普通单元：连接表里存的就是它的点，连接表长度 = 点数。
 *
 * 多面体 IG_POLYHEDRON 是**变长单元**，连接表是展开格式：
 *     [面数, 面1点数, 面1点索引..., 面2点数, 面2点索引..., ...]
 * 这时连接表长度是"这段展开数据的长度"（立方体 = 31），并不是顶点数。
 * 必须解析出所有面的点索引并**去重**（同一顶点会被相邻多个面重复引用），
 * 得到真实顶点数（立方体 = 8）。
 * 参照：主仓库 CellSizeFilter 同样按 cellType 分派，注释明确提醒
 *     "avoid misusing formulas on variable-length cells (IG_POLYHEDRON etc.)"。
 */
IGsize CountVerticesOfCell(CellArray::Pointer cells, IGsize cellId, IGenum cellType) {
    if (cells == nullptr) { return 0; }
    // 动态缓冲：多面体展开连接表可能超 IGAME_CELL_MAX_SIZE(256)，
    // 而 CellArray::GetCellIds 不做边界检查，固定数组会越界写内存。
    const IGuint needed = cells->GetCellSize(cellId);
    std::vector<igIndex> ids(needed > 0 ? static_cast<size_t>(needed) : 1, 0);
    const int size = cells->GetCellIds(cellId, ids.data());
    if (size <= 0) { return 0; }
    if (cellType != IG_POLYHEDRON) {
        return static_cast<IGsize>(size);
    }
    std::set<igIndex> uniquePoints;
    int index = 1;  // 跳过开头的"面数"
    while (index < size) {
        const int facePointCount = ids[index++];
        for (int k = 0; k < facePointCount && index < size; ++k) {
            uniquePoints.insert(ids[index++]);
        }
    }
    return static_cast<IGsize>(uniquePoints.size());
}

/**
 * 深拷贝单元连接表（逐单元 AddCellIds 重建）。
 *
 * 为什么不用 CellArray::DeepCopy：框架的 CellArray::DeepCopy 对"变长单元"
 * （m_UseOffsets == true，即各单元点数不一的网格）存在缺陷——它对 m_Offsets 做的是
 * "追加"而非"覆盖"，而 CellArray 构造时 m_Offsets 已预置一个 0，导致偏移数组错位
 * （变成 [0, 0, 8, ...]），GetCellSize 会算错。逐单元 AddCellIds 能正确重建
 * 连接表与偏移表，规避该 bug。
 */
CellArray::Pointer DeepCopyCellArray(CellArray::Pointer src) {
    if (src == nullptr) { return nullptr; }
    auto dst = CellArray::New();
    const IGsize n = src->GetNumberOfCells();
    igIndex ids[IGAME_CELL_MAX_SIZE] = {0};
    for (IGsize i = 0; i < n; ++i) {
        const int vcnt = src->GetCellIds(i, ids);
        dst->AddCellIds(ids, vcnt);
    }
    return dst;
}

}  // namespace

CountCellVerticesFilter::CountCellVerticesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool CountCellVerticesFilter::Execute() {
    UpdateProgress(0);
    m_Message.clear();
    SetOutput(0, nullptr);  // 重入/失败时不残留旧输出

    try {
        return ExecuteInternal();
    } catch (const std::exception& e) {
        SetOutput(0, nullptr);
        m_Message = std::string("CountCellVerticesFilter exception: ") + e.what();
        IGAME_CORE_ERROR("{}", m_Message);
        return false;
    } catch (...) {
        SetOutput(0, nullptr);
        m_Message = "CountCellVerticesFilter unknown exception";
        return false;
    }
}

bool CountCellVerticesFilter::ExecuteInternal() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "no input data";
        return false;
    }
    auto input = m_Inputs->GetElement(0);
    if (input == nullptr) {
        m_Message = "input data is null";
        return false;
    }

    // —— 按数据类型取出"单元数组 / 单元类型数组 / 点"，并公开支持范围 ——
    CellArray::Pointer cells = nullptr;
    UnsignedIntArray::Pointer cellTypes = nullptr;
    Points::Pointer points = nullptr;
    switch (input->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH: {
            auto um = DynamicCast<UnstructuredMesh>(input);
            if (um == nullptr) {
                m_Message = "UnstructuredMesh cast failed";
                return false;
            }
            cells = um->GetCells();
            cellTypes = um->GetCellTypes();
            points = um->GetPoints();
            break;
        }
        case IG_SURFACE_MESH: {
            auto sm = DynamicCast<SurfaceMesh>(input);
            if (sm == nullptr) {
                m_Message = "SurfaceMesh cast failed";
                return false;
            }
            cells = sm->GetFaces();
            points = sm->GetPoints();
            cellTypes = BuildCellTypesFromPointCount(cells, true);
            break;
        }
        case IG_VOLUME_MESH: {
            auto vm = DynamicCast<VolumeMesh>(input);
            if (vm == nullptr) {
                m_Message = "VolumeMesh cast failed";
                return false;
            }
            cells = vm->GetCells();
            points = vm->GetPoints();
            cellTypes = BuildCellTypesFromPointCount(cells, false);
            break;
        }
        default:
            m_Message = "unsupported data type, only UnstructuredMesh / SurfaceMesh / VolumeMesh are supported";
            IGAME_CORE_ERROR("CountCellVerticesFilter: unsupported data type {}",
                             static_cast<int>(input->GetDataObjectType()));
            return false;
    }
    if (cells == nullptr) {
        m_Message = "input mesh has no cells";
        return false;
    }
    if (cellTypes == nullptr) {
        m_Message = "cannot determine cell types";
        return false;
    }
    if (points == nullptr) {
        m_Message = "input mesh has no points";
        return false;
    }

    const IGsize numCells = cells->GetNumberOfCells();

    // —— 独立输出节点：几何/拓扑/属性全部深拷贝，指针级独立，绝不共享输入 ——
    auto outMesh = UnstructuredMesh::New();
    outMesh->SetName(input->GetName() + "_VertexCount");

    auto outPoints = Points::New();
    outPoints->DeepCopy(points);
    outMesh->SetPoints(outPoints);

    auto outCells = DeepCopyCellArray(cells);
    auto outTypes = UnsignedIntArray::New();
    outTypes->DeepCopy(cellTypes);
    outMesh->SetCells(outCells, outTypes);

    auto outAttrs = DeepCopyAttributes(input->GetAttributeSet());
    outMesh->SetAttributeSet(outAttrs);

    // —— 结果数组：写入最新统计值 ——
    auto vertexCounts = DoubleArray::New();
    vertexCounts->SetName(kArrayName);
    vertexCounts->SetDimension(1);
    vertexCounts->Reserve(numCells);
    double minCount = 0.0;
    double maxCount = 0.0;
    for (IGsize i = 0; i < numCells; ++i) {
        const IGenum cellType = cellTypes->GetValue(i);
        const double count = static_cast<double>(CountVerticesOfCell(cells, i, cellType));
        vertexCounts->AddElement(&count);
        if (i == 0 || count < minCount) { minCount = count; }
        if (i == 0 || count > maxCount) { maxCount = count; }
    }
    if (numCells > 0 && minCount == maxCount) {
        // Uniform meshes (for example, all tetrahedra) have a zero-width data
        // range. The color mapper rejects [value, value] and otherwise keeps
        // whichever range belonged to the previously selected point array.
        // Give this result a stable display interval with the actual value at
        // its lower end, so repeated selection maps to the same visible color.
        auto displayRange = DoubleArray::New();
        displayRange->SetDimension(2);
        displayRange->AddElement2(minCount, minCount + 1.0);
        displayRange->AddElement2(minCount, minCount + 1.0);
        outAttrs->AddScalar(IG_CELL, vertexCounts, displayRange);
    } else {
        outAttrs->AddScalar(IG_CELL, vertexCounts);
    }

    // —— 空的模型：仍然产出（长度为 0 的）数组，界面不会出现"执行成功却找不到数组" ——
    if (numCells == 0) {
        m_Message = "mesh has 0 cells, an empty cell_vertex_count array was produced";
    }

    UpdateProgress(1);
    SetOutput(0, outMesh);
    return true;
}

IGAME_NAMESPACE_END
