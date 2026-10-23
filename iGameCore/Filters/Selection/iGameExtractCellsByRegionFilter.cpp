#include "iGameExtractCellsByRegionFilter.h"
#include <iGameAttributeSet.h>
#include <iGameCell.h>
#include <iGamePoints.h>
#include <iGameType.h>
#include <cmath>
IGAME_NAMESPACE_BEGIN

ExtractCellsByRegionFilter::ExtractCellsByRegionFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1); // 输出网格，不是 0
}

void ExtractCellsByRegionFilter::SetBox(const Vector3d& min, const Vector3d& max) {
    m_RegionType = BOX;
    // 校验每维 min <= max；任一维不满足即视为非法区域，置空后 Execute 会因 m_Box.isNull() 返回 false
    if (min[0] > max[0] || min[1] > max[1] || min[2] > max[2]) {
        m_Box.setNull();
        return;
    }
    m_Box = BoundingBox(min, max);
}

void ExtractCellsByRegionFilter::SetSphere(const Vector3d& center, double radius) {
    m_RegionType = SPHERE;
    m_Center = center;
    m_Radius = radius;
}

void ExtractCellsByRegionFilter::SetRequireAllPoints(bool requireAllPoints) {
    m_RequireAllPoints = requireAllPoints;
}

void ExtractCellsByRegionFilter::SetOrientedBox(const Vector3d& center, const Vector3d& halfSize,
                                               const std::array<Vector3d, 3>& axes) {
    m_RegionType = ORIENTED_BOX;
    m_Center = center;
    m_HalfSize = halfSize;
    m_BoxAxes = axes;
}

void ExtractCellsByRegionFilter::SetPlane(const Vector3d& origin, const Vector3d& normal) {
    m_RegionType = PLANE;
    m_Center = origin;
    m_Normal = normal;
}

bool ExtractCellsByRegionFilter::IsPointInRegion(const Vector3d& p) const {
    if (m_RegionType == PLANE) return (p - m_Center).dot(m_Normal) <= 0.;
    if (m_RegionType == ORIENTED_BOX) {
        const auto offset = p - m_Center;
        for (int i = 0; i < 3; ++i)
            if (std::abs(offset.dot(m_BoxAxes[i])) > m_HalfSize[i]) return false;
        return true;
    }
    if (m_RegionType == BOX) {
        return m_Box.isIn(p); // 含边界：min <= p <= max
    }
    return (p - m_Center).squaredNorm() <= m_Radius * m_Radius; // 平方避免开方
}

bool ExtractCellsByRegionFilter::Preview() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    m_Ids.clear();
    if (m_Mesh.IsNull()) return false;

    // 区域合法性校验（防止 BOX 忘赋值 → 静默 0 个 cell）
    if (m_RegionType == BOX && m_Box.isNull()) return false;
    if (m_RegionType == SPHERE && m_Radius <= 0) return false;
    for (int i = 0; i < 3; ++i) {
        if (m_RegionType != BOX && !std::isfinite(m_Center[i])) return false;
        if (m_RegionType == PLANE && !std::isfinite(m_Normal[i])) return false;
        if (m_RegionType == ORIENTED_BOX) {
            if (!std::isfinite(m_HalfSize[i]) || m_HalfSize[i] <= 0.) return false;
            for (int j = 0; j < 3; ++j) if (!std::isfinite(m_BoxAxes[i][j])) return false;
            if (std::abs(m_BoxAxes[i].squaredNorm() - 1.) > 1.e-5) return false;
            for (int j = 0; j < i; ++j) if (std::abs(m_BoxAxes[i].dot(m_BoxAxes[j])) > 1.e-5) return false;
        }
    }
    if (m_RegionType == PLANE && (!std::isfinite(m_Normal.squaredNorm()) || m_Normal.squaredNorm() < 1.e-24)) return false;
    if (m_RegionType == SPHERE && !std::isfinite(m_Radius)) return false;

    const IGsize cellNum = m_Mesh->GetNumberOfCells();
    for (IGsize cellId = 0; cellId < cellNum; cellId++) {
        auto cell = m_Mesh->GetCell(cellId);
        if (!cell || cell->GetNumberOfPoints() == 0) continue;
        int n = cell->GetNumberOfPoints();
        bool inRegion;
        if (m_RequireAllPoints) { // 严格：所有顶点都在区域内
            inRegion = true;
            for (int i = 0; i < n; i++) {
                if (!IsPointInRegion(cell->GetPoint(i))) {
                    inRegion = false;
                    break;
                }
            }
        } else { // 宽松：任一顶点在区域内
            inRegion = false;
            for (int i = 0; i < n; i++) {
                if (IsPointInRegion(cell->GetPoint(i))) {
                    inRegion = true;
                    break;
                }
            }
        }
        if (inRegion) m_Ids.push_back(cellId);
        if (cellNum > 0) UpdateProgress((double)(cellId + 1) / cellNum); // 汇报进度
    }

    return true;
}

bool ExtractCellsByRegionFilter::Execute() {
    if (!Preview()) return false;
    BuildOutputMesh();
    SetOutput(m_OutputMesh);
    return true;
}

namespace {

// 按输入数组的实际底层类型创建输出数组，避免属性复制时统一转换成 float。
ArrayObject::Pointer CreateArrayOfSameType(ArrayObject* inArray) {
    switch (inArray->GetArrayType()) {
        case IG_FloatArray: return FloatArray::New();
        case IG_DoubleArray: return DoubleArray::New();
        case IG_IntArray:
        case IG_INTARRAY: return IntArray::New();
        case IG_UnsignedIntArray: return UnsignedIntArray::New();
        case IG_CharArray: return CharArray::New();
        case IG_UnsignedCharArray: return UnsignedCharArray::New();
        case IG_ShortArray: return ShortArray::New();
        case IG_UnsignedShortArray: return UnsignedShortArray::New();
        case IG_LongLongArray: return LongLongArray::New();
        case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
        default: return FloatArray::New();
    }
}

} // namespace

void ExtractCellsByRegionFilter::BuildOutputMesh() {
    auto outMesh = UnstructuredMesh::New();
    auto outPoints = Points::New();
    Points::Pointer inPoints = m_Mesh->GetPoints();
    const IGsize inPointNum = m_Mesh->GetNumberOfPoints();

    // 原网格点号 -> 输出网格点号；-1 表示该点没被任何被选 cell 引用，直接丢弃
    std::vector<int> oldToNew(inPointNum, -1);

    for (int cellId : m_Ids) {
        const igIndex* oldIds = nullptr;
        const int n = m_Mesh->GetCellPointIds(cellId, oldIds);
        std::vector<igIndex> newIds(oldIds, oldIds + n);
        auto remapPoint = [&](int i) {
            const igIndex old = oldIds[i];
            if (oldToNew[old] < 0) { // 首次用到才拷入新点集，输出点数随提取变小
                oldToNew[old] = static_cast<int>(outPoints->GetNumberOfPoints());
                outPoints->AddPoint(inPoints->GetPoint(old));
            }
            newIds[i] = static_cast<igIndex>(oldToNew[old]); // 重映射成新点号
        };
        if (m_Mesh->GetCellType(cellId) == IG_POLYHEDRON) {
            // 多面体存储为 [面数, 面顶点数, 点号..., ...]，计数必须原样保留。
            // 使用动态数组，避免复杂多面体的面流超过固定单元大小。
            int offset = 1;
            for (int face = 0; face < oldIds[0]; ++face) {
                const int faceSize = oldIds[offset++];
                for (int p = 0; p < faceSize; ++p) remapPoint(offset++);
            }
        } else {
            for (int i = 0; i < n; ++i) remapPoint(i);
        }
        outMesh->AddCell(newIds.data(), n, m_Mesh->GetCellType(cellId));
    }
    outMesh->SetPoints(outPoints); // 独立的新点集，不再与输入共享

    CopyAttributeDataToOutput(outMesh, oldToNew); // 点/单元属性按映射搬运，避免整份丢失

    m_OutputMesh = outMesh;
}

void ExtractCellsByRegionFilter::CopyAttributeDataToOutput(const UnstructuredMesh::Pointer& outMesh,
                                                           const std::vector<int>& oldToNew) {
    AttributeSet::Pointer inData = m_Mesh->GetAttributeSet();
    if (inData.IsNull()) return;
    auto inAllAttr = inData->GetAllAttributes();
    if (inAllAttr.IsNull()) return;

    const IGsize outPointNum = outMesh->GetNumberOfPoints();
    const IGsize outCellNum = outMesh->GetNumberOfCells();
    if (outPointNum == 0 && outCellNum == 0) return; // 空提取，无需搬运属性

    // 输出新点号 -> 原网格点号（oldToNew 的反查表）
    std::vector<int> newToOld(outPointNum, -1);
    for (IGsize o = 0; o < oldToNew.size(); o++) {
        if (oldToNew[o] >= 0) newToOld[oldToNew[o]] = static_cast<int>(o);
    }

    auto outData = AttributeSet::New();
    const IGsize attrNum = inAllAttr->GetNumberOfElements();
    double values[IGAME_CELL_MAX_SIZE]{};
    for (IGsize a = 0; a < attrNum; a++) {
        auto attr = inAllAttr->GetElement(a);
        if (attr.isDeleted || attr.pointer.IsNull()) continue;
        auto inArray = attr.pointer;
        auto outArray = CreateArrayOfSameType(inArray);
        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        if (attr.attachmentType == IG_CELL) { // 单元属性：按“被选中 cell 的原序号”逐行搬运
            outArray->Resize(outCellNum);
            for (IGsize j = 0; j < outCellNum; j++) {
                inArray->GetElement(m_Ids[j], values);
                outArray->SetElement(j, values);
            }
        } else if (attr.attachmentType == IG_POINT) { // 点属性：按“输出点 -> 原网格点”逐行搬运
            outArray->Resize(outPointNum);
            for (IGsize j = 0; j < outPointNum; j++) {
                inArray->GetElement(newToOld[j], values);
                outArray->SetElement(j, values);
            }
        } else {
            continue;
        }
        outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
    }
    outMesh->SetAttributeSet(outData);
}

IGAME_NAMESPACE_END
