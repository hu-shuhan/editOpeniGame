#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"

#include "Interpolation/iGamePointKdTree.h"

#include "iGameAttributeSet.h"
#include "iGameBoundingBox.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameType.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// 输出格点数上限（防止误设超大分辨率导致内存爆炸）。
constexpr IGsize kMaxGridPoints = 100000000; // 1e8

struct SourceArray {
    ArrayObject::Pointer array;
    IGenum type{IG_SCALAR};
    int dim{1};
    std::string name;
};

} // namespace

PointVolumeInterpolatorFilter::PointVolumeInterpolatorFilter() {
    SetNumberOfInputs(1); // input[0]: 数据点云/数据集（只用其点与点属性）
    SetNumberOfOutputs(1);
}

void PointVolumeInterpolatorFilter::SetSamplingBounds(const double bounds[6]) {
    for (int i = 0; i < 6; ++i) { m_SamplingBounds[i] = bounds[i]; }
}

void PointVolumeInterpolatorFilter::SetSamplingBounds(double x0, double x1,
                                                      double y0, double y1,
                                                      double z0, double z1) {
    m_SamplingBounds[0] = x0;
    m_SamplingBounds[1] = x1;
    m_SamplingBounds[2] = y0;
    m_SamplingBounds[3] = y1;
    m_SamplingBounds[4] = z0;
    m_SamplingBounds[5] = z1;
}

void PointVolumeInterpolatorFilter::GetSamplingBounds(double bounds[6]) const {
    for (int i = 0; i < 6; ++i) { bounds[i] = m_SamplingBounds[i]; }
}

void PointVolumeInterpolatorFilter::SetResolution(int i, int j, int k) {
    m_Resolution[0] = i;
    m_Resolution[1] = j;
    m_Resolution[2] = k;
}

void PointVolumeInterpolatorFilter::SetResolution(int dims[3]) {
    if (dims == nullptr) return;
    m_Resolution[0] = dims[0];
    m_Resolution[1] = dims[1];
    m_Resolution[2] = dims[2];
}

void PointVolumeInterpolatorFilter::GetResolution(int dims[3]) const {
    if (dims == nullptr) return;
    dims[0] = m_Resolution[0];
    dims[1] = m_Resolution[1];
    dims[2] = m_Resolution[2];
}

bool PointVolumeInterpolatorFilter::Execute() {
    m_Message.clear();

    // ==================== 输入 ====================
    auto input = DynamicCast<PointSet>(GetInput(0));
    if (input == nullptr) {
        m_Message = "input is not a point set";
        igError("PointVolumeInterpolatorFilter: input is null or not a PointSet.");
        return false;
    }
    Points* pts = input->GetPoints();
    const IGsize numPoints = (pts != nullptr) ? pts->GetNumberOfPoints() : 0;
    if (numPoints == 0) {
        m_Message = "input has no points";
        igError("PointVolumeInterpolatorFilter: input has no points.");
        return false;
    }

    // ==================== 采样区域 ====================
    double bounds[6];
    if (m_UseInputBounds) {
        const BoundingBox& box = input->GetBoundingBox();
        bounds[0] = box.min[0];
        bounds[1] = box.max[0];
        bounds[2] = box.min[1];
        bounds[3] = box.max[1];
        bounds[4] = box.min[2];
        bounds[5] = box.max[2];
    } else {
        for (int i = 0; i < 6; ++i) { bounds[i] = m_SamplingBounds[i]; }
    }

    int dims[3] = {
            std::max(1, m_Resolution[0]),
            std::max(1, m_Resolution[1]),
            std::max(1, m_Resolution[2])};
    const double origin[3] = {bounds[0], bounds[2], bounds[4]};
    double spacing[3];
    for (int i = 0; i < 3; ++i) {
        spacing[i] = (dims[i] == 1)
                         ? 0.0
                         : (bounds[2 * i + 1] - bounds[2 * i]) /
                                   static_cast<double>(dims[i] - 1);
    }
    const IGsize numberOfGridPoints =
            static_cast<IGsize>(dims[0]) * dims[1] * dims[2];
    if (numberOfGridPoints > kMaxGridPoints) {
        m_Message = "resolution too large (grid points exceed limit)";
        igError("PointVolumeInterpolatorFilter: resolution too large ({} grid points).",
                numberOfGridPoints);
        return false;
    }

    // ==================== 输入点属性 ====================
    std::vector<SourceArray> sourceArrays;
    if (AttributeSet* attrs = input->GetAttributeSet()) {
        auto all = attrs->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.attachmentType != IG_POINT) continue;
            if (a.pointer->GetNumberOfElements() != numPoints) continue;
            if (!m_InterpolateArrayNames.empty()) {
                const std::string& arrayName = a.pointer->GetName();
                bool selected = false;
                for (const auto& n : m_InterpolateArrayNames) {
                    if (n == arrayName) { selected = true; break; }
                }
                if (!selected) continue;
            }
            sourceArrays.push_back({a.pointer, a.type,
                                    a.pointer->GetDimension(),
                                    a.pointer->GetName()});
        }
    }
    if (sourceArrays.empty()) {
        m_Message = "input has no point attribute to interpolate";
        igError("PointVolumeInterpolatorFilter: no point attribute found.");
        return false;
    }

    // ==================== 邻域检索 ====================
    PointKdTree kdTree;
    kdTree.Build(pts);

    // ==================== 输出网格与数组 ====================
    StructuredMesh::Pointer output = StructuredMesh::New();
    output->SetName(input->GetName() + "_point_volume");
    igIndex idims[3] = {static_cast<igIndex>(dims[0]),
                        static_cast<igIndex>(dims[1]),
                        static_cast<igIndex>(dims[2])};
    output->SetDimensionSize(idims);

    Points::Pointer gridPoints = Points::New();
    gridPoints->Reserve(numberOfGridPoints);

    std::vector<FloatArray::Pointer> outArrays(sourceArrays.size());
    for (size_t s = 0; s < sourceArrays.size(); ++s) {
        auto arr = FloatArray::New();
        arr->SetName(sourceArrays[s].name);
        arr->SetDimension(sourceArrays[s].dim);
        arr->Resize(numberOfGridPoints);
        outArrays[s] = arr;
    }

    CharArray::Pointer mask = CharArray::New();
    mask->SetName(ValidPointsMaskName);
    mask->SetDimension(1);
    mask->Resize(numberOfGridPoints);
    for (IGsize p = 0; p < numberOfGridPoints; ++p) { mask->ValueAt(p) = 0; }

    // ==================== 逐格点插值 ====================
    std::vector<igIndex> ids;
    std::vector<double> distSq;
    std::vector<double> weights;
    std::vector<double> vals;

    const int kNeighbors = std::max(1, m_NumberOfPoints);

    for (int k = 0; k < dims[2]; ++k) {
        for (int j = 0; j < dims[1]; ++j) {
            for (int i = 0; i < dims[0]; ++i) {
                const float x = static_cast<float>(origin[0] + i * spacing[0]);
                const float y = static_cast<float>(origin[1] + j * spacing[1]);
                const float z = static_cast<float>(origin[2] + k * spacing[2]);
                gridPoints->AddPoint(x, y, z);
                const IGsize ptId =
                        static_cast<IGsize>(i) +
                        static_cast<IGsize>(dims[0]) *
                                (static_cast<IGsize>(j) +
                                 static_cast<IGsize>(dims[1]) * k);

                const Point q(x, y, z);
                ids.clear();
                distSq.clear();

                if (m_KernelType == PointKernelType::Voronoi) {
                    kdTree.QueryKNearest(q, 1, ids, distSq);
                } else if (m_KernelFootprint == PointKernelFootprint::Radius) {
                    kdTree.QueryRadius(q, m_Radius, ids, distSq);
                    if (ids.empty() &&
                        m_NullPointsStrategy == PointNullPointsStrategy::ClosestPoint) {
                        kdTree.QueryKNearest(q, 1, ids, distSq);
                    }
                } else {
                    kdTree.QueryKNearest(q, kNeighbors, ids, distSq);
                }

                // 空邻域：MaskPoints / NullValue 都把输出值置为 NullValue（掩码保持 0）
                if (ids.empty()) {
                    for (size_t s = 0; s < outArrays.size(); ++s) {
                        const int dim = sourceArrays[s].dim;
                        // 必须按 tuple 写(SetElement)：SetValue(pos,v) 的 pos 是标量值索引，
                        // 对多分量数组会把 0 写到别的 tuple 上，冲掉已写好的命中点数据。
                        std::vector<double> nullVals(static_cast<size_t>(dim), m_NullValue);
                        outArrays[s]->SetElement(ptId, nullVals.data());
                    }
                    continue;
                }

                weights.assign(ids.size(), 0.0);

                if (m_KernelType == PointKernelType::Voronoi) {
                    // Voronoi：最近点（QueryKNearest 已按距离升序）
                    weights[0] = 1.0;
                } else {
                    // 只有 Shepard 在 d=0 时有明确的极限（该点独占权重）；
                    // 其余核按各自公式自然计算（对齐 VTK，不做全局"精确命中短路"）。
                    bool exactHit = false;
                    size_t exactIndex = 0;
                    if (m_KernelType == PointKernelType::Shepard) {
                        for (size_t n = 0; n < ids.size(); ++n) {
                            if (distSq[n] <= PointKernelExactHitToleranceSq) {
                                exactHit = true;
                                exactIndex = n;
                                break;
                            }
                        }
                    }
                    if (exactHit) {
                        std::fill(weights.begin(), weights.end(), 0.0);
                        weights[exactIndex] = 1.0;
                    } else {
                        for (size_t n = 0; n < ids.size(); ++n) {
                            double w = 1.0; // Linear 等权
                            if (m_KernelType == PointKernelType::Gaussian) {
                                w = PointGaussianWeight(distSq[n], m_Radius, m_Sharpness);
                            } else if (m_KernelType == PointKernelType::Shepard) {
                                w = PointShepardWeight(distSq[n], m_PowerParameter);
                            }
                            weights[n] = w;
                        }
                    }
                    double weightSum = 0.0;
                    for (size_t n = 0; n < weights.size(); ++n) { weightSum += weights[n]; }
                    if (weightSum <= 0.0) {
                        std::fill(weights.begin(), weights.end(), 0.0);
                        weights[0] = 1.0;
                    }
                }

                double weightSum = 0.0;
                for (size_t n = 0; n < weights.size(); ++n) { weightSum += weights[n]; }

                for (size_t s = 0; s < sourceArrays.size(); ++s) {
                    const int dim = sourceArrays[s].dim;
                    vals.assign(static_cast<size_t>(dim), 0.0);
                    for (size_t n = 0; n < ids.size(); ++n) {
                        const double w = weights[n];
                        if (w == 0.0) continue;
                        for (int c = 0; c < dim; ++c) {
                            vals[static_cast<size_t>(c)] +=
                                    w * sourceArrays[s].array->GetElementValue(ids[n], c);
                        }
                    }
                    if (weightSum > 0.0) {
                        for (int c = 0; c < dim; ++c) {
                            vals[static_cast<size_t>(c)] /= weightSum;
                        }
                    }
                    outArrays[s]->SetElement(ptId, vals.data());
                }
                mask->ValueAt(ptId) = 1;
            }
        }
    }

    output->SetPoints(gridPoints);
    output->GenStructuredCellConnectivities();

    AttributeSet* outAttrs = output->GetAttributeSet();
    for (size_t s = 0; s < sourceArrays.size(); ++s) {
        outAttrs->AddAttribute(sourceArrays[s].type, IG_POINT, outArrays[s]);
    }
    outAttrs->AddAttribute(IG_SCALAR, IG_POINT, mask);

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
