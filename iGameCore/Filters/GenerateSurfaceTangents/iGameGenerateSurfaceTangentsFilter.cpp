#include "iGameGenerateSurfaceTangentsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <cmath>
#include <string>

IGAME_NAMESPACE_BEGIN

namespace
{
constexpr double IG_TANGENT_UV_EPS = 1.0e-7;

double Length3(const double v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

bool IsFinite(double value) { return std::isfinite(value); }
} // namespace

GenerateSurfaceTangentsFilter::GenerateSurfaceTangentsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool GenerateSurfaceTangentsFilter::CollectTriangles(DataObject::Pointer input,
                                                     std::vector<Triangle>& triangles,
                                                     IGsize& cellCount) const {
    triangles.clear();
    cellCount = 0;

    // 扇形三角化一个多边形（顶点数 n >= 3）：(v0, vi, vi+1)
    auto appendFan = [&](const igIndex* ids, int n, IGsize cellId) {
        for (int i = 1; i + 1 < n; i++) {
            Triangle tri;
            tri.ids[0] = ids[0];
            tri.ids[1] = ids[i];
            tri.ids[2] = ids[i + 1];
            tri.cellId = cellId;
            triangles.push_back(tri);
        }
    };

    // 1) 体网格单独判断
    if (DynamicCast<VolumeMesh>(input) != nullptr) {
        return false;
    }
    // 2) 表面网格
    if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        const IGsize faceCount = mesh->GetNumberOfFaces();
        cellCount = faceCount;
        igIndex ptIds[IGAME_CELL_MAX_SIZE]{};
        for (IGsize f = 0; f < faceCount; f++) {
            const int n = mesh->GetFacePointIds(f, ptIds);
            if (n >= 3) { appendFan(ptIds, n, f); }
        }
        return true;
    }
    // 3) 非结构网格
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        const IGsize count = mesh->GetNumberOfCells();
        cellCount = count;
        igIndex ptIds[IGAME_CELL_MAX_SIZE]{};
        for (IGsize c = 0; c < count; c++) {
            const IGenum type = mesh->GetCellType(c);
            if (type != IG_TRIANGLE && type != IG_QUAD && type != IG_POLYGON && type != IG_FACE) {
                continue;
            }
            const int n = mesh->GetCellPointIds(c, ptIds);
            if (n >= 3) { appendFan(ptIds, n, c); }
        }
        return true;
    }
    return false;
}

int GenerateSurfaceTangentsFilter::FindTextureCoordinates(DataObject::Pointer input) const {
    auto attributeSet = input->GetAttributeSet();
    if (attributeSet == nullptr) { return -1; }

    const IGsize count = static_cast<IGsize>(attributeSet->GetNumberOfAttributes());
    
    if (!m_TexCoordsArrayName.empty()) {
        const int index = attributeSet->GetAttributeIndex(m_TexCoordsArrayName);
        if (index < 0) { return -1; }
        auto& attribute = attributeSet->GetAttribute(static_cast<IGsize>(index));
        if (attribute.isDeleted || attribute.pointer.IsNull()) { return -1; }
        if (attribute.attachmentType != IG_POINT) { return -1; }
        if (attribute.pointer->GetDimension() < 2) { return -1; }
        return index;
    }
    
    for (IGsize i = 0; i < count; i++) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer.IsNull()) { continue; }
        if (attribute.type != IG_TCOORD) { continue; }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetDimension() < 2) { continue; }
        return static_cast<int>(i);
    }
    return -1;
}

DataObject::Pointer GenerateSurfaceTangentsFilter::CopyInput(DataObject::Pointer input) const {
    auto attributeSet = AttributeSet::New();
    attributeSet->DeepCopy(input->GetAttributeSet());

    if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto faces = CellArray::New();
        faces->DeepCopy(mesh->GetFaces());

        auto out = SurfaceMesh::New();
        out->SetPoints(points);
        out->SetFaces(faces);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto cells = CellArray::New();
        cells->DeepCopy(mesh->GetCells());

        auto types = UnsignedIntArray::New();
        types->Resize(mesh->GetNumberOfCells());
        auto inputTypes = mesh->GetCellTypes();
        if (inputTypes != nullptr) {
            for (IGsize i = 0; i < mesh->GetNumberOfCells(); i++) {
                types->SetValue(i, inputTypes->GetValue(i));
            }
        }

        auto out = UnstructuredMesh::New();
        out->SetPoints(points);
        out->SetCells(cells, types);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    return nullptr;
}

bool GenerateSurfaceTangentsFilter::SetAttribute(DataObject::Pointer output, const std::string& name,
                                                 IGenum type, IGenum attachmentType,
                                                 ArrayObject::Pointer array) {
    auto attributeSet = output->GetAttributeSet();
    if (attributeSet == nullptr) { return false; }

    const IGsize count = static_cast<IGsize>(attributeSet->GetNumberOfAttributes());
    for (IGsize i = 0; i < count; i++) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.pointer.IsNull()) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (attribute.pointer->GetName() != name) { continue; }

        attribute.pointer = array;
        attribute.isDeleted = false;
        attribute.type = type;
        attribute.attachmentType = attachmentType;
        attribute.UpdateAllDataRange();
        return true;
    }

    const IGsize index = attributeSet->AddAttribute(type, attachmentType, array);
    if (index < 0) { return false; }
    attributeSet->GetAttribute(index).UpdateAllDataRange();
    return true;
}

bool GenerateSurfaceTangentsFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) {
        igError("GenerateSurfaceTangentsFilter: 输入为空。");
        return false;
    }

    auto points = input->GetPoints();
    if (points.IsNull() || points->GetNumberOfPoints() == 0) {
        igError("GenerateSurfaceTangentsFilter: 输入数据里没有点，无法计算表面切向量。");
        return false;
    }
    const IGsize pointCount = points->GetNumberOfPoints();

    // 1) 纹理坐标
    auto attributeSet = input->GetAttributeSet();
    const int texIndex = FindTextureCoordinates(input);
    if (texIndex < 0) {
        if (!m_TexCoordsArrayName.empty()) {
            igError("GenerateSurfaceTangentsFilter: 找不到可用的纹理坐标点属性 \"{}\""
                    "（要求是点属性、至少 2 个分量）。", m_TexCoordsArrayName);
        } else {
            igError("GenerateSurfaceTangentsFilter: 输入里没有纹理坐标点属性（IG_TCOORD，至少 2 个分量）。"
                    "切向量必须由纹理坐标算出来，请先给模型生成纹理坐标（例如「球面纹理坐标」filter）。");
        }
        return false;
    }
    auto& texAttribute = attributeSet->GetAttribute(static_cast<IGsize>(texIndex));
    auto texCoords = texAttribute.pointer;
    if (texCoords->GetNumberOfElements() < pointCount) {
        igError("GenerateSurfaceTangentsFilter: 纹理坐标数组 \"{}\" 的元素个数（{}）比点数（{}）少。",
                texCoords->GetName(), texCoords->GetNumberOfElements(), pointCount);
        return false;
    }
    const int texDim = texCoords->GetDimension();
    igDebug("GenerateSurfaceTangentsFilter: 纹理坐标 = \"{}\"（{} 个分量，用前两个）",
            texCoords->GetName(), texDim);

    // 2) 把面/单元拆成三角形
    std::vector<Triangle> triangles;
    IGsize cellCount = 0;
    if (!CollectTriangles(input, triangles, cellCount)) {
        igError("GenerateSurfaceTangentsFilter: 不支持的输入类型（支持表面网格 SurfaceMesh、"
                "非结构网格 UnstructuredMesh 的三角形/四边形/多边形单元）。"
                "如果输入是体网格，请先做「转换为表面网格 (Convert To Surface Mesh)」。");
        return false;
    }
    if (triangles.empty()) {
        igError("GenerateSurfaceTangentsFilter: 输入里没有三角形/四边形/多边形单元，"
                "无法计算表面切向量（点集和体网格都不行）。");
        return false;
    }

    // 3) 逐三角形累加 dP/du（点切向量）和 dP/dv（副切向量），并记录每个单元的 dP/du
    std::vector<double> accumTangent(3 * pointCount, 0.0);
    std::vector<double> accumBitangent(3 * pointCount, 0.0);
    std::vector<double> cellTangents(3 * cellCount, 0.0);
    IGsize skippedTriangles = 0;
    IGsize handledTriangles = 0;

    const IGsize triangleCount = static_cast<IGsize>(triangles.size());
    for (IGsize t = 0; t < triangleCount; t++) {
        const Triangle& tri = triangles[t];
        const auto& p0 = points->GetPoint(static_cast<IGsize>(tri.ids[0]));
        const auto& p1 = points->GetPoint(static_cast<IGsize>(tri.ids[1]));
        const auto& p2 = points->GetPoint(static_cast<IGsize>(tri.ids[2]));

        const double uv0[2] = {texCoords->GetElementValue(static_cast<IGsize>(tri.ids[0]), 0),
                               texCoords->GetElementValue(static_cast<IGsize>(tri.ids[0]), 1)};
        const double uv1[2] = {texCoords->GetElementValue(static_cast<IGsize>(tri.ids[1]), 0),
                               texCoords->GetElementValue(static_cast<IGsize>(tri.ids[1]), 1)};
        const double uv2[2] = {texCoords->GetElementValue(static_cast<IGsize>(tri.ids[2]), 0),
                               texCoords->GetElementValue(static_cast<IGsize>(tri.ids[2]), 1)};

        const double e1[3] = {static_cast<double>(p1[0]) - p0[0], static_cast<double>(p1[1]) - p0[1],
                              static_cast<double>(p1[2]) - p0[2]};
        const double e2[3] = {static_cast<double>(p2[0]) - p0[0], static_cast<double>(p2[1]) - p0[1],
                              static_cast<double>(p2[2]) - p0[2]};
        const double d1[2] = {uv1[0] - uv0[0], uv1[1] - uv0[1]};
        const double d2[2] = {uv2[0] - uv0[0], uv2[1] - uv0[1]};

        const double den = d1[0] * d2[1] - d2[0] * d1[1];
        const double denScale = std::sqrt((d1[0] * d1[0] + d1[1] * d1[1]) *
                                          (d2[0] * d2[0] + d2[1] * d2[1]));
        if (!(denScale > 0.0) || std::fabs(den) <= IG_TANGENT_UV_EPS * denScale) {
            // 纹理坐标退化
            skippedTriangles++;
            continue;
        }

        const double r = 1.0 / den;
        const double sdir[3] = {(e1[0] * d2[1] - e2[0] * d1[1]) * r,
                                (e1[1] * d2[1] - e2[1] * d1[1]) * r,
                                (e1[2] * d2[1] - e2[2] * d1[1]) * r};
        const double tdir[3] = {(e2[0] * d1[0] - e1[0] * d2[0]) * r,
                                (e2[1] * d1[0] - e1[1] * d2[0]) * r,
                                (e2[2] * d1[0] - e1[2] * d2[0]) * r};
        if (!IsFinite(sdir[0]) || !IsFinite(sdir[1]) || !IsFinite(sdir[2]) ||
            !IsFinite(tdir[0]) || !IsFinite(tdir[1]) || !IsFinite(tdir[2])) {
            skippedTriangles++;
            continue;
        }

        // 点切向量 / 副切向量：每个三角形等权累加到它的三个点上（和 vtkPolyDataTangents 一致）
        for (int k = 0; k < 3; k++) {
            const IGsize id = static_cast<IGsize>(tri.ids[k]);
            accumTangent[3 * id + 0] += sdir[0];
            accumTangent[3 * id + 1] += sdir[1];
            accumTangent[3 * id + 2] += sdir[2];
            accumBitangent[3 * id + 0] += tdir[0];
            accumBitangent[3 * id + 1] += tdir[1];
            accumBitangent[3 * id + 2] += tdir[2];
        }
        // 单元切向量：单元自己的 dP/du（三角形单元就是该三角形的值，不归一化）
        if (m_ComputeCellTangents && tri.cellId < cellCount) {
            cellTangents[3 * tri.cellId + 0] += sdir[0];
            cellTangents[3 * tri.cellId + 1] += sdir[1];
            cellTangents[3 * tri.cellId + 2] += sdir[2];
        }
        handledTriangles++;

        if ((t & 0x3FF) == 0) {
            UpdateProgress(0.7 * static_cast<double>(t) / static_cast<double>(triangleCount));
        }
    }

    if (handledTriangles == 0) {
        igError("GenerateSurfaceTangentsFilter: 所有三角形的纹理坐标都退化（uv 共线），"
                "无法计算切向量。请检查模型的纹理坐标。");
        return false;
    }
    if (skippedTriangles > 0) {
        igDebug("GenerateSurfaceTangentsFilter: 有 {} 个三角形的纹理坐标退化，已跳过"
                "（VTK 在这些单元上会算出 NaN）。", skippedTriangles);
    }

    // 4) 归一化点切向量；副切向量再对点切向量做一次正交化
    auto tangents = FloatArray::New();
    tangents->SetDimension(3);
    tangents->SetName(m_PointTangentsArrayName);
    tangents->Resize(pointCount);

    auto bitangents = FloatArray::New();
    bitangents->SetDimension(3);
    bitangents->SetName(m_BitangentsArrayName);
    bitangents->Resize(pointCount);

    IGsize zeroTangentPoints = 0;
    for (IGsize i = 0; i < pointCount; i++) {
        const double t[3] = {accumTangent[3 * i + 0], accumTangent[3 * i + 1], accumTangent[3 * i + 2]};
        double tangent[3] = {0.0, 0.0, 0.0};
        const double tangentLength = Length3(t);
        if (tangentLength > 0.0 && IsFinite(tangentLength)) {
            tangent[0] = t[0] / tangentLength;
            tangent[1] = t[1] / tangentLength;
            tangent[2] = t[2] / tangentLength;
            if (!IsFinite(tangent[0]) || !IsFinite(tangent[1]) || !IsFinite(tangent[2])) {
                tangent[0] = tangent[1] = tangent[2] = 0.0;
            }
        }
        if (tangentLength <= 0.0) { zeroTangentPoints++; }

        tangents->SetValue(3 * i + 0, tangent[0]);
        tangents->SetValue(3 * i + 1, tangent[1]);
        tangents->SetValue(3 * i + 2, tangent[2]);

        // 副切向量 = dP/dv 去掉和点切向量重合的部分，再归一化
        double bitangent[3] = {accumBitangent[3 * i + 0], accumBitangent[3 * i + 1],
                               accumBitangent[3 * i + 2]};
        const double projection = bitangent[0] * tangent[0] + bitangent[1] * tangent[1] +
                                  bitangent[2] * tangent[2];
        bitangent[0] -= projection * tangent[0];
        bitangent[1] -= projection * tangent[1];
        bitangent[2] -= projection * tangent[2];
        const double bitangentLength = Length3(bitangent);
        if (bitangentLength > 0.0 && IsFinite(bitangentLength)) {
            bitangent[0] /= bitangentLength;
            bitangent[1] /= bitangentLength;
            bitangent[2] /= bitangentLength;
        } else {
            bitangent[0] = bitangent[1] = bitangent[2] = 0.0;
        }
        bitangents->SetValue(3 * i + 0, bitangent[0]);
        bitangents->SetValue(3 * i + 1, bitangent[1]);
        bitangents->SetValue(3 * i + 2, bitangent[2]);

        if ((i & 0x3FF) == 0) {
            UpdateProgress(0.7 + 0.2 * static_cast<double>(i) / static_cast<double>(pointCount));
        }
    }
    if (zeroTangentPoints > 0) {
        igDebug("GenerateSurfaceTangentsFilter: 有 {} 个点的切向量正好抵消为零向量"
                "（孤立点，或周围三角形的切向贡献正好抵消）。", zeroTangentPoints);
    }

    auto cellTangentArray = FloatArray::New();
    cellTangentArray->SetDimension(3);
    cellTangentArray->SetName(m_CellTangentsArrayName);
    cellTangentArray->Resize(cellCount);
    for (IGsize c = 0; c < cellCount; c++) {
        cellTangentArray->SetValue(3 * c + 0, cellTangents[3 * c + 0]);
        cellTangentArray->SetValue(3 * c + 1, cellTangents[3 * c + 1]);
        cellTangentArray->SetValue(3 * c + 2, cellTangents[3 * c + 2]);
    }

    // 5) 独立输出：复制输入后在副本上写切向量，输入保持不变
    auto out = CopyInput(input);
    if (out.IsNull()) {
        igError("GenerateSurfaceTangentsFilter: 不支持的网格类型（支持表面网格 / 非结构网格）。");
        return false;
    }
    if (m_ComputePointTangents) {
        if (!SetAttribute(out, m_PointTangentsArrayName, IG_VECTOR, IG_POINT, tangents)) {
            igError("GenerateSurfaceTangentsFilter: 写入点切向量数组 \"{}\" 失败。", m_PointTangentsArrayName);
            return false;
        }
    }
    if (m_ComputeCellTangents) {
        if (!SetAttribute(out, m_CellTangentsArrayName, IG_VECTOR, IG_CELL, cellTangentArray)) {
            igError("GenerateSurfaceTangentsFilter: 写入单元切向量数组 \"{}\" 失败。", m_CellTangentsArrayName);
            return false;
        }
    }
    if (m_ComputeBitangents) {
        if (!SetAttribute(out, m_BitangentsArrayName, IG_VECTOR, IG_POINT, bitangents)) {
            igError("GenerateSurfaceTangentsFilter: 写入副切向量数组 \"{}\" 失败。", m_BitangentsArrayName);
            return false;
        }
    }
    out->SetName(input->GetName() + "_tangents");

    UpdateProgress(1.0);
    SetOutput(0, out);
    return true;
}

IGAME_NAMESPACE_END
