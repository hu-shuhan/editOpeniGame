#include "iGamePointPlaneInterpolatorFilter.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "Interpolation/iGamePointKdTree.h"
#include "Interpolation/iGamePointInterpolationKernel.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace
{
constexpr double ExactHitTolerance = 1e-12; // 与 PointVolume 保持一致的精确命中容差
}

PointPlaneInterpolatorFilter::PointPlaneInterpolatorFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool PointPlaneInterpolatorFilter::Execute() {
    auto input = GetInput(0);
    if (!input || !input->GetPoints() || input->GetPoints()->GetNumberOfPoints() == 0) return false;
    if (m_Resolution < 2) return false;

    // 【新增】拦截非正半径
    if (m_Radius <= 0.0) return false;

    // 【新增】拦截零向量法向（防止后续求导和归一化时出现 NaN）
    if (m_Normal[0] * m_Normal[0] + m_Normal[1] * m_Normal[1] + m_Normal[2] * m_Normal[2] < 1e-12) return false;

    auto sourcePoints = input->GetPoints();
    const IGsize sourcePointCount = sourcePoints->GetNumberOfPoints();

    // ================= 1. 建立 KD-Tree =================
    PointKdTree kdTree;
    kdTree.Build(sourcePoints);

    // ================= 2. 构建平面网格 (精准对齐包围盒) =================
    // 计算平面的两个正交切向量 U 和 V
    Point U(1.0, 0.0, 0.0);
    if (std::abs(m_Normal[0]) > 0.9) U = Point(0.0, 1.0, 0.0);
    Point V = m_Normal.cross(U);
    U = V.cross(m_Normal);
    U = U.normalized();
    V = V.normalized();

    // 将包围盒 8 个角点投影到 U、V 轴，计算准确的范围
    auto box = input->GetBoundingBox();
    double minU = 1e30, maxU = -1e30;
    double minV = 1e30, maxV = -1e30;

    for (int cx = 0; cx < 2; ++cx) {
        for (int cy = 0; cy < 2; ++cy) {
            for (int cz = 0; cz < 2; ++cz) {
                Point corner(cx ? box.max[0] : box.min[0], cy ? box.max[1] : box.min[1], cz ? box.max[2] : box.min[2]);
                Point rel = corner - m_Origin;

                // 向量点乘 (防止 dot() 方法名不一致导致编译报错，这里直接手写)
                double pu = rel[0] * U[0] + rel[1] * U[1] + rel[2] * U[2];
                double pv = rel[0] * V[0] + rel[1] * V[1] + rel[2] * V[2];

                minU = std::min(minU, pu);
                maxU = std::max(maxU, pu);
                minV = std::min(minV, pv);
                maxV = std::max(maxV, pv);
            }
        }
    }

    // 稍微向外扩大 5% 作为边缘余量
    double sizeU = (maxU - minU) * 1.05;
    double sizeV = (maxV - minV) * 1.05;
    double halfU = std::max(sizeU, sizeV) * 0.5;
    double halfV = halfU;

    // 计算包围盒投影的物理中心点（修正原点在非中心时的偏移）
    Point projCenter = m_Origin + U * ((minU + maxU) * 0.5) + V * ((minV + maxV) * 0.5);

    auto output = UnstructuredMesh::New();
    output->SetName(input->GetName() + "_PointPlaneInterpolator");
    auto outputPoints = Points::New();
    int totalPts = m_Resolution * m_Resolution;
    outputPoints->Reserve(totalPts);

    double stepU = (2.0 * halfU) / (m_Resolution - 1);
    double stepV = (2.0 * halfV) / (m_Resolution - 1);

    for (int i = 0; i < m_Resolution; i++) {
        for (int j = 0; j < m_Resolution; j++) {
            double tu = -halfU + i * stepU;
            double tv = -halfV + j * stepV;
            Point pt = projCenter + U * tu + V * tv;
            outputPoints->AddPoint(pt);
        }
    }
    output->SetPoints(outputPoints);

    // 构建 Quad 拓扑
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();
    for (int i = 0; i < m_Resolution - 1; i++) {
        for (int j = 0; j < m_Resolution - 1; j++) {
            igIndex idx[4] = {static_cast<igIndex>(i * m_Resolution + j),
                              static_cast<igIndex>(i * m_Resolution + j + 1),
                              static_cast<igIndex>((i + 1) * m_Resolution + j + 1),
                              static_cast<igIndex>((i + 1) * m_Resolution + j)};
            outputCells->AddCellIds(idx, 4);
            outputTypes->AddValue(IG_QUAD);
        }
    }
    output->SetCells(outputCells, outputTypes);

    // ================= 3. 准备属性数组 =================
    struct OutputAttribute {
        ArrayObject::Pointer source;
        ArrayObject::Pointer output;
        int dim;
        IGenum type;
    };
    std::vector<OutputAttribute> outAttrs;
    auto inputAttributes = input->GetAttributeSet();
    auto outputAttributeSet = output->GetAttributeSet();

    if (inputAttributes) {
        for (IGsize i = 0; i < inputAttributes->GetNumberOfAttributes(); ++i) {
            auto& attr = inputAttributes->GetAttribute(i);
            if (attr.IsDeleted() || attr.attachmentType != IG_POINT || !attr.pointer ||
                attr.pointer->GetNumberOfElements() != sourcePointCount)
                continue;

            auto outArray = FloatArray::New();
            outArray->SetName(attr.pointer->GetName());
            outArray->SetDimension(attr.pointer->GetDimension());
            outArray->Resize(totalPts);
            outAttrs.push_back({attr.pointer, outArray, attr.pointer->GetDimension(), attr.type});
        }
    }

    CharArray::Pointer validMask{};
    if (m_NullPointsStrategy == MASK_POINTS) {
        validMask = CharArray::New();
        validMask->SetName(m_ValidPointsMaskArrayName);
        validMask->SetDimension(1);
        validMask->Resize(totalPts);
    }

    // ================= 4. 核心插值循环 (KD-Tree 终极版) =================
    std::vector<igIndex> ids;
    std::vector<double> distSq;
    std::vector<double> weights;
    std::vector<double> vals;

    const int kNeighbors = std::max(1, m_NumberOfPoints);

    for (IGsize sampleId = 0; sampleId < totalPts; ++sampleId) {
        const Point& sample = outputPoints->GetPoint(sampleId);
        ids.clear();
        distSq.clear();

        // 4.1 空间搜索
        if (m_KernelType == VORONOI) {
            kdTree.QueryKNearest(sample, 1, ids, distSq);
        } else if (m_KernelFootprint == RADIUS) {
            kdTree.QueryRadius(sample, m_Radius, ids, distSq);
            if (ids.empty() && m_NullPointsStrategy == CLOSEST_POINT) { kdTree.QueryKNearest(sample, 1, ids, distSq); }
        } else {
            kdTree.QueryKNearest(sample, kNeighbors, ids, distSq);
        }

        // 4.2 空邻域处理
        if (ids.empty()) {
            for (auto& att: outAttrs) {
                vals.assign(att.dim, m_NullValue);
                att.output->SetElement(sampleId, vals.data());
            }
            if (validMask) validMask->SetValue(sampleId, 0);
            continue;
        }
        if (validMask) validMask->SetValue(sampleId, 1);

        // 4.3 计算权重
        weights.assign(ids.size(), 0.0);
        if (m_KernelType == VORONOI) {
            weights[0] = 1.0;
        } else {
            bool exactHit = false;
            size_t exactIndex = 0;
            if (m_KernelType == SHEPARD) {
                for (size_t n = 0; n < ids.size(); ++n) {
                    if (distSq[n] <= ExactHitTolerance) {
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
                    if (m_KernelType == GAUSSIAN) {
                        weights[n] = PointGaussianWeight(distSq[n], m_Radius, m_Sharpness);
                    } else { // SHEPARD
                        weights[n] = PointShepardWeight(distSq[n], m_PowerParameter);
                    }
                }
            }

            double weightSum = 0.0;
            for (double w: weights) weightSum += w;
            if (weightSum <= 0.0) {
                std::fill(weights.begin(), weights.end(), 0.0);
                weights[0] = 1.0;
            } else {
                for (double& w: weights) w /= weightSum;
            }
        }

        // 4.4 写入属性 (采用 SetElement 保证多分量安全)
        for (auto& att: outAttrs) {
            vals.assign(att.dim, 0.0);
            for (size_t n = 0; n < ids.size(); ++n) {
                if (weights[n] == 0.0) continue;
                for (int c = 0; c < att.dim; ++c) { vals[c] += weights[n] * att.source->GetElementValue(ids[n], c); }
            }
            att.output->SetElement(sampleId, vals.data());
        }

        if (sampleId % 1000 == 0) this->UpdateProgress((double) sampleId / totalPts);
    }

    // ================= 5. 组装输出 =================
    for (auto& att: outAttrs) { outputAttributeSet->AddAttribute(att.type, IG_POINT, att.output); }
    if (validMask) outputAttributeSet->AddAttribute(IG_SCALAR, IG_POINT, validMask);

    output->Modified();
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END