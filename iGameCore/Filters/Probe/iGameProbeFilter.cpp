// ============================================================================
// ProbeFilter — 见 iGameProbeFilter.h
//
// 算法说明（v3）:
//   1. 单元定位：遍历模型单元。面网格走面单元、体网格走体单元
//      （v1 仅三角形 / 四面体，其余类型未命中）；命中后按重心坐标对全部
//      点属性（标量 / 矢量等）做线性插值。
//   2. 结果原地写入查询点集：复用/新建同名点属性数组，另加
//      ValidPointMask（找到单元 = 1，未找到 = 0，未找到时插值属性填 0）。
//   3. 输出 0 与输入 1 为同一个对象，重复执行不新建点集。
// ============================================================================
#include "iGameProbeFilter.h"

#include "iGameProbeLocator.h"

#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

IGAME_NAMESPACE_BEGIN

ProbeFilter::ProbeFilter() {
    SetNumberOfInputs(2);
    SetNumberOfOutputs(1);
}

void ProbeFilter::GenerateSpherePoints(PointSet::Pointer points, const Point& center,
                                       float radius, int count, unsigned seed) {
    // radius < 0 非法；radius == 0 时按退化球处理：全部点落在球心。
    if (points.IsNull() || count <= 0 || radius < 0.0f) return;
    auto pts = points->GetPoints();
    if (pts.IsNull()) return;

    std::mt19937 gen;
    if (seed == 0) {
        std::random_device rd;
        gen.seed(rd());
    } else {
        gen.seed(seed);
    }
    std::uniform_real_distribution<double> u01(0.0, 1.0);
    constexpr double kTwoPi = 6.283185307179586476925286766559;

    // 球体体积均匀采样：r = R * u^(1/3)，θ 均匀，φ = acos(1 - 2u)
    pts->Reset();
    pts->Reserve(static_cast<IGsize>(count));
    for (int i = 0; i < count; ++i) {
        const double r = static_cast<double>(radius) * std::cbrt(u01(gen));
        const double theta = kTwoPi * u01(gen);
        const double phi = std::acos(1.0 - 2.0 * u01(gen));
        const double sp = std::sin(phi);
        pts->AddPoint(static_cast<float>(center[0] + r * sp * std::cos(theta)),
                      static_cast<float>(center[1] + r * sp * std::sin(theta)),
                      static_cast<float>(center[2] + r * std::cos(phi)));
    }
    points->Modified();
}

bool ProbeFilter::Execute() {
    // ================= 取输入并校验 =================
    auto in = GetInput(0);
    auto query = DynamicCast<PointSet>(GetInput(1));
    if (in.IsNull() || query.IsNull()) return false;

    auto srcPoints = in->GetPoints();
    auto queryPoints = query->GetPoints();
    if (srcPoints.IsNull() || srcPoints->GetNumberOfPoints() == 0) return false;
    if (queryPoints.IsNull() || queryPoints->GetNumberOfPoints() == 0) return false;

    const IGsize numQuery = queryPoints->GetNumberOfPoints();

    // ================= 容差与包围盒 =================
    const double tolerance = HasAutoTolerance() ? kProbeDefaultTolerance : m_Tolerance;
    double bboxDiag = in->GetBoundingBox().diag();
    if (bboxDiag <= kProbeNumericalEps) bboxDiag = 1.0;

    // ================= 输出属性准备（原地写入查询点集）=================
    AttributeSet* inAttrs = in->GetAttributeSet();
    AttributeSet* outAttrs = query->GetAttributeSet();
    if (inAttrs == nullptr || outAttrs == nullptr) return false;

    struct OutAttribute {
        ArrayObject::Pointer inArray;
        FloatArray::Pointer outArray;
        int dimension;
    };
    std::vector<OutAttribute> outAttributes;

    {
        auto allAttributes = inAttrs->GetAllPointAttributes();
        for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
            auto& attr = allAttributes->GetElement(i);
            if (attr.isDeleted || attr.pointer.IsNull()) continue;
            if (attr.attachmentType != IG_POINT) continue;  // 只插值点属性

            const std::string name = attr.pointer->GetName();
            const int dimension = std::max(1, attr.pointer->GetDimension());

            FloatArray::Pointer outArray = nullptr;
            const int existingIndex = outAttrs->GetAttributeIndex(name);
            if (existingIndex >= 0) {
                auto existing = DynamicCast<FloatArray>(
                    outAttrs->GetAttribute(existingIndex).pointer);
                if (!existing.IsNull()) {
                    outArray = existing;
                } else {
                    outAttrs->DeleteAttribute(existingIndex);  // 类型不符，删除重建
                }
            }
            if (outArray.IsNull()) {
                outArray = FloatArray::New();
                outArray->SetName(name);
                outArray->SetDimension(dimension);
                outArray->Resize(numQuery);
                outAttrs->AddAttribute(attr.type, IG_POINT, outArray,
                                       attr.GetDataRange());
            } else {
                if (outArray->GetDimension() != dimension) {
                    outArray->SetDimension(dimension);
                }
                outArray->Resize(numQuery);
            }
            outAttributes.push_back({attr.pointer, outArray, dimension});
        }
    }

    // ValidPointMask：找到单元 = 1，未找到 = 0
    IntArray::Pointer mask = nullptr;
    {
        const int maskIndex = outAttrs->GetAttributeIndex("ValidPointMask");
        if (maskIndex >= 0) {
            auto existing = DynamicCast<IntArray>(
                outAttrs->GetAttribute(maskIndex).pointer);
            if (!existing.IsNull()) {
                mask = existing;
            } else {
                outAttrs->DeleteAttribute(maskIndex);
            }
        }
        if (mask.IsNull()) {
            mask = IntArray::New();
            mask->SetName("ValidPointMask");
            mask->SetDimension(1);
            mask->Resize(numQuery);
            outAttrs->AddAttribute(IG_SCALAR, IG_POINT, mask);
        } else {
            mask->Resize(numQuery);
        }
    }

    // ================= 单元访问 =================
    auto um = DynamicCast<UnstructuredMesh>(in);
    auto vm = DynamicCast<VolumeMesh>(in);
    auto sm = DynamicCast<SurfaceMesh>(in);

    CellArray::Pointer cellArray = in->GetCellArray();
    const IGsize numCells = (cellArray.IsNull()) ? 0 : cellArray->GetNumberOfCells();

    // 对模型全部点属性做一次带权组合：value(q) = Σ w_i * value(pointId_i)
    const auto interpolateWith = [&outAttributes](IGsize queryId, Cell* cell,
                                                  const ProbeCellHit& hit) {
        double rawValues[IGAME_CELL_MAX_SIZE] = {};
        double interpolated[IGAME_CELL_MAX_SIZE] = {};
        for (auto& out : outAttributes) {
            const int dimension = out.dimension;
            for (int c = 0; c < dimension; ++c) interpolated[c] = 0.0;
            for (int v = 0; v < hit.numVertices; ++v) {
                out.inArray->GetElement(cell->GetPointId(hit.localVertIds[v]),
                                        rawValues);
                for (int c = 0; c < dimension; ++c) {
                    interpolated[c] += hit.weights[v] * rawValues[c];
                }
            }
            out.outArray->SetElement(queryId, interpolated);
        }
    };

    // ================= 逐查询点点定位 + 插值 =================
    for (IGsize qi = 0; qi < numQuery; ++qi) {
        const Point& q = queryPoints->GetPoint(qi);

        bool found = false;
        if (numCells > 0) {
            for (IGsize cellId = 0; cellId < numCells; ++cellId) {
                Cell* cell = nullptr;
                if (vm) {
                    cell = vm->GetVolume(cellId);
                } else if (sm) {
                    cell = sm->GetFace(cellId);
                } else if (um) {
                    cell = um->GetCell(cellId);
                }
                if (cell == nullptr) continue;

                ProbeCellHit hit;
                if (EvaluatePosition(cell, q, tolerance, bboxDiag, hit)) {
                    // 容差可能导致同时命中多个单元：取第一个找到的单元
                    interpolateWith(qi, cell, hit);
                    found = true;
                    break;
                }
            }
        }

        mask->SetValue(qi, found ? 1 : 0);
        if (!found) {
            double zeros[IGAME_CELL_MAX_SIZE] = {};
            for (auto& out : outAttributes) {
                out.outArray->SetElement(qi, zeros);
            }
        }
    }

    query->Modified();

    // 输出 0 与输入 1 为同一个对象（原地更新，不新建点集）
    SetOutput(0, query);
    return true;
}

IGAME_NAMESPACE_END
