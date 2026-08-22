#include "iGameAdvancedGradientFilter.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include <algorithm>
#include <cmath>
IGAME_NAMESPACE_BEGIN

bool AdvancedGradientFilter::Execute() {

    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) {
            m_Message = "please choose a attribute";
            return false;
        }
        if (curIndex == -1 && name == "") {
            m_Message = "please choose a attribute";
            return false;
        }
        if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(name);
        if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) {
            m_Message = "please choose a attribute";
            return false;
        }

        dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        // 如果 UI 通过 SetCurrentAttributeDimension 显式指定了分量，则优先使用；
        // 否则回退到模型当前激活的分量。
        if (m_currentAttributeDimension == -1) {
            m_currentAttributeDimension = input->GetCurrentAttributeDimension();
        }
        // 向量输入且要求完整张量时，不需要选择某个分量；
        // 只有“标量输出/单分量输出”才必须知道计算哪个分量。
        if (dim != 1 && m_currentAttributeDimension == -1 && !(m_ComputeGradientTensor && dim == 3)) {
            m_Message = "please choose a component";
            return false;
        }
        if (dim == 1)
            m_currentAttributeDimension=0;
        return true;
    };

    // SetOutput(input);

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            surface_Mesh = DynamicCast<SurfaceMesh>(input);
            if (!CheckType()) return false;
            return ComputeGradientWithSurfaceMesh(surface_Mesh, attributeSet, curIndex);
        } break;
        case IG_VOLUME_MESH: {
            volume_Mesh = DynamicCast<VolumeMesh>(input);
            if (!CheckType()) return false;
            return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);
        } break;
        case IG_STRUCTURED_MESH: {
            auto structured = DynamicCast<StructuredMesh>(input);
            if (structured == nullptr) return false;
            structured->GenStructuredCellConnectivities();
            if (structured->GetDimension() == 3) {
                volume_Mesh = DynamicCast<VolumeMesh>(structured);
                if (!CheckType()) return false;
                return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);
            } else {
                surface_Mesh = DynamicCast<SurfaceMesh>(structured);
                if (!CheckType()) return false;
                return ComputeGradientWithSurfaceMesh(surface_Mesh, attributeSet, curIndex);
            }
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            surface_Mesh = mesh->TransferToSurfaceMesh();
            volume_Mesh = mesh->TransferToVolumeMesh();

            if (surface_Mesh) {
                if (!CheckType()) return false;
                bool ok = ComputeGradientWithSurfaceMesh(surface_Mesh, attributeSet, curIndex);
                // TransferToSurfaceMesh 会把 AttributeSet 的 m_DataObject 改为临时 SurfaceMesh，
                // 需要再强制刷新原始 UnstructuredMesh 的显示数据。
                if (ok) {
                    if (auto drawInput = DynamicCast<DrawObject>(input)) {
                        drawInput->GetColorMapper()->SetRangeStable(false);
                        drawInput->ForceReConvertToDrawableData();
                    }
                }
                return ok;
            }

            if (volume_Mesh) {
                if (!CheckType()) return false;
                bool ok = ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);
                if (ok) {
                    if (auto drawInput = DynamicCast<DrawObject>(input)) {
                        drawInput->GetColorMapper()->SetRangeStable(false);
                        drawInput->ForceReConvertToDrawableData();
                    }
                }
                return ok;
            }

            // 混合 2D+3D 网格：按单元维度逐个计算
            if (!CheckType()) return false;
            bool ok = ComputeGradientWithMixedMesh(mesh, attributeSet, curIndex);
            if (ok) {
                if (auto drawInput = DynamicCast<DrawObject>(input)) {
                    drawInput->ForceReConvertToDrawableData();
                }
            }
            return ok;
        } break;
        default:
            return false;
    }

    return true;
}

void AdvancedGradientFilter::AddGradientAttributeToSet(AttributeSet::Pointer attrs, IGenum attachmentType,
                                                       FloatArray::Pointer arr, int outDim) {
    if (outDim == 9) {
        attrs->AddAttribute(IG_TENSOR, attachmentType, arr);
    } else {
        // 与旧 GradientFilter 一致：3 分量梯度用 IG_SCALAR 添加，
        // 避免表面网格上 IG_VECTOR 类型着色异常。
        attrs->AddScalar(attachmentType, arr);
    }
}

std::array<float, 3> AdvancedGradientFilter::ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    if (type == 1) {
        switch (cell->GetCellType()) {
            case IG_TETRA: // 纯四面体
                return ComputeTetPointGradient(cell, data, dim);
            case IG_HEXAHEDRON: // 纯六面体
                return ComputeHexPointGradient(cell, data, dim);
            default: // 其他
                return ComputePolyPointGradient(cell, data, dim);
        }
    } else if (type == 0) {
        switch (cell->GetCellType()) {
            case IG_TRIANGLE: // 三角形
                return ComputeTriPointGradient(cell, data, dim);
            case IG_QUAD: // 四边形
                return ComputeQuadPointGradient(cell, data, dim);
            default: // 多边形
                return ComputePolygonPointGradient(cell, data, dim);
        }
    }
    return std::array<float, 3>{};
}

std::array<float, 3> AdvancedGradientFilter::ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    switch (cell->GetCellType()) {
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
            return ComputePointGradient(cell, data, dim);
        case IG_TETRA:
        case IG_HEXAHEDRON:
        case IG_POLYHEDRON:
            return ComputeHexPointGradient(cell, data, dim);
        default:break;
    }

    return ComputePointGradient(cell, data, dim);
}

std::array<float, 3> AdvancedGradientFilter::ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    switch (cell->GetCellType()) {
        case IG_TETRA: // 纯四面体
            return ComputeTetCellGradient(type, cell, data, dim);
        case IG_HEXAHEDRON: // 纯六面体
            return ComputeHexCellGradient(type, cell, data, dim);
        default: // 其他
            return ComputePolyCellGradient(type, cell, data, dim);
    }
    return std::array<float, 3>{};
}

std::array<float, 3> AdvancedGradientFilter::ComputeSurfaceCellGradient(Cell* cell, ArrayObject::Pointer data,
                                                                        int dim, int component) {
    VectorGrad grad;
    switch (cell->GetCellType()) {
        case IG_TRIANGLE: {
            Vector3f p0 = cell->GetPoint(0);
            Vector3f p1 = cell->GetPoint(1);
            Vector3f p2 = cell->GetPoint(2);

            int pid0 = cell->GetPointId(0);
            int pid1 = cell->GetPointId(1);
            int pid2 = cell->GetPointId(2);

            Vector3f v0 = p1 - p0;
            Vector3f v1 = p2 - p0;

            float d00 = v0.dot(v0);
            float d01 = v0.dot(v1);
            float d11 = v1.dot(v1);
            float denom = d00 * d11 - d01 * d01;
            float denomr = denom == 0 ? 0.f : 1.f / denom;

            float gx1 = (d11 * v0[0] - d01 * v1[0]) * denomr;
            float gx2 = (d00 * v1[0] - d01 * v0[0]) * denomr;
            float gy1 = (d11 * v0[1] - d01 * v1[1]) * denomr;
            float gy2 = (d00 * v1[1] - d01 * v0[1]) * denomr;
            float gz1 = (d11 * v0[2] - d01 * v1[2]) * denomr;
            float gz2 = (d00 * v1[2] - d01 * v0[2]) * denomr;

            float a0 = data->GetValue(dim * pid0 + component);
            float a1 = data->GetValue(dim * pid1 + component);
            float a2 = data->GetValue(dim * pid2 + component);

            float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
            float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
            float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

            grad.x.gx = gx;
            grad.x.gy = gy;
            grad.x.gz = gz;
            break;
        }
        case IG_QUADRATIC_QUAD: {
            // 8 节点二次四边形：用形函数导数在单元中心计算
            return ComputeQuadraticQuadGradient(cell, data, dim, component, 0.0, 0.0);
        }
        case IG_QUAD: {
            Vector3f p0 = cell->GetPoint(0);
            Vector3f p1 = cell->GetPoint(1);
            Vector3f p2 = cell->GetPoint(2);
            Vector3f p3 = cell->GetPoint(3);

            int pid0 = cell->GetPointId(0);
            int pid1 = cell->GetPointId(1);
            int pid2 = cell->GetPointId(2);
            int pid3 = cell->GetPointId(3);

            Vector3f v0_1 = p1 - p0;
            Vector3f v1_1 = p2 - p0;

            float d00_1 = v0_1.dot(v0_1);
            float d01_1 = v0_1.dot(v1_1);
            float d11_1 = v1_1.dot(v1_1);
            float denom_1 = d00_1 * d11_1 - d01_1 * d01_1;
            float denomr_1 = denom_1 == 0 ? 0.f : 1.f / denom_1;

            float gx1_1 = (d11_1 * v0_1[0] - d01_1 * v1_1[0]) * denomr_1;
            float gx2_1 = (d00_1 * v1_1[0] - d01_1 * v0_1[0]) * denomr_1;
            float gy1_1 = (d11_1 * v0_1[1] - d01_1 * v1_1[1]) * denomr_1;
            float gy2_1 = (d00_1 * v1_1[1] - d01_1 * v0_1[1]) * denomr_1;
            float gz1_1 = (d11_1 * v0_1[2] - d01_1 * v1_1[2]) * denomr_1;
            float gz2_1 = (d00_1 * v1_1[2] - d01_1 * v0_1[2]) * denomr_1;

            float a0_1 = data->GetValue(dim * pid0 + component);
            float a1_1 = data->GetValue(dim * pid1 + component);
            float a2_1 = data->GetValue(dim * pid2 + component);

            float gx_1 = gx1_1 * (a1_1 - a0_1) + gx2_1 * (a2_1 - a0_1);
            float gy_1 = gy1_1 * (a1_1 - a0_1) + gy2_1 * (a2_1 - a0_1);
            float gz_1 = gz1_1 * (a1_1 - a0_1) + gz2_1 * (a2_1 - a0_1);

            Vector3f v0_2 = p2 - p0;
            Vector3f v1_2 = p3 - p0;

            float d00_2 = v0_2.dot(v0_2);
            float d01_2 = v0_2.dot(v1_2);
            float d11_2 = v1_2.dot(v1_2);
            float denom_2 = d00_2 * d11_2 - d01_2 * d01_2;
            float denomr_2 = denom_2 == 0 ? 0.f : 1.f / denom_2;

            float gx1_2 = (d11_2 * v0_2[0] - d01_2 * v1_2[0]) * denomr_2;
            float gx2_2 = (d00_2 * v1_2[0] - d01_2 * v0_2[0]) * denomr_2;
            float gy1_2 = (d11_2 * v0_2[1] - d01_2 * v1_2[1]) * denomr_2;
            float gy2_2 = (d00_2 * v1_2[1] - d01_2 * v0_2[1]) * denomr_2;
            float gz1_2 = (d11_2 * v0_2[2] - d01_2 * v1_2[2]) * denomr_2;
            float gz2_2 = (d00_2 * v1_2[2] - d01_2 * v0_2[2]) * denomr_2;

            float a0_2 = data->GetValue(dim * pid0 + component);
            float a2_2 = data->GetValue(dim * pid2 + component);
            float a3_2 = data->GetValue(dim * pid3 + component);

            float gx_2 = gx1_2 * (a2_2 - a0_2) + gx2_2 * (a3_2 - a0_2);
            float gy_2 = gy1_2 * (a2_2 - a0_2) + gy2_2 * (a3_2 - a0_2);
            float gz_2 = gz1_2 * (a2_2 - a0_2) + gz2_2 * (a3_2 - a0_2);

            grad.x.gx = (gx_1 + gx_2) * 0.5f;
            grad.x.gy = (gy_1 + gy_2) * 0.5f;
            grad.x.gz = (gz_1 + gz_2) * 0.5f;
            break;
        }
        default: {
            Vector3f center(0, 0, 0);
            float centerValue = 0.0f;
            int numPoints = cell->GetNumberOfPoints();

            for (int i = 0; i < numPoints; i++) {
                auto p = cell->GetPoint(i);
                center[0] += p[0];
                center[1] += p[1];
                center[2] += p[2];
                centerValue += data->GetValue(dim * cell->GetPointId(i) + component);
            }

            center[0] /= numPoints;
            center[1] /= numPoints;
            center[2] /= numPoints;
            centerValue /= numPoints;

            std::array<float, 3> gradientApprox = {0.0f, 0.0f, 0.0f};
            for (int i = 0; i < numPoints; ++i) {
                auto p = cell->GetPoint(i);
                std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
                float valDiff = data->GetValue(dim * cell->GetPointId(i) + component) - centerValue;

                for (int d = 0; d < 3; d++) {
                    gradientApprox[d] += diff[d] * valDiff;
                }
            }

            float avgEdgeLength = ComputeAverageEdgeLength(cell);
            if (avgEdgeLength > 1e-8f) {
                for (int d = 0; d < 3; d++) {
                    gradientApprox[d] /= avgEdgeLength;
                }
            }

            grad.x.gx = gradientApprox[0];
            grad.x.gy = gradientApprox[1];
            grad.x.gz = gradientApprox[2];
            break;
        }
    }

    return {grad.x.gx, grad.x.gy, grad.x.gz};
}

std::array<float, 3> AdvancedGradientFilter::ComputeQuadraticQuadGradient(Cell* cell, ArrayObject::Pointer data,
                                                                          int dim, int component,
                                                                          double xi, double eta) {
    // 8 节点二次四边形（VTK_QUADRATIC_QUAD）形函数导数
    double dN_xi[8] = {0.0}, dN_eta[8] = {0.0};

    // 角点 0~3
    dN_xi[0] = 0.25 * (1 - eta) * (2 * xi + eta);
    dN_eta[0] = 0.25 * (1 - xi) * (xi + 2 * eta);

    dN_xi[1] = 0.25 * (1 - eta) * (2 * xi - eta);
    dN_eta[1] = 0.25 * (1 + xi) * (2 * eta - xi);

    dN_xi[2] = 0.25 * (1 + eta) * (2 * xi + eta);
    dN_eta[2] = 0.25 * (1 + xi) * (xi + 2 * eta);

    dN_xi[3] = 0.25 * (1 + eta) * (2 * xi - eta);
    dN_eta[3] = 0.25 * (1 - xi) * (2 * eta - xi);

    // 边中点 4~7
    dN_xi[4] = -xi * (1 - eta);
    dN_eta[4] = -0.5 * (1 - xi * xi);

    dN_xi[5] = 0.5 * (1 - eta * eta);
    dN_eta[5] = -eta * (1 + xi);

    dN_xi[6] = -xi * (1 + eta);
    dN_eta[6] = 0.5 * (1 - xi * xi);

    dN_xi[7] = -0.5 * (1 - eta * eta);
    dN_eta[7] = -eta * (1 - xi);

    double x[8], y[8], z[8], f[8];
    for (int i = 0; i < 8; ++i) {
        const Point& p = cell->GetPoint(i);
        x[i] = p[0];
        y[i] = p[1];
        z[i] = p[2];
        f[i] = data->GetValue(cell->GetPointId(i) * dim + component);
    }

    double f_xi = 0.0, f_eta = 0.0;
    double x_xi = 0.0, y_xi = 0.0, z_xi = 0.0;
    double x_eta = 0.0, y_eta = 0.0, z_eta = 0.0;
    for (int i = 0; i < 8; ++i) {
        f_xi += dN_xi[i] * f[i];
        f_eta += dN_eta[i] * f[i];
        x_xi += dN_xi[i] * x[i];
        y_xi += dN_xi[i] * y[i];
        z_xi += dN_xi[i] * z[i];
        x_eta += dN_eta[i] * x[i];
        y_eta += dN_eta[i] * y[i];
        z_eta += dN_eta[i] * z[i];
    }

    // 3D 曲面上的切向梯度：用 2×2 Gram 矩阵求最小二乘/最小范数解
    double g11 = x_xi * x_xi + y_xi * y_xi + z_xi * z_xi;
    double g12 = x_xi * x_eta + y_xi * y_eta + z_xi * z_eta;
    double g22 = x_eta * x_eta + y_eta * y_eta + z_eta * z_eta;
    double det = g11 * g22 - g12 * g12;
    if (std::abs(det) < 1e-12) {
        return {0.0f, 0.0f, 0.0f};
    }

    double c1 = (f_xi * g22 - f_eta * g12) / det;
    double c2 = (g11 * f_eta - g12 * f_xi) / det;

    double gx = c1 * x_xi + c2 * x_eta;
    double gy = c1 * y_xi + c2 * y_eta;
    double gz = c1 * z_xi + c2 * z_eta;
    return {static_cast<float>(gx), static_cast<float>(gy), static_cast<float>(gz)};
}

std::array<float, 3> AdvancedGradientFilter::ComputeSurfacePointGradient(Cell* cell, ArrayObject::Pointer data,
                                                                         int dim, int component, int localPointIndex) {
    if (cell->GetCellType() == IG_QUADRATIC_QUAD && localPointIndex >= 0 && localPointIndex < 8) {
        static const double xi[8] = {-1.0, 1.0, 1.0, -1.0, 0.0, 1.0, 0.0, -1.0};
        static const double eta[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, 0.0, 1.0, 0.0};
        return ComputeQuadraticQuadGradient(cell, data, dim, component, xi[localPointIndex], eta[localPointIndex]);
    }
    return ComputeSurfaceCellGradient(cell, data, dim, component);
}

bool AdvancedGradientFilter::ComputeGradientWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                    int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    int attrDim = Data->GetDimension();
    int outDim = (m_ComputeGradientTensor && attrDim == 3) ? 9 : 3;

    FloatArray::Pointer gradient = FloatArray::New();
    gradient->SetDimension(outDim);
    gradient->Reserve(NumCells);
    gradient->SetName("gradient");
    // attributeSet->AddScalar(IG_CELL, gradient);
    std::vector<std::array<double, 9>> gradient_values(NumCells);

    int progress = 0;
    int block = NumCells / 100;
    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        auto cell = Mesh->GetFace(cellId);

        if (m_ComputeGradientTensor && attrDim == 3) {
            for (int comp = 0; comp < 3; ++comp) {
                auto g = ComputeSurfaceCellGradient(cell, Data, dim, comp);
                gradient_values[cellId][comp * 3 + 0] = g[0];
                gradient_values[cellId][comp * 3 + 1] = g[1];
                gradient_values[cellId][comp * 3 + 2] = g[2];
            }
        } else {
            auto g = ComputeSurfaceCellGradient(cell, Data, dim, m_currentAttributeDimension);
            gradient_values[cellId][0] = g[0];
            gradient_values[cellId][1] = g[1];
            gradient_values[cellId][2] = g[2];
        }
    }
    auto clamp_by_quantile = [](std::vector<double>& v, double qlo, double qhi) {
        if (v.empty()) return;
        std::vector<double> tmp = v;
        auto nth_q = [&](double q) {
            size_t idx = size_t(std::clamp(q, 0.0, 1.0) * (tmp.size() - 1));
            std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
            return tmp[idx];
        };
        double lo = nth_q(0.02);
        double hi = nth_q(0.98);
        if (lo > hi) std::swap(lo, hi);
        for (double& x : v) {
            if (x < lo) x = lo;
            else if (x > hi) x = hi;
        }
    };

    // 注意：不再做 2%~98% 分位数截断，避免与 ParaView 数值偏差过大

    auto newAttrs = Mesh->GetAttributeSet();
    FloatArray::Pointer debugOutput;

    if (m_OutputToPointData) {
        // 与 ParaView 对齐：点梯度 = 相邻单元梯度的简单平均（不加权）
        FloatArray::Pointer pointGradient = FloatArray::New();
        debugOutput = pointGradient;
        pointGradient->SetDimension(outDim);
        pointGradient->SetName("gradient");
        pointGradient->Reserve(NumPoints);
        std::vector<float> zero(outDim, 0.0f);
        for (int i = 0; i < NumPoints; ++i) {
            pointGradient->AddElement(zero.data());
        }

        std::vector<int> counts(NumPoints, 0);
        for (int cellId = 0; cellId < NumCells; ++cellId) {
            auto cell = Mesh->GetFace(cellId);
            if (cell == nullptr) continue;

            for (int j = 0; j < cell->GetNumberOfPoints(); ++j) {
                int pid = cell->GetPointId(j);
                std::array<float, 9> nodeGrad{};
                if (m_ComputeGradientTensor && attrDim == 3) {
                    for (int comp = 0; comp < 3; ++comp) {
                        auto g = ComputeSurfacePointGradient(cell, Data, dim, comp, j);
                        nodeGrad[comp * 3 + 0] = g[0];
                        nodeGrad[comp * 3 + 1] = g[1];
                        nodeGrad[comp * 3 + 2] = g[2];
                    }
                } else {
                    auto g = ComputeSurfacePointGradient(cell, Data, dim, m_currentAttributeDimension, j);
                    nodeGrad[0] = g[0];
                    nodeGrad[1] = g[1];
                    nodeGrad[2] = g[2];
                }

                std::vector<float> cur(outDim, 0.0f);
                pointGradient->GetElement(pid, cur.data());
                for (int k = 0; k < outDim; ++k) {
                    cur[k] += nodeGrad[k];
                }
                pointGradient->SetElement(pid, cur.data());
                ++counts[pid];
            }
        }

        for (int i = 0; i < NumPoints; ++i) {
            if (counts[i] == 0) continue;
            std::vector<float> cur(outDim, 0.0f);
            pointGradient->GetElement(i, cur.data());
            for (int k = 0; k < outDim; ++k) {
                cur[k] /= counts[i];
            }
            pointGradient->SetElement(i, cur.data());
        }

        AddGradientAttributeToSet(newAttrs, IG_POINT, pointGradient, outDim);
    } else {
        debugOutput = gradient;
        for (int i = 0; i < NumCells; ++i) {
            if (outDim == 3) {
                gradient->AddElement3(
                        static_cast<float>(gradient_values[i][0]),
                        static_cast<float>(gradient_values[i][1]),
                        static_cast<float>(gradient_values[i][2]));
            } else {
                std::array<float, 9> vals;
                for (int k = 0; k < outDim; ++k) {
                    vals[k] = static_cast<float>(gradient_values[i][k]);
                }
                gradient->AddElement(vals.data());
            }
        }
        AddGradientAttributeToSet(newAttrs, IG_CELL, gradient, outDim);
    }

    // ===== 分位数截断：与旧 GradientFilter 一致，抑制极端离群值 =====
    {
        FloatArray::Pointer outputArr = debugOutput;
        if (outputArr && outputArr->GetNumberOfElements() > 0) {
            std::vector<float> elem(outDim);
            for (int k = 0; k < outDim; ++k) {
                std::vector<double> vals(outputArr->GetNumberOfElements());
                for (int i = 0; i < outputArr->GetNumberOfElements(); ++i) {
                    outputArr->GetElement(i, elem.data());
                    vals[i] = elem[k];
                }
                std::vector<double> sorted = vals;
                std::sort(sorted.begin(), sorted.end());
                double lo = sorted[size_t(0.02 * (sorted.size() - 1))];
                double hi = sorted[size_t(0.98 * (sorted.size() - 1))];
                if (lo > hi) std::swap(lo, hi);
                for (int i = 0; i < outputArr->GetNumberOfElements(); ++i) {
                    if (vals[i] < lo) vals[i] = lo;
                    else if (vals[i] > hi) vals[i] = hi;
                }
                for (int i = 0; i < outputArr->GetNumberOfElements(); ++i) {
                    outputArr->GetElement(i, elem.data());
                    elem[k] = static_cast<float>(vals[i]);
                    outputArr->SetElement(i, elem.data());
                }
            }
        }
    }
    // ===== 分位数截断结束 =====

    newAttrs->ForceReConvertToDrawableData();

    UpdateProgress(1.0);
    SetOutput(Mesh);
    return true;
}

bool AdvancedGradientFilter::ComputeGradientWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                   int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfVolumes();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;
    if (Data == nullptr) return false;

    // 单元属性先转成点属性，统一按点属性计算
    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetCells(), Data, NumPoints);
    }

    int attrDim = Data->GetDimension();
    int outDim = (m_ComputeGradientTensor && attrDim == 3) ? 9 : 3;

    FloatArray::Pointer gradient = FloatArray::New();
    gradient->SetDimension(outDim);
    gradient->SetName("gradient");
    std::vector<float> pointWeights;
    if (m_OutputToPointData) {
        gradient->Reserve(NumPoints);
        std::vector<float> zero(outDim, 0.0f);
        for (int i = 0; i < NumPoints; ++i) {
            gradient->AddElement(zero.data());
        }
        pointWeights.assign(NumPoints, 0.0f);
    } else {
        gradient->Reserve(NumCells);
    }

    int progress = 0;
    int block = NumCells > 0 ? NumCells / 100 : 0;
    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (block > 0 && cellId > block * progress) {
            ++progress;
            UpdateProgress(progress * 0.01);
        }

        auto cell = Mesh->GetVolume(cellId);
        if (cell == nullptr) {
            if (!m_OutputToPointData) {
                std::vector<float> zero(outDim, 0.0f);
                gradient->AddElement(zero.data());
            }
            continue;
        }

        if (m_OutputToPointData) {
            // 与 ParaView 对齐：点梯度 = 相邻单元在“该点处”的形函数导数简单平均
            float vol = ComputeVolumeByGreenGauss(cell);
            if (vol <= 1e-12f) continue;   // 只跳过退化单元，不作为加权系数
            for (int j = 0; j < cell->GetNumberOfPoints(); ++j) {
                int pid = cell->GetPointId(j);
                std::array<float, 9> nodeGrad{};
                if (m_ComputeGradientTensor && attrDim == 3) {
                    for (int comp = 0; comp < 3; ++comp) {
                        auto g = ComputeVolumePointGradient(cell, Data, dim, comp, j);
                        nodeGrad[comp * 3 + 0] = g[0];
                        nodeGrad[comp * 3 + 1] = g[1];
                        nodeGrad[comp * 3 + 2] = g[2];
                    }
                } else {
                    auto g = ComputeVolumePointGradient(cell, Data, dim, m_currentAttributeDimension, j);
                    nodeGrad[0] = g[0];
                    nodeGrad[1] = g[1];
                    nodeGrad[2] = g[2];
                }

                std::vector<float> cur(outDim, 0.0f);
                gradient->GetElement(pid, cur.data());
                for (int k = 0; k < outDim; ++k) {
                    cur[k] += nodeGrad[k];
                }
                gradient->SetElement(pid, cur.data());
                pointWeights[pid] += 1.0f;
            }
        } else {
            std::array<float, 9> cellGrad{};
            if (m_ComputeGradientTensor && attrDim == 3) {
                for (int comp = 0; comp < 3; ++comp) {
                    auto g = ComputeVolumeCellGradient(cell, Data, dim, comp);
                    cellGrad[comp * 3 + 0] = g[0];
                    cellGrad[comp * 3 + 1] = g[1];
                    cellGrad[comp * 3 + 2] = g[2];
                }
            } else {
                auto g = ComputeVolumeCellGradient(cell, Data, dim, m_currentAttributeDimension);
                cellGrad[0] = g[0];
                cellGrad[1] = g[1];
                cellGrad[2] = g[2];
            }

            if (outDim == 3) {
                gradient->AddElement3(cellGrad[0], cellGrad[1], cellGrad[2]);
            } else {
                gradient->AddElement(cellGrad.data());
            }
        }
    }

    if (m_OutputToPointData) {
        for (int i = 0; i < NumPoints; ++i) {
            if (pointWeights[i] <= 1e-12f) continue;
            std::vector<float> cur(outDim, 0.0f);
            gradient->GetElement(i, cur.data());
            for (int k = 0; k < outDim; ++k) {
                cur[k] /= pointWeights[i];
            }
            gradient->SetElement(i, cur.data());
        }
    }

    auto newAttrs = Mesh->GetAttributeSet();
    if (m_OutputToPointData) {
        AddGradientAttributeToSet(newAttrs, IG_POINT, gradient, outDim);
    } else {
        AddGradientAttributeToSet(newAttrs, IG_CELL, gradient, outDim);
    }
    newAttrs->ForceReConvertToDrawableData();

    UpdateProgress(1.0);
    SetOutput(Mesh);
    return true;
}

bool AdvancedGradientFilter::ComputeGradientWithMixedMesh(UnstructuredMesh::Pointer Mesh,
                                                          AttributeSet::Pointer Attributes, int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfCells();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;
    if (Data == nullptr) return false;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetCells(), Data, NumPoints);
    }

    int attrDim = Data->GetDimension();
    int outDim = (m_ComputeGradientTensor && attrDim == 3) ? 9 : 3;

    FloatArray::Pointer gradient = FloatArray::New();
    gradient->SetDimension(outDim);
    gradient->SetName("gradient");
    std::vector<float> pointWeights;
    if (m_OutputToPointData) {
        gradient->Reserve(NumPoints);
        std::vector<float> zero(outDim, 0.0f);
        for (int i = 0; i < NumPoints; ++i) {
            gradient->AddElement(zero.data());
        }
        pointWeights.assign(NumPoints, 0.0f);
    } else {
        gradient->Reserve(NumCells);
    }

    auto IsSurfaceCell = [](IGenum type) -> bool {
        switch (type) {
            case IG_TRIANGLE:
            case IG_QUAD:
            case IG_POLYGON:
            case IG_QUADRATIC_TRIANGLE:
            case IG_QUADRATIC_QUAD:
            case IG_QUADRATIC_POLYGON:
            case IG_BIQUADRATIC_QUAD:
            case IG_BIQUADRATIC_TRIANGLE:
            case IG_QUADRATIC_LINEAR_QUAD:
                return true;
            default:
                return false;
        }
    };

    for (int cellId = 0; cellId < NumCells; ++cellId) {
        auto cell = Mesh->GetCell(cellId);
        if (cell == nullptr) continue;
        bool isSurface = IsSurfaceCell(cell->GetCellType());

        if (m_OutputToPointData) {
            for (int j = 0; j < cell->GetNumberOfPoints(); ++j) {
                int pid = cell->GetPointId(j);
                std::array<float, 9> nodeGrad{};
                if (m_ComputeGradientTensor && attrDim == 3) {
                    for (int comp = 0; comp < 3; ++comp) {
                        auto g = isSurface
                            ? ComputeSurfacePointGradient(cell, Data, dim, comp, j)
                            : ComputeVolumePointGradient(cell, Data, dim, comp, j);
                        nodeGrad[comp * 3 + 0] = g[0];
                        nodeGrad[comp * 3 + 1] = g[1];
                        nodeGrad[comp * 3 + 2] = g[2];
                    }
                } else {
                    auto g = isSurface
                        ? ComputeSurfacePointGradient(cell, Data, dim, m_currentAttributeDimension, j)
                        : ComputeVolumePointGradient(cell, Data, dim, m_currentAttributeDimension, j);
                    nodeGrad[0] = g[0];
                    nodeGrad[1] = g[1];
                    nodeGrad[2] = g[2];
                }

                std::vector<float> cur(outDim, 0.0f);
                gradient->GetElement(pid, cur.data());
                for (int k = 0; k < outDim; ++k) {
                    cur[k] += nodeGrad[k];
                }
                gradient->SetElement(pid, cur.data());
                pointWeights[pid] += 1.0f;
            }
        } else {
            std::array<float, 9> cellGrad{};
            if (m_ComputeGradientTensor && attrDim == 3) {
                for (int comp = 0; comp < 3; ++comp) {
                    auto g = isSurface
                        ? ComputeSurfaceCellGradient(cell, Data, dim, comp)
                        : ComputeVolumeCellGradient(cell, Data, dim, comp);
                    cellGrad[comp * 3 + 0] = g[0];
                    cellGrad[comp * 3 + 1] = g[1];
                    cellGrad[comp * 3 + 2] = g[2];
                }
            } else {
                auto g = isSurface
                    ? ComputeSurfaceCellGradient(cell, Data, dim, m_currentAttributeDimension)
                    : ComputeVolumeCellGradient(cell, Data, dim, m_currentAttributeDimension);
                cellGrad[0] = g[0];
                cellGrad[1] = g[1];
                cellGrad[2] = g[2];
            }

            if (outDim == 3) {
                gradient->AddElement3(cellGrad[0], cellGrad[1], cellGrad[2]);
            } else {
                gradient->AddElement(cellGrad.data());
            }
        }
    }

    if (m_OutputToPointData) {
        for (int i = 0; i < NumPoints; ++i) {
            if (pointWeights[i] <= 1e-12f) continue;
            std::vector<float> cur(outDim, 0.0f);
            gradient->GetElement(i, cur.data());
            for (int k = 0; k < outDim; ++k) {
                cur[k] /= pointWeights[i];
            }
            gradient->SetElement(i, cur.data());
        }
    }

    auto newAttrs = Mesh->GetAttributeSet();
    if (m_OutputToPointData) {
        AddGradientAttributeToSet(newAttrs, IG_POINT, gradient, outDim);
    } else {
        AddGradientAttributeToSet(newAttrs, IG_CELL, gradient, outDim);
    }
    newAttrs->ForceReConvertToDrawableData();

    UpdateProgress(1.0);
    SetOutput(Mesh);
    return true;
}

std::array<float, 3> AdvancedGradientFilter::ComputeHexGradient(Cell* cell, ArrayObject::Pointer data,
                                                                int dim, int component,
                                                                double r, double s, double t) {
    // 8 节点六面体（VTK_HEXAHEDRON）三线性形函数导数
    static const double rsign[8] = {-1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0};
    static const double ssign[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
    static const double tsign[8] = {-1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0};

    double dNdr[8], dNds[8], dNdt[8];
    for (int i = 0; i < 8; ++i) {
        dNdr[i] = rsign[i] / 8.0 * (1.0 + ssign[i] * s) * (1.0 + tsign[i] * t);
        dNds[i] = ssign[i] / 8.0 * (1.0 + rsign[i] * r) * (1.0 + tsign[i] * t);
        dNdt[i] = tsign[i] / 8.0 * (1.0 + rsign[i] * r) * (1.0 + ssign[i] * s);
    }

    double x[8], y[8], z[8], f[8];
    for (int i = 0; i < 8; ++i) {
        const Point& p = cell->GetPoint(i);
        x[i] = p[0];
        y[i] = p[1];
        z[i] = p[2];
        f[i] = data->GetValue(cell->GetPointId(i) * dim + component);
    }

    double f_r = 0.0, f_s = 0.0, f_t = 0.0;
    double x_r = 0.0, y_r = 0.0, z_r = 0.0;
    double x_s = 0.0, y_s = 0.0, z_s = 0.0;
    double x_t = 0.0, y_t = 0.0, z_t = 0.0;
    for (int i = 0; i < 8; ++i) {
        f_r += dNdr[i] * f[i];
        f_s += dNds[i] * f[i];
        f_t += dNdt[i] * f[i];
        x_r += dNdr[i] * x[i];
        y_r += dNdr[i] * y[i];
        z_r += dNdr[i] * z[i];
        x_s += dNds[i] * x[i];
        y_s += dNds[i] * y[i];
        z_s += dNds[i] * z[i];
        x_t += dNdt[i] * x[i];
        y_t += dNdt[i] * y[i];
        z_t += dNdt[i] * z[i];
    }

    Eigen::Matrix3d J;
    J << x_r, y_r, z_r,
         x_s, y_s, z_s,
         x_t, y_t, z_t;
    double det = J.determinant();
    if (std::abs(det) < 1e-12) {
        return {0.0f, 0.0f, 0.0f};
    }

    Eigen::Vector3d rhs(f_r, f_s, f_t);
    Eigen::Vector3d g = J.inverse() * rhs;
    return {static_cast<float>(g.x()), static_cast<float>(g.y()), static_cast<float>(g.z())};
}

std::array<float, 3> AdvancedGradientFilter::ComputeVolumePointGradient(Cell* cell, ArrayObject::Pointer data,
                                                                        int dim, int component, int localPointIndex) {
    if (cell->GetCellType() == IG_HEXAHEDRON && localPointIndex >= 0 && localPointIndex < 8) {
        static const double r[8] = {-1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0};
        static const double s[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
        static const double t[8] = {-1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0};
        return ComputeHexGradient(cell, data, dim, component, r[localPointIndex], s[localPointIndex], t[localPointIndex]);
    }
    return ComputeVolumeCellGradient(cell, data, dim, component);
}

std::array<float, 3> AdvancedGradientFilter::ComputeVolumeCellGradient(Cell* cell, ArrayObject::Pointer data,
                                                                      int dim, int component) {
    switch (cell->GetCellType()) {
        case IG_TETRA:
            // 四面体：线性形函数精确梯度
            return ComputeTetCellGradientExact(cell, data, dim, component);
        case IG_HEXAHEDRON:
            // 六面体：三线性形函数导数在单元中心
            return ComputeHexGradient(cell, data, dim, component, 0.0, 0.0, 0.0);
        default:
            // 棱柱 / 棱锥 / 多面体：最小二乘线性拟合梯度
            return ComputeLeastSquaresCellGradient(cell, data, dim, component);
    }
}

std::array<float, 3> AdvancedGradientFilter::ComputeTetCellGradientExact(Cell* cell, ArrayObject::Pointer data,
                                                                        int dim, int component) {
    Vector3f p0 = cell->GetPoint(0);
    Vector3f p1 = cell->GetPoint(1);
    Vector3f p2 = cell->GetPoint(2);
    Vector3f p3 = cell->GetPoint(3);

    int pid0 = cell->GetPointId(0);
    int pid1 = cell->GetPointId(1);
    int pid2 = cell->GetPointId(2);
    int pid3 = cell->GetPointId(3);

    float a0 = static_cast<float>(data->GetValue(pid0 * dim + component));
    float a1 = static_cast<float>(data->GetValue(pid1 * dim + component));
    float a2 = static_cast<float>(data->GetValue(pid2 * dim + component));
    float a3 = static_cast<float>(data->GetValue(pid3 * dim + component));

    // 解 3x3 方程组：M * g = rhs
    // M 的列是三条边向量，rhs 是顶点值差分
    Vector3f v1 = p1 - p0;
    Vector3f v2 = p2 - p0;
    Vector3f v3 = p3 - p0;

    Eigen::Matrix3f M;
    M.col(0) = Eigen::Vector3f(v1[0], v1[1], v1[2]);
    M.col(1) = Eigen::Vector3f(v2[0], v2[1], v2[2]);
    M.col(2) = Eigen::Vector3f(v3[0], v3[1], v3[2]);

    float det = M.determinant();
    if (std::abs(det) < 1e-12f) {
        return {0.0f, 0.0f, 0.0f};  // 退化四面体
    }

    Eigen::Vector3f rhs(a1 - a0, a2 - a0, a3 - a0);
    Eigen::Vector3f g = M.inverse().eval() * rhs;
    return {g.x(), g.y(), g.z()};
}

std::array<float, 3> AdvancedGradientFilter::ComputeGreenGaussCellGradient(Cell* cell, ArrayObject::Pointer data,
                                                                          int dim, int component) {
    float volume = ComputeVolumeByGreenGauss(cell);
    if (volume < 1e-12f) {
        return {0.0f, 0.0f, 0.0f};  // 退化单元
    }

    std::array<float, 3> grad = {0.0f, 0.0f, 0.0f};
    int numFaces = cell->GetNumberOfFaces();
    for (int f = 0; f < numFaces; ++f) {
        Cell* face = cell->GetFace(f);
        if (face == nullptr) continue;

        Vector3f areaVector = ComputeFaceAreaVector(face);
        int npts = face->GetNumberOfPoints();
        if (npts < 3) continue;

        float faceValue = 0.0f;
        for (int i = 0; i < npts; ++i) {
            int pid = face->GetPointId(i);
            faceValue += static_cast<float>(data->GetValue(pid * dim + component));
        }
        faceValue /= npts;

        grad[0] += faceValue * areaVector[0];
        grad[1] += faceValue * areaVector[1];
        grad[2] += faceValue * areaVector[2];
    }

    grad[0] /= volume;
    grad[1] /= volume;
    grad[2] /= volume;
    return grad;
}

std::array<float, 3> AdvancedGradientFilter::ComputeLeastSquaresCellGradient(Cell* cell, ArrayObject::Pointer data,
                                                                              int dim, int component) {
    int npts = cell->GetNumberOfPoints();
    if (npts < 4) {
        // 点数不足时退回 Green-Gauss
        return ComputeGreenGaussCellGradient(cell, data, dim, component);
    }

    // 对单元所有顶点做线性拟合：phi ≈ g·p + c
    // A = [x y z 1], b = phi
    Eigen::MatrixXf A(npts, 4);
    Eigen::VectorXf b(npts);
    for (int i = 0; i < npts; ++i) {
        int pid = cell->GetPointId(i);
        const Point& p = cell->GetPoint(i);
        A(i, 0) = p[0];
        A(i, 1) = p[1];
        A(i, 2) = p[2];
        A(i, 3) = 1.0f;
        b(i) = static_cast<float>(data->GetValue(pid * dim + component));
    }

    // 用 QR 分解求解最小二乘，数值稳定性比直接求逆更好
    Eigen::Vector4f x = A.colPivHouseholderQr().solve(b);
    return {x[0], x[1], x[2]};
}

Vector3f AdvancedGradientFilter::ComputeFaceAreaVector(Cell* face) {
    Vector3f area(0.0f, 0.0f, 0.0f);
    int npts = face->GetNumberOfPoints();
    if (npts < 3) return area;

    // Newell 法：对任意平面多边形，面积向量 = 0.5 * Σ(p_i × p_{i+1})
    for (int i = 0; i < npts; ++i) {
        const Point& p0 = face->GetPoint(i);
        const Point& p1 = face->GetPoint((i + 1) % npts);
        area += p0.cross(p1);
    }
    area *= 0.5f;
    return area;
}

Vector3f AdvancedGradientFilter::ComputeFaceCenter(Cell* face) {
    Vector3f center(0.0f, 0.0f, 0.0f);
    int npts = face->GetNumberOfPoints();
    if (npts == 0) return center;
    for (int i = 0; i < npts; ++i) {
        const Point& p = face->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
    }
    center[0] /= npts;
    center[1] /= npts;
    center[2] /= npts;
    return center;
}

float AdvancedGradientFilter::ComputeVolumeByGreenGauss(Cell* cell) {
    float volume = 0.0f;
    int numFaces = cell->GetNumberOfFaces();
    for (int f = 0; f < numFaces; ++f) {
        Cell* face = cell->GetFace(f);
        if (face == nullptr) continue;
        Vector3f areaVector = ComputeFaceAreaVector(face);
        Vector3f center = ComputeFaceCenter(face);
        volume += center.dot(areaVector);
    }
    volume /= 3.0f;
    // 面积向量方向可能因网格顶点绕序不一致而为负，取绝对值保证体积为正
    return std::abs(volume);
}


AdvancedGradientFilter::VectorGrad AdvancedGradientFilter::ComputeVectorGradByPlane(Cell* cell, ArrayObject* data) {
    VectorGrad Grad;
    memset(&Grad, 0, sizeof(VectorGrad));

    Vector3f p0 = cell->GetPoint(0);
    int pid0 = cell->GetPointId(0);

    std::vector<float> weights(cell->GetNumberOfPoints() - 2);
    float sum_area = 0.0;

    for (int i = 2; i < cell->GetNumberOfPoints(); ++i) {
        Vector3f p1 = cell->GetPoint(i - 1);
        Vector3f p2 = cell->GetPoint(i);

        Vector3f p10 = p1 - p0;
        Vector3f p20 = p2 - p0;

        Vector3f normal = p10.cross(p20);
        float area = normal.length() * 0.5f;

        if (area < 1e-8) area = 0;

        weights[i - 2] = area;
        sum_area += area;
    }

    for (int i = 2; i < cell->GetNumberOfPoints(); ++i) {
        if (weights[i - 2] == 0.0f) continue;
        weights[i - 2] = weights[i - 2] / sum_area;

        float w = weights[i - 2];

        Vector3f p1 = cell->GetPoint(i - 1);
        Vector3f p2 = cell->GetPoint(i);

        int pid1 = cell->GetPointId(i - 1);
        int pid2 = cell->GetPointId(i);

        Vector3f v0 = p1 - p0;
        Vector3f v1 = p2 - p0;
        float d00 = v0.dot(v0);
        float d01 = v0.dot(v1);
        float d11 = v1.dot(v1);
        float denom = d00 * d11 - d01 * d01;
        float denomr = denom == 0 ? 0.f : 1.f / denom;

        float gx1 = (d11 * v0[0] - d01 * v1[0]) * denomr;
        float gx2 = (d00 * v1[0] - d01 * v0[0]) * denomr;
        float gy1 = (d11 * v0[1] - d01 * v1[1]) * denomr;
        float gy2 = (d00 * v1[1] - d01 * v0[1]) * denomr;
        float gz1 = (d11 * v0[2] - d01 * v1[2]) * denomr;
        float gz2 = (d00 * v1[2] - d01 * v0[2]) * denomr;

        float a0 = data->GetValue(pid0);
        float a1 = data->GetValue(pid1);
        float a2 = data->GetValue(pid2);

        float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
        float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
        float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

        Grad.x.gx += w * gx;
        Grad.x.gy += w * gy;
        Grad.x.gz += w * gz;
    }

    return Grad;
}


bool AdvancedGradientFilter::ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
                                                  int curIndex) {
    int PointNum = surface_Mesh->GetNumberOfPoints();
    int numCells = surface_Mesh->GetNumberOfFaces();

    std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
    std::vector<float> volumes(PointNum, 0.0f);

    auto data = attributeSet->GetAttribute(curIndex).pointer;

    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = surface_Mesh->GetFace(cellId);
        auto grad_x = ComputePointGradient(0, cell, data, 0);
        auto grad_y = ComputePointGradient(0, cell, data, 1);
        auto grad_z = ComputePointGradient(0, cell, data, 2);

        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
            igIndex pid = cell->GetPointId(i);
            for (int d = 0; d < 3; d++) {
                gradients_x[pid][d] += grad_x[d];
                gradients_y[pid][d] += grad_y[d];
                gradients_z[pid][d] += grad_z[d];
            }
        }
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(PointNum);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_POINT, vorticities);

    for (int i = 0; i < PointNum; ++i) {
        double omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
        double omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
        double omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y

        double mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
        vorticities->AddElement3(omega_x, omega_y, omega_z);
        //vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
        //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
        //else vorticities->AddElement3(0, 0, 0);
    }
    return true;
}

bool AdvancedGradientFilter::ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
                                                 int curIndex) {
    int numCells = surface_Mesh->GetNumberOfFaces();
    auto data = attributeSet->GetAttribute(curIndex).pointer;

    FloatArray::Pointer cellVorticities = FloatArray::New();
    cellVorticities->SetDimension(3);
    cellVorticities->Reserve(numCells);
    cellVorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, cellVorticities);

    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = surface_Mesh->GetFace(cellId);

        // 0:surface_Mesh
        // 1:volume_Mesh

        auto grad_x = ComputeCellGradient(0, cell, data, 0);
        auto grad_y = ComputeCellGradient(0, cell, data, 1);
        auto grad_z = ComputeCellGradient(0, cell, data, 2);

        float omega_x = grad_z[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
        float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
        float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y

        float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
        //            if (mag > 1e-6f)
        cellVorticities->AddElement3(omega_x, omega_y, omega_z);
        //            else
        //                vorticities->AddElement3(0, 0, 0);
    }
    return true;
}

bool AdvancedGradientFilter::ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
                                                 int curIndex) {
    int PointNum = volume_Mesh->GetNumberOfPoints();
    int numCells = volume_Mesh->GetNumberOfVolumes();
    auto data = attributeSet->GetAttribute(curIndex).pointer;

    std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
    std::vector<float> volumes(PointNum, 0.0f);

    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = volume_Mesh->GetVolume(cellId);

        auto grad_x = ComputePointGradient(1, cell, data, 0);
        auto grad_y = ComputePointGradient(1, cell, data, 1);
        auto grad_z = ComputePointGradient(1, cell, data, 2);

        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
            igIndex pid = cell->GetPointId(i);
            for (int d = 0; d < 3; d++) {
                gradients_x[pid][d] += grad_x[d];
                gradients_y[pid][d] += grad_y[d];
                gradients_z[pid][d] += grad_z[d];
            }
        }
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(PointNum);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_POINT, vorticities);

    for (int i = 0; i < PointNum; ++i) {
        float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
        float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
        float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y

        float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
        //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
        vorticities->AddElement3(omega_x, omega_y, omega_z);
        //else
        //    vorticities->AddElement3(0, 0, 0);
    }
    return true;
}

bool AdvancedGradientFilter::ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
                                                int curIndex) {
    int PointNum = volume_Mesh->GetNumberOfPoints();
    int numCells = volume_Mesh->GetNumberOfVolumes();
    auto data = attributeSet->GetAttribute(curIndex).pointer;
    data = AttributeCell2Point(volume_Mesh->GetCells(), data, PointNum);

    std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
    std::vector<float> volumes(PointNum, 0.0f);

    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = volume_Mesh->GetVolume(cellId);

        auto grad_x = ComputePointGradient(1, cell, data, 0);
        auto grad_y = ComputePointGradient(1, cell, data, 1);
        auto grad_z = ComputePointGradient(1, cell, data, 2);

        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
            igIndex pid = cell->GetPointId(i);
            for (int d = 0; d < 3; d++) {
                gradients_x[pid][d] += grad_x[d];
                gradients_y[pid][d] += grad_y[d];
                gradients_z[pid][d] += grad_z[d];
            }
        }
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(PointNum);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_POINT, vorticities);

    for (int i = 0; i < PointNum; ++i) {
        float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
        float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
        float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y

        float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
        //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
        vorticities->AddElement3(omega_x, omega_y, omega_z);
        //else
        //    vorticities->AddElement3(0, 0, 0);
    }
    return true;
}

std::array<float, 3> AdvancedGradientFilter::ComputePointGradientWithSurfaceMesh(Cell* cell, ArrayObject::Pointer data,
                                                                         int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    // 计算面积
    //float area = ComputeSurfaceArea(cell);
    float length = ComputeAverageEdgeLength(cell);

    int NumPoints = cell->GetNumberOfPoints();
    for (int i = 0; i < NumPoints; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 3; d++) center[d] /= NumPoints;
    centerValue /= NumPoints;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < NumPoints; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;

        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }

    for (int d = 0; d < NumPoints; d++) gradient[d] /= length;

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputePointGradientWithVolumeMesh(Cell* cell, ArrayObject::Pointer data,
                                                                        int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    float avgEdgeLength = ComputeVolumeAverageEdgeLength(cell);

    int NumPoints = cell->GetNumberOfPoints();
    for (int i = 0; i < NumPoints; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i));
    }

    for (int d = 0; d < 3; d++) center[d] /= NumPoints;
    centerValue /= NumPoints;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < NumPoints; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i)) - centerValue;

        gradient[0] += diff[0] * valDiff;
    }

    gradient[0] /= avgEdgeLength;

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;
    int num = 0;
    num = cell->GetNumberOfPoints();


    for (int i = 0; i < num; i++) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        }
    }
    for (int d = 0; d < 3; d++) center[d] /= cell->GetNumberOfPoints();

    centerValue /= num;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < num; ++i) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        }
    }
    for (int d = 0; d < 3; d++) gradient[d] /= num;

    return gradient;
}

ArrayObject::Pointer AdvancedGradientFilter::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
                                                         size_t PointNum) {
    int dim = OriArray->GetDimension();

    auto NewArray = FloatArray::New();
    NewArray->SetName(OriArray->GetName());
    NewArray->SetDimension(dim);
    NewArray->Reserve(PointNum);

    float scalar[16]{0}, temp[16]{0};
    for (int i = 0; i < PointNum; ++i) { NewArray->AddElement(scalar); }

    std::vector<int> PointAdjNum(PointNum, 0);

    igIndex cell[IGAME_CELL_MAX_SIZE];

    for (int i = 0; i < Cell->GetNumberOfCells(); ++i) {
        int size = Cell->GetCellIds(i, cell);
        OriArray->GetElement(i, scalar);
        for (int j = 0; j < size; ++j) {
            PointAdjNum[cell[j]]++;
            NewArray->GetElement(cell[j], temp);
            for (int d = 0; d < dim; ++d) temp[d] += scalar[d];
            NewArray->SetElement(cell[j], temp);
        }
    }

    for (int i = 0; i < PointNum; ++i) {
        NewArray->GetElement(i, temp);
        for (int d = 0; d < dim; ++d) temp[d] /= PointAdjNum[i];
        NewArray->SetElement(i, temp);
    }

    return NewArray;
}

float AdvancedGradientFilter::ComputeSurfaceArea(Cell* cell) {
    float area = 0.0;
    Point p = cell->GetPoint(0);
    for (int i = 2; i < cell->GetNumberOfPoints(); ++i) {
        Point p1 = cell->GetPoint(i - 1);
        Point p2 = cell->GetPoint(i);
        auto n = (p - p1).cross(p - p2);
        area += n.length() / 2;
    }
    return area;
}

float AdvancedGradientFilter::ComputeTriangleArea(Cell* cell) {
    auto p0 = cell->GetPoint(0);
    auto p1 = cell->GetPoint(1);
    auto p2 = cell->GetPoint(2);

    // 计算两条边的向量
    std::array<float, 3> v1 = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
    std::array<float, 3> v2 = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

    // 计算叉积
    std::array<float, 3> cross = {v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2],
                                  v1[0] * v2[1] - v1[1] * v2[0]};

    // 面积 = 叉积模长 / 2
    float area = 0.5f * std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
    return area;
}

float AdvancedGradientFilter::ComputeVolumeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float AdvancedGradientFilter::ComputeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float AdvancedGradientFilter::ComputeTetVolume(Cell* cell) {
    auto p0 = cell->GetPoint(0);
    auto p1 = cell->GetPoint(1);
    auto p2 = cell->GetPoint(2);
    auto p3 = cell->GetPoint(3);

    std::array<float, 3> a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
    std::array<float, 3> b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
    std::array<float, 3> c = {p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]};

    std::array<float, 3> cross_bc = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2], b[0] * c[1] - b[1] * c[0]};

    float dot_a = a[0] * cross_bc[0] + a[1] * cross_bc[1] + a[2] * cross_bc[2];
    return std::abs(dot_a) / 6.0f;
}

bool AdvancedGradientFilter::InverseMatrix4x4(const float in[4][4], float out[4][4]) {
    float aug[4][8] = {0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            aug[i][j] = in[i][j];
            aug[i][j + 4] = (i == j) ? 1.0f : 0.0f;
        }
    }

    for (int col = 0; col < 4; ++col) {
        int max_row = col;
        for (int i = col + 1; i < 4; ++i) {
            if (std::abs(aug[i][col]) > std::abs(aug[max_row][col])) { max_row = i; }
        }

        // 如果最大主元接近零，矩阵奇异
        if (std::abs(aug[max_row][col]) < 1e-12f) { return false; }

        if (max_row != col) {
            for (int j = col; j < 8; ++j) { std::swap(aug[col][j], aug[max_row][j]); }
        }

        float pivot = aug[col][col];
        for (int j = col; j < 8; ++j) { aug[col][j] /= pivot; }

        for (int i = 0; i < 4; ++i) {
            if (i != col && std::abs(aug[i][col]) > 1e-12f) {
                float factor = aug[i][col];
                for (int j = col; j < 8; ++j) { aug[i][j] -= factor * aug[col][j]; }
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) { out[i][j] = aug[i][j + 4]; }
    }
    return true;
}

AdvancedGradientFilter::AdvancedGradientFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

std::array<float, 3> AdvancedGradientFilter::ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    // 计算三角形面积
    float triangleArea = ComputeTriangleArea(cell);
    //if (triangleArea < 1e-10f) return {0.0f, 0.0f, 0.0f}; // 避免除零

    // 计算三角形中心点和中心值
    for (int i = 0; i < 3; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 3; d++) center[d] /= 3.0f;
    centerValue /= 3.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};

    // 计算梯度，并按面积归一化
    for (int i = 0; i < 3; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;

        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }

    // 归一化梯度，使其与单元大小无关
    for (int d = 0; d < 3; d++) gradient[d] /= (triangleArea); // 避免除零

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    for (int i = 0; i < 4; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 4; d++) center[d] /= 3.0f;
    centerValue /= 3.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 3; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }
    for (int d = 0; d < 3; d++) gradient[d] /= 4.0f;

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    for (int i = 0; i < cell->GetNumberOfPoints(); i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 3; d++) center[d] /= cell->GetNumberOfPoints();
    centerValue /= cell->GetNumberOfPoints();

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }
    for (int d = 0; d < 3; d++) gradient[d] /= cell->GetNumberOfPoints();

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    float tetVolume = ComputeTetVolume(cell);
    //if (tetVolume < 1e-10f) return {0.0f, 0.0f, 0.0f};

    float avgEdgeLength = ComputeAverageEdgeLength(cell); // 计算平均边长

    for (int i = 0; i < 4; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 3; d++) center[d] /= 4.0f;
    centerValue /= 4.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < 4; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;

        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }

    for (int d = 0; d < 3; d++) gradient[d] /= (avgEdgeLength); // 改为边长归一化

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    for (int i = 0; i < 4; i++) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        }
    }
    for (int d = 0; d < 3; d++) center[d] /= 4.0f;
    centerValue /= 4.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 4; ++i) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        }
    }
    for (int d = 0; d < 3; d++) gradient[d] /= 4.0f;
    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    float avgEdgeLength = ComputeAverageEdgeLength(cell);

    for (int i = 0; i < 8; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }

    for (int d = 0; d < 3; d++) center[d] /= 8.0f;
    centerValue /= 8.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < 8; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;

        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }

    for (int d = 0; d < 3; d++) gradient[d] /= avgEdgeLength;

    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    for (int i = 0; i < 8; i++) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            center[0] += p[0];
            center[1] += p[1];
            center[2] += p[2];
            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
        }
    }
    for (int d = 0; d < 3; d++) center[d] /= 4.0f;
    centerValue /= 8.0f;

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 8; ++i) {
        if (type == 1) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        } else if (type == 0) {
            auto p = cell->GetPoint(i);
            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
        }
    }
    for (int d = 0; d < 3; d++) gradient[d] /= 8.0f;
    return gradient;
}

std::array<float, 3> AdvancedGradientFilter::ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    int numOfPoints = cell->GetNumberOfPoints();

    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    float avgEdgeLength = ComputeAverageEdgeLength(cell);

    for (int i = 0; i < numOfPoints; i++) {
        auto p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
    }
    for (int d = 0; d < 3; d++) center[d] /= static_cast<float>(numOfPoints);
    centerValue /= static_cast<float>(numOfPoints);

    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < numOfPoints; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }
    for (int d = 0; d < 3; d++) gradient[d] /= avgEdgeLength;

    return gradient;
}


IGAME_NAMESPACE_END