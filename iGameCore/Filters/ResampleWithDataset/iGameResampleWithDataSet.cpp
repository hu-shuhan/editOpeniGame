#include "iGameResampleWithDataSet.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameCompositeDataObject.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

namespace {

// 自动容差比例：容差 = 包围盒对角线 × kAutoToleranceRatio
constexpr double kAutoToleranceRatio = 1e-6;
// 形函数参数坐标容差
constexpr double kParamTol = 1e-6;
// 单元包含判定的参数域容差，对齐 VTK EvaluatePosition：pcoords ∈ [-0.001, 1.001]（[0,1] 参数）
constexpr double kVTKParamDomainTol = 1e-3;
// 支持插值的最大单元点数
constexpr int kMaxCellPointNum = 32;

// 复制单元数组，避免 CellArray::DeepCopy 在混合单元尺寸下产生错误偏移
CellArray::Pointer DeepCopyCellArray(CellArray* source) {
    auto output = CellArray::New();
    if (!source) {
        return output;
    }

    for (IGsize i = 0; i < source->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = source->GetCellIds(i, ids);
        output->AddCellIds(ids, count);
    }

    return output;
}

// 复制 PointSet 共有的点坐标和属性
void DeepCopyPointSetBase(PointSet* output, PointSet* input, AttributeSet::Pointer attributes) {
    auto points = Points::New();
    if (input->GetPoints()) {
        points->DeepCopy(input->GetPoints());
    }
    output->SetPoints(points);

    if (attributes != nullptr) {
        // 调用方给定属性集：几何取自 input、属性来自别处
        output->SetAttributeSet(attributes);
    } else {
        auto copiedAttributes = AttributeSet::New();
        if (input->GetAttributeSet()) {
            copiedAttributes->DeepCopy(input->GetAttributeSet());
        }
        output->SetAttributeSet(copiedAttributes);
    }
    output->SetName(input->GetName());
}

// 按数据类型创建并深拷贝一个独立输出对象
DataObject::Pointer DeepCopyDataObject(DataObject::Pointer input, AttributeSet::Pointer attributes = nullptr) {
    if (input == nullptr) {
        return nullptr;
    }

    const IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output && type == IG_LAGRANGE_UNSTRUCTURED_MESH) {
        output = LagrangeUnstructuredMesh::New();
    }
    if (!output) {
        return nullptr;
    }

    auto inputPointSet = DynamicCast<PointSet>(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (!inputPointSet || !outputPointSet) {
        return nullptr;
    }
    DeepCopyPointSetBase(outputPointSet.get(), inputPointSet.get(), attributes);

    switch (type) {
        case IG_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            auto outputMesh = DynamicCast<UnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            auto cellTypes = UnsignedIntArray::New();
            if (inputMesh->GetCellTypes()) {
                cellTypes->DeepCopy(inputMesh->GetCellTypes());
            }
            outputMesh->SetCells(DeepCopyCellArray(inputMesh->GetCells()), cellTypes);
            break;
        }
        case IG_SURFACE_MESH: {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            auto outputMesh = DynamicCast<SurfaceMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_VOLUME_MESH: {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            auto outputMesh = DynamicCast<VolumeMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetIsPolyhedronType()) {
                // 多面体网格需要同时重建面片和体-面索引
                auto faces = DeepCopyCellArray(inputMesh->GetFaces());
                auto volumeFaces = CellArray::New();
                for (IGsize i = 0; i < inputMesh->GetNumberOfVolumes(); ++i) {
                    std::vector<igIndex> faceIds(IGAME_CELL_MAX_SIZE);
                    const int faceCount = inputMesh->GetVolumeFaceIds(i, faceIds.data());
                    volumeFaces->AddCellIds(faceIds.data(), faceCount);
                }
                outputMesh->InitVolumesWithPolyhedron(faces, volumeFaces);
                break;
            }

            if (inputMesh->GetVolumes()) {
                outputMesh->SetVolumes(DeepCopyCellArray(inputMesh->GetVolumes()));
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            outputMesh->SetIsPolyhedronType(false);
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto inputMesh = DynamicCast<StructuredMesh>(input);
            auto outputMesh = DynamicCast<StructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            igIndex* dimensions = inputMesh->GetDimensionSize();
            if (dimensions) {
                igIndex copiedDimensions[3]{dimensions[0], dimensions[1], dimensions[2]};
                outputMesh->SetDimensionSize(copiedDimensions);
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<LagrangeUnstructuredMesh>(input);
            auto outputMesh = DynamicCast<LagrangeUnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            for (IGsize i = 0; i < inputMesh->GetNumberOfCells(); ++i) {
                const igIndex* ids = nullptr;
                const int count = inputMesh->GetCellPointIds(i, ids);
                std::vector<igIndex> cellIds;
                if (count > 0 && ids) {
                    cellIds.assign(ids, ids + count);
                }
                outputMesh->AddCell(cellIds.data(), count,
                                    inputMesh->GetSpecificCellType(i),
                                    inputMesh->GetCellOrder(i));
            }
            break;
        }
        case IG_POINT_SET:
        default:
            break;
    }

    return output;
}

/** 3x3 行列式（等价于 c1 . (c2 x c3)） */
inline double Det3(const double c1[3], const double c2[3], const double c3[3]) {
    return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
           c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

/**
 * 单元插值类型：
 *   - 线性面单元 → 线性插值（重心坐标 / 双线性 / 扇形三角化）
 *   - 体单元     → 均值坐标（Mean Value Coordinates）
 *   - 二次单元   → 二次形函数（Newton 参数坐标反解）
 */
enum ShapeKind {
    SHAPE_NONE = 0,
    // 线性面单元
    SHAPE_LINEAR_TRIANGLE,
    SHAPE_LINEAR_QUAD,
    SHAPE_LINEAR_POLYGON,
    // 二次面单元
    SHAPE_QUADRATIC_TRIANGLE, // 6 节点
    SHAPE_QUADRATIC_QUAD,     // 8 节点
    // 体单元
    SHAPE_TETRA,              // 4 节点：解析重心坐标（等价于均值坐标，精确）
    SHAPE_MVC_VOLUME,         // 六面体/棱柱/金字塔/多面体：均值坐标
    // 二次体单元
    SHAPE_QUADRATIC_TETRA,    // 10 节点
    SHAPE_QUADRATIC_HEX,      // 20 节点
    SHAPE_QUADRATIC_PRISM,    // 15 节点（三棱柱 / 楔形）
    SHAPE_QUADRATIC_PYRAMID,  // 13 节点
    // 高次单元（节点序与形函数按 VTK 硬编码；VTK 中 pcoords 为 [0,1]，此处直接使用其 [-1,1] 内部形式）
    SHAPE_BIQUADRATIC_QUAD,       // 9 节点：0-3 角点、4-7 棱中点(0-1)(1-2)(2-3)(3-0)、8 面心
    SHAPE_QUADRATIC_LINEAR_QUAD,  // 6 节点：0-3 角点、4=棱(0,1)中点、5=棱(2,3)中点
    SHAPE_QUADRATIC_LINEAR_WEDGE, // 12 节点：0-5 角点、6-8 底面棱中点、9-11 顶面棱中点
    SHAPE_TRIQUADRATIC_HEX,       // 27 节点：0-7 角点、8-19 棱中点、20-25 面心、26 体心
};

/** 是否走「二次 / 高次形函数」插值路径 */
inline bool IsQuadraticKind(ShapeKind kind) {
    switch (kind) {
        case SHAPE_QUADRATIC_TRIANGLE:
        case SHAPE_QUADRATIC_QUAD:
        case SHAPE_QUADRATIC_TETRA:
        case SHAPE_QUADRATIC_HEX:
        case SHAPE_QUADRATIC_PRISM:
        case SHAPE_QUADRATIC_PYRAMID:
        case SHAPE_BIQUADRATIC_QUAD:
        case SHAPE_QUADRATIC_LINEAR_QUAD:
        case SHAPE_QUADRATIC_LINEAR_WEDGE:
        case SHAPE_TRIQUADRATIC_HEX:
            return true;
        default:
            return false;
    }
}

/** 二次「面」单元（走平面投影 + 2D Newton） */
inline bool IsQuadraticFaceKind(ShapeKind kind) {
    return kind == SHAPE_QUADRATIC_TRIANGLE || kind == SHAPE_QUADRATIC_QUAD || kind == SHAPE_BIQUADRATIC_QUAD ||
           kind == SHAPE_QUADRATIC_LINEAR_QUAD;
}

/** 四个角点的四边形类单元（用于构造投影基） */
inline bool IsQuadKind(ShapeKind kind) {
    return kind == SHAPE_LINEAR_QUAD || kind == SHAPE_QUADRATIC_QUAD || kind == SHAPE_BIQUADRATIC_QUAD ||
           kind == SHAPE_QUADRATIC_LINEAR_QUAD;
}

/** 按单元类型与点数判定插值方式 */
ShapeKind ClassifyCell(IGenum cellType, int npts) {
    switch (cellType) {
        case IG_TRIANGLE:
            return (npts == 3) ? SHAPE_LINEAR_TRIANGLE : SHAPE_NONE;
        case IG_QUAD:
            return (npts == 4) ? SHAPE_LINEAR_QUAD : SHAPE_NONE;
        case IG_POLYGON:
            return (npts >= 3) ? SHAPE_LINEAR_POLYGON : SHAPE_NONE;
        case IG_TETRA:
            return (npts == 4) ? SHAPE_TETRA : SHAPE_NONE;
        case IG_HEXAHEDRON:
        case IG_PRISM:
        case IG_PYRAMID:
        case IG_VOLUME:
        case IG_POLYHEDRON:
            return (npts >= 4) ? SHAPE_MVC_VOLUME : SHAPE_NONE;
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
            return (npts == 6) ? SHAPE_QUADRATIC_TRIANGLE : SHAPE_NONE;
        case IG_QUADRATIC_QUAD:
            return (npts == 8) ? SHAPE_QUADRATIC_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_TETRA:
            return (npts == 10) ? SHAPE_QUADRATIC_TETRA : SHAPE_NONE;
        case IG_QUADRATIC_HEXAHEDRON:
            return (npts == 20) ? SHAPE_QUADRATIC_HEX : SHAPE_NONE;
        case IG_QUADRATIC_PRISM:
            return (npts == 15) ? SHAPE_QUADRATIC_PRISM : SHAPE_NONE;
        case IG_QUADRATIC_PYRAMID:
            return (npts == 13) ? SHAPE_QUADRATIC_PYRAMID : SHAPE_NONE;
        // 以下节点序见 VTK：vtkBiQuadraticQuad / vtkQuadraticLinearQuad /
        // vtkQuadraticLinearWedge / vtkTriQuadraticHexahedron
        case IG_BIQUADRATIC_QUAD:
            return (npts == 9) ? SHAPE_BIQUADRATIC_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_LINEAR_QUAD:
            return (npts == 6) ? SHAPE_QUADRATIC_LINEAR_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_LINEAR_WEDGE:
            return (npts == 12) ? SHAPE_QUADRATIC_LINEAR_WEDGE : SHAPE_NONE;
        case IG_TRIQUADRATIC_HEXAHEDRON:
            return (npts == 27) ? SHAPE_TRIQUADRATIC_HEX : SHAPE_NONE;
        default:
            return SHAPE_NONE;
    }
}

/** 是否为「二次 / 高次」单元族（用于退化提示统计） */
bool IsHighOrderCell(IGenum cellType) {
    switch (cellType) {
        case IG_QUADRATIC_EDGE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
        case IG_QUADRATIC_TETRA:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_PYRAMID:
        case IG_BIQUADRATIC_QUAD:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_CURVE:
        case IG_LAGRANGE_TRIANGLE:
        case IG_LAGRANGE_QUADRILATERAL:
        case IG_LAGRANGE_TETRAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
        case IG_LAGRANGE_PRISM:
        case IG_LAGRANGE_PYRAMID:
            return true;
        default:
            return false;
    }
}

/** 不支持的高次「面」单元退化到线性角点时，需要使用的角点数量（0 = 无法退化） */
int LinearCornerCount(IGenum cellType, int npts) {
    switch (cellType) {
        case IG_TRIANGLE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_TRIANGLE:
            return (npts >= 3) ? 3 : 0;
        case IG_QUAD:
        case IG_QUADRATIC_QUAD:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_LAGRANGE_QUADRILATERAL:
            return (npts >= 4) ? 4 : 0;
        case IG_POLYGON:
        case IG_QUADRATIC_POLYGON:
            return (npts >= 3) ? npts : 0;
        default:
            return 0;
    }
}

/* ================================================================== */
/* 形函数：线性四边形                                                  */
/* ================================================================== */
void LinearQuadShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1];
    sf[0] = (1.0 - r) * (1.0 - s);
    sf[1] = r * (1.0 - s);
    sf[2] = r * s;
    sf[3] = (1.0 - r) * s;
}

void LinearQuadDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1];
    d[0] = -(1.0 - s);
    d[1] = (1.0 - s);
    d[2] = s;
    d[3] = -s;
    d[4] = -(1.0 - r);
    d[5] = -r;
    d[6] = r;
    d[7] = (1.0 - r);
}

/* ================================================================== */
/* 形函数：二次单元                                                    */
/* ================================================================== */

/** 二次三角形（6 节点）：参数坐标 r,s ∈ [0,1], r+s ≤ 1 */
void QuadraticTriangleShape(const double pc[3], double* sf) {
    const double L0 = 1.0 - pc[0] - pc[1];
    const double L1 = pc[0];
    const double L2 = pc[1];
    sf[0] = L0 * (2.0 * L0 - 1.0);
    sf[1] = L1 * (2.0 * L1 - 1.0);
    sf[2] = L2 * (2.0 * L2 - 1.0);
    sf[3] = 4.0 * L0 * L1; // 棱 0-1
    sf[4] = 4.0 * L1 * L2; // 棱 1-2
    sf[5] = 4.0 * L2 * L0; // 棱 2-0
}

void QuadraticTriangleDerivs(const double pc[3], double* d) {
    const double L[3] = {1.0 - pc[0] - pc[1], pc[0], pc[1]};
    const double dL[3][2] = {{-1.0, -1.0}, {1.0, 0.0}, {0.0, 1.0}};
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 2; ++k) { d[k * 6 + i] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[3][2] = {{0, 1}, {1, 2}, {2, 0}};
    for (int e = 0; e < 3; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 2; ++k) { d[k * 6 + 3 + e] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}

/** 二次四边形（8 节点，Serendipity）：参数坐标 r,s ∈ [0,1] */
void QuadraticQuadShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1];
    sf[0] = (1.0 - r) * (1.0 - s) * (1.0 - 2.0 * r - 2.0 * s);
    sf[1] = r * (1.0 - s) * (2.0 * r - 2.0 * s - 1.0);
    sf[2] = r * s * (2.0 * r + 2.0 * s - 3.0);
    sf[3] = (1.0 - r) * s * (-2.0 * r + 2.0 * s - 1.0);
    sf[4] = 4.0 * r * (1.0 - r) * (1.0 - s);
    sf[5] = 4.0 * r * s * (1.0 - s);
    sf[6] = 4.0 * r * s * (1.0 - r);
    sf[7] = 4.0 * s * (1.0 - r) * (1.0 - s);
}

void QuadraticQuadDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1];
    // d/dr
    d[0] = (1.0 - s) * (4.0 * r + 2.0 * s - 3.0);
    d[1] = (1.0 - s) * (4.0 * r - 2.0 * s - 1.0);
    d[2] = s * (4.0 * r + 2.0 * s - 3.0);
    d[3] = s * (4.0 * r - 2.0 * s - 1.0);
    d[4] = 4.0 * (1.0 - 2.0 * r) * (1.0 - s);
    d[5] = 4.0 * s * (1.0 - s);
    d[6] = 4.0 * s * (1.0 - 2.0 * r);
    d[7] = -4.0 * s * (1.0 - s);
    // d/ds
    d[8] = (1.0 - r) * (2.0 * r + 4.0 * s - 3.0);
    d[9] = r * (-2.0 * r + 4.0 * s - 1.0);
    d[10] = r * (2.0 * r + 4.0 * s - 3.0);
    d[11] = (1.0 - r) * (-2.0 * r + 4.0 * s - 1.0);
    d[12] = -4.0 * r * (1.0 - r);
    d[13] = 4.0 * r * (1.0 - 2.0 * s);
    d[14] = 4.0 * r * (1.0 - r);
    d[15] = 4.0 * (1.0 - r) * (1.0 - 2.0 * s);
}

/** 二次四面体（10 节点）：参数坐标 (L1,L2,L3)，L0 = 1 - L1 - L2 - L3 */
void QuadraticTetraShape(const double pc[3], double* sf) {
    const double L[4] = {1.0 - pc[0] - pc[1] - pc[2], pc[0], pc[1], pc[2]};
    for (int i = 0; i < 4; ++i) { sf[i] = L[i] * (2.0 * L[i] - 1.0); }
    const int pairs[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
    for (int e = 0; e < 6; ++e) { sf[4 + e] = 4.0 * L[pairs[e][0]] * L[pairs[e][1]]; }
}

void QuadraticTetraDerivs(const double pc[3], double* d) {
    const double L[4] = {1.0 - pc[0] - pc[1] - pc[2], pc[0], pc[1], pc[2]};
    const double dL[4][3] = {{-1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    for (int i = 0; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) { d[k * 10 + i] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
    for (int e = 0; e < 6; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 3; ++k) { d[k * 10 + 4 + e] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}

// 二次六面体（20 节点）节点自然坐标符号表（VTK 节点顺序）
const int kQuadHexSign[20][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
        {0, -1, -1},  {1, 0, -1},  {0, 1, -1},  {-1, 0, -1},  {0, -1, 1},  {1, 0, 1},  {0, 1, 1},  {-1, 0, 1},
        {-1, -1, 0},  {1, -1, 0},  {1, 1, 0},   {-1, 1, 0}};

/** 二次六面体（20 节点，Serendipity）：参数坐标 r,s,t ∈ [-1,1] */
void QuadraticHexShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    for (int i = 0; i < 8; ++i) {
        const double A = 1.0 + r * kQuadHexSign[i][0];
        const double B = 1.0 + s * kQuadHexSign[i][1];
        const double C = 1.0 + t * kQuadHexSign[i][2];
        const double S = kQuadHexSign[i][0] * r + kQuadHexSign[i][1] * s + kQuadHexSign[i][2] * t - 2.0;
        sf[i] = 0.125 * A * B * C * S;
    }
    for (int i = 8; i < 20; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        if (sx == 0) {
            sf[i] = 0.25 * (1.0 - r * r) * (1.0 + s * sy) * (1.0 + t * sz);
        } else if (sy == 0) {
            sf[i] = 0.25 * (1.0 + r * sx) * (1.0 - s * s) * (1.0 + t * sz);
        } else {
            sf[i] = 0.25 * (1.0 + r * sx) * (1.0 + s * sy) * (1.0 - t * t);
        }
    }
}

void QuadraticHexDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    for (int i = 0; i < 8; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        const double A = 1.0 + r * sx, B = 1.0 + s * sy, C = 1.0 + t * sz;
        const double S = sx * r + sy * s + sz * t - 2.0;
        d[0 * 20 + i] = 0.125 * B * C * sx * (S + A);
        d[1 * 20 + i] = 0.125 * A * C * sy * (S + B);
        d[2 * 20 + i] = 0.125 * A * B * sz * (S + C);
    }
    for (int i = 8; i < 20; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        if (sx == 0) {
            d[0 * 20 + i] = -0.5 * r * (1.0 + s * sy) * (1.0 + t * sz);
            d[1 * 20 + i] = 0.25 * (1.0 - r * r) * sy * (1.0 + t * sz);
            d[2 * 20 + i] = 0.25 * (1.0 - r * r) * (1.0 + s * sy) * sz;
        } else if (sy == 0) {
            d[0 * 20 + i] = 0.25 * sx * (1.0 - s * s) * (1.0 + t * sz);
            d[1 * 20 + i] = -0.5 * s * (1.0 + r * sx) * (1.0 + t * sz);
            d[2 * 20 + i] = 0.25 * (1.0 + r * sx) * (1.0 - s * s) * sz;
        } else {
            d[0 * 20 + i] = 0.25 * sx * (1.0 + s * sy) * (1.0 - t * t);
            d[1 * 20 + i] = 0.25 * (1.0 + r * sx) * sy * (1.0 - t * t);
            d[2 * 20 + i] = -0.5 * t * (1.0 + r * sx) * (1.0 + s * sy);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 二次三棱柱（15 节点）                                               */
/* ------------------------------------------------------------------ */
// 节点顺序（与本工程 QuadraticPrism::edges 一致）：
//   0,1,2 底面角点；3,4,5 顶面角点；
//   6(0-1), 7(1-2), 8(2-0) 底面棱中点；9(3-4), 10(4-5), 11(5-3) 顶面棱中点；
//   12(0-3), 13(1-4), 14(2-5) 竖棱中点。
// 三角形部分用 6 节点二次三角形基函数 T[0..5]，底面层因子 phi、顶面层因子 psi、
// 竖向气泡 b = 4t(1-t)：Sum(N) == 1，且在每个节点取 Kronecker 值（已逐点验证）。
namespace {
// 节点 → 二次三角形基函数索引（-1 表示竖棱中点）
const int kPrismTIndex[15] = {0, 1, 2, 0, 1, 2, 3, 4, 5, 3, 4, 5, -1, -1, -1};
// 0 = 底面层(phi)，1 = 顶面层(psi)，-1 = 竖棱
const int kPrismLevel[15] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, -1, -1, -1};

/** 二次三角形的 6 个基函数及其偏导（r,s 方向） */
void Prism15TriangleBasis(const double pc[3], double T[6], double dT[6][3], double L[3], double dL[3][3]) {
    L[0] = 1.0 - pc[0] - pc[1];
    L[1] = pc[0];
    L[2] = pc[1];
    dL[0][0] = -1.0; dL[0][1] = -1.0; dL[0][2] = 0.0;
    dL[1][0] = 1.0;  dL[1][1] = 0.0;  dL[1][2] = 0.0;
    dL[2][0] = 0.0;  dL[2][1] = 1.0;  dL[2][2] = 0.0;

    T[0] = L[0] * (2.0 * L[0] - 1.0);
    T[1] = L[1] * (2.0 * L[1] - 1.0);
    T[2] = L[2] * (2.0 * L[2] - 1.0);
    T[3] = 4.0 * L[0] * L[1]; // 棱 0-1
    T[4] = 4.0 * L[1] * L[2]; // 棱 1-2
    T[5] = 4.0 * L[2] * L[0]; // 棱 2-0
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { dT[i][k] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[3][2] = {{0, 1}, {1, 2}, {2, 0}};
    for (int e = 0; e < 3; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 3; ++k) { dT[3 + e][k] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}
} // namespace

void QuadraticPrism15Shape(const double pc[3], double* sf) {
    double T[6], dT[6][3], L[3], dL[3][3];
    Prism15TriangleBasis(pc, T, dT, L, dL);

    const double t = pc[2];
    const double b = 4.0 * t * (1.0 - t);
    const double phi = 1.0 - t - 0.5 * b; // 底面层
    const double psi = t - 0.5 * b;       // 顶面层

    for (int i = 0; i < 15; ++i) {
        const int ti = kPrismTIndex[i];
        if (ti >= 0) {
            sf[i] = T[ti] * (kPrismLevel[i] == 1 ? psi : phi);
        } else {
            sf[i] = L[i - 12] * b;
        }
    }
}

void QuadraticPrism15Derivs(const double pc[3], double* d) {
    double T[6], dT[6][3], L[3], dL[3][3];
    Prism15TriangleBasis(pc, T, dT, L, dL);

    const double t = pc[2];
    const double b = 4.0 * t * (1.0 - t);
    const double db = 4.0 - 8.0 * t;
    const double phi = 1.0 - t - 0.5 * b, dphi = -1.0 - 0.5 * db;
    const double psi = t - 0.5 * b, dpsi = 1.0 - 0.5 * db;

    for (int i = 0; i < 15; ++i) {
        const int ti = kPrismTIndex[i];
        if (ti >= 0) {
            const bool top = (kPrismLevel[i] == 1);
            const double f = top ? psi : phi;
            const double df = top ? dpsi : dphi;
            for (int k = 0; k < 3; ++k) { d[k * 15 + i] = dT[ti][k] * f; }
            d[2 * 15 + i] = T[ti] * df; // 三角形基函数与 t 无关，直接赋值
        } else {
            const int li = i - 12;
            for (int k = 0; k < 3; ++k) { d[k * 15 + i] = dL[li][k] * b; }
            d[2 * 15 + i] = L[li] * db;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 二次金字塔（13 节点）                                               */
/* ------------------------------------------------------------------ */
// 节点顺序（与本工程 QuadraticPyramid::edges 一致）：
//   0,1,2,3 底面角点；4 顶点；5(0-1),6(1-2),7(2-3),8(3-0) 底棱中点；
//   9(0-4),10(1-4),11(2-4),12(3-4) 侧棱中点。
// 底面用 8 节点二次四边形 Q[0..7]，侧棱用线性四边形角点函数 q[0..3]：
//   phi = (1-t)(1-2t)（底面层）、g = t(2t-1)(5-4t)（顶点）、h = 8t(1-t)^2（侧棱）
//   Sum(N) == 1，且在每个节点取 Kronecker 值（已逐点验证）。
void QuadraticPyramid13Shape(const double pc[3], double* sf) {
    const double pc2[3] = {pc[0], pc[1], 0.0};
    double Q[8], q[4];
    QuadraticQuadShape(pc2, Q);
    LinearQuadShape(pc2, q);

    const double t = pc[2];
    const double phi = (1.0 - t) * (1.0 - 2.0 * t);
    const double g = t * (2.0 * t - 1.0) * (5.0 - 4.0 * t);
    const double h = 8.0 * t * (1.0 - t) * (1.0 - t);

    sf[0] = Q[0] * phi;
    sf[1] = Q[1] * phi;
    sf[2] = Q[2] * phi;
    sf[3] = Q[3] * phi;
    sf[5] = Q[4] * phi;
    sf[6] = Q[5] * phi;
    sf[7] = Q[6] * phi;
    sf[8] = Q[7] * phi;

    sf[4] = g;

    sf[9] = h * q[0];
    sf[10] = h * q[1];
    sf[11] = h * q[2];
    sf[12] = h * q[3];
}

void QuadraticPyramid13Derivs(const double pc[3], double* d) {
    const double pc2[3] = {pc[0], pc[1], 0.0};
    double Q[8], q[4], dQ[3 * 8], dq[3 * 4];
    QuadraticQuadShape(pc2, Q);
    LinearQuadShape(pc2, q);
    QuadraticQuadDerivs(pc2, dQ);
    LinearQuadDerivs(pc2, dq);

    const double t = pc[2];
    const double phi = (1.0 - t) * (1.0 - 2.0 * t), dphi = -3.0 + 4.0 * t;
    const double dg = -24.0 * t * t + 28.0 * t - 5.0;
    const double h = 8.0 * t * (1.0 - t) * (1.0 - t);
    const double dh = 8.0 * (1.0 - t) * (1.0 - 3.0 * t);

    const int base[8] = {0, 1, 2, 3, 5, 6, 7, 8}; // 单元节点 → Q 索引
    for (int i = 0; i < 8; ++i) {
        const int n = base[i];
        for (int k = 0; k < 2; ++k) { d[k * 13 + n] = dQ[k * 8 + i] * phi; }
        d[2 * 13 + n] = Q[i] * dphi;
    }

    for (int k = 0; k < 3; ++k) { d[k * 13 + 4] = 0.0; }
    d[2 * 13 + 4] = dg;

    const int lat[4] = {9, 10, 11, 12}; // 单元节点 → q 索引
    for (int i = 0; i < 4; ++i) {
        const int n = lat[i];
        for (int k = 0; k < 2; ++k) { d[k * 13 + n] = dq[k * 4 + i] * h; }
        d[2 * 13 + n] = q[i] * dh;
    }
}

/* ------------------------------------------------------------------ */
/* 高次单元（按 VTK 硬编码）                                           */
/* ------------------------------------------------------------------ */
// VTK 的二次单元形函数内部统一在 [-1,1] 上定义（对外 pcoords 为 [0,1]，
// 通过 r = 2*(pcoords - 0.5) 转换）。这里直接使用其 [-1,1] 形式，公式与
// VTK 源码逐行一致，因此插值结果与 VTK 相同。
namespace {
/** 一维二次 Lagrange（节点 -1, 0, +1）：对应 VTK 的 g1/g2/g3 */
inline void LagrangeG(double x, double g[3], double dg[3]) {
    g[0] = -0.5 * x * (1.0 - x);  // 节点 -1
    g[1] = (1.0 + x) * (1.0 - x); // 节点 0
    g[2] = 0.5 * x * (1.0 + x);   // 节点 +1
    dg[0] = x - 0.5;
    dg[1] = -2.0 * x;
    dg[2] = x + 0.5;
}
} // namespace

// 9 节点双二次四边形：VTK vtkBiQuadraticQuad
// 节点 → (gx 索引, gy 索引)，索引 0 = -1、1 = 0、2 = +1
namespace {
const int kBiQuadIdx[9][2] = {{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 0}, {2, 1}, {1, 2}, {0, 1}, {1, 1}};
} // namespace

void BiQuadraticQuadShape(const double pc[3], double* sf) {
    double gx[3], dx[3], gy[3], dy[3];
    LagrangeG(pc[0], gx, dx);
    LagrangeG(pc[1], gy, dy);
    for (int i = 0; i < 9; ++i) { sf[i] = gx[kBiQuadIdx[i][0]] * gy[kBiQuadIdx[i][1]]; }
}

void BiQuadraticQuadDerivs(const double pc[3], double* d) {
    double gx[3], dx[3], gy[3], dy[3];
    LagrangeG(pc[0], gx, dx);
    LagrangeG(pc[1], gy, dy);
    for (int i = 0; i < 9; ++i) {
        d[0 * 9 + i] = dx[kBiQuadIdx[i][0]] * gy[kBiQuadIdx[i][1]];
        d[1 * 9 + i] = gx[kBiQuadIdx[i][0]] * dy[kBiQuadIdx[i][1]];
    }
}

// 6 节点「二次-线性」四边形：VTK vtkQuadraticLinearQuad
// 节点 0-3 角点 (-1,-1)(1,-1)(1,1)(-1,1)，4=(0,-1)，5=(0,1)：x 方向二次、y 方向线性
void QuadraticLinearQuadShape(const double pc[3], double* sf) {
    double gx[3], dx[3];
    LagrangeG(pc[0], gx, dx);
    const double m0 = 0.5 * (1.0 - pc[1]); // y = -1
    const double m1 = 0.5 * (1.0 + pc[1]); // y = +1
    sf[0] = gx[0] * m0;
    sf[1] = gx[2] * m0;
    sf[4] = gx[1] * m0;
    sf[3] = gx[0] * m1;
    sf[2] = gx[2] * m1;
    sf[5] = gx[1] * m1;
}

void QuadraticLinearQuadDerivs(const double pc[3], double* d) {
    double gx[3], dx[3];
    LagrangeG(pc[0], gx, dx);
    const double m0 = 0.5 * (1.0 - pc[1]);
    const double m1 = 0.5 * (1.0 + pc[1]);
    const double dm0 = -0.5, dm1 = 0.5;
    d[0 * 6 + 0] = dx[0] * m0;
    d[0 * 6 + 1] = dx[2] * m0;
    d[0 * 6 + 4] = dx[1] * m0;
    d[0 * 6 + 3] = dx[0] * m1;
    d[0 * 6 + 2] = dx[2] * m1;
    d[0 * 6 + 5] = dx[1] * m1;
    d[1 * 6 + 0] = gx[0] * dm0;
    d[1 * 6 + 1] = gx[2] * dm0;
    d[1 * 6 + 4] = gx[1] * dm0;
    d[1 * 6 + 3] = gx[0] * dm1;
    d[1 * 6 + 2] = gx[2] * dm1;
    d[1 * 6 + 5] = gx[1] * dm1;
}

// 12 节点「二次-线性」楔形：VTK vtkQuadraticLinearWedge（形函数逐行照抄 VTK，
// 仅去掉 VTK 末尾对 [0,1] 参数的 *2 缩放，因为此处直接使用 [-1,1]）
void QuadraticLinearWedgeShape(const double pc[3], double* sf) {
    const double x = pc[0], y = pc[1], z = pc[2];
    // 角点（两个二次三角形）
    sf[0] = (x + y) * 0.5 * (x + y + 1.0) * (1 - z) * 0.5;
    sf[1] = x * (x + 1.0) * 0.5 * (1 - z) * 0.5;
    sf[2] = y * (1.0 + y) * 0.5 * (1 - z) * 0.5;
    sf[3] = (x + y) * 0.5 * (x + y + 1.0) * (1 + z) * 0.5;
    sf[4] = x * (x + 1.0) * 0.5 * (1 + z) * 0.5;
    sf[5] = y * (1.0 + y) * 0.5 * (1 + z) * 0.5;
    // 三角形棱中点
    sf[6] = -(x + 1) * (x + y) * (1 - z) * 0.5;
    sf[7] = (x + 1) * (y + 1) * (1 - z) * 0.5;
    sf[8] = -(y + 1) * (x + y) * (1 - z) * 0.5;
    sf[9] = -(x + 1) * (x + y) * (1 + z) * 0.5;
    sf[10] = (x + 1) * (y + 1) * (1 + z) * 0.5;
    sf[11] = -(y + 1) * (x + y) * (1 + z) * 0.5;
}

void QuadraticLinearWedgeDerivs(const double pc[3], double* d) {
    const double x = pc[0], y = pc[1], z = pc[2];
    // d/dx（对应 VTK InterpolationDerivs 的 x 分块，去掉 *2）
    d[0 * 12 + 0] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
    d[0 * 12 + 1] = (1.0 + 2.0 * x) * 0.5 * (1.0 - z) * 0.5;
    d[0 * 12 + 2] = 0.0;
    d[0 * 12 + 3] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
    d[0 * 12 + 4] = (1.0 + 2.0 * x) * 0.5 * (1.0 + z) * 0.5;
    d[0 * 12 + 5] = 0.0;
    d[0 * 12 + 6] = -(2.0 * x + y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 7] = (y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 8] = -(y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 9] = -(2.0 * x + y + 1.0) * (1.0 + z) * 0.5;
    d[0 * 12 + 10] = (y + 1.0) * (1.0 + z) * 0.5;
    d[0 * 12 + 11] = -(y + 1.0) * (1.0 + z) * 0.5;
    // d/dy
    d[1 * 12 + 0] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
    d[1 * 12 + 1] = 0.0;
    d[1 * 12 + 2] = (1.0 + 2.0 * y) * 0.5 * (1.0 - z) * 0.5;
    d[1 * 12 + 3] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
    d[1 * 12 + 4] = 0.0;
    d[1 * 12 + 5] = (1.0 + 2.0 * y) * 0.5 * (1.0 + z) * 0.5;
    d[1 * 12 + 6] = -(x + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 7] = (x + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 8] = -(x + 2.0 * y + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 9] = -(x + 1.0) * (1.0 + z) * 0.5;
    d[1 * 12 + 10] = (x + 1.0) * (1.0 + z) * 0.5;
    d[1 * 12 + 11] = -(x + 2.0 * y + 1.0) * (1.0 + z) * 0.5;
    // d/dz
    d[2 * 12 + 0] = (x + y) * 0.5 * (x + y + 1.0) * -0.5;
    d[2 * 12 + 1] = x * (x + 1.0) * 0.5 * -0.5;
    d[2 * 12 + 2] = y * (1.0 + y) * 0.5 * -0.5;
    d[2 * 12 + 3] = (x + y) * 0.5 * (x + y + 1.0) * 0.5;
    d[2 * 12 + 4] = x * (x + 1.0) * 0.5 * 0.5;
    d[2 * 12 + 5] = y * (1.0 + y) * 0.5 * 0.5;
    d[2 * 12 + 6] = -(x + 1.0) * (x + y) * -0.5;
    d[2 * 12 + 7] = (x + 1.0) * (y + 1.0) * -0.5;
    d[2 * 12 + 8] = -(y + 1.0) * (x + y) * -0.5;
    d[2 * 12 + 9] = -(x + 1.0) * (x + y) * 0.5;
    d[2 * 12 + 10] = (x + 1.0) * (y + 1.0) * 0.5;
    d[2 * 12 + 11] = -(y + 1.0) * (x + y) * 0.5;
}

// 27 节点三二次六面体：VTK vtkTriQuadraticHexahedron（张量积二次 Lagrange）
// 节点序 0-7 角点、8-19 棱中点、20-25 面心、26 体心（VTK 源码中的赋值顺序照抄）
void TriQuadraticHexShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    double g1r = -0.5 * r * (1 - r), g1s = -0.5 * s * (1 - s), g1t = -0.5 * t * (1 - t);
    double g2r = (1 + r) * (1 - r), g2s = (1 + s) * (1 - s), g2t = (1 + t) * (1 - t);
    double g3r = 0.5 * r * (1 + r), g3s = 0.5 * s * (1 + s), g3t = 0.5 * t * (1 + t);

    // 八个角点
    sf[0] = g1r * g1s * g1t;
    sf[1] = g3r * g1s * g1t;
    sf[2] = g3r * g3s * g1t;
    sf[3] = g1r * g3s * g1t;
    sf[4] = g1r * g1s * g3t;
    sf[5] = g3r * g1s * g3t;
    sf[6] = g3r * g3s * g3t;
    sf[7] = g1r * g3s * g3t;
    // 棱中点
    sf[8] = g2r * g1s * g1t;
    sf[9] = g3r * g2s * g1t;
    sf[10] = g2r * g3s * g1t;
    sf[11] = g1r * g2s * g1t;
    sf[12] = g2r * g1s * g3t;
    sf[13] = g3r * g2s * g3t;
    sf[14] = g2r * g3s * g3t;
    sf[15] = g1r * g2s * g3t;
    sf[16] = g1r * g1s * g2t;
    sf[17] = g3r * g1s * g2t;
    sf[18] = g3r * g3s * g2t;
    sf[19] = g1r * g3s * g2t;
    // 面心（此处的赋值顺序与 VTK 源码一致）
    sf[22] = g2r * g1s * g2t;
    sf[21] = g3r * g2s * g2t;
    sf[23] = g2r * g3s * g2t;
    sf[20] = g1r * g2s * g2t;
    sf[24] = g2r * g2s * g1t;
    sf[25] = g2r * g2s * g3t;
    // 体心
    sf[26] = g2r * g2s * g2t;
}

void TriQuadraticHexDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    double g1r = -0.5 * r * (1 - r), g1s = -0.5 * s * (1 - s), g1t = -0.5 * t * (1 - t);
    double g2r = (1 + r) * (1 - r), g2s = (1 + s) * (1 - s), g2t = (1 + t) * (1 - t);
    double g3r = 0.5 * r * (1 + r), g3s = 0.5 * s * (1 + s), g3t = 0.5 * t * (1 + t);
    const double g1r_r = r - 0.5, g1s_s = s - 0.5, g1t_t = t - 0.5;
    const double g2r_r = -2 * r, g2s_s = -2 * s, g2t_t = -2 * t;
    const double g3r_r = r + 0.5, g3s_s = s + 0.5, g3t_t = t + 0.5;

    // d/dr
    d[0 * 27 + 0] = g1r_r * g1s * g1t;
    d[0 * 27 + 1] = g3r_r * g1s * g1t;
    d[0 * 27 + 2] = g3r_r * g3s * g1t;
    d[0 * 27 + 3] = g1r_r * g3s * g1t;
    d[0 * 27 + 4] = g1r_r * g1s * g3t;
    d[0 * 27 + 5] = g3r_r * g1s * g3t;
    d[0 * 27 + 6] = g3r_r * g3s * g3t;
    d[0 * 27 + 7] = g1r_r * g3s * g3t;
    d[0 * 27 + 8] = g2r_r * g1s * g1t;
    d[0 * 27 + 9] = g3r_r * g2s * g1t;
    d[0 * 27 + 10] = g2r_r * g3s * g1t;
    d[0 * 27 + 11] = g1r_r * g2s * g1t;
    d[0 * 27 + 12] = g2r_r * g1s * g3t;
    d[0 * 27 + 13] = g3r_r * g2s * g3t;
    d[0 * 27 + 14] = g2r_r * g3s * g3t;
    d[0 * 27 + 15] = g1r_r * g2s * g3t;
    d[0 * 27 + 16] = g1r_r * g1s * g2t;
    d[0 * 27 + 17] = g3r_r * g1s * g2t;
    d[0 * 27 + 18] = g3r_r * g3s * g2t;
    d[0 * 27 + 19] = g1r_r * g3s * g2t;
    d[0 * 27 + 20] = g1r_r * g2s * g2t;
    d[0 * 27 + 21] = g3r_r * g2s * g2t;
    d[0 * 27 + 22] = g2r_r * g1s * g2t;
    d[0 * 27 + 23] = g2r_r * g3s * g2t;
    d[0 * 27 + 24] = g2r_r * g2s * g1t;
    d[0 * 27 + 25] = g2r_r * g2s * g3t;
    d[0 * 27 + 26] = g2r_r * g2s * g2t;

    // d/ds
    d[1 * 27 + 0] = g1r * g1s_s * g1t;
    d[1 * 27 + 1] = g3r * g1s_s * g1t;
    d[1 * 27 + 2] = g3r * g3s_s * g1t;
    d[1 * 27 + 3] = g1r * g3s_s * g1t;
    d[1 * 27 + 4] = g1r * g1s_s * g3t;
    d[1 * 27 + 5] = g3r * g1s_s * g3t;
    d[1 * 27 + 6] = g3r * g3s_s * g3t;
    d[1 * 27 + 7] = g1r * g3s_s * g3t;
    d[1 * 27 + 8] = g2r * g1s_s * g1t;
    d[1 * 27 + 9] = g3r * g2s_s * g1t;
    d[1 * 27 + 10] = g2r * g3s_s * g1t;
    d[1 * 27 + 11] = g1r * g2s_s * g1t;
    d[1 * 27 + 12] = g2r * g1s_s * g3t;
    d[1 * 27 + 13] = g3r * g2s_s * g3t;
    d[1 * 27 + 14] = g2r * g3s_s * g3t;
    d[1 * 27 + 15] = g1r * g2s_s * g3t;
    d[1 * 27 + 16] = g1r * g1s_s * g2t;
    d[1 * 27 + 17] = g3r * g1s_s * g2t;
    d[1 * 27 + 18] = g3r * g3s_s * g2t;
    d[1 * 27 + 19] = g1r * g3s_s * g2t;
    d[1 * 27 + 20] = g1r * g2s_s * g2t;
    d[1 * 27 + 21] = g3r * g2s_s * g2t;
    d[1 * 27 + 22] = g2r * g1s_s * g2t;
    d[1 * 27 + 23] = g2r * g3s_s * g2t;
    d[1 * 27 + 24] = g2r * g2s_s * g1t;
    d[1 * 27 + 25] = g2r * g2s_s * g3t;
    d[1 * 27 + 26] = g2r * g2s_s * g2t;

    // d/dt
    d[2 * 27 + 0] = g1r * g1s * g1t_t;
    d[2 * 27 + 1] = g3r * g1s * g1t_t;
    d[2 * 27 + 2] = g3r * g3s * g1t_t;
    d[2 * 27 + 3] = g1r * g3s * g1t_t;
    d[2 * 27 + 4] = g1r * g1s * g3t_t;
    d[2 * 27 + 5] = g3r * g1s * g3t_t;
    d[2 * 27 + 6] = g3r * g3s * g3t_t;
    d[2 * 27 + 7] = g1r * g3s * g3t_t;
    d[2 * 27 + 8] = g2r * g1s * g1t_t;
    d[2 * 27 + 9] = g3r * g2s * g1t_t;
    d[2 * 27 + 10] = g2r * g3s * g1t_t;
    d[2 * 27 + 11] = g1r * g2s * g1t_t;
    d[2 * 27 + 12] = g2r * g1s * g3t_t;
    d[2 * 27 + 13] = g3r * g2s * g3t_t;
    d[2 * 27 + 14] = g2r * g3s * g3t_t;
    d[2 * 27 + 15] = g1r * g2s * g3t_t;
    d[2 * 27 + 16] = g1r * g1s * g2t_t;
    d[2 * 27 + 17] = g3r * g1s * g2t_t;
    d[2 * 27 + 18] = g3r * g3s * g2t_t;
    d[2 * 27 + 19] = g1r * g3s * g2t_t;
    d[2 * 27 + 20] = g1r * g2s * g2t_t;
    d[2 * 27 + 21] = g3r * g2s * g2t_t;
    d[2 * 27 + 22] = g2r * g1s * g2t_t;
    d[2 * 27 + 23] = g2r * g3s * g2t_t;
    d[2 * 27 + 24] = g2r * g2s * g1t_t;
    d[2 * 27 + 25] = g2r * g2s * g3t_t;
    d[2 * 27 + 26] = g2r * g2s * g2t_t;
}

/* ================================================================== */
/* 形函数统一入口                                                      */
/* ================================================================== */
int ShapePointCount(ShapeKind kind) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: return 4;
        case SHAPE_QUADRATIC_TRIANGLE: return 6;
        case SHAPE_QUADRATIC_QUAD: return 8;
        case SHAPE_TETRA: return 4;
        case SHAPE_QUADRATIC_TETRA: return 10;
        case SHAPE_QUADRATIC_HEX: return 20;
        case SHAPE_QUADRATIC_PRISM: return 15;
        case SHAPE_QUADRATIC_PYRAMID: return 13;
        case SHAPE_BIQUADRATIC_QUAD: return 9;
        case SHAPE_QUADRATIC_LINEAR_QUAD: return 6;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: return 12;
        case SHAPE_TRIQUADRATIC_HEX: return 27;
        default: return 0;
    }
}

void ShapeFunctions(ShapeKind kind, const double pc[3], double* sf) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: LinearQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_TRIANGLE: QuadraticTriangleShape(pc, sf); break;
        case SHAPE_QUADRATIC_QUAD: QuadraticQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_TETRA: QuadraticTetraShape(pc, sf); break;
        case SHAPE_QUADRATIC_HEX: QuadraticHexShape(pc, sf); break;
        case SHAPE_QUADRATIC_PRISM: QuadraticPrism15Shape(pc, sf); break;
        case SHAPE_QUADRATIC_PYRAMID: QuadraticPyramid13Shape(pc, sf); break;
        case SHAPE_BIQUADRATIC_QUAD: BiQuadraticQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_LINEAR_QUAD: QuadraticLinearQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: QuadraticLinearWedgeShape(pc, sf); break;
        case SHAPE_TRIQUADRATIC_HEX: TriQuadraticHexShape(pc, sf); break;
        default: break;
    }
}

void ShapeDerivs(ShapeKind kind, const double pc[3], double* d) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: LinearQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_TRIANGLE: QuadraticTriangleDerivs(pc, d); break;
        case SHAPE_QUADRATIC_QUAD: QuadraticQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_TETRA: QuadraticTetraDerivs(pc, d); break;
        case SHAPE_QUADRATIC_HEX: QuadraticHexDerivs(pc, d); break;
        case SHAPE_QUADRATIC_PRISM: QuadraticPrism15Derivs(pc, d); break;
        case SHAPE_QUADRATIC_PYRAMID: QuadraticPyramid13Derivs(pc, d); break;
        case SHAPE_BIQUADRATIC_QUAD: BiQuadraticQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_LINEAR_QUAD: QuadraticLinearQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: QuadraticLinearWedgeDerivs(pc, d); break;
        case SHAPE_TRIQUADRATIC_HEX: TriQuadraticHexDerivs(pc, d); break;
        default: break;
    }
}

void ShapeInitParams(ShapeKind kind, double pc[3]) {
    pc[0] = pc[1] = pc[2] = 0.0;
    if (kind == SHAPE_QUADRATIC_TETRA) {
        pc[0] = pc[1] = pc[2] = 0.25;
    } else if (kind == SHAPE_QUADRATIC_TRIANGLE) {
        pc[0] = pc[1] = 1.0 / 3.0;
    } else if (kind == SHAPE_LINEAR_QUAD || kind == SHAPE_QUADRATIC_QUAD) {
        pc[0] = pc[1] = 0.5;
    } else if (kind == SHAPE_QUADRATIC_PRISM) {
        pc[0] = pc[1] = 1.0 / 3.0;
        pc[2] = 0.5;
    } else if (kind == SHAPE_QUADRATIC_PYRAMID) {
        pc[0] = pc[1] = 0.5;
        pc[2] = 0.5;
    }
    // SHAPE_QUADRATIC_HEX 中心为 (0,0,0)
}

/**
 * VTK 的参数域接受容差：EvaluatePosition 判定 pcoords ∈ [-0.001, 1.001]（[0,1] 参数），
 * 即允许 1e-3 的越界；[-1,1] 参数化的单元域长为 2，等价容差为 2e-3。
 */
inline double ParamDomainTol(ShapeKind kind) {
    switch (kind) {
        case SHAPE_QUADRATIC_HEX:
        case SHAPE_BIQUADRATIC_QUAD:
        case SHAPE_QUADRATIC_LINEAR_QUAD:
        case SHAPE_TRIQUADRATIC_HEX:
        case SHAPE_QUADRATIC_LINEAR_WEDGE:
            return 2.0 * kVTKParamDomainTol;
        default:
            return kVTKParamDomainTol;
    }
}

bool ShapeDomainOk(ShapeKind kind, const double pc[3], double tol) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD:
        case SHAPE_QUADRATIC_QUAD:
            return pc[0] >= -tol && pc[0] <= 1.0 + tol && pc[1] >= -tol && pc[1] <= 1.0 + tol;
        case SHAPE_QUADRATIC_PRISM:
            return pc[0] >= -tol && pc[1] >= -tol && (pc[0] + pc[1]) <= 1.0 + tol && pc[2] >= -tol &&
                   pc[2] <= 1.0 + tol;
        case SHAPE_QUADRATIC_PYRAMID:
            return pc[0] >= -tol && pc[0] <= 1.0 + tol && pc[1] >= -tol && pc[1] <= 1.0 + tol && pc[2] >= -tol &&
                   pc[2] <= 1.0 + tol;
        case SHAPE_QUADRATIC_TRIANGLE:
            return pc[0] >= -tol && pc[1] >= -tol && (pc[0] + pc[1]) <= 1.0 + tol;
        case SHAPE_QUADRATIC_TETRA: {
            const double L0 = 1.0 - pc[0] - pc[1] - pc[2];
            return L0 >= -tol && pc[0] >= -tol && pc[1] >= -tol && pc[2] >= -tol && L0 <= 1.0 + tol;
        }
        case SHAPE_QUADRATIC_HEX:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol &&
                   pc[2] >= -1.0 - tol && pc[2] <= 1.0 + tol;
        // 以下 kind 的参数坐标直接使用 VTK 内部的 [-1,1] 形式
        case SHAPE_BIQUADRATIC_QUAD:
        case SHAPE_QUADRATIC_LINEAR_QUAD:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol;
        case SHAPE_TRIQUADRATIC_HEX:
        case SHAPE_QUADRATIC_LINEAR_WEDGE:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol &&
                   pc[2] >= -1.0 - tol && pc[2] <= 1.0 + tol;
        default:
            return false;
    }
}

/** 二次 3D 单元的 Newton 逆映射（参数坐标 → 权重 + 残差） */
bool NewtonQuadratic3D(ShapeKind kind, const std::vector<Point>& pts, const Point& target, double pc[3],
                       std::vector<double>& weights, double& residual) {
    const int npts = ShapePointCount(kind);
    if (npts <= 0 || static_cast<int>(pts.size()) < npts) { return false; }

    ShapeInitParams(kind, pc);

    double sf[kMaxCellPointNum];
    double dN[3 * kMaxCellPointNum];

    for (int iter = 0; iter < 32; ++iter) {
        ShapeFunctions(kind, pc, sf);
        ShapeDerivs(kind, pc, dN);

        double rcol[3] = {0.0, 0.0, 0.0};
        double scol[3] = {0.0, 0.0, 0.0};
        double tcol[3] = {0.0, 0.0, 0.0};
        double fcol[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < npts; ++i) {
            const Point& x = pts[static_cast<size_t>(i)];
            for (int j = 0; j < 3; ++j) {
                const double xv = static_cast<double>(x[j]);
                fcol[j] += sf[i] * xv;
                rcol[j] += dN[i] * xv;
                scol[j] += dN[i + npts] * xv;
                tcol[j] += dN[i + 2 * npts] * xv;
            }
        }
        for (int j = 0; j < 3; ++j) { fcol[j] -= static_cast<double>(target[j]); }

        const double detJ = Det3(rcol, scol, tcol);
        if (std::fabs(detJ) < 1e-30) { break; }

        const double negF[3] = {-fcol[0], -fcol[1], -fcol[2]};
        const double dp0 = Det3(negF, scol, tcol) / detJ;
        const double dp1 = Det3(rcol, negF, tcol) / detJ;
        const double dp2 = Det3(rcol, scol, negF) / detJ;

        pc[0] += dp0;
        pc[1] += dp1;
        pc[2] += dp2;

        if (std::fabs(dp0) + std::fabs(dp1) + std::fabs(dp2) < 1e-11) { break; }
        if (std::fabs(pc[0]) > 1e3 || std::fabs(pc[1]) > 1e3 || std::fabs(pc[2]) > 1e3) { return false; }
    }

    ShapeFunctions(kind, pc, sf);
    weights.assign(sf, sf + npts);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < npts; ++i) { x += pts[static_cast<size_t>(i)] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - target).length());
    return true;
}

/** 面单元（线性四边形 / 二次四边形 / 二次三角形）的 2D Newton（投影到单元平面） */
bool NewtonParametric2D(ShapeKind kind, const std::vector<Point>& pts, const Point& p, double pc[2],
                        std::vector<double>& weights, double& residual) {
    const int npts = ShapePointCount(kind);
    if (npts <= 0 || static_cast<int>(pts.size()) < npts) { return false; }

    // 用角点构造局部正交基
    const int lastCorner = IsQuadKind(kind) ? 3 : 2;
    Vector3f n = CrossProduct(pts[lastCorner] - pts[0], pts[1] - pts[0]);
    const double nl = n.length();
    if (nl < 1e-20) { return false; }
    n = n / static_cast<float>(nl);

    Vector3f u = pts[1] - pts[0];
    u = u - n * DotProduct(u, n);
    double ul = u.length();
    if (ul < 1e-20) { return false; }
    u = u / static_cast<float>(ul);
    const Vector3f v = CrossProduct(n, u);

    std::vector<double> qx(static_cast<size_t>(npts)), qy(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) {
        const Vector3f diff = pts[static_cast<size_t>(i)] - pts[0];
        qx[i] = static_cast<double>(DotProduct(diff, u));
        qy[i] = static_cast<double>(DotProduct(diff, v));
    }
    const Vector3f pd = p - pts[0];
    const double tx = static_cast<double>(DotProduct(pd, u));
    const double ty = static_cast<double>(DotProduct(pd, v));

    double p3[3] = {0.0, 0.0, 0.0};
    ShapeInitParams(kind, p3);

    double sf[kMaxCellPointNum];
    double dN[3 * kMaxCellPointNum];

    for (int iter = 0; iter < 32; ++iter) {
        ShapeFunctions(kind, p3, sf);
        ShapeDerivs(kind, p3, dN);

        double F[2] = {0.0, 0.0};
        double J[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
        for (int i = 0; i < npts; ++i) {
            F[0] += sf[i] * qx[i];
            F[1] += sf[i] * qy[i];
            J[0][0] += dN[i] * qx[i];
            J[1][0] += dN[i] * qy[i];
            J[0][1] += dN[i + npts] * qx[i];
            J[1][1] += dN[i + npts] * qy[i];
        }
        F[0] -= tx;
        F[1] -= ty;

        const double detJ = J[0][0] * J[1][1] - J[0][1] * J[1][0];
        if (std::fabs(detJ) < 1e-30) { break; }

        const double dr = (-F[0] * J[1][1] + F[1] * J[0][1]) / detJ;
        const double ds = (-F[1] * J[0][0] + F[0] * J[1][0]) / detJ;
        p3[0] += dr;
        p3[1] += ds;

        if (std::fabs(dr) + std::fabs(ds) < 1e-12) { break; }
        if (p3[0] < -4.0 || p3[0] > 5.0 || p3[1] < -4.0 || p3[1] > 5.0) { break; }
    }

    pc[0] = p3[0];
    pc[1] = p3[1];

    ShapeFunctions(kind, p3, sf);
    weights.assign(sf, sf + npts);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < npts; ++i) { x += pts[static_cast<size_t>(i)] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - p).length());
    return true;
}

/* ================================================================== */
/* 均值坐标（Mean Value Coordinates）                                  */
/* ================================================================== */

/**
 * 单个多边形面对采样点的均值坐标贡献。
 * 算法与 iGameStreamTracer / VTK vtkMeanValueCoordinatesInterpolator 一致：
 * 把面的顶点方向投影到单位球面，再用球面多边形的角度权重求和。
 */
bool MeanValueFaceWeights(const std::vector<Point>& face, const Point& x, std::vector<double>& w) {
    const int n = static_cast<int>(face.size());
    if (n < 3) { return false; }
    w.assign(static_cast<size_t>(n), 0.0);

    const double eps = 1e-14;
    std::vector<double> dist(static_cast<size_t>(n), 0.0);
    std::vector<Vector3f> u(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        const Vector3f d = face[static_cast<size_t>(i)] - x;
        const double l = d.length();
        if (l < 1e-12) { // 采样点与该面顶点重合
            w.assign(static_cast<size_t>(n), 0.0);
            w[static_cast<size_t>(i)] = 1.0;
            return true;
        }
        dist[i] = l;
        u[i] = d / static_cast<float>(l);
    }

    // 球面多边形的「面积向量」
    Vector3f vsum(0.f, 0.f, 0.f);
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        Vector3f cross = CrossProduct(u[i], u[j]);
        const double cl = cross.length();
        if (cl > eps) { cross = cross / static_cast<float>(cl); }
        const double chord = static_cast<double>((u[j] - u[i]).length());
        const double angle = 2.0 * std::asin(std::min(1.0, chord * 0.5));
        vsum += cross * static_cast<float>(0.5 * angle);
    }
    const double vNorm = vsum.length();
    if (vNorm < eps) { return false; }

    Vector3f v = vsum / static_cast<float>(vNorm);
    if (DotProduct(v, u[0]) < 0.f) { v = -v; }

    std::vector<double> alpha(static_cast<size_t>(n), 0.0);
    std::vector<double> theta(static_cast<size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        Vector3f n0 = CrossProduct(u[i], v);
        Vector3f n1 = CrossProduct(u[j], v);
        const double l0 = n0.length();
        const double l1 = n1.length();
        if (l0 > eps) { n0 = n0 / static_cast<float>(l0); }
        if (l1 > eps) { n1 = n1 / static_cast<float>(l1); }
        const double chord = static_cast<double>((n0 - n1).length());
        alpha[i] = 2.0 * std::asin(std::min(1.0, chord * 0.5));
        if (DotProduct(CrossProduct(n0, n1), v) < 0.f) { alpha[i] = -alpha[i]; }

        const double chordU = static_cast<double>((u[i] - v).length());
        theta[i] = 2.0 * std::asin(std::min(1.0, chordU * 0.5));
    }

    // 退化情形 1：采样点落在该面所在平面内
    for (int i = 0; i < n; ++i) {
        if (std::fabs(theta[i]) < 1e-12) {
            w[i] = vNorm / dist[i];
            return true;
        }
    }

    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        const int prev = (i - 1 + n) % n;
        const double ti = std::tan(theta[i]);
        if (std::fabs(ti) < 1e-12) { return false; }
        sum += (1.0 / ti) * (std::tan(alpha[i] * 0.5) + std::tan(alpha[prev] * 0.5));
    }

    if (std::fabs(sum) < eps) {
        // 退化情形 2：改用与球面角度相关的备选权重
        std::vector<double> tmp(static_cast<size_t>(n), 0.0);
        double sumW = 0.0;
        for (int i = 0; i < n; ++i) {
            const int prev = (i - 1 + n) % n;
            tmp[i] = (std::tan(theta[i] * 0.5) + std::tan(theta[prev] * 0.5)) / dist[i];
            sumW += tmp[i];
        }
        if (std::fabs(sumW) < eps) { return false; }
        for (int i = 0; i < n; ++i) { w[i] = tmp[i] / sumW; }
        return true;
    }

    for (int i = 0; i < n; ++i) {
        const int prev = (i - 1 + n) % n;
        const double st = std::sin(theta[i]);
        if (std::fabs(st) < 1e-12) { return false; }
        w[i] = vNorm / sum / dist[i] / st * (std::tan(alpha[i] * 0.5) + std::tan(alpha[prev] * 0.5));
    }
    return true;
}

/* ================================================================== */
/* 基础几何工具                                                        */
/* ================================================================== */

/** 点到线段的最近点（返回距离平方） */
void ClosestPointOnSegment(const Point& p, const Point& a, const Point& b, Point& closest, double& distSq) {
    const Vector3f ab = b - a;
    const double denom = static_cast<double>(DotProduct(ab, ab));
    if (denom < 1e-24) {
        closest = a;
        distSq = static_cast<double>((p - a).squaredLength());
        return;
    }
    double t = static_cast<double>(DotProduct(p - a, ab)) / denom;
    t = std::min(1.0, std::max(0.0, t));
    closest = a + ab * static_cast<float>(t);
    distSq = static_cast<double>((closest - p).squaredLength());
}

/** 点到三角形的最近点（返回距离平方） */
void ClosestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, Point& closest,
                            double& distSq) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ap = p - a;

    const double d1 = static_cast<double>(DotProduct(ab, ap));
    const double d2 = static_cast<double>(DotProduct(ac, ap));
    if (d1 <= 0.0 && d2 <= 0.0) {
        closest = a;
        distSq = static_cast<double>(DotProduct(ap, ap));
        return;
    }

    const Vector3f bp = p - b;
    const double d3 = static_cast<double>(DotProduct(ab, bp));
    const double d4 = static_cast<double>(DotProduct(ac, bp));
    if (d3 >= 0.0 && d4 <= d3) {
        closest = b;
        distSq = static_cast<double>(DotProduct(bp, bp));
        return;
    }

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double t = (d1 - d3) != 0.0 ? d1 / (d1 - d3) : 0.0;
        closest = a + ab * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const Vector3f cp = p - c;
    const double d5 = static_cast<double>(DotProduct(ab, cp));
    const double d6 = static_cast<double>(DotProduct(ac, cp));
    if (d6 >= 0.0 && d5 <= d6) {
        closest = c;
        distSq = static_cast<double>(DotProduct(cp, cp));
        return;
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double t = (d2 - d6) != 0.0 ? d2 / (d2 - d6) : 0.0;
        closest = a + ac * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double denom = (d4 - d3) + (d5 - d6);
        const double t = denom != 0.0 ? (d4 - d3) / denom : 0.0;
        closest = b + (c - b) * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const Vector3f n = CrossProduct(ab, ac);
    const double denom = static_cast<double>(DotProduct(n, n));
    if (denom > 1e-24) {
        const double t = static_cast<double>(DotProduct(n, ap)) / denom;
        closest = p - n * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    closest = a;
    double minDistSq = static_cast<double>(DotProduct(ap, ap));
    const double distB = static_cast<double>(DotProduct(bp, bp));
    if (distB < minDistSq) {
        closest = b;
        minDistSq = distB;
    }
    const double distC = static_cast<double>(DotProduct(cp, cp));
    if (distC < minDistSq) {
        closest = c;
        minDistSq = distC;
    }
    distSq = minDistSq;
}

/** 三角形重心坐标（按投影点计算），同时返回点到三角面的距离 */
bool TriangleBarycentric(const Point& p, const Point& a, const Point& b, const Point& c, double w[3],
                         double& planeDist) {
    const Vector3f n = CrossProduct(b - a, c - a);
    const double nn = static_cast<double>(DotProduct(n, n));
    if (nn < 1e-24) { return false; }

    const Vector3f ap = p - a;
    const double t = static_cast<double>(DotProduct(n, ap)) / nn;
    planeDist = std::fabs(t) * std::sqrt(nn);
    const Vector3f proj = p - n * static_cast<float>(t);

    const Vector3f v0 = b - a;
    const Vector3f v1 = c - a;
    const Vector3f v2 = proj - a;
    const double d00 = static_cast<double>(DotProduct(v0, v0));
    const double d01 = static_cast<double>(DotProduct(v0, v1));
    const double d11 = static_cast<double>(DotProduct(v1, v1));
    const double d20 = static_cast<double>(DotProduct(v2, v0));
    const double d21 = static_cast<double>(DotProduct(v2, v1));
    const double denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-24) { return false; }

    const double l1 = (d11 * d20 - d01 * d21) / denom;
    const double l2 = (d00 * d21 - d01 * d20) / denom;
    w[0] = 1.0 - l1 - l2;
    w[1] = l1;
    w[2] = l2;
    return true;
}

/** 权重规整：去掉微小负值并归一化（线性 / 二次插值用） */
void NormalizeWeights(std::vector<double>& w) {
    double sum = 0.0;
    for (double& v : w) {
        if (v < 0.0) { v = 0.0; }
        sum += v;
    }
    if (sum <= 1e-30) {
        const double v = w.empty() ? 0.0 : 1.0 / static_cast<double>(w.size());
        for (double& x : w) { x = v; }
        return;
    }
    for (double& v : w) { v /= sum; }
}

/** 权重仅归一化（均值坐标允许出现负权重） */
bool NormalizeKeepSign(std::vector<double>& w) {
    double sum = 0.0;
    for (const double v : w) { sum += v; }
    if (std::fabs(sum) <= 1e-30) { return false; }
    for (double& v : w) { v /= sum; }
    return true;
}

/** 按输入数组类型创建同类型的输出数组（保留数组数据类型） */
ArrayObject::Pointer CreateArrayObject(IGenum arrayType) {
    switch (arrayType) {
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
            return FloatArray::New();
    }
}

/** 广义绕数（Generalized Winding Number）：判断点是否在闭合三角面片边界内部 */
double WindingNumber(const std::vector<Point>& triA, const std::vector<Point>& triB, const std::vector<Point>& triC,
                     const Point& p) {
    double total = 0.0;
    const size_t n = triA.size();
    for (size_t t = 0; t < n; ++t) {
        const double A[3] = {triA[t][0] - p[0], triA[t][1] - p[1], triA[t][2] - p[2]};
        const double B[3] = {triB[t][0] - p[0], triB[t][1] - p[1], triB[t][2] - p[2]};
        const double C[3] = {triC[t][0] - p[0], triC[t][1] - p[1], triC[t][2] - p[2]};
        const double la = std::sqrt(A[0] * A[0] + A[1] * A[1] + A[2] * A[2]);
        const double lb = std::sqrt(B[0] * B[0] + B[1] * B[1] + B[2] * B[2]);
        const double lc = std::sqrt(C[0] * C[0] + C[1] * C[1] + C[2] * C[2]);
        if (la < 1e-30 || lb < 1e-30 || lc < 1e-30) { continue; }

        const double num = A[0] * (B[1] * C[2] - B[2] * C[1]) - A[1] * (B[0] * C[2] - B[2] * C[0]) +
                           A[2] * (B[0] * C[1] - B[1] * C[0]);
        const double den = la * lb * lc + (A[0] * B[0] + A[1] * B[1] + A[2] * B[2]) * lc +
                           (B[0] * C[0] + B[1] * C[1] + B[2] * C[2]) * la +
                           (C[0] * A[0] + C[1] * A[1] + C[2] * A[2]) * lb;
        total += 2.0 * std::atan2(num, den);
    }
    return total / (4.0 * 3.14159265358979323846);
}

/* ================================================================== */
/* 体单元边界面工具（面 = pts 下标序列）                               */
/* ================================================================== */
using CellFaceList = std::vector<std::vector<int>>;

/**
 * 收集单元边界面。
 * globalIds == nullptr 时认为面的点 id 就是 pts 下标（角点线性单元用）；
 * 否则按网格全局 id 映射到 pts 下标（多面体可能重复出现同一顶点，取首个匹配）。
 */
void CollectCellFaces(Cell* cell, const igIndex* globalIds, int npts, CellFaceList& faces) {
    faces.clear();
    if (cell == nullptr) { return; }
    const int nfaces = cell->GetNumberOfFaces();
    if (nfaces <= 0) { return; }

    for (int f = 0; f < nfaces; ++f) {
        auto face = cell->GetFace(f);
        if (face == nullptr) { continue; }
        const int fn = face->GetNumberOfPoints();
        if (fn < 3) { continue; }

        std::vector<int> ids;
        ids.reserve(static_cast<size_t>(fn));
        for (int k = 0; k < fn; ++k) {
            const igIndex fid = face->GetPointId(k);
            int local = -1;
            if (globalIds == nullptr) {
                local = static_cast<int>(fid);
            } else {
                for (int i = 0; i < npts; ++i) {
                    if (globalIds[i] == fid) {
                        local = i;
                        break;
                    }
                }
            }
            if (local >= 0 && local < npts) { ids.push_back(local); }
        }
        if (ids.size() >= 3) { faces.push_back(std::move(ids)); }
    }
}

/** 边界面扇形三角化 */
void TriangulateFaces(const std::vector<Point>& pts, const CellFaceList& faces, std::vector<Point>& triA,
                      std::vector<Point>& triB, std::vector<Point>& triC) {
    triA.clear();
    triB.clear();
    triC.clear();
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        for (size_t k = 1; k + 1 < face.size(); ++k) {
            triA.push_back(pts[static_cast<size_t>(face[0])]);
            triB.push_back(pts[static_cast<size_t>(face[k])]);
            triC.push_back(pts[static_cast<size_t>(face[k + 1])]);
        }
    }
}

/** 点到单元边界的最小距离 */
double DistanceToFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, Point& closest) {
    double best = DBL_MAX;
    closest = p;
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        for (size_t k = 1; k + 1 < face.size(); ++k) {
            Point c(0.f, 0.f, 0.f);
            double d2 = 0.0;
            ClosestPointOnTriangle(p, pts[static_cast<size_t>(face[0])], pts[static_cast<size_t>(face[k])],
                                   pts[static_cast<size_t>(face[k + 1])], c, d2);
            if (d2 < best) {
                best = d2;
                closest = c;
            }
        }
    }
    return (best == DBL_MAX) ? DBL_MAX : std::sqrt(best);
}

/** 闭合边界面片 → 点是否在体内（贴面 或 广义绕数 |w| > 0.5） */
bool ContainsByFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, double distTol) {
    Point closest;
    if (DistanceToFaces(pts, faces, p, closest) <= distTol) { return true; }

    std::vector<Point> triA, triB, triC;
    TriangulateFaces(pts, faces, triA, triB, triC);
    if (triA.empty()) { return false; }
    return std::fabs(WindingNumber(triA, triB, triC, p)) > 0.5;
}

/** 逐面均值坐标（权重按 pts 下标返回，含归一化与线性精度校验） */
bool MeanValueWeightsFromFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, double distTol,
                               std::vector<double>& w) {
    w.assign(pts.size(), 0.0);

    std::vector<double> faceWeights;
    std::vector<Point> facePts;
    bool any = false;
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        facePts.clear();
        for (const int idx : face) { facePts.push_back(pts[static_cast<size_t>(idx)]); }
        if (!MeanValueFaceWeights(facePts, p, faceWeights)) { continue; }
        for (size_t k = 0; k < face.size(); ++k) { w[static_cast<size_t>(face[k])] += faceWeights[k]; }
        any = true;
    }
    if (!any) { return false; }
    if (!NormalizeKeepSign(w)) { return false; }

    // 线性精度校验：均值坐标应能重建采样点位置，否则退化为距离反比权重
    Point rebuild(0.f, 0.f, 0.f);
    for (size_t i = 0; i < pts.size(); ++i) { rebuild += pts[i] * static_cast<float>(w[i]); }
    const double err = static_cast<double>((rebuild - p).length());
    double scale = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) { scale = std::max(scale, static_cast<double>((pts[i] - p).length())); }
    if (err > std::max(distTol * 10.0, scale * 1e-3)) {
        for (size_t i = 0; i < pts.size(); ++i) {
            const double d2 = static_cast<double>((pts[i] - p).squaredLength());
            w[i] = 1.0 / (d2 + 1e-30);
        }
        if (!NormalizeKeepSign(w)) { return false; }
    }
    return true;
}

/** 由单元类型得到「角点线性体单元」的对象与角点数（用于高次体单元的退化处理） */
int LinearCornerVolumeCell(IGenum cellType, Cell::Pointer& cell) {
    switch (cellType) {
        case IG_QUADRATIC_TETRA:
        case IG_LAGRANGE_TETRAHEDRON:
            cell = Tetra::New();
            return 4;
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
            cell = Hexahedron::New();
            return 8;
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_LAGRANGE_PRISM:
            cell = Prism::New();
            return 6;
        case IG_QUADRATIC_PYRAMID:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_LAGRANGE_PYRAMID:
            cell = Pyramid::New();
            return 5;
        default:
            cell = nullptr;
            return 0;
    }
}

/* ================================================================== */
/* 输入归一化：被采样网格（任意受支持网格 / 容器）→ UnstructuredMesh     */
/*                                                                    */
/* ParaView 的 Resample With Dataset 直接接受任意 vtkDataSet，并逐块展开 */
/* 复合数据集；这里补齐同样的能力，否则结构化网格 / 点云 / 时间序列这类   */
/* 容器会以「无法转换为 UnstructuredMesh」失败（ResampleToLine 已有同样  */
/* 的归一化内核）。                                                     */
/* ================================================================== */

/** 数据对象类型名，用于失败 / 结果消息 */
const char* SourceTypeName(IGenum type) {
    switch (type) {
        case IG_DATA_OBJECT: return "DataObject";
        case IG_COMPOSITE_DATA_OBJECT: return "CompositeDataObject";
        case IG_DRAW_OBJECT: return "DrawObject";
        case IG_POINT_SET: return "PointSet";
        case IG_SURFACE_MESH: return "SurfaceMesh";
        case IG_VOLUME_MESH: return "VolumeMesh";
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_STRUCTURED_MESH: return "StructuredMesh";
        case IG_MULTIBLOCK_MESH: return "MultiBlockMesh";
        case IG_SPLINE_GEOMETRY: return "SplineGeometry";
        case IG_LAGRANGE_UNSTRUCTURED_MESH: return "LagrangeUnstructuredMesh";
        default: return "Unknown";
    }
}

/** 结构化网格点索引：i 变化最快，其次 j，最后 k（与 iGameStructuredMesh 一致） */
inline igIndex StructuredPointId(const igIndex n[3], igIndex i, igIndex j, igIndex k) {
    return i + j * n[0] + k * n[0] * n[1];
}

/**
 * StructuredMesh → UnstructuredMesh
 *
 * 按 (ni, nj, nk) 中「尺寸大于 1 的维数」生成单元：
 *   0 维 → IG_VERTEX（单点）；1 维 → IG_LINE（曲线）；2 维 → IG_QUAD（平面）；3 维 → IG_HEXAHEDRON。
 * 节点序与 StructuredMesh::GenStructuredCellConnectivities() 保持一致。这里不修改输入网格，
 * 也不依赖其连接关系是否已经生成。
 */
UnstructuredMesh::Pointer MeshFromStructured(StructuredMesh* structured) {
    if (structured == nullptr || structured->GetPoints() == nullptr) { return nullptr; }

    igIndex n[3] = {1, 1, 1};
    igIndex* size = structured->GetDimensionSize();
    if (size != nullptr) {
        for (int a = 0; a < 3; ++a) { n[a] = std::max<igIndex>(1, size[a]); }
    }
    if (structured->GetNumberOfPoints() < n[0] * n[1] * n[2]) { return nullptr; }

    auto mesh = UnstructuredMesh::New();
    mesh->SetName(structured->GetName());
    mesh->SetPoints(structured->GetPoints());
    mesh->SetAttributeSet(structured->GetAttributeSet());

    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    const auto pid = [&n](igIndex i, igIndex j, igIndex k) { return StructuredPointId(n, i, j, k); };

    int varying[3] = {0, 0, 0};
    int nv = 0;
    for (int a = 0; a < 3; ++a) {
        if (n[a] > 1) { varying[nv++] = a; }
    }

    if (nv == 0) {
        // 仅一个点
        igIndex id = 0;
        cells->AddCellIds(&id, 1);
        types->AddValue(IG_VERTEX);
    } else if (nv == 1) {
        // 1D 曲线
        const int a = varying[0];
        for (igIndex t = 0; t + 1 < n[a]; ++t) {
            igIndex c[3] = {0, 0, 0};
            igIndex id[2];
            c[a] = t;
            id[0] = pid(c[0], c[1], c[2]);
            c[a] = t + 1;
            id[1] = pid(c[0], c[1], c[2]);
            cells->AddCellIds(id, 2);
            types->AddValue(IG_LINE);
        }
    } else if (nv == 2) {
        // 2D 平面（含 (1, nj, nk) 这类退化排布）
        const int a = varying[0], b = varying[1];
        for (igIndex tb = 0; tb + 1 < n[b]; ++tb) {
            for (igIndex ta = 0; ta + 1 < n[a]; ++ta) {
                igIndex c[3] = {0, 0, 0};
                igIndex id[4];
                c[a] = ta;
                c[b] = tb;
                id[0] = pid(c[0], c[1], c[2]);
                c[a] = ta + 1;
                id[1] = pid(c[0], c[1], c[2]);
                c[b] = tb + 1;
                id[2] = pid(c[0], c[1], c[2]);
                c[a] = ta;
                id[3] = pid(c[0], c[1], c[2]);
                cells->AddCellIds(id, 4);
                types->AddValue(IG_QUAD);
            }
        }
    } else {
        // 3D 六面体
        for (igIndex k = 0; k + 1 < n[2]; ++k) {
            for (igIndex j = 0; j + 1 < n[1]; ++j) {
                for (igIndex i = 0; i + 1 < n[0]; ++i) {
                    igIndex id[8];
                    id[0] = pid(i, j, k);
                    id[1] = pid(i + 1, j, k);
                    id[2] = pid(i + 1, j, k + 1);
                    id[3] = pid(i, j, k + 1);
                    id[4] = pid(i, j + 1, k);
                    id[5] = pid(i + 1, j + 1, k);
                    id[6] = pid(i + 1, j + 1, k + 1);
                    id[7] = pid(i, j + 1, k + 1);
                    cells->AddCellIds(id, 8);
                    types->AddValue(IG_HEXAHEDRON);
                }
            }
        }
    }

    mesh->SetCells(cells, types);
    return mesh;
}

/** PointSet（点云）→ UnstructuredMesh：每个点一个 IG_VERTEX 单元（等价 vtkPolyData 的 verts） */
UnstructuredMesh::Pointer MeshFromPointSet(PointSet* pointSet) {
    if (pointSet == nullptr || pointSet->GetPoints() == nullptr) { return nullptr; }

    auto mesh = UnstructuredMesh::New();
    mesh->SetName(pointSet->GetName());
    mesh->SetPoints(pointSet->GetPoints());
    mesh->SetAttributeSet(pointSet->GetAttributeSet());

    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    const IGsize pointNum = pointSet->GetNumberOfPoints();
    for (IGsize i = 0; i < pointNum; ++i) {
        igIndex id = static_cast<igIndex>(i);
        cells->AddCellIds(&id, 1);
        types->AddValue(IG_VERTEX);
    }
    mesh->SetCells(cells, types);
    return mesh;
}

/** LagrangeUnstructuredMesh → UnstructuredMesh：保留 IG_LAGRANGE_* 单元类型 */
UnstructuredMesh::Pointer MeshFromLagrange(LagrangeUnstructuredMesh* lagrange) {
    if (lagrange == nullptr || lagrange->GetPoints() == nullptr) { return nullptr; }

    auto mesh = UnstructuredMesh::New();
    mesh->SetName(lagrange->GetName());
    mesh->SetPoints(lagrange->GetPoints());
    mesh->SetAttributeSet(lagrange->GetAttributeSet());

    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    const IGsize cellNum = lagrange->GetNumberOfCells();
    for (IGsize c = 0; c < cellNum; ++c) {
        const igIndex* ids = nullptr;
        const int cnt = lagrange->GetCellPointIds(c, ids);
        // 即使单元退化也要占位，保证单元序号与 Cell Data 对齐
        cells->AddCellIds((cnt > 0) ? ids : nullptr, std::max(0, cnt));
        types->AddValue(lagrange->GetSpecificCellType(c));
    }
    mesh->SetCells(cells, types);
    return mesh;
}

/** 多块合并时需要的属性布局（同名 + 同附着类型视为同一个输出属性） */
struct SourceAttributeLayout {
    std::string name;
    IGenum type{IG_NONE};
    IGenum attachmentType{IG_POINT};
    IGenum arrayType{IG_FloatArray};
    int dimension{1};
};

void CollectSourceAttributeLayout(AttributeSet* set, std::vector<SourceAttributeLayout>& layouts) {
    if (set == nullptr) { return; }
    auto all = set->GetAllAttributes();
    if (all == nullptr) { return; }

    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) { continue; }
        const int dim = attr.pointer->GetDimension();
        if (dim <= 0) { continue; }

        bool found = false;
        for (auto& layout : layouts) {
            if (layout.name == attr.pointer->GetName() && layout.attachmentType == attr.attachmentType) {
                layout.dimension = std::max(layout.dimension, dim);
                found = true;
                break;
            }
        }
        if (found) { continue; }

        SourceAttributeLayout layout;
        layout.name = attr.pointer->GetName();
        layout.type = attr.type;
        layout.attachmentType = attr.attachmentType;
        layout.arrayType = attr.pointer->GetArrayType();
        layout.dimension = dim;
        layouts.push_back(std::move(layout));
    }
}

/**
 * 多块网格合并为一个 UnstructuredMesh（等价先做 Merge Blocks 再采样）：
 * 点 / 单元按块顺序追加并做序号偏移，属性按「名字 + 附着类型」取并集后逐块填写。
 * 采样点同时落在多个块中时，序号靠前的块先命中（LocateSample 按单元序号取第一个命中）。
 */
UnstructuredMesh::Pointer MergeSourceBlocks(const std::vector<UnstructuredMesh::Pointer>& blocks) {
    if (blocks.empty()) { return nullptr; }
    if (blocks.size() == 1) { return blocks[0]; }

    IGsize totalPoints = 0;
    IGsize totalCells = 0;
    std::vector<SourceAttributeLayout> layouts;
    for (const auto& block : blocks) {
        if (block == nullptr) { continue; }
        totalPoints += block->GetNumberOfPoints();
        totalCells += block->GetNumberOfCells();
        CollectSourceAttributeLayout(block->GetAttributeSet(), layouts);
    }

    auto mesh = UnstructuredMesh::New();
    for (const auto& block : blocks) {
        if (block != nullptr) {
            mesh->SetName(block->GetName());
            break;
        }
    }

    // 1) 合并点
    auto points = Points::New();
    for (const auto& block : blocks) {
        if (block == nullptr) { continue; }
        const IGsize pointNum = block->GetNumberOfPoints();
        for (IGsize i = 0; i < pointNum; ++i) { points->AddPoint(block->GetPoint(i)); }
    }
    mesh->SetPoints(points);

    // 2) 合并单元（点序号偏移）
    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    std::vector<igIndex> ids;
    igIndex pointOffset = 0;
    for (const auto& block : blocks) {
        if (block == nullptr) { continue; }
        const IGsize cellNum = block->GetNumberOfCells();
        for (IGsize c = 0; c < cellNum; ++c) {
            const igIndex* cellIds = nullptr;
            const int cnt = block->GetCellPointIds(c, cellIds);
            ids.resize(static_cast<size_t>(std::max(0, cnt)));
            for (int k = 0; k < cnt; ++k) { ids[static_cast<size_t>(k)] = cellIds[k] + pointOffset; }
            cells->AddCellIds((cnt > 0) ? ids.data() : nullptr, std::max(0, cnt));
            types->AddValue(block->GetCellType(c));
        }
        pointOffset += static_cast<igIndex>(block->GetNumberOfPoints());
    }
    mesh->SetCells(cells, types);

    // 3) 合并属性：先按总长度建数组（多出的位置由 Resize 置 0），再逐块填写
    AttributeSet::Pointer outSet = AttributeSet::New();
    std::vector<ArrayObject::Pointer> arrays;
    arrays.reserve(layouts.size());
    for (const auto& layout : layouts) {
        auto array = CreateArrayObject(layout.arrayType);
        if (array == nullptr) {
            arrays.push_back(nullptr);
            continue;
        }
        array->SetName(layout.name);
        array->SetDimension(layout.dimension);
        array->Resize(layout.attachmentType == IG_POINT ? totalPoints : totalCells);
        arrays.push_back(array);
    }

    std::vector<double> values;
    igIndex pointBase = 0;
    igIndex cellBase = 0;
    for (const auto& block : blocks) {
        if (block == nullptr) { continue; }
        AttributeSet* set = block->GetAttributeSet();
        const IGsize pointNum = block->GetNumberOfPoints();
        const IGsize cellNum = block->GetNumberOfCells();

        if (set != nullptr) {
            for (size_t a = 0; a < layouts.size(); ++a) {
                if (arrays[a] == nullptr) { continue; }
                const SourceAttributeLayout& layout = layouts[a];
                const bool isPoint = (layout.attachmentType == IG_POINT);
                const IGsize count = isPoint ? pointNum : cellNum;
                const igIndex base = isPoint ? pointBase : cellBase;

                ArrayObject* src = set->GetArrayPointer(IG_NONE, layout.attachmentType, layout.name);
                if (src == nullptr || src->GetDimension() <= 0) { continue; }

                values.assign(static_cast<size_t>(layout.dimension), 0.0);
                for (IGsize i = 0; i < count; ++i) {
                    std::fill(values.begin(), values.end(), 0.0);
                    src->GetElement(i, values.data());
                    arrays[a]->SetElement(base + i, values.data());
                }
            }
        }

        pointBase += static_cast<igIndex>(pointNum);
        cellBase += static_cast<igIndex>(cellNum);
    }

    for (size_t a = 0; a < layouts.size(); ++a) {
        if (arrays[a] == nullptr) { continue; }
        outSet->AddAttribute(layouts[a].type, layouts[a].attachmentType, arrays[a]);
    }
    mesh->SetAttributeSet(outSet);
    return mesh;
}

/** 单个叶子网格 → UnstructuredMesh；不支持的类型返回 nullptr */
UnstructuredMesh::Pointer MeshFromLeaf(DataObject* object) {
    switch (object->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH:
            return DynamicCast<UnstructuredMesh>(object);
        case IG_LAGRANGE_UNSTRUCTURED_MESH:
            return MeshFromLagrange(DynamicCast<LagrangeUnstructuredMesh>(object));
        case IG_SURFACE_MESH: {
            auto surface = DynamicCast<SurfaceMesh>(object);
            if (surface == nullptr) { return nullptr; }
            auto mesh = UnstructuredMesh::New();
            return mesh->GenerateFromSurfaceMesh(surface) ? mesh : nullptr;
        }
        case IG_VOLUME_MESH: {
            auto volume = DynamicCast<VolumeMesh>(object);
            if (volume == nullptr) { return nullptr; }
            auto mesh = UnstructuredMesh::New();
            return mesh->GenerateFromVolumeMesh(volume) ? mesh : nullptr;
        }
        case IG_STRUCTURED_MESH:
            return MeshFromStructured(DynamicCast<StructuredMesh>(object));
        case IG_POINT_SET:
            return MeshFromPointSet(DynamicCast<PointSet>(object));
        default:
            return nullptr;
    }
}

/**
 * 递归收集输入中的所有网格块：
 *   - 目标是具体网格类型 → 直接转换；
 *   - 目标是容器（DrawObject / DataObject / CompositeDataObject 等）→ 遍历子数据对象，
 *     使多帧时间序列、多 zone 文件这类「父对象 + 多个子网格」的输入也能整体采样。
 */
void CollectSourceBlocks(DataObject* object, std::vector<UnstructuredMesh::Pointer>& blocks, int depth) {
    if (object == nullptr || depth > 8) { return; }

    switch (object->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH:
        case IG_LAGRANGE_UNSTRUCTURED_MESH:
        case IG_SURFACE_MESH:
        case IG_VOLUME_MESH:
        case IG_STRUCTURED_MESH:
        case IG_POINT_SET: {
            auto mesh = MeshFromLeaf(object);
            if (mesh != nullptr) { blocks.push_back(mesh); }
            return;
        }
        default:
            break;
    }

    if (auto composite = DynamicCast<CompositeDataObject>(object)) {
        const unsigned int childNum = composite->GetNumberOfChildren();
        for (unsigned int i = 0; i < childNum; ++i) {
            CollectSourceBlocks(composite->GetChild(static_cast<int>(i)), blocks, depth + 1);
        }
        if (childNum > 0) { return; }
    }

    if (object->GetNumberOfSubDataObjects() > 0) {
        for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it) {
            CollectSourceBlocks(it->second.GetPointer(), blocks, depth + 1);
        }
    }
}

/**
 * 归一化入口：任意受支持网格 / 容器 → 一份 UnstructuredMesh。
 * 单块输入不做拷贝（直接返回转换结果）；blockCount 返回合并前的块数。
 */
UnstructuredMesh::Pointer BuildSourceMesh(DataObject* object, int& blockCount) {
    blockCount = 0;
    if (object == nullptr) { return nullptr; }

    std::vector<UnstructuredMesh::Pointer> blocks;
    CollectSourceBlocks(object, blocks, 0);
    blockCount = static_cast<int>(blocks.size());
    if (blocks.empty()) { return nullptr; }
    return MergeSourceBlocks(blocks);
}

} // namespace

/* ------------------------------------------------------------------ */
/* 主流程                                                              */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::Execute() {
    // 输入 0：被采样网格（数据来源，VTK 的 Source）
    // 输入 1：采样点网格（探针几何，输出是它的深拷贝；VTK 的 Input）
    auto source = GetInput(0);
    auto probe = GetInput(1);

    m_UnsupportedQuadraticCellCount = 0;
    m_SnappedPointCount = 0;
    m_Output = nullptr;
    SetOutput(0, nullptr);
    m_SampleCellIds.clear();
    m_SampleValidMask.clear();

    if (source == nullptr) {
        m_Message = "未选择「被采样网格」";
        return false;
    }
    if (probe == nullptr) {
        m_Message = "未选择「采样点网格」";
        return false;
    }

    auto probePoints = probe->GetPoints();
    if (probePoints == nullptr || probePoints->GetNumberOfPoints() == 0) {
        m_Message = std::string("「采样点网格」没有点，无法确定采样位置（数据类型：") +
                    SourceTypeName(probe->GetDataObjectType()) + "）";
        return false;
    }

    // 被采样网格：归一化为一份 UnstructuredMesh。
    // 对齐 ParaView 的 Resample With Dataset（任意 vtkDataSet + 复合数据集逐块展开）：
    //   UnstructuredMesh 直接用；SurfaceMesh / VolumeMesh 转换；StructuredMesh 按 (ni,nj,nk) 生成单元；
    //   PointSet 每点生成 IG_VERTEX；LagrangeUnstructuredMesh 保留原单元类型；
    //   容器（DrawObject / CompositeDataObject / 带子数据对象）递归展开后合并为一份网格。
    int sourceBlockCount = 0;
    auto mesh = BuildSourceMesh(source.GetPointer(), sourceBlockCount);
    if (mesh == nullptr) {
        m_Message = std::string("「被采样网格」不支持的数据类型：") + SourceTypeName(source->GetDataObjectType()) +
                    "（支持 PointSet / SurfaceMesh / VolumeMesh / UnstructuredMesh / StructuredMesh / "
                    "LagrangeUnstructuredMesh，以及包含它们的容器网格）";
        return false;
    }
    if (mesh->GetNumberOfPoints() == 0 || mesh->GetNumberOfCells() == 0) {
        m_Message = "「被采样网格」没有单元或点";
        return false;
    }

    const BoundingBox& bbox = mesh->GetBoundingBox();
    const double diag = std::max(bbox.diag(), 1e-12);

    // 容差：自动（包围盒对角线 × 1e-6）或手动；吸附半径按同一比例换算成绝对长度
    m_EffectiveTolerance = m_AutoTolerance ? (diag * kAutoToleranceRatio) : (m_Tolerance * diag);
    const double distTol = m_EffectiveTolerance;
    const bool snappingEnabled = (m_SnappingRadius > 0.0);
    m_EffectiveSnappingRadius = snappingEnabled ? (m_SnappingRadius * diag) : -1.0;

    // 采样位置就是「采样点网格」的所有点
    const int sampleNum = static_cast<int>(probePoints->GetNumberOfPoints());
    std::vector<SampleLocation> locations(static_cast<size_t>(sampleNum));
    for (int i = 0; i < sampleNum; ++i) {
        locations[static_cast<size_t>(i)].point = probePoints->GetPoint(i);
    }

    // 均匀网格加速结构
    const double cellNum = static_cast<double>(mesh->GetNumberOfCells());
    igIndex gridDim = static_cast<igIndex>(std::cbrt(cellNum) + 0.5);
    gridDim = std::max<igIndex>(4, std::min<igIndex>(64, gridDim));
    const igIndex nx = gridDim, ny = gridDim, nz = gridDim;
    auto grid = BuildUniformGrid(mesh, bbox, nx, ny, nz);

    // 逐采样点定位单元；未命中且开启吸附半径时，尝试吸附到半径内最近单元
    m_SampleCellIds.assign(static_cast<size_t>(sampleNum), -1);
    m_SampleValidMask.assign(static_cast<size_t>(sampleNum), 0);
    int validCount = 0;
    for (int i = 0; i < sampleNum; ++i) {
        const Point p = probePoints->GetPoint(i);

        SampleLocation loc;
        if (LocateSample(mesh, p, bbox, grid, nx, ny, nz, distTol, loc) && loc.cellId >= 0) {
            locations[static_cast<size_t>(i)] = loc;
            m_SampleCellIds[static_cast<size_t>(i)] = loc.cellId;
            m_SampleValidMask[static_cast<size_t>(i)] = 1;
            ++validCount;
            continue;
        }

        if (!snappingEnabled) { continue; }

        igIndex snapCellId = -1;
        Point closest;
        // 半径内找不到单元 → 保持无效（validpointmask = 0，各属性填 0）
        if (!LocateNearestCell(mesh, p, bbox, grid, nx, ny, nz, m_EffectiveSnappingRadius, snapCellId, closest)) {
            continue;
        }
        // 用单元边界上的最近点求权重（该点落在单元上），采样点自身坐标保持不变
        std::vector<double> weights;
        if (!ComputeCellWeights(mesh, snapCellId, closest, distTol, weights) || weights.empty()) { continue; }

        igIndex ptIds[IGAME_CELL_MAX_SIZE];
        const int npts = mesh->GetCellPointIds(snapCellId, ptIds);
        if (npts <= 0) { continue; }

        SampleLocation snapped;
        snapped.cellId = snapCellId;
        snapped.point = p;
        snapped.pointIds.assign(ptIds, ptIds + npts);
        snapped.weights = std::move(weights);
        locations[static_cast<size_t>(i)] = snapped;
        m_SampleCellIds[static_cast<size_t>(i)] = snapCellId;
        m_SampleValidMask[static_cast<size_t>(i)] = 1;
        ++validCount;
        ++m_SnappedPointCount;
    }

    // 属性：① 被采样网格的点数据插值 ② 其单元数据按命中单元拷到输出点 ③ validpointmask
    // 与 vtkProbeFilter 一致：源单元数组进入**输出点数据**（VTK 内部的 InputCellArrays），
    // 同名时点数据优先；无效采样点各属性填 0。
    // 注意：对齐 vtkProbeFilter 的默认值（PassPointArrays = 0、PassCellArrays = 0），
    // 「采样点网格」自身的属性数组不复制到输出里（这两个开关针对的是探针输入，不是被采样网格）。
    AttributeSet* inAttr = mesh->GetAttributeSet();
    AttributeSet::Pointer outAttr = AttributeSet::New();
    const std::vector<std::string> producedPointNames =
            InterpolatePointData(inAttr, outAttr, locations, sampleNum);
    CopyCellData(inAttr, outAttr, locations, sampleNum, producedPointNames);
    AddValidPointMask(outAttr, sampleNum);

    // 输出 = 采样点网格的同类型深拷贝（几何/拓扑独立），属性换成采样结果
    auto output = DeepCopyDataObject(probe, outAttr);
    if (output == nullptr) {
        m_Message = "输出构建失败：不支持「采样点网格」的数据类型";
        return false;
    }
    output->SetName(probe->GetName() + "_resample");
    m_Output = output;
    SetOutput(0, output);

    m_Message = "ResampleWithDataSet: samples=" + std::to_string(sampleNum) + ", valid=" + std::to_string(validCount) +
                ", invalid=" + std::to_string(sampleNum - validCount) +
                ", tolerance=" + std::to_string(m_EffectiveTolerance) +
                (m_AutoTolerance ? " (auto = bboxDiag*1e-6)" : " (manual)");
    if (sourceBlockCount > 1) {
        m_Message += ", source blocks=" + std::to_string(sourceBlockCount);
    }
    if (m_SnappedPointCount > 0) {
        m_Message += ", snapped=" + std::to_string(m_SnappedPointCount) +
                     " (radius=" + std::to_string(m_EffectiveSnappingRadius) + ")";
    }
    if (m_UnsupportedQuadraticCellCount > 0) {
        m_Message += ", degraded high-order cells=" + std::to_string(m_UnsupportedQuadraticCellCount);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 均匀网格                                                            */
/* ------------------------------------------------------------------ */
std::vector<std::vector<igIndex>> ResampleWithDataSet::BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                                  const BoundingBox& bbox, igIndex nx, igIndex ny,
                                                                  igIndex nz) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    std::vector<std::vector<igIndex>> grid(static_cast<size_t>(nx) * ny * nz);

    m_TriangleGrid = true;
    m_TriangleBounds.clear();
    for (igIndex id = 0; id < static_cast<igIndex>(mesh->GetNumberOfCells()); ++id) {
        if (mesh->GetCellType(id) != IG_TRIANGLE || mesh->GetCells()->GetCellSize(id) != 3) {
            m_TriangleGrid = false;
            break;
        }
    }
    if (m_TriangleGrid) { m_TriangleBounds.resize(mesh->GetNumberOfCells()); }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    for (igIndex cellId = 0; cellId < static_cast<igIndex>(mesh->GetNumberOfCells()); ++cellId) {
        const int npts = mesh->GetCellPointIds(cellId, ptIds);
        if (npts <= 0) { continue; }

        double minv[3] = {DBL_MAX, DBL_MAX, DBL_MAX};
        double maxv[3] = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
        for (int k = 0; k < npts; ++k) {
            const Point& p = mesh->GetPoint(ptIds[k]);
            for (int j = 0; j < 3; ++j) {
                const double v = static_cast<double>(p[j]);
                minv[j] = std::min(minv[j], v);
                maxv[j] = std::max(maxv[j], v);
            }
        }

        if (m_TriangleGrid) {
            auto& bounds = m_TriangleBounds[static_cast<size_t>(cellId)];
            for (int j = 0; j < 3; ++j) {
                // Two barycentric weights can be negative by kVTKParamDomainTol.
                // Add the normal-distance tolerance as well, so a valid hit just
                // across a voxel boundary cannot be discarded by the index.
                const double padding = m_EffectiveTolerance + 2. * kVTKParamDomainTol * (maxv[j] - minv[j]);
                minv[j] -= padding;
                maxv[j] += padding;
                bounds[j] = minv[j];
                bounds[j + 3] = maxv[j];
            }
        }

        const igIndex ixMin = std::max<igIndex>(0, static_cast<igIndex>((minv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMin = std::max<igIndex>(0, static_cast<igIndex>((minv[1] - bbox.min[1]) / voxelY));
        const igIndex izMin = std::max<igIndex>(0, static_cast<igIndex>((minv[2] - bbox.min[2]) / voxelZ));
        const igIndex ixMax = std::min<igIndex>(nx - 1, static_cast<igIndex>((maxv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMax = std::min<igIndex>(ny - 1, static_cast<igIndex>((maxv[1] - bbox.min[1]) / voxelY));
        const igIndex izMax = std::min<igIndex>(nz - 1, static_cast<igIndex>((maxv[2] - bbox.min[2]) / voxelZ));

        for (igIndex ix = ixMin; ix <= ixMax; ++ix) {
            for (igIndex iy = iyMin; iy <= iyMax; ++iy) {
                for (igIndex iz = izMin; iz <= izMax; ++iz) {
                    grid[static_cast<size_t>(ix) + static_cast<size_t>(iy) * nx + static_cast<size_t>(iz) * nx * ny]
                            .push_back(cellId);
                }
            }
        }
    }

    return grid;
}

/* ------------------------------------------------------------------ */
/* 采样点定位                                                          */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                                  const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                                  double distTol, SampleLocation& out) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    const igIndex ix = std::max<igIndex>(0, std::min<igIndex>(nx - 1, static_cast<igIndex>((p[0] - bbox.min[0]) / voxelX)));
    const igIndex iy = std::max<igIndex>(0, std::min<igIndex>(ny - 1, static_cast<igIndex>((p[1] - bbox.min[1]) / voxelY)));
    const igIndex iz = std::max<igIndex>(0, std::min<igIndex>(nz - 1, static_cast<igIndex>((p[2] - bbox.min[2]) / voxelZ)));

    // For triangles, tolerance-expanded bounds guarantee that every possible
    // hit is in this bucket. Empty space needs no expanding nearest-cell search;
    // explicit snapping remains handled by LocateNearestCell in Execute().
    if (m_TriangleGrid) {
        const auto& candidates = grid[static_cast<size_t>(ix) + static_cast<size_t>(iy) * nx +
                                      static_cast<size_t>(iz) * nx * ny];
        for (const igIndex cellId : candidates) {
            const auto& bounds = m_TriangleBounds[static_cast<size_t>(cellId)];
            bool outside = false;
            for (int j = 0; j < 3; ++j) {
                if (p[j] < bounds[j] || p[j] > bounds[j + 3]) { outside = true; break; }
            }
            if (outside) { continue; }
            std::vector<double> weights;
            if (!ComputeCellWeights(mesh, cellId, p, distTol, weights) || weights.empty()) { continue; }
            igIndex ids[3];
            mesh->GetCellPointIds(cellId, ids);
            out.cellId = cellId;
            out.point = p;
            out.pointIds.assign(ids, ids + 3);
            out.weights = std::move(weights);
            return true;
        }
        return false;
    }

    std::vector<igIndex> candidates;
    const igIndex maxRadius = std::max<igIndex>(nx, std::max<igIndex>(ny, nz));
    for (igIndex r = 0; r <= maxRadius && candidates.empty(); ++r) {
        for (igIndex dx = -r; dx <= r; ++dx) {
            for (igIndex dy = -r; dy <= r; ++dy) {
                for (igIndex dz = -r; dz <= r; ++dz) {
                    if (std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz))) != r) { continue; }
                    const igIndex cx = ix + dx, cy = iy + dy, cz = iz + dz;
                    if (cx < 0 || cx >= nx || cy < 0 || cy >= ny || cz < 0 || cz >= nz) { continue; }
                    const auto& list = grid[static_cast<size_t>(cx) + static_cast<size_t>(cy) * nx +
                                            static_cast<size_t>(cz) * nx * ny];
                    candidates.insert(candidates.end(), list.begin(), list.end());
                }
            }
        }
    }
    if (candidates.empty()) { return false; }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    // 只接受真正包含采样点的单元；不再在容差外寻找最近点
    for (const igIndex cellId : candidates) {
        std::vector<double> weights;
        if (!ComputeCellWeights(mesh, cellId, p, distTol, weights)) { continue; }
        if (weights.empty()) { continue; }

        igIndex ptIds[IGAME_CELL_MAX_SIZE];
        const int npts = mesh->GetCellPointIds(cellId, ptIds);

        out.cellId = cellId;
        out.point = p;
        out.pointIds.assign(ptIds, ptIds + npts);
        out.weights = std::move(weights);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* 单元权重分发                                                        */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                        double distTol, std::vector<double>& weights) {
    weights.clear();

    auto cells = mesh->GetCells();
    if (cells == nullptr) { return false; }
    const IGuint cellSize = cells->GetCellSize(cellId);
    if (cellSize == 0 || cellSize > static_cast<IGuint>(kMaxCellPointNum)) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts <= 0) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    ShapeKind kind = ClassifyCell(cellType, npts);
    if (kind == SHAPE_NONE) {
        // 暂不支持的高次单元：退化处理（并在结果信息中提示）
        if (IsHighOrderCell(cellType)) { ++m_UnsupportedQuadraticCellCount; }
        if (Cell::GetCellDimension(static_cast<igIndex>(cellType)) == 3 && npts >= 4) {
            // 高次体单元：用其线性角点单元的面拓扑做均值坐标（含绕数包含判定）
            if (ComputeDegradedVolumeWeights(mesh, cellId, p, distTol, weights)) { return true; }
            return ComputeMeanValueWeights(mesh, cellId, p, distTol, weights);
        }
        const int corners = LinearCornerCount(cellType, npts);
        if (corners == 0) { return false; }
        return ComputeLinearFaceWeights(mesh, cellId, p, distTol, weights);
    }

    if (kind == SHAPE_LINEAR_TRIANGLE || kind == SHAPE_LINEAR_QUAD || kind == SHAPE_LINEAR_POLYGON) {
        return ComputeLinearFaceWeights(mesh, cellId, p, distTol, weights);
    }
    if (kind == SHAPE_TETRA || kind == SHAPE_MVC_VOLUME) {
        return ComputeMeanValueWeights(mesh, cellId, p, distTol, weights);
    }
    if (IsQuadraticKind(kind)) {
        return ComputeQuadraticWeights(mesh, cellId, p, distTol, weights);
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* 线性面单元：线性插值（重心坐标 / 双线性 / 扇形三角化）               */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::ComputeLinearFaceWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                              double distTol, std::vector<double>& weights) {
    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 3) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    const ShapeKind kind = ClassifyCell(cellType, npts);
    int used = npts;
    if (kind == SHAPE_LINEAR_TRIANGLE || cellType == IG_QUADRATIC_TRIANGLE || cellType == IG_BIQUADRATIC_TRIANGLE ||
        cellType == IG_LAGRANGE_TRIANGLE) {
        used = 3;
    } else if (kind == SHAPE_LINEAR_QUAD || cellType == IG_QUADRATIC_QUAD || cellType == IG_BIQUADRATIC_QUAD ||
               cellType == IG_QUADRATIC_LINEAR_QUAD || cellType == IG_LAGRANGE_QUADRILATERAL) {
        used = 4;
    }
    if (used < 3 || used > npts) { return false; }

    std::vector<Point> pts(static_cast<size_t>(used));
    for (int i = 0; i < used; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    std::vector<double> local;
    if (used == 3) {
        double w[3];
        double planeDist = 0.0;
        if (!TriangleBarycentric(p, pts[0], pts[1], pts[2], w, planeDist)) { return false; }
        if (planeDist > distTol) { return false; }
        if (w[0] < -kVTKParamDomainTol || w[1] < -kVTKParamDomainTol || w[2] < -kVTKParamDomainTol) { return false; }
        local.assign(w, w + 3);
        NormalizeWeights(local);
    } else if (used == 4) {
        double pc[2] = {0.5, 0.5};
        double residual = 0.0;
        if (!NewtonParametric2D(SHAPE_LINEAR_QUAD, pts, p, pc, local, residual)) { return false; }
        if (residual > distTol) { return false; }
        if (pc[0] < -kVTKParamDomainTol || pc[0] > 1.0 + kVTKParamDomainTol || pc[1] < -kVTKParamDomainTol || pc[1] > 1.0 + kVTKParamDomainTol) {
            return false;
        }
        NormalizeWeights(local);
    } else {
        // 多边形：以 0 号点为扇心做扇形三角剖分
        bool found = false;
        for (int k = 1; k + 1 < used && !found; ++k) {
            double w[3];
            double planeDist = 0.0;
            if (!TriangleBarycentric(p, pts[0], pts[static_cast<size_t>(k)], pts[static_cast<size_t>(k + 1)], w,
                                     planeDist)) {
                continue;
            }
            if (planeDist <= distTol && w[0] >= -kVTKParamDomainTol && w[1] >= -kVTKParamDomainTol && w[2] >= -kVTKParamDomainTol) {
                local.assign(static_cast<size_t>(used), 0.0);
                local[0] = w[0];
                local[static_cast<size_t>(k)] = w[1];
                local[static_cast<size_t>(k + 1)] = w[2];
                found = true;
            }
        }
        if (!found) { return false; }
        NormalizeWeights(local);
    }

    // 权重向量长度与单元点数一致（高次单元的角点回退时其余为 0）
    weights.assign(static_cast<size_t>(npts), 0.0);
    for (int i = 0; i < used; ++i) { weights[static_cast<size_t>(i)] = local[static_cast<size_t>(i)]; }
    return true;
}

/* ------------------------------------------------------------------ */
/* 体单元：包含判定                                                    */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::IsPointInVolumeCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                         double distTol) {
    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 4) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    CellFaceList faces;
    CollectCellFaces(cell, ptIds, npts, faces);
    if (faces.empty()) { return false; }
    return ContainsByFaces(pts, faces, p, distTol);
}

/* ------------------------------------------------------------------ */
/* 体单元：均值坐标插值                                                */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::ComputeMeanValueWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                             double distTol, std::vector<double>& weights) {
    // Eight-node hexahedra use their trilinear isoparametric basis, rather
    // than rebuilding faces and evaluating polyhedral mean-value coordinates.
    if (mesh->GetCellType(cellId) == IG_HEXAHEDRON && mesh->GetCells()->GetCellSize(cellId) == 8) {
        igIndex ids[8];
        mesh->GetCellPointIds(cellId, ids);
        double xyz[8][3], lo[3] = {DBL_MAX, DBL_MAX, DBL_MAX};
        double hi[3] = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
        for (int i = 0; i < 8; ++i) {
            const auto& q = mesh->GetPoint(ids[i]);
            for (int j = 0; j < 3; ++j) {
                xyz[i][j] = q[j];
                lo[j] = std::min(lo[j], xyz[i][j]);
                hi[j] = std::max(hi[j], xyz[i][j]);
            }
        }
        double scale2 = 0.;
        for (int j = 0; j < 3; ++j) { scale2 += (hi[j] - lo[j]) * (hi[j] - lo[j]); }
        const double scale = std::sqrt(scale2);
        const double padding = std::max(distTol, 4. * kVTKParamDomainTol * scale);
        for (int j = 0; j < 3; ++j) {
            if (p[j] < lo[j] - padding || p[j] > hi[j] + padding) { return false; }
        }
        double pc[3] = {0., 0., 0.}, sf[8];
        for (int iter = 0; iter < 32; ++iter) {
            double residual[3] = {-double(p[0]), -double(p[1]), -double(p[2])};
            double jac[3][3] = {};
            for (int i = 0; i < 8; ++i) {
                const auto& sign = kQuadHexSign[i];
                const double a = 1. + sign[0] * pc[0], b = 1. + sign[1] * pc[1], c = 1. + sign[2] * pc[2];
                sf[i] = a * b * c * 0.125;
                const double deriv[3] = {sign[0] * b * c * .125, sign[1] * a * c * .125,
                                         sign[2] * a * b * .125};
                for (int j = 0; j < 3; ++j) {
                    residual[j] += sf[i] * xyz[i][j];
                    for (int k = 0; k < 3; ++k) { jac[k][j] += deriv[k] * xyz[i][j]; }
                }
            }
            const double det = Det3(jac[0], jac[1], jac[2]);
            if (!std::isfinite(det) || std::abs(det) < 1e-30) { return false; }
            const double error = std::sqrt(residual[0]*residual[0] + residual[1]*residual[1] + residual[2]*residual[2]);
            if (error <= std::max(1e-12, scale * 1e-10)) {
                // Bound extrapolation by the requested physical tolerance.
                const double tol = 2. * distTol / std::max(scale, 1e-12);
                for (double v : pc) { if (!std::isfinite(v) || std::abs(v) > 1. + tol) { return false; } }
                weights.assign(sf, sf + 8);
                return true;
            }
            const double step[3] = {Det3(residual, jac[1], jac[2]) / det,
                                    Det3(jac[0], residual, jac[2]) / det,
                                    Det3(jac[0], jac[1], residual) / det};
            for (int j = 0; j < 3; ++j) {
                pc[j] -= step[j];
                if (!std::isfinite(pc[j]) || std::abs(pc[j]) > 1e3) { return false; }
            }
        }
        return false;
    }
    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 4 || npts > kMaxCellPointNum) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    const IGenum cellType = mesh->GetCellType(cellId);

    // 四面体：解析重心坐标（包含判定与插值权重，数学上等同均值坐标）
    if (cellType == IG_TETRA && npts == 4) {
        const Point& a = pts[0];
        const Point& b = pts[1];
        const Point& c = pts[2];
        const Point& d = pts[3];
        double p10[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        double p20[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        double p30[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
        double rhs[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};

        const double det = Det3(p10, p20, p30);
        if (std::fabs(det) < 1e-30) { return false; }

        double w[4];
        w[1] = Det3(rhs, p20, p30) / det;
        w[2] = Det3(p10, rhs, p30) / det;
        w[3] = Det3(p10, p20, rhs) / det;
        w[0] = 1.0 - w[1] - w[2] - w[3];

        const double scale = std::max({1e-12, static_cast<double>((a - b).length()),
                                       static_cast<double>((a - c).length()), static_cast<double>((a - d).length())});
        // VTK 的接受域：重心坐标允许 ±1e-3 越界（pcoords ∈ [-0.001, 1.001]）
        const double baryTol = std::max(kVTKParamDomainTol, distTol / scale);
        if (w[0] < -baryTol || w[1] < -baryTol || w[2] < -baryTol || w[3] < -baryTol) { return false; }

        weights.assign(static_cast<size_t>(npts), 0.0);
        for (int i = 0; i < 4; ++i) { weights[static_cast<size_t>(i)] = w[i]; }
        return NormalizeKeepSign(weights);
    }

    // 其他体单元：包含判定（贴面 / 绕数）+ 逐面均值坐标
    CellFaceList faces;
    CollectCellFaces(cell, ptIds, npts, faces);
    if (faces.empty()) { return false; }
    if (!ContainsByFaces(pts, faces, p, distTol)) { return false; }

    std::vector<double> w;
    if (!MeanValueWeightsFromFaces(pts, faces, p, distTol, w)) { return false; }

    weights = std::move(w);
    return true;
}

/* ------------------------------------------------------------------ */
/* 高次体单元退化：用线性角点单元的面表做均值坐标                      */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::ComputeDegradedVolumeWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                                 double distTol, std::vector<double>& weights) {
    weights.clear();

    const IGenum cellType = mesh->GetCellType(cellId);
    Cell::Pointer linear;
    const int corners = LinearCornerVolumeCell(cellType, linear);
    if (corners == 0 || linear == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < corners) { return false; }

    // 用局部下标填充角点单元：其面表给出的点 id 即 pts 下标
    linear->Reset();
    std::vector<Point> pts(static_cast<size_t>(corners));
    for (int i = 0; i < corners; ++i) {
        pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]);
        linear->m_PointIds->AddId(i);
        linear->m_Points->AddPoint(pts[static_cast<size_t>(i)]);
    }

    CellFaceList faces;
    CollectCellFaces(linear.get(), nullptr, corners, faces);
    if (faces.empty()) { return false; }
    if (!ContainsByFaces(pts, faces, p, distTol)) { return false; }

    std::vector<double> w;
    if (!MeanValueWeightsFromFaces(pts, faces, p, distTol, w)) { return false; }

    // 权重向量长度与单元点数一致（角点之外为 0）
    weights.assign(static_cast<size_t>(npts), 0.0);
    for (int i = 0; i < corners; ++i) { weights[static_cast<size_t>(i)] = w[static_cast<size_t>(i)]; }
    return true;
}

/* ------------------------------------------------------------------ */
/* 二次单元：二次形函数插值                                            */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::ComputeQuadraticWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                             double distTol, std::vector<double>& weights) {
    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts <= 0) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    const ShapeKind kind = ClassifyCell(cellType, npts);
    const int need = ShapePointCount(kind);
    if (need == 0 || npts != need) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    if (IsQuadraticFaceKind(kind)) {
        double pc[2] = {0.5, 0.5};
        double residual = 0.0;
        std::vector<double> w;
        if (!NewtonParametric2D(kind, pts, p, pc, w, residual)) { return false; }
        const double pc3[3] = {pc[0], pc[1], 0.0};
        if (!ShapeDomainOk(kind, pc3, ParamDomainTol(kind))) { return false; }
        if (residual > distTol) { return false; }
        NormalizeWeights(w);
        weights = std::move(w);
        return true;
    }

    double pc[3] = {0.0, 0.0, 0.0};
    double residual = 0.0;
    std::vector<double> w;
    if (!NewtonQuadratic3D(kind, pts, p, pc, w, residual)) { return false; }
    if (!ShapeDomainOk(kind, pc, ParamDomainTol(kind))) { return false; }
    if (residual > distTol) { return false; }
    NormalizeWeights(w);
    weights = std::move(w);
    return true;
}

/* ------------------------------------------------------------------ */
/* 点到单元边界的距离                                                  */
/* ------------------------------------------------------------------ */
double ResampleWithDataSet::DistanceToCellBoundary(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                              Point& closest) {
    double best = DBL_MAX;
    closest = p;

    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return best; }

    const int nfaces = cell->GetNumberOfFaces();
    for (int f = 0; f < nfaces; ++f) {
        auto face = cell->GetFace(f);
        if (face == nullptr) { continue; }
        const int fn = face->GetNumberOfPoints();
        if (fn < 3) {
            for (int k = 0; k + 1 < fn; ++k) {
                Point c(0.f, 0.f, 0.f);
                double d2 = 0.0;
                ClosestPointOnSegment(p, face->GetPoint(k), face->GetPoint(k + 1), c, d2);
                if (d2 < best) {
                    best = d2;
                    closest = c;
                }
            }
            continue;
        }
        for (int k = 1; k + 1 < fn; ++k) {
            Point c(0.f, 0.f, 0.f);
            double d2 = 0.0;
            ClosestPointOnTriangle(p, face->GetPoint(0), face->GetPoint(k), face->GetPoint(k + 1), c, d2);
            if (d2 < best) {
                best = d2;
                closest = c;
            }
        }
    }

    if (best == DBL_MAX) {
        const int npts = cell->GetNumberOfPoints();
        for (int i = 0; i < npts; ++i) {
            const double d2 = static_cast<double>((cell->GetPoint(i) - p).squaredLength());
            if (d2 < best) {
                best = d2;
                closest = cell->GetPoint(i);
            }
        }
    }
    return (best == DBL_MAX) ? DBL_MAX : std::sqrt(best);
}

/* ------------------------------------------------------------------ */
/* 属性插值                                                            */
/* ------------------------------------------------------------------ */
std::vector<std::string> ResampleWithDataSet::InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                          const std::vector<SampleLocation>& locations, int sampleNum) {
    std::vector<std::string> producedPointNames;
    if (inSet == nullptr || outSet == nullptr) { return producedPointNames; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return producedPointNames; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }

        auto src = attr.pointer;
        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // 保留数组数据类型（与 VTK vtkProbeFilter 一致：整型 / 字符型数组同样用原类型承载插值
        // 结果，由数组自身截断），不再统一转成 float
        IGenum outArrayType = src->GetArrayType();

        auto dst = CreateArrayObject(outArrayType);
        if (dst == nullptr) { dst = CreateArrayObject(IG_FloatArray); } // 兜底：未知类型退化为 float
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0); // 无效采样点填 0
            const SampleLocation& loc = locations[static_cast<size_t>(j)];
            const size_t npts = loc.pointIds.size();
            if (loc.cellId >= 0 && npts > 0 && loc.weights.size() == npts) {
                for (size_t k = 0; k < npts; ++k) {
                    const double w = loc.weights[k];
                    if (w == 0.0) { continue; }
                    const igIndex ptId = loc.pointIds[k];
                    for (int c = 0; c < dim; ++c) {
                        values[static_cast<size_t>(c)] += w * src->GetElementValue(ptId, c);
                    }
                }
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        producedPointNames.push_back(src->GetName());
        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
    return producedPointNames;
}

/**
 * 输入 0 的单元数据 → 输出点（与 vtkProbeFilter 的 InputCellArrays 一致）：
 * 每个采样点取「命中单元」的整个 tuple 拷到输出点上，不做单元内插值；
 * 无效采样点填 0；若同名数组已由源点数据产出（pointNames），则点数据优先、跳过单元数组
 * （对应 VTK 源码 InitializeForProbing 里的 !outPD->GetArray(inArray->GetName()) 判断）。
 */
void ResampleWithDataSet::CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                  const std::vector<SampleLocation>& locations, int sampleNum,
                                  const std::vector<std::string>& pointNames) {
    if (inSet == nullptr || outSet == nullptr) { return; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_CELL) { continue; }

        auto src = attr.pointer;

        // 同名数组若已由源点数据产出，则点数据优先（与 vtkProbeFilter 一致：
        // “If an array of the same name exists both in source's point and cell data,
        //   only the one from the point data is probed.”）
        if (std::find(pointNames.begin(), pointNames.end(), src->GetName()) != pointNames.end()) { continue; }

        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // Cell Data 是复制关系，完整保留原数组数据类型与分量数
        auto dst = CreateArrayObject(src->GetArrayType());
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        const IGsize cellNum = src->GetNumberOfElements();
        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0); // 无效采样点填 0
            const igIndex cellId = locations[static_cast<size_t>(j)].cellId;
            if (cellId >= 0 && static_cast<IGsize>(cellId) < cellNum) {
                src->GetElement(static_cast<IGsize>(cellId), values.data());
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        // 被采样网格的单元数据拷贝到采样点上，因此附着类型是 IG_POINT
        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
}

/** 有效采样点 id 列表（等价 VTK vtkProbeFilter::GetValidPoints()） */
std::vector<igIndex> ResampleWithDataSet::GetValidPoints() const {
    std::vector<igIndex> ids;
    ids.reserve(m_SampleValidMask.size());
    for (size_t i = 0; i < m_SampleValidMask.size(); ++i) {
        if (m_SampleValidMask[i] != 0) { ids.push_back(static_cast<igIndex>(i)); }
    }
    return ids;
}

void ResampleWithDataSet::AddValidPointMask(AttributeSet::Pointer outSet, int sampleNum) {
    if (outSet == nullptr) { return; }

    auto mask = UnsignedCharArray::New();
    mask->SetName(m_ValidPointMaskArrayName);
    mask->SetDimension(1);
    mask->Resize(static_cast<IGsize>(sampleNum));
    for (int i = 0; i < sampleNum; ++i) {
        const unsigned char v = (static_cast<size_t>(i) < m_SampleValidMask.size())
                                        ? m_SampleValidMask[static_cast<size_t>(i)]
                                        : static_cast<unsigned char>(0);
        mask->SetValue(static_cast<IGsize>(i), static_cast<double>(v));
    }

    // 掩膜附着在采样点上：1 = 可插值，0 = 无效。默认名 validpointmask（与 ResampleToLine 一致），可用 SetValidPointMaskArrayName("vtkValidPointMask") 对齐 VTK 默认命名。
    // 掩膜数组名 = m_ValidPointMaskArrayName
    outSet->AddAttribute(IG_SCALAR, IG_POINT, mask);
}

/* ------------------------------------------------------------------ */
/* 吸附搜索                                                            */
/* ------------------------------------------------------------------ */
bool ResampleWithDataSet::LocateNearestCell(const UnstructuredMesh::Pointer& mesh, const Point& p,
                                           const BoundingBox& bbox, const std::vector<std::vector<igIndex>>& grid,
                                           igIndex nx, igIndex ny, igIndex nz, double radius, igIndex& cellId,
                                           Point& closest) {
    cellId = -1;
    closest = p;
    if (radius <= 0.0) { return false; }

    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    const igIndex ix =
            std::max<igIndex>(0, std::min<igIndex>(nx - 1, static_cast<igIndex>((p[0] - bbox.min[0]) / voxelX)));
    const igIndex iy =
            std::max<igIndex>(0, std::min<igIndex>(ny - 1, static_cast<igIndex>((p[1] - bbox.min[1]) / voxelY)));
    const igIndex iz =
            std::max<igIndex>(0, std::min<igIndex>(nz - 1, static_cast<igIndex>((p[2] - bbox.min[2]) / voxelZ)));

    // 半径覆盖的环数：吸附半径通常只有若干单元大小，扫描代价可控
    const double minVoxel = std::max(1e-12, std::min(voxelX, std::min(voxelY, voxelZ)));
    igIndex maxRing = static_cast<igIndex>(std::ceil(radius / minVoxel)) + 1;
    maxRing = std::min(maxRing, std::max(nx, std::max(ny, nz)));

    std::vector<igIndex> candidates;
    for (igIndex r = 0; r <= maxRing; ++r) {
        for (igIndex dx = -r; dx <= r; ++dx) {
            for (igIndex dy = -r; dy <= r; ++dy) {
                for (igIndex dz = -r; dz <= r; ++dz) {
                    if (std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz))) != r) { continue; }
                    const igIndex cx = ix + dx, cy = iy + dy, cz = iz + dz;
                    if (cx < 0 || cx >= nx || cy < 0 || cy >= ny || cz < 0 || cz >= nz) { continue; }
                    const auto& list = grid[static_cast<size_t>(cx) + static_cast<size_t>(cy) * nx +
                                            static_cast<size_t>(cz) * nx * ny];
                    candidates.insert(candidates.end(), list.begin(), list.end());
                }
            }
        }
    }
    if (candidates.empty()) { return false; }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    double best = DBL_MAX;
    for (const igIndex id : candidates) {
        Point c;
        const double d = DistanceToCellBoundary(mesh, id, p, c);
        if (d < best) {
            best = d;
            closest = c;
            cellId = id;
        }
    }

    if (cellId < 0 || best > radius) {
        cellId = -1;
        return false;
    }
    return true;
}

IGAME_NAMESPACE_END
