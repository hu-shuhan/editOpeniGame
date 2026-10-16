#include "iGameBoundaryMeshQualityFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"

#include <cfloat>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

BoundaryMeshQualityFilter::BoundaryMeshQualityFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

BoundaryMeshQualityFilter::~BoundaryMeshQualityFilter() = default;

// 检查面是否为退化面（面积为零或点数不足）
bool BoundaryMeshQualityFilter::IsDegenerateFace(igIndex faceId) const {
    if (!m_VolumeMesh) return true;

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    int npts = m_VolumeMesh->GetFacePointIds(faceId, ptIds);
    if (npts < 3) return true;

    // 计算面面积（使用叉积的模长）
    if (npts < 3) return true;

    Point p0 = m_VolumeMesh->GetPoint(ptIds[0]);
    Point p1 = m_VolumeMesh->GetPoint(ptIds[1]);
    Point p2 = m_VolumeMesh->GetPoint(ptIds[2]);

    Vector3f v01 = p1 - p0;
    Vector3f v02 = p2 - p0;
    Vector3f cross = v01.cross(v02);

    // 面积为零
    if (cross.norm() < 1e-12f) return true;

    // 检查是否有重复点
    for (int i = 0; i < npts; ++i) {
        for (int j = i + 1; j < npts; ++j) {
            if (ptIds[i] == ptIds[j]) return true;
        }
    }

    return false;
}

// 检查体单元是否为退化单元（体积为零或点数不足）
bool BoundaryMeshQualityFilter::IsDegenerateVolume(Volume* vol) const {
    if (vol == nullptr) return true;
    int npts = vol->GetNumberOfPoints();
    if (npts < 4) return true;

    // 简单检查：获取体单元的顶点，如果所有顶点都在同一平面上或体积为零则为退化
    Point p[4];
    for (int i = 0; i < 4 && i < npts; ++i) {
        p[i] = vol->GetPoint(i);
    }

    if (npts >= 4) {
        // 计算四面体体积（使用混合积）
        Vector3f v1 = p[1] - p[0];
        Vector3f v2 = p[2] - p[0];
        Vector3f v3 = p[3] - p[0];
        double volScalar = std::abs(v1.dot(v2.cross(v3))) / 6.0;
        if (volScalar < 1e-12) return true;
    }

    return false;
}

bool BoundaryMeshQualityFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "No input";
        return false;
    }

    auto input = m_Inputs->GetElement(0);
    if (!input) {
        m_Message = "Empty input";
        return false;
    }

    // 将输入统一转换为体网格
    m_VolumeMesh = nullptr;
    switch (input->GetDataObjectType()) {
        case IG_VOLUME_MESH:
            m_VolumeMesh = DynamicCast<VolumeMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            auto converter = ConvertToVolumeMeshFilter::New();
            converter->SetInput(mesh);
            if (converter->Execute()) {
                m_VolumeMesh = converter->GetVolumeMesh();
            }
            break;
        }
        default:
            break;
    }

    if (!m_VolumeMesh) {
        m_Message = "Boundary mesh quality requires a volume mesh as input. "
                    "Please load a volume mesh or extract the surface mesh first.";
        return false;
    }

    // 确保面表、面-体邻接等拓扑已构建
    m_VolumeMesh->RequestEditStatus();

    igIndex faceNum = m_VolumeMesh->GetNumberOfFaces();
    if (faceNum <= 0) {
        m_Message = "Volume mesh has no faces.";
        return false;
    }

    // 收集所有边界面
    std::vector<igIndex> boundaryFaceIds;
    boundaryFaceIds.reserve(faceNum);
    for (igIndex fid = 0; fid < faceNum; ++fid) {
        if (m_VolumeMesh->IsBoundaryFace(fid)) {
            boundaryFaceIds.push_back(fid);
        }
    }

    if (boundaryFaceIds.empty()) {
        m_Message = "No boundary faces found in the volume mesh.";
        return false;
    }

    const double NaN = std::numeric_limits<double>::quiet_NaN();

    // ============================================================
    // 创建独立的 SurfaceMesh 输出，只包含边界面对应的数据
    // ============================================================
    SurfaceMesh::Pointer outputMesh = SurfaceMesh::New();
    outputMesh->SetName(input->GetName() + "_BoundaryQuality");

    // 创建点数组：收集所有边界面的顶点
    // 映射：原始点ID -> 新点ID
    std::vector<igIndex> pointIdMapping(m_VolumeMesh->GetNumberOfPoints(), -1);
    Points::Pointer newPoints = Points::New();

    for (igIndex faceId : boundaryFaceIds) {
        igIndex ptIds[IGAME_CELL_MAX_SIZE];
        int npts = m_VolumeMesh->GetFacePointIds(faceId, ptIds);
        for (int i = 0; i < npts; ++i) {
            if (pointIdMapping[ptIds[i]] == -1) {
                pointIdMapping[ptIds[i]] = newPoints->GetNumberOfPoints();
                Point p = m_VolumeMesh->GetPoint(ptIds[i]);
                newPoints->AddPoint(p);
            }
        }
    }

    // 创建面数组
    CellArray::Pointer newFaces = CellArray::New();
    for (igIndex faceId : boundaryFaceIds) {
        igIndex origPtIds[IGAME_CELL_MAX_SIZE];
        int npts = m_VolumeMesh->GetFacePointIds(faceId, origPtIds);

        // 转换为新点ID
        igIndex newPtIds[IGAME_CELL_MAX_SIZE];
        for (int i = 0; i < npts; ++i) {
            newPtIds[i] = pointIdMapping[origPtIds[i]];
        }
        newFaces->AddCellIds(newPtIds, npts);
    }

    outputMesh->SetPoints(newPoints);
    outputMesh->SetFaces(newFaces);

    // ============================================================
    // 创建面属性：每个边界面对应一个数组元素
    // ============================================================
    DoubleArray::Pointer metricArray = DoubleArray::New();
    switch (m_Metric) {
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER:
            metricArray->SetName("DistanceFromCellCenterToFaceCenter");
            break;
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE:
            metricArray->SetName("DistanceFromCellCenterToFacePlane");
            break;
        case ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR:
            metricArray->SetName("AngleFaceNormalAndCellCenterToFaceCenterVector");
            break;
        default:
            metricArray->SetName("BoundaryMetric");
            break;
    }

    metricArray->SetDimension(1);
    metricArray->Reserve(static_cast<int>(boundaryFaceIds.size()));

    // 计算每个边界面的指标值
    int progressCount = 0;
    int totalBoundaryFaces = static_cast<int>(boundaryFaceIds.size());
    int reportBlock = std::max(1, totalBoundaryFaces / 50);

    for (size_t i = 0; i < boundaryFaceIds.size(); ++i) {
        if (static_cast<int>(i) > reportBlock * progressCount) {
            progressCount++;
            UpdateProgress(progressCount * 0.02);
        }

        igIndex faceId = boundaryFaceIds[i];

        // 检查退化面
        bool isDegenerate = IsDegenerateFace(faceId);

        if (isDegenerate) {
            // 退化面写入 NaN
            metricArray->AddValue(NaN);
        } else {
            // 正常计算
            double metric = ComputeMetricForBoundaryFace(faceId);
            metricArray->AddValue(metric);
        }
    }

    // 对于 AngleFaceNormalAndCellCenterToFaceCenterVector，ComputeMetricForBoundaryFace
    // 已经返回 [0, 180]° 的夹角（与 ParaView/VTK 一致），无需再做归一化。

    // 将属性附加到面（IG_CELL 对应 SurfaceMesh 的面）
    auto attrs = outputMesh->GetAttributeSet();
    attrs->AddAttribute(IG_SCALAR, IG_CELL, metricArray);

    // 确保 GPU 数据已更新
    attrs->ForceReConvertToDrawableData();
    outputMesh->Modified();

    this->SetOutput(outputMesh);
    return true;
}

Point BoundaryMeshQualityFilter::ComputeCellCenter(Volume* cell) {
    Point center(0, 0, 0);
    int n = cell->GetNumberOfPoints();
    if (n <= 0) return center;
    for (int i = 0; i < n; ++i) {
        Point p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
    }
    center[0] /= n;
    center[1] /= n;
    center[2] /= n;
    return center;
}

double BoundaryMeshQualityFilter::AngleInDegrees(const Vector3f& a, const Vector3f& b) {
    double la = a.length();
    double lb = b.length();
    if (la < 1e-12 || lb < 1e-12) return 0.0;
    double cosTheta = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (la * lb);
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta));
    // std::acos 输出 [0, π]，再乘 180/π -> [0, 180]°，与 ParaView/VTK 行为一致。
    return std::acos(cosTheta) * 180.0 / PI;
}

// ParaView/VTK 的面法线采用 Newell 法（vtkPolyDataNormals 在 ConsistencyOff 时的默认实现）。
// 相比 (p1-p0)×(p2-p0) 用前三点叉乘，Newell 法对非平面多边形更稳健。
Vector3f BoundaryMeshQualityFilter::ComputeFaceNormalNewell(const igIndex* facePids,
                                                            int facePcnt,
                                                            const VolumeMesh* mesh) {
    Vector3f n(0, 0, 0);
    if (facePcnt < 3 || mesh == nullptr) return n;

    Point prev = mesh->GetPoint(facePids[facePcnt - 1]);
    for (int i = 0; i < facePcnt; ++i) {
        Point cur = mesh->GetPoint(facePids[i]);
        n[0] += (prev[1] - cur[1]) * (prev[2] + cur[2]);
        n[1] += (prev[2] - cur[2]) * (prev[0] + cur[0]);
        n[2] += (prev[0] - cur[0]) * (prev[1] + cur[1]);
        prev = cur;
    }
    double len = n.length();
    if (len > 1e-12) {
        n[0] /= static_cast<float>(len);
        n[1] /= static_cast<float>(len);
        n[2] /= static_cast<float>(len);
    } else {
        n[0] = n[1] = n[2] = 0.0f;
    }
    return n;
}

double BoundaryMeshQualityFilter::ComputeMetricForBoundaryFace(igIndex faceId) {
    if (!m_VolumeMesh) return 0.0;

    // 获取与该边界面相邻的体单元
    igIndex volIds[64];
    int volSize = m_VolumeMesh->GetFaceToNeighborVolumes(faceId, volIds);
    if (volSize <= 0) return 0.0;
    igIndex volId = volIds[0];

    // 获取体单元、面及对应的几何信息
    Volume* vol = m_VolumeMesh->GetVolume(volId);
    if (vol == nullptr) return 0.0;

    // 检查体单元是否为退化单元
    if (IsDegenerateVolume(vol)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    igIndex facePids[IGAME_CELL_MAX_SIZE];
    int facePcnt = m_VolumeMesh->GetFacePointIds(faceId, facePids);
    if (facePcnt <= 0) return 0.0;

    Point cellCenter = ComputeCellCenter(vol);

    // 手动计算面中心
    Point faceCenter(0, 0, 0);
    for (int i = 0; i < facePcnt; ++i) {
        Point p = m_VolumeMesh->GetPoint(facePids[i]);
        faceCenter[0] += p[0];
        faceCenter[1] += p[1];
        faceCenter[2] += p[2];
    }
    faceCenter[0] /= facePcnt;
    faceCenter[1] /= facePcnt;
    faceCenter[2] /= facePcnt;

    // 面法线：使用 Newell 法（与 vtkPolyDataNormals 在 ConsistencyOff 时一致）
    Vector3f faceNormal = ComputeFaceNormalNewell(facePids, facePcnt, m_VolumeMesh);
    const double nlen = faceNormal.length();

    // 体单元中心 -> 面中心向量
    Vector3f centerVec(
        faceCenter[0] - cellCenter[0],
        faceCenter[1] - cellCenter[1],
        faceCenter[2] - cellCenter[2]);

    switch (m_Metric) {
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER:
            return centerVec.length();

        case DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE: {
            // 平面方程: n . (x - faceP0) = 0; 距离 = |n . (cellCenter - faceP0)|
            if (nlen < 1e-12) return std::numeric_limits<double>::quiet_NaN();
            Point faceP0 = m_VolumeMesh->GetPoint(facePids[0]);
            double d = faceNormal[0] * (cellCenter[0] - faceP0[0]) +
                       faceNormal[1] * (cellCenter[1] - faceP0[1]) +
                       faceNormal[2] * (cellCenter[2] - faceP0[2]);
            return std::fabs(d);
        }

        case ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR: {
            // 模仿 ParaView：归一化 (faceCenter - cellCenter) 与面法线的夹角
            // AngleInDegrees 用 acos → [0, 180]°，与 vtkMath::AngleBetweenVectors 一致。
            if (nlen < 1e-12) return std::numeric_limits<double>::quiet_NaN();
            double lv = centerVec.length();
            if (lv < 1e-12) return std::numeric_limits<double>::quiet_NaN();
            Vector3f normVec(
                centerVec[0] / lv,
                centerVec[1] / lv,
                centerVec[2] / lv);
            return AngleInDegrees(faceNormal, normVec);
        }

        default:
            return 0.0;
    }
}

IGAME_NAMESPACE_END
