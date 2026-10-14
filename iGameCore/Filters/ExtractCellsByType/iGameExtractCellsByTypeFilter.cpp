#include "iGameExtractCellsByTypeFilter.h"

#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

namespace {
// 按输入数组的实际类型创建同类型空数组：
// 保留 Double/整数/64 位 ID 的类型与精度，避免统一转成 FloatArray 造成降精度。
ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& src) {
    switch (src->GetArrayType()) {
    case IG_FloatArray: return FloatArray::New();
    case IG_DoubleArray: return DoubleArray::New();
    case IG_IntArray: return IntArray::New();
    case IG_UnsignedIntArray: return UnsignedIntArray::New();
    case IG_CharArray: return CharArray::New();
    case IG_UnsignedCharArray: return UnsignedCharArray::New();
    case IG_ShortArray: return ShortArray::New();
    case IG_UnsignedShortArray: return UnsignedShortArray::New();
    case IG_LongLongArray: return LongLongArray::New();
    case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
    default:
        // 非常规数组类型（如 IdArray）：回退 FloatArray，仅极端情况下触发
        return FloatArray::New();
    }
}

// 类型化拷贝：dst[i] = src[srcIdx[i]]，按底层值类型逐分量复制（不经 double 中转，完全保真）
template <typename TArray, typename TValue>
void CopyArrayElements(ArrayObject::Pointer& dst, const ArrayObject::Pointer& src,
                       const std::vector<IGsize>& srcIdx) {
    auto s = DynamicCast<TArray>(src);
    auto d = DynamicCast<TArray>(dst);
    if (!s || !d) return;
    const int dim = src->GetDimension();
    const TValue* sRaw = s->RawPointer();
    TValue* dRaw = d->RawPointer();
    const IGsize n = static_cast<IGsize>(srcIdx.size());
    for (IGsize i = 0; i < n; i++) {
        const IGsize si = srcIdx[i] * static_cast<IGsize>(dim);
        const IGsize di = i * static_cast<IGsize>(dim);
        for (int c = 0; c < dim; c++) { dRaw[di + c] = sRaw[si + c]; }
    }
}

// 按源数组类型分发到对应的类型化拷贝
void CopyArrayByIndex(ArrayObject::Pointer& dst, const ArrayObject::Pointer& src,
                      const std::vector<IGsize>& srcIdx) {
    switch (src->GetArrayType()) {
    case IG_FloatArray: CopyArrayElements<FloatArray, float>(dst, src, srcIdx); break;
    case IG_DoubleArray: CopyArrayElements<DoubleArray, double>(dst, src, srcIdx); break;
    case IG_IntArray: CopyArrayElements<IntArray, int>(dst, src, srcIdx); break;
    case IG_UnsignedIntArray: CopyArrayElements<UnsignedIntArray, unsigned int>(dst, src, srcIdx); break;
    case IG_CharArray: CopyArrayElements<CharArray, char>(dst, src, srcIdx); break;
    case IG_UnsignedCharArray: CopyArrayElements<UnsignedCharArray, unsigned char>(dst, src, srcIdx); break;
    case IG_ShortArray: CopyArrayElements<ShortArray, short>(dst, src, srcIdx); break;
    case IG_UnsignedShortArray: CopyArrayElements<UnsignedShortArray, unsigned short>(dst, src, srcIdx); break;
    case IG_LongLongArray: CopyArrayElements<LongLongArray, long long>(dst, src, srcIdx); break;
    case IG_UnsignedLongLongArray: CopyArrayElements<UnsignedLongLongArray, unsigned long long>(dst, src, srcIdx); break;
    default: {
        // 回退：double 中转（与旧行为一致，仅非常规数组类型触发）
        const int dim = src->GetDimension();
        double buf[IGAME_CELL_MAX_SIZE] = {0};
        for (IGsize i = 0; i < static_cast<IGsize>(srcIdx.size()); i++) {
            src->GetElement(srcIdx[i], buf);
            dst->SetElement(i, buf);
        }
    } break;
    }
}
} // namespace

int ExtractCellsByTypeFilter::s_InstanceCounter = 0;

ExtractCellsByTypeFilter::ExtractCellsByTypeFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
    // 全局计数，用于命名 ExtractCellsByType_n（n = 第几个提取的模型）
    m_InstanceId = ++s_InstanceCounter;
}

std::string ExtractCellsByTypeFilter::GetCellTypeDisplayName(IGenum type) {
    switch (type) {
    case IG_EMPTY_CELL: return "空单元 (Empty)";
    case IG_VERTEX: return "点 (Vertex)";
    case IG_LINE: return "线 (Edge)";
    case IG_FACE: return "面 (Face)";
    case IG_TRIANGLE: return "三角形 (Triangle)";
    case IG_QUAD: return "四边形 (Quad)";
    case IG_POLYGON: return "多边形 (Polygon)";
    case IG_VOLUME: return "体 (Volume)";
    case IG_POLYHEDRON: return "多面体 (Polyhedron)";
    case IG_TETRA: return "四面体 (Tetra)";
    case IG_HEXAHEDRON: return "六面体 (Hexahedron)";
    case IG_PYRAMID: return "金字塔 (Pyramid)";
    case IG_PRISM: return "三棱柱 (Prism)";
    case IG_QUADRATIC_EDGE: return "二次线 (Quadratic Edge)";
    case IG_QUADRATIC_TRIANGLE: return "二次三角形 (Quadratic Triangle)";
    case IG_QUADRATIC_QUAD: return "二次四边形 (Quadratic Quad)";
    case IG_QUADRATIC_POLYGON: return "二次多边形 (Quadratic Polygon)";
    case IG_QUADRATIC_TETRA: return "二次四面体 (Quadratic Tetra)";
    case IG_QUADRATIC_HEXAHEDRON: return "二次六面体 (Quadratic Hexahedron)";
    case IG_QUADRATIC_PRISM: return "二次三棱柱 (Quadratic Prism)";
    case IG_QUADRATIC_PYRAMID: return "二次金字塔 (Quadratic Pyramid)";
    case IG_BIQUADRATIC_QUAD: return "双二次四边形 (BiQuadratic Quad)";
    case IG_TRIQUADRATIC_HEXAHEDRON: return "三二次六面体 (TriQuadratic Hexahedron)";
    case IG_TRIQUADRATIC_PYRAMID: return "三二次金字塔 (TriQuadratic Pyramid)";
    case IG_QUADRATIC_LINEAR_QUAD: return "二次线性四边形 (Quadratic Linear Quad)";
    case IG_QUADRATIC_LINEAR_WEDGE: return "二次线性楔形 (Quadratic Linear Wedge)";
    case IG_BIQUADRATIC_QUADRATIC_WEDGE: return "双二次二次楔形 (BiQuadratic Quadratic Wedge)";
    case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON: return "双二次二次六面体 (BiQuadratic Quadratic Hexahedron)";
    case IG_BIQUADRATIC_TRIANGLE: return "双二次三角形 (BiQuadratic Triangle)";
    case IG_POLY_LINE: return "折线 (Polyline)";
    case IG_LAGRANGE_CURVE: return "拉格朗日曲线 (Lagrange Curve)";
    case IG_LAGRANGE_TRIANGLE: return "拉格朗日三角形 (Lagrange Triangle)";
    case IG_LAGRANGE_QUADRILATERAL: return "拉格朗日四边形 (Lagrange Quadrilateral)";
    case IG_LAGRANGE_TETRAHEDRON: return "拉格朗日四面体 (Lagrange Tetrahedron)";
    case IG_LAGRANGE_HEXAHEDRON: return "拉格朗日六面体 (Lagrange Hexahedron)";
    case IG_LAGRANGE_PRISM: return "拉格朗日三棱柱 (Lagrange Prism)";
    case IG_LAGRANGE_PYRAMID: return "拉格朗日金字塔 (Lagrange Pyramid)";
    default: return "未知类型 (Unknown)";
    }
}

// 把任意网格统一成"单元连接表 + 每单元类型"。
// 适配：非结构网格(UnstructuredMesh) / 表面网格(SurfaceMesh) / 体积网格(VolumeMesh) / 结构化网格(StructuredMesh)
bool ExtractCellsByTypeFilter::BuildUnifiedCells(DataObject::Pointer input, CellArray::Pointer& outCells,
                                                 std::vector<IGenum>& outTypes) const {
    if (!input) return false;
    outCells = nullptr;
    outTypes.clear();

    igIndex ids[IGAME_CELL_MAX_SIZE] = {0};
    const IGenum objType = input->GetDataObjectType();

    switch (objType) {
    case IG_UNSTRUCTURED_MESH: {
        // 非结构网格：CellArray + 每单元类型（权威来源）
        auto um = DynamicCast<UnstructuredMesh>(input);
        if (!um) return false;
        auto cells = um->GetCellArray();
        if (!cells) return false;
        const IGsize cellNum = cells->GetNumberOfCells();
        for (IGsize i = 0; i < cellNum; i++) {
            outTypes.push_back(um->GetCellType(i));
        }
        outCells = cells;
        return true;
    }
    case IG_SURFACE_MESH: {
        // 表面网格：面单元，类型由顶点数推导（3→三角形，4→四边形，其他→多边形）
        auto sm = DynamicCast<SurfaceMesh>(input);
        if (!sm) return false;
        auto faces = sm->GetFaces();
        if (!faces) return false;
        const IGsize faceNum = faces->GetNumberOfCells();
        for (IGsize i = 0; i < faceNum; i++) {
            const int vcnt = faces->GetCellIds(i, ids);
            outTypes.push_back(SurfaceMesh::GetFaceTypeWithPointNum(vcnt));
        }
        outCells = faces;
        return true;
    }
    case IG_VOLUME_MESH: {
        auto vm = DynamicCast<VolumeMesh>(input);
        if (!vm) return false;
        if (vm->GetIsPolyhedronType()) {
            // 多面体体网格：需要把"顶点集合"编码成 UnstructuredMesh 认识的多面体格式
            // [面数 fcnt, 面0顶点数, 面0顶点ids..., 面1顶点数, 面1顶点ids..., ...]
            auto faces = vm->GetFaces();
            if (!faces) return false;
            const IGsize volumeNum = vm->GetNumberOfVolumes();
            CellArray::Pointer encodedCells = CellArray::New();
            igIndex fhs[IGAME_CELL_MAX_SIZE] = {0};
            igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
            for (IGsize i = 0; i < volumeNum; i++) {
                const int fcnt = vm->GetVolumeFaceIds(i, fhs);
                igIndex encoded[IGAME_CELL_MAX_SIZE] = {0};
                int encPos = 0;
                encoded[encPos++] = static_cast<igIndex>(fcnt);
                for (int f = 0; f < fcnt; f++) {
                    const int vcnt = faces->GetCellIds(fhs[f], vhs);
                    encoded[encPos++] = static_cast<igIndex>(vcnt);
                    for (int k = 0; k < vcnt; k++) { encoded[encPos++] = vhs[k]; }
                }
                encodedCells->AddCellIds(encoded, encPos);
                outTypes.push_back(IG_POLYHEDRON);
            }
            outCells = encodedCells;
            return true;
        }
        // 常规体网格：类型由顶点数推导（4→四面体，5→金字塔，6→三棱柱，8→六面体）
        auto volumes = vm->GetVolumes();
        if (!volumes) return false;
        const IGsize volumeNum = volumes->GetNumberOfCells();
        for (IGsize i = 0; i < volumeNum; i++) {
            const int vcnt = volumes->GetCellIds(i, ids);
            IGenum type = VolumeMesh::GetVolumeTypeWithPointNum(vcnt);
            if (type == IG_EMPTY_CELL) {
                // 非常规顶点数的体：既不是标准体类型也不是多面体标记，无法安全编码，拒绝处理
                return false;
            }
            outTypes.push_back(type);
        }
        outCells = volumes;
        return true;
    }
    case IG_STRUCTURED_MESH: {
        // 结构化网格：隐式拓扑，先生成连接表；3D 取体单元(六面体)，2D 取面单元(四边形)
        auto structured = DynamicCast<StructuredMesh>(input);
        if (!structured) return false;
        structured->GenStructuredCellConnectivities();
        const igIndex dim = structured->GetDimension();
        if (dim >= 3) {
            auto volumes = structured->GetVolumes();
            if (!volumes) return false;
            const IGsize volumeNum = volumes->GetNumberOfCells();
            for (IGsize i = 0; i < volumeNum; i++) { outTypes.push_back(IG_HEXAHEDRON); }
            outCells = volumes;
            return true;
        } else if (dim == 2) {
            auto faces = structured->GetFaces();
            if (!faces) return false;
            const IGsize faceNum = faces->GetNumberOfCells();
            for (IGsize i = 0; i < faceNum; i++) { outTypes.push_back(IG_QUAD); }
            outCells = faces;
            return true;
        }
        return false; // 1D 结构化网格（线）暂不支持
    }
    default:
        // 点集/多块网格等：没有可提取的单元
        return false;
    }
}

std::vector<IGenum> ExtractCellsByTypeFilter::GetAvailableCellTypes() {
    std::vector<IGenum> result;
    auto input = GetInput(0);
    if (!input) return result;

    CellArray::Pointer cells;
    std::vector<IGenum> types;
    if (!BuildUnifiedCells(input, cells, types)) return result;

    // 按出现顺序去重
    for (IGenum t : types) {
        bool duplicated = false;
        for (IGenum r : result) {
            if (r == t) { duplicated = true; break; }
        }
        if (!duplicated) result.push_back(t);
    }
    return result;
}

bool ExtractCellsByTypeFilter::Execute() {
    auto input = GetInput(0);
    if (!input) return false;

    // 1. 统一单元数据（适配所有网格类型）
    CellArray::Pointer inCells;
    std::vector<IGenum> cellTypes;
    if (!BuildUnifiedCells(input, inCells, cellTypes)) return false;

    auto inPoints = input->GetPoints();
    if (!inPoints) return false;

    const IGsize inCellNum = inCells->GetNumberOfCells();
    const IGsize inPointNum = inPoints->GetNumberOfPoints();

    // 2. 选择要提取的单元
    if (m_ExtractTypes.empty()) return false; // 未选择任何单元类型
    std::vector<IGsize> selectedCells;
    selectedCells.reserve(inCellNum);
    for (IGsize i = 0; i < inCellNum; i++) {
        for (IGenum t : m_ExtractTypes) {
            if (t == cellTypes[i]) { selectedCells.push_back(i); break; }
        }
    }
    if (selectedCells.empty()) return false; // 没有符合所选类型的单元

    // 3. 构建输出网格（UnstructuredMesh）
    UnstructuredMesh::Pointer out = UnstructuredMesh::New();
    out->SetName("ExtractCellsByType_" + std::to_string(m_InstanceId));

    auto outPoints = out->GetPoints();
    outPoints->Reset();
    outPoints->Reserve(inPointNum);

    auto outCells = CellArray::New();
    auto outTypes = UnsignedIntArray::New();

    // 旧点编号 -> 新点编号（-1 表示未被使用）
    std::vector<igIndex> oldToNew(inPointNum, -1);
    igIndex ids[IGAME_CELL_MAX_SIZE] = {0};
    igIndex newIds[IGAME_CELL_MAX_SIZE] = {0};

    for (IGsize k = 0; k < selectedCells.size(); k++) {
        const IGsize cellId = selectedCells[k];
        const int n = inCells->GetCellIds(cellId, ids);
        if (n <= 0) continue;

        const IGenum cellType = cellTypes[cellId];
        if (cellType == IG_POLYHEDRON) {
            // 多面体：ids 是编码序列 [面数, 面0顶点数, 顶点...]，只对真正的顶点做重映射
            newIds[0] = ids[0];
            int pos = 1;
            const int faceNum = static_cast<int>(ids[0]);
            for (int f = 0; f < faceNum; f++) {
                const int vcnt = static_cast<int>(ids[pos]);
                newIds[pos] = ids[pos]; // 面顶点数原样保留
                pos++;
                for (int v = 0; v < vcnt; v++) {
                    const igIndex oldId = ids[pos];
                    if (oldToNew[oldId] < 0) {
                        oldToNew[oldId] = static_cast<igIndex>(outPoints->GetNumberOfPoints());
                        outPoints->AddPoint(inPoints->GetPoint(oldId));
                    }
                    newIds[pos] = oldToNew[oldId];
                    pos++;
                }
            }
            outCells->AddCellIds(newIds, n);
            outTypes->AddValue(cellType);
        } else {
            // 常规单元：所有 ids 都是顶点索引，直接重映射
            for (int j = 0; j < n; j++) {
                const igIndex oldId = ids[j];
                if (oldToNew[oldId] < 0) {
                    oldToNew[oldId] = static_cast<igIndex>(outPoints->GetNumberOfPoints());
                    outPoints->AddPoint(inPoints->GetPoint(oldId));
                }
                newIds[j] = oldToNew[oldId];
            }
            outCells->AddCellIds(newIds, n);
            outTypes->AddValue(cellType);
        }
    }

    out->SetCells(outCells, outTypes);

    // 4. 属性搬运（不丢弃属性，且保留数组原始类型与精度）
    //    点属性：只保留被使用到的点（按 oldToNew 映射）
    //    单元属性：只保留被选中的单元（按 selectedCells 映射）
    auto inAttr = input->GetAttributeSet();
    if (inAttr) {
        auto outAttr = AttributeSet::New();
        auto allAttrs = inAttr->GetAllAttributes();
        const IGsize outPointNum = outPoints->GetNumberOfPoints();
        const IGsize outCellNum = selectedCells.size();

        // 输出点 i 对应的源点号（仅被保留的点）
        std::vector<IGsize> pointSrcIdx(outPointNum);
        for (IGsize oldId = 0; oldId < inPointNum; oldId++) {
            if (oldToNew[oldId] >= 0) { pointSrcIdx[static_cast<IGsize>(oldToNew[oldId])] = oldId; }
        }

        for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); i++) {
            auto& attr = allAttrs->GetElement(i);
            if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) continue;

            // 按输入数组的实际类型创建同类型数组（Double/整数/64 位 ID 均保留）
            auto outArray = CreateArrayLike(attr.pointer);
            outArray->SetName(attr.pointer->GetName());
            outArray->SetDimension(attr.pointer->GetDimension());

            if (attr.attachmentType == IG_POINT) {
                outArray->Resize(outPointNum);
                CopyArrayByIndex(outArray, attr.pointer, pointSrcIdx);
                outAttr->AddAttribute(attr.type, IG_POINT, outArray, attr.GetDataRange());
            } else if (attr.attachmentType == IG_CELL) {
                outArray->Resize(outCellNum);
                CopyArrayByIndex(outArray, attr.pointer, selectedCells);
                outAttr->AddAttribute(attr.type, IG_CELL, outArray, attr.GetDataRange());
            }
        }
        out->SetAttributeSet(outAttr);
    }

    // 5. 挂到输出端口
    SetOutput(out);
    return true;
}

IGAME_NAMESPACE_END
