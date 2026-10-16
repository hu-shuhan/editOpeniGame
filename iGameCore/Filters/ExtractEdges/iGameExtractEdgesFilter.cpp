#include "iGameExtractEdgesFilter.h"

// —— 各数据类型头文件 ——
#include "iGameAttributeSet.h"      // 属性集（点/单元数据数组）
#include "iGameCellArray.h"         // 单元数组（连接关系）
#include "iGameCellType.h"          // 单元类型枚举与类型名
#include "iGameFlatArray.h"         // FlatArray 模板（UnsignedIntArray 等）
#include "iGameSurfaceMesh.h"       // 表面网格
#include "iGameVolumeMesh.h"        // 体网格

#include <algorithm>  // std::minmax
#include <set>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

/// 输出 Cell Data：每条边的 VTK 单元类型编号（数组名，便于在属性面板里查看）
constexpr const char* kCellTypeArrayName = "CellType";

/// VTK 的 vtkLine 编号。注意：iGame 内部枚举是 IG_LINE = 2，VTK 是 3，两套体系不能混用。
constexpr unsigned char kVtkLine = 3;

/// 按实际类型创建一个同类型的空数组（点数据深拷贝 / 单元数据重映射共用）
ArrayObject::Pointer NewArrayLike(ArrayObject::Pointer src) {
    if (src == nullptr) { return nullptr; }
    switch (src->GetArrayType()) {
#define NEW_ARRAY_LIKE(Type) case IG_##Type: return Type::New();
        NEW_ARRAY_LIKE(FloatArray)
        NEW_ARRAY_LIKE(DoubleArray)
        NEW_ARRAY_LIKE(IntArray)
        NEW_ARRAY_LIKE(UnsignedIntArray)
        NEW_ARRAY_LIKE(CharArray)
        NEW_ARRAY_LIKE(UnsignedCharArray)
        NEW_ARRAY_LIKE(ShortArray)
        NEW_ARRAY_LIKE(UnsignedShortArray)
        NEW_ARRAY_LIKE(LongLongArray)
        NEW_ARRAY_LIKE(UnsignedLongLongArray)
#undef NEW_ARRAY_LIKE
        default: return nullptr;
    }
}

/**
 * 深拷贝一个属性数组（同类型 + 名字 + 维度 + 全部值）。
 * 用 DeepCopy/ShallowCopy 之外的显式逐值拷贝，保证输出与输入指针级独立。
 */
ArrayObject::Pointer DeepCopyArray(ArrayObject::Pointer src) {
    if (src == nullptr) { return nullptr; }
    const int dim = src->GetDimension();
    if (dim <= 0) { return nullptr; }
    auto dst = NewArrayLike(src);
    if (dst == nullptr) {
        dst = DoubleArray::New();  // 未知数组类型兜底为 double，绝不静默丢弃属性
    }
    dst->SetName(src->GetName());
    dst->SetDimension(dim);
    const IGsize values = src->GetNumberOfValues();
    // 注意：FlatArray::Resize 的参数是"元素个数"，框架内部会再乘一次维度
    // （resize(_NewElementNum * m_Dimension)）。这里必须传元素数，
    // 传标量数会让多分量数组的长度被放大 dim 倍。
    dst->Resize(src->GetNumberOfElements());
    for (IGsize i = 0; i < values; ++i) {
        dst->SetValue(i, src->GetValue(i));
    }
    return dst;
}

/**
 * 深拷贝"点数据"（IG_POINT）属性：输出网格点数与输入相同，语义仍然成立。
 * 单元数据（IG_CELL）不在这一步处理——它由提取过程按"每条边的来源单元"重映射。
 * 之所以深拷贝而不是共享：与 CountCellVertices 保持一致的独立性（指针级独立），
 * 避免输出与输入共享同一份数组导致下游修改互相污染。
 */
void DeepCopyPointAttributes(AttributeSet::Pointer src, AttributeSet::Pointer dst) {
    if (src == nullptr || dst == nullptr) { return; }
    auto all = src->GetAllAttributes();
    if (all == nullptr) { return; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }
        auto copy = DeepCopyArray(attr.pointer);
        if (copy == nullptr) { continue; }
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
}

/**
 * 写结果前先删掉同名数组，保证重复执行不会堆积同名数组。
 *
 * 必须同时匹配"挂载类型"：AttributeSet::GetAttributeIndex 只按名字匹配、
 * 忽略 IG_POINT / IG_CELL，而 VTK 允许点数据与单元数据同名（如都叫 "Normals"）。
 * 如果只按名字删，就会把合法的同名点数据误删。
 */
bool RemoveArrayIfExists(AttributeSet::Pointer attrs, const std::string& name,
                         IGenum attachmentType) {
    if (attrs == nullptr) { return false; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return false; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachmentType) { continue; }
        if (std::string(attr.pointer->GetName()) == name) {
            attrs->DeleteAttribute(i);
            return true;
        }
    }
    return false;
}

/**
 * 把输入的"单元数据"按每条边的来源单元重映射到输出：
 *   输出第 i 条边的值 = 输入第 sourceCells[i] 个单元的对应值。
 *
 * 为什么不能原样沿用：输入单元数是 N、输出是边数 M，长度对不上，
 * 直接搬会造成属性与单元错位（复测反馈的 cell data mismatch）。
 * 为什么不是丢弃：输出的每条边都来自某个输入单元，理应继承它的单元数据，
 * 否则 CellValue/OriginalCellTag 这类信息在提取后就彻底丢失了。
 * 共享边在提取时只记录"来源单元 ID 较小"的那一个，因此它的数据取自 ID 较小的单元。
 */
ArrayObject::Pointer RemapCellArrayBySource(ArrayObject::Pointer src,
                                            const std::vector<IGuint>& sourceCells) {
    if (src == nullptr) { return nullptr; }
    const int dim = src->GetDimension();
    if (dim <= 0) { return nullptr; }
    auto out = NewArrayLike(src);
    if (out == nullptr) {
        out = DoubleArray::New();  // 未知数组类型兜底为 double，绝不静默丢弃单元数据
    }
    out->SetName(src->GetName());
    out->SetDimension(dim);

    const IGsize n = static_cast<IGsize>(sourceCells.size());
    const IGsize d = static_cast<IGsize>(dim);
    // Resize 收"元素个数"（内部再乘 dim），这里传 n 而不是 n*d，
    // 否则多分量单元数据（向量/张量）的长度会被放大 dim 倍。
    out->Resize(n);
    for (IGsize i = 0; i < n; ++i) {
        const IGsize s = static_cast<IGsize>(sourceCells[i]);
        for (IGsize k = 0; k < d; ++k) {
            out->SetValue(i * d + k, src->GetValue(s * d + k));
        }
    }
    return out;
}

}  // namespace

ExtractEdgesFilter::ExtractEdgesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// ------------------------------------------------------------------
// Execute：执行导出
// ------------------------------------------------------------------
bool ExtractEdgesFilter::Execute() {
    UpdateProgress(0);
    m_Message.clear();
    m_SkippedCellCount = 0;
    m_SkippedCellTypes.clear();

    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "no input data";
        return false;
    }
    auto input = m_Inputs->GetElement(0);
    if (input == nullptr) {
        m_Message = "input data is null";
        return false;
    }
    return ExecuteWithPointSet(input);
}

bool ExtractEdgesFilter::ExecuteWithPointSet(DataObject::Pointer input) {
    // 统一转成 UnstructuredMesh 表示（SurfaceMesh / VolumeMesh 会新建转换，UnstructuredMesh 直接用自身）
    UnstructuredMesh::Pointer um = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (um == nullptr) {
        m_Message = "unsupported data type (support: UnstructuredMesh / SurfaceMesh / VolumeMesh)";
        igError("ExtractEdgesFilter: unsupported data type {}", static_cast<int>(input->GetDataObjectType()));
        return false;
    }
    if (um->GetNumberOfPoints() == 0) {
        m_Message = "input mesh has no points";
        return false;
    }
    if (um->GetNumberOfCells() == 0) {
        // 明确失败：绝不把原模型当作结果返回
        m_Message = "input mesh has 0 cells, there is no edge to extract";
        return false;
    }

    // —— 独立输出节点：新建边网格；点与点数据全部深拷贝，指针级独立 ——
    auto outMesh = UnstructuredMesh::New();
    outMesh->SetName(input->GetName() + "_Edges");

    auto outPoints = Points::New();
    outPoints->DeepCopy(um->GetPoints());
    outMesh->SetPoints(outPoints);

    // 属性集新建：Point Data 深拷贝保留；Cell Data 稍后由提取过程按"来源单元"重建
    auto outAttrs = AttributeSet::New();
    DeepCopyPointAttributes(input->GetAttributeSet(), outAttrs);
    outMesh->SetAttributeSet(outAttrs);

    if (!ExtractEdgesFromMesh(um, outMesh)) {
        m_Message = "failed to extract edges (no cell connectivity)";
        return false;
    }

    // —— 执行信息：空结果与跳过统计都要能被界面看见 ——
    const IGsize edgeNum = outMesh->GetNumberOfCells();
    if (edgeNum == 0) {
        m_Message = "no edge could be extracted from this mesh";
    }
    if (m_SkippedCellCount > 0) {
        std::string detail;
        for (const auto& entry : m_SkippedCellTypes) {
            if (!detail.empty()) { detail += ", "; }
            const char* typeName = GetCellTypeAsString(entry.first);
            if (typeName != nullptr && typeName[0] != '\0') {
                detail += typeName;
            } else {
                detail += "type " + std::to_string(static_cast<int>(entry.first));
            }
            detail += " x" + std::to_string(entry.second);
        }
        const std::string skipped =
            "skipped " + std::to_string(m_SkippedCellCount) + " cell(s): " + detail;
        m_Message = m_Message.empty() ? skipped : (m_Message + "; " + skipped);
    }

    UpdateProgress(1);
    outMesh->ForceReConvertToDrawableData();
    SetOutput(0, outMesh);
    return true;
}

// ------------------------------------------------------------------
// 核心：遍历单元提取唯一边（去重），并记录每条边的来源单元
// ------------------------------------------------------------------
bool ExtractEdgesFilter::ExtractEdgesFromMesh(UnstructuredMesh::Pointer input,
                                              UnstructuredMesh::Pointer output) {
    auto cells = input->GetCells();
    UnsignedIntArray::Pointer types = input->GetCellTypes();
    if (cells == nullptr || types == nullptr) { return false; }

    auto edges = CellArray::New();          // 边的连接表（每条边 2 个点）
    auto edgeTypes = UnsignedIntArray::New();  // 每条边的类型：IG_LINE

    const IGsize numCells = cells->GetNumberOfCells();
    // 用动态缓冲而不是固定数组：多面体的展开连接表可能远超 IGAME_CELL_MAX_SIZE(256)，
    // 而 CellArray::GetCellIds 不做边界检查，固定数组会越界写内存。
    std::vector<igIndex> vhs(IGAME_CELL_MAX_SIZE, 0);
    std::set<std::pair<igIndex, igIndex>> seen;  // 去重：无向边用 (小, 大) 作为键

    // 每条边的"来源单元"编号。遍历按单元 ID 递增，且已出现过的边不再记录，
    // 所以共享边留下来的必然是"来源单元 ID 较小"的那一个（与复测要求一致）。
    std::vector<IGuint> sourceCells;
    sourceCells.reserve(numCells * 3);

    edges->Reserve(numCells * 3);

    // 追加一条边（已存在则跳过），同时记录来源单元
    auto addEdge = [&](igIndex a, igIndex b, IGsize sourceCell) {
        auto key = std::minmax(a, b);
        if (!seen.insert(key).second) { return; }
        edges->AddCellId2(a, b);
        edgeTypes->AddValue(IG_LINE);
        sourceCells.push_back(static_cast<IGuint>(sourceCell));
    };

    for (IGsize cid = 0; cid < numCells; ++cid) {
        const IGuint needed = cells->GetCellSize(cid);
        if (vhs.size() < needed) { vhs.resize(needed); }
        const int vcnt = cells->GetCellIds(cid, vhs.data());
        const IGenum cellType = types->GetValue(cid);

        // 少于 2 个点的单元本来就没有边（正常无贡献，不算"跳过"）
        if (vcnt < 2) {
            continue;
        }

        // 输入本身就是线单元：直接作为边保留（折线逐段拆分）
        if (cellType == IG_LINE || cellType == IG_POLY_LINE) {
            if (cellType == IG_LINE && vcnt == 2) {
                addEdge(vhs[0], vhs[1], cid);
            } else if (cellType == IG_POLY_LINE && vcnt > 2) {
                for (int e = 0; e + 1 < vcnt; ++e) { addEdge(vhs[e], vhs[e + 1], cid); }
            } else {
                RecordSkippedCell(cellType);  // 数据异常：类型与点数不匹配
            }
            continue;
        }

        // 点单元 / 空单元本来就没有边，同样不算"跳过"
        if (cellType == IG_VERTEX || cellType == IG_EMPTY_CELL) {
            continue;
        }

        // 多面体是**变长单元**：连接表为展开格式
        //   [面数, 面1点数, 面1点索引..., 面2点数, 面2点索引..., ...]
        // 框架的 Polyhedron::GetNumberOfEdges() 恒返回 0、GetEdge() 恒返回 nullptr，
        // 属于"它其实有边、只是通用接口拿不到"，不能当跳过处理。
        // 这里直接从展开表解析每个面，取该面的边（相邻点对 + 首尾闭合），去重后即多面体的边。
        if (cellType == IG_POLYHEDRON) {
            bool extracted = false;
            int index = 1;  // 跳过开头的"面数"
            while (index < vcnt) {
                const int facePointCount = vhs[index++];
                if (facePointCount < 2 || index + facePointCount > vcnt) {
                    break;  // 数据异常，停止解析
                }
                for (int k = 0; k < facePointCount; ++k) {
                    const igIndex a = vhs[index + k];
                    const igIndex b = vhs[index + (k + 1) % facePointCount];
                    addEdge(a, b, cid);
                    extracted = true;
                }
                index += facePointCount;
            }
            if (!extracted) { RecordSkippedCell(cellType); }
            continue;
        }

        // 面 / 体单元：取它的拓扑边（三角形 3 条、四面体 6 条、六面体 12 条 ……）
        Cell::Pointer cell = nullptr;
        input->GetCell(cid, cell);
        if (cell == nullptr) {
            RecordSkippedCell(cellType);
            continue;
        }

        const int nEdges = cell->GetNumberOfEdges();
        if (nEdges <= 0) {
            RecordSkippedCell(cellType);
            continue;
        }

        bool extracted = false;
        for (int e = 0; e < nEdges; ++e) {
            Cell* edge = cell->GetEdge(e);
            if (edge == nullptr) { continue; }
            const int edgePointCount = edge->GetCellSize();
            if (edgePointCount < 2) { continue; }
            // 普通单元的边是 2 个端点；二次单元的边（QuadraticFace::GetEdge）返回的是
            // 含中点的"二次边"（3 个点）。这里取前两个端点，与 ParaView 提线性边的语义一致，
            // 绝不能因为"点数不是 2"就把整条边丢掉。
            addEdge(edge->GetPointId(0), edge->GetPointId(1), cid);
            extracted = true;
        }
        if (!extracted) { RecordSkippedCell(cellType); }

        if (cid % 10000 == 0) {
            UpdateProgress(static_cast<double>(cid) / numCells * 0.9);
        }
    }

    output->SetCells(edges, edgeTypes);

    // —— Cell Data 按输出边数重建（长度 = 边数）——
    // 注意：属性面板 / 导出文件里数组的排列顺序 = 这里向属性集添加的顺序，
    // 所以 CellType 先加，让它排在 Cell Data 的第一位（紧跟单元编号列）。
    auto outAttrs = output->GetAttributeSet();
    if (outAttrs != nullptr) {
        // 1) CellType：每条输出单元的"VTK 单元类型编号"（Cell Data，长度 = 边数）。
        //    提取结果全部是 1 维线单元，即 VTK 的 vtkLine = 3。
        //    与 ParaView 中给数据集加 "Cell Types" 数组的做法同义（值取自 GetCellTypesArray()），
        //    便于直接以数组形式查看/筛选每条边的单元类型。
        //    注意：写的是 VTK 编号（3），不是 iGame 内部的 IG_LINE(2) —— 两套体系不能混。
        auto cellTypes = UnsignedCharArray::New();
        cellTypes->SetName(kCellTypeArrayName);
        cellTypes->SetDimension(1);
        cellTypes->Reserve(output->GetNumberOfCells());
        for (IGsize i = 0; i < output->GetNumberOfCells(); ++i) {
            cellTypes->AddValue(kVtkLine);
        }
        RemoveArrayIfExists(outAttrs, kCellTypeArrayName, IG_CELL);
        outAttrs->AddScalar(IG_CELL, cellTypes);

        // 2) 输入原有的单元数据按"每条边的来源单元"逐值重映射到输出
        //    （不额外生成"边→来源单元"的映射数组，与 vtkExtractEdges 一致）。
        if (auto inAttrs = input->GetAttributeSet(); inAttrs != nullptr) {
            auto all = inAttrs->GetAllAttributes();
            if (all != nullptr) {
                for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
                    auto& attr = all->GetElement(i);
                    if (attr.isDeleted || attr.pointer == nullptr) { continue; }
                    if (attr.attachmentType != IG_CELL) { continue; }
                    auto remapped = RemapCellArrayBySource(attr.pointer, sourceCells);
                    if (remapped == nullptr) { continue; }
                    RemoveArrayIfExists(outAttrs, remapped->GetName(), IG_CELL);
                    outAttrs->AddAttribute(attr.type, IG_CELL, remapped);
                }
            }
        }
    }
    return true;
}

void ExtractEdgesFilter::RecordSkippedCell(IGenum cellType) {
    ++m_SkippedCellCount;
    ++m_SkippedCellTypes[cellType];
}

IGAME_NAMESPACE_END
