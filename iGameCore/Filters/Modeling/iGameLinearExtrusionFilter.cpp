#include "iGameLinearExtrusionFilter.h"

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

/*
 * 逐行对齐 vtkLinearExtrusionFilter（Filters/Modeling/vtkLinearExtrusionFilter.cxx）：
 *   - 位移：x = x + ScaleFactor * Offset，其中 Offset 按模式取
 *           Vector = Vector、Point = x - ExtrusionPoint、Normal = 该点法向；
 *     Normal 模式在输入没有点法向时落到 Vector 分支（与 VTK 的 if/else 链一致）；
 *   - 顶点（dim 0）-> 线 [p, p + N]；线/折线（dim 1）-> 每段 [p1, p2, p1 + N, p2 + N]；
 *   - 面（dim 2）-> Capping 开时输出原端点与位移端点（点序不反转），
 *     再对每条无邻居单元的边输出侧裙 [p1, p2, p1 + N, p2 + N]；
 *   - 单元属性按原单元顺序复制（线单元 -> 端点/侧裙 -> strip），法向属性不复制。
 * iGame 没有可持久化的 triangle strip 容器（VTK reader 对 TRIANGLESTRIP 也是直接
 * 跳过、不产出单元），因此 VTK 里的 strip 用等价点序的 IG_QUAD 表达。
 * 输出统一为 UnstructuredMesh，因为它能同时容纳线、四边形和多边形等混合单元。
 */

namespace {

// 这只是输入适配记录，不是新的底层数据类。
// 用来统一保存来自 SurfaceMesh face 或 UnstructuredMesh cell 的单元信息。
struct InputCell {
    IGenum type{IG_EMPTY_CELL};
    std::vector<igIndex> pointIds;
    igIndex sourceCellId{-1};
};

// 判断某个单元类型是否是本 filter 能处理的 0/1/2 维单元。
// IG_FACE 在本仓库其他 filter（ValidateCellsFilter、CountCellFacesFilter、
// AxisAlignedReflectionFilter）中都与 IG_POLYGON 并列按二维面处理，这里同样处理；
// 体单元、高阶单元、多面体等仍明确拒绝，不静默跳过。
bool IsSupportedCellType(IGenum type) {
    switch (type) {
        case IG_VERTEX:
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_FACE:
            return true;
        default:
            return false;
    }
}

// 按输入数组的具体类型创建同类型空数组（不把数组偷换成 DoubleArray）。
ArrayObject::Pointer CreateArrayByTypeExact(IGenum dataType) {
    switch (dataType) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        default:
            return nullptr;
    }
}

// 在输入的点属性中查找可用的点法向数组，用于 Normal 拉伸模式。
// 只按「挂载类型 + 属性类型 + 维度 + 元组数」判断，不靠数组名猜测。
ArrayObject* FindPointNormalArray(AttributeSet* attributeSet, IGsize numberOfPoints) {
    if (attributeSet == nullptr) { return nullptr; }
    auto pointAttributes = attributeSet->GetAllPointAttributes();
    if (pointAttributes == nullptr) { return nullptr; }

    for (IGsize i = 0; i < pointAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = pointAttributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != IG_POINT || attribute.type != IG_NORMAL) { continue; }
        if (attribute.pointer->GetDimension() < 3) { continue; }
        if (attribute.pointer->GetNumberOfElements() < numberOfPoints) { continue; }
        return attribute.pointer;
    }
    return nullptr;
}

// 把输入的点 / 单元属性按来源映射复制到输出。
// 与 VTK 一致地跳过旧法向（几何已经被拉伸，旧法向不再成立），
// 需要法向时应在之后重新计算（如 SurfaceNormalsFilter）。
void CopyAttributesBySourceId(DataObject* input, DataObject* output, IGsize numberOfOutputElements,
                              const std::vector<igIndex>& sourceIds, IGenum attachmentType) {
    if (input == nullptr || output == nullptr) { return; }

    auto inputAttributeSet = input->GetAttributeSet();
    if (inputAttributeSet == nullptr) { return; }
    auto inputAttributes = (attachmentType == IG_POINT) ? inputAttributeSet->GetAllPointAttributes()
                                                        : inputAttributeSet->GetAllCellAttributes();
    if (inputAttributes == nullptr || inputAttributes->GetNumberOfElements() == 0) { return; }

    AttributeSet::Pointer outputAttributeSet = output->GetAttributeSet();
    if (outputAttributeSet == nullptr) {
        outputAttributeSet = AttributeSet::New();
        output->SetAttributeSet(outputAttributeSet);
    }

    for (IGsize i = 0; i < inputAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = inputAttributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.type == IG_NORMAL) { continue; }

        ArrayObject::Pointer inputArray = attribute.pointer;
        const int dimension = inputArray->GetDimension();
        if (dimension <= 0) { continue; }
        const IGsize numberOfInputElements = inputArray->GetNumberOfElements();

        ArrayObject::Pointer outputArray = CreateArrayByTypeExact(inputArray->GetArrayType());
        if (outputArray == nullptr) { continue; }
        outputArray->SetName(inputArray->GetName());
        outputArray->SetDimension(dimension);
        outputArray->Resize(numberOfOutputElements);

        for (IGsize outId = 0; outId < numberOfOutputElements; ++outId) {
            const igIndex sourceId = sourceIds[outId];
            if (sourceId < 0 || static_cast<IGsize>(sourceId) >= numberOfInputElements) {
                // 来源单元没有对应的输入元组（例如来源网格本身缺少该 cell 数据）：
                // 明确写 0，不留下未初始化内存。
                for (int d = 0; d < dimension; ++d) { outputArray->SetValue(outId * dimension + d, 0.0); }
                continue;
            }
            for (int d = 0; d < dimension; ++d) {
                outputArray->SetValue(outId * dimension + d,
                                      inputArray->GetValue(static_cast<IGsize>(sourceId) * dimension + d));
            }
        }

        // 数据范围与属性类型原样保留；数组类型已经按原类型创建。
        outputAttributeSet->AddAttribute(attribute.type, attachmentType, outputArray, attribute.GetDataRange());
    }
}

}  // namespace

bool LinearExtrusionFilter::Execute() {
    m_Message.clear();
    // 执行失败时不保留上一次的结果
    this->SetOutput(0, nullptr);

    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "LinearExtrusionFilter has no input DataObject.";
        return false;
    }

    // 不要只转换为 SurfaceMesh：VTK PolyData 可以同时含 Vertex、Line、Polygon。
    auto surfaceMesh = DynamicCast<SurfaceMesh>(input);
    auto unstructuredMesh = DynamicCast<UnstructuredMesh>(input);
    if (surfaceMesh == nullptr && unstructuredMesh == nullptr) {
        m_Message = "LinearExtrusionFilter requires a SurfaceMesh or UnstructuredMesh input.";
        return false;
    }

    // 后续所有「复制两层点」的代码都使用 inputPoints，不再依赖具体网格类型。
    Points::Pointer inputPoints =
            (surfaceMesh != nullptr) ? surfaceMesh->GetPoints() : unstructuredMesh->GetPoints();
    if (inputPoints == nullptr || inputPoints->GetNumberOfPoints() == 0) {
        m_Message = "LinearExtrusionFilter input has no points.";
        return false;
    }
    const IGsize numberOfInputPoints = inputPoints->GetNumberOfPoints();

    // ------------------------------------------------------------------
    // 1. 输入适配：把 SurfaceMesh 的 face 或 UnstructuredMesh 的 cell
    //    统一成 InputCell，并拒绝 VTK PolyData 无法表达的单元类型。
    // ------------------------------------------------------------------
    std::vector<InputCell> inputCells;
    if (surfaceMesh != nullptr) {
        CellArray* faces = surfaceMesh->GetFaces();
        if (faces == nullptr || faces->GetNumberOfCells() == 0) {
            m_Message = "LinearExtrusionFilter input SurfaceMesh has no faces.";
            return false;
        }
        const IGsize numberOfFaces = faces->GetNumberOfCells();
        inputCells.reserve(numberOfFaces);
        for (IGsize faceId = 0; faceId < numberOfFaces; ++faceId) {
            const igIndex* pointIds = nullptr;
            const int count = faces->GetCellIds(faceId, pointIds);
            if (count < 3 || pointIds == nullptr) {
                m_Message = "LinearExtrusionFilter input SurfaceMesh face " + std::to_string(faceId) +
                            " has fewer than 3 points.";
                return false;
            }
            InputCell cell;
            // SurfaceMesh 只存多边形面：按点数区分三角面 / 四边面 / 一般多边形。
            cell.type = SurfaceMesh::GetFaceTypeWithPointNum(count);
            cell.sourceCellId = static_cast<igIndex>(faceId);
            cell.pointIds.assign(pointIds, pointIds + count);
            inputCells.push_back(std::move(cell));
        }
    } else {
        CellArray::Pointer cells = unstructuredMesh->GetCells();
        UnsignedIntArray* types = unstructuredMesh->GetCellTypes();
        if (cells == nullptr || types == nullptr || cells->GetNumberOfCells() == 0) {
            m_Message = "LinearExtrusionFilter input UnstructuredMesh has no cells.";
            return false;
        }
        const IGsize numberOfCells = cells->GetNumberOfCells();
        inputCells.reserve(numberOfCells);
        for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {
            const IGenum type = static_cast<IGenum>(types->GetValue(cellId));
            if (!IsSupportedCellType(type)) {
                // 体单元、高阶单元、多面体等不处理：明确报错，不静默跳过。
                m_Message = "LinearExtrusionFilter does not support cell " + std::to_string(cellId) +
                            " with type " + std::to_string(type) + " (" + GetCellTypeAsString(type) +
                            "); supported types are Vertex, Line, PolyLine, Triangle, Quad, Polygon and Face.";
                return false;
            }
            const igIndex* pointIds = nullptr;
            const int count = cells->GetCellIds(cellId, pointIds);
            if (count <= 0 || pointIds == nullptr) {
                m_Message = "LinearExtrusionFilter input cell " + std::to_string(cellId) + " has no point ids.";
                return false;
            }
            InputCell cell;
            cell.type = type;
            cell.sourceCellId = static_cast<igIndex>(cellId);
            cell.pointIds.assign(pointIds, pointIds + count);
            inputCells.push_back(std::move(cell));
        }
    }

    // ------------------------------------------------------------------
    // 2. 确定本次 Execute 真正使用的位移方式。
    //    Normal 找不到可用点法向时按 VTK 行为退回 Vector，但不改写 m_ExtrusionType。
    // ------------------------------------------------------------------
    ExtrusionType effectiveType = m_ExtrusionType;
    ArrayObject* normalArray = nullptr;
    if (effectiveType == ExtrusionType::Normal) {
        normalArray = FindPointNormalArray(input->GetAttributeSet(), numberOfInputPoints);
        if (normalArray == nullptr) {
            effectiveType = ExtrusionType::Vector;
            m_Message = "LinearExtrusionFilter found no valid point normals; "
                        "Normal extrusion fell back to Vector extrusion.";
        }
    }

    std::vector<double> pointNormals;
    if (effectiveType == ExtrusionType::Normal) {
        const int normalDimension = normalArray->GetDimension();
        pointNormals.resize(numberOfInputPoints * 3, 0.0);
        for (IGsize pointId = 0; pointId < numberOfInputPoints; ++pointId) {
            for (int d = 0; d < 3; ++d) {
                pointNormals[pointId * 3 + d] = normalArray->GetValue(pointId * normalDimension + d);
            }
        }
    }

    // ------------------------------------------------------------------
    // 3. 生成两层点，并同时建立「输出点 -> 输入点」的来源映射。
    //    第 1 层：输出 ID [0, N)；第 2 层：输出 ID [N, 2N)。
    // ------------------------------------------------------------------
    auto newPoints = Points::New();
    newPoints->Reserve(2 * numberOfInputPoints);
    std::vector<igIndex> sourcePointIds;
    sourcePointIds.reserve(2 * numberOfInputPoints);

    for (IGsize i = 0; i < numberOfInputPoints; ++i) {
        const Point& oldPoint = inputPoints->GetPoint(i);
        newPoints->AddPoint(oldPoint);
        sourcePointIds.push_back(static_cast<igIndex>(i));
    }
    for (IGsize i = 0; i < numberOfInputPoints; ++i) {
        Vector3d oldPoint;
        inputPoints->GetPoint(i, oldPoint);

        // 三种模式的位移公式与 vtkLinearExtrusionFilter 一致：
        // Vector: displacement = m_Vector
        // Point : displacement = oldPoint - m_ExtrusionPoint
        // Normal: displacement = 该点的点法向
        Vector3d displacement;
        if (effectiveType == ExtrusionType::Vector) {
            displacement = m_Vector;
        } else if (effectiveType == ExtrusionType::Point) {
            displacement = oldPoint - m_ExtrusionPoint;
        } else {
            displacement = Vector3d(pointNormals[i * 3 + 0], pointNormals[i * 3 + 1], pointNormals[i * 3 + 2]);
        }

        const Vector3d newPoint = oldPoint + m_ScaleFactor * displacement;
        newPoints->AddPoint(newPoint);
        sourcePointIds.push_back(static_cast<igIndex>(i));
    }

    // ------------------------------------------------------------------
    // 4~6. 生成输出单元：先按单元类型生成线与侧带，再生成端面，
    //      最后用自由边生成多边形侧裙面。每个输出单元都记录来源 cell ID。
    // ------------------------------------------------------------------
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();
    std::vector<igIndex> sourceCellIds;

    // 记录每个输出单元，同时写入它与来源输入单元的对应关系。
    auto addOutputCell = [&](const igIndex* pointIds, int count, IGenum type, igIndex sourceCellId) {
        outputCells->AddCellIds(pointIds, count);
        outputTypes->AddValue(type);
        sourceCellIds.push_back(sourceCellId);
    };

    // 多边形自由边统计：无向边 (min,max) -> 使用它的多边形单元。
    // 只统计多边形；Vertex / Line / PolyLine 已在下面独立处理，不能参与面边邻接。
    // value 被第二个多边形命中时置空，表示这是内部共享边。
    std::map<std::pair<igIndex, igIndex>, InputCell*> edgeToCell;

    for (auto& cell: inputCells) {
        const igIndex sourceCellId = cell.sourceCellId;
        const std::vector<igIndex>& ids = cell.pointIds;
        const igIndex pointOffset = static_cast<igIndex>(numberOfInputPoints);

        switch (cell.type) {
            case IG_VERTEX: {
                // 顶点拉伸为线，Capping 与它无关（对应 VTK 的 verts -> lines）。
                const igIndex lineIds[2] = {ids[0], static_cast<igIndex>(ids[0] + pointOffset)};
                addOutputCell(lineIds, 2, IG_LINE, sourceCellId);
                break;
            }
            case IG_LINE: {
                // 线拉伸为四边带 [0, 1, 1', 0']，Capping 与它无关。
                if (ids.size() >= 2) {
                    const igIndex quadIds[4] = {ids[0], ids[1], static_cast<igIndex>(ids[1] + pointOffset),
                                                static_cast<igIndex>(ids[0] + pointOffset)};
                    addOutputCell(quadIds, 4, IG_QUAD, sourceCellId);
                }
                break;
            }
            case IG_POLY_LINE: {
                // 折线逐段生成四边带；每段都归属同一条折线。
                for (size_t s = 0; s + 1 < ids.size(); ++s) {
                    const igIndex quadIds[4] = {ids[s], ids[s + 1], static_cast<igIndex>(ids[s + 1] + pointOffset),
                                                static_cast<igIndex>(ids[s] + pointOffset)};
                    addOutputCell(quadIds, 4, IG_QUAD, sourceCellId);
                }
                break;
            }
            default: {
                // Triangle / Quad / Polygon / Face（二维单元）
                if (m_Capping) {
                    // 与 VTK 一致：两端保留相同顶点顺序，不自行反转其中一端。
                    addOutputCell(ids.data(), static_cast<int>(ids.size()), cell.type, sourceCellId);

                    std::vector<igIndex> capIds(ids.size());
                    for (size_t k = 0; k < ids.size(); ++k) {
                        capIds[k] = static_cast<igIndex>(ids[k] + pointOffset);
                    }
                    addOutputCell(capIds.data(), static_cast<int>(capIds.size()), cell.type, sourceCellId);
                }

                // 统计该二维单元的所有边（含首尾闭合边）。
                for (size_t k = 0; k < ids.size(); ++k) {
                    igIndex a = ids[k];
                    igIndex b = ids[(k + 1) % ids.size()];
                    if (a == b) { continue; }
                    if (a > b) { std::swap(a, b); }
                    auto inserted = edgeToCell.emplace(std::make_pair(a, b), &cell);
                    if (!inserted.second) {
                        // 该边已被另一个二维单元使用 -> 内部共享边，不生成侧裙面。
                        inserted.first->second = nullptr;
                    }
                }
                break;
            }
        }
    }

    // 只为自由边（没有邻居单元的边）生成侧裙四边面。
    // 顶点顺序必须是「环状」的 [a, b, b+N, a+N]，不能照抄 VTK 内部 strip 的存储顺序 [a, b, a+N, b+N]：
    // 后者几何上是个自交的蝴蝶结四边形，iGame 按「从 0 号点出发的扇形」拆三角形
    // （见 iGameSurfaceMesh.cpp 的 ConvertToDrawableData），会拆成 (a,b,a+N) 与 (a,a+N,b+N)，
    // 既互相重叠又各留一条缝，侧面上就表现为条状空隙；环状顺序拆出 (a,b,b+N) 与 (a,b+N,a+N)，正好铺满。
    for (const auto& entry: edgeToCell) {
        const InputCell* owner = entry.second;
        if (owner == nullptr) { continue; }  // 内部共享边不生成侧裙面
        const igIndex pointOffset = static_cast<igIndex>(numberOfInputPoints);
        const igIndex quadIds[4] = {entry.first.first, entry.first.second,
                                    static_cast<igIndex>(entry.first.second + pointOffset),
                                    static_cast<igIndex>(entry.first.first + pointOffset)};
        addOutputCell(quadIds, 4, IG_QUAD, owner->sourceCellId);
    }

    // ------------------------------------------------------------------
    // 7~8. 组装输出对象并重建独立的属性集。
    // ------------------------------------------------------------------
    auto output = UnstructuredMesh::New();
    const std::string inputName = input->GetName();
    output->SetName(inputName.empty() ? std::string("LinearExtrusion") : inputName + "_LinearExtrusion");
    output->SetPoints(newPoints);
    output->SetCells(outputCells, outputTypes);

    CopyAttributesBySourceId(input, output, newPoints->GetNumberOfPoints(), sourcePointIds, IG_POINT);
    CopyAttributesBySourceId(input, output, outputCells->GetNumberOfCells(), sourceCellIds, IG_CELL);

    output->Modified();
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
