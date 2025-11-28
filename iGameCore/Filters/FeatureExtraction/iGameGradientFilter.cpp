#include "iGameGradientFilter.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>
IGAME_NAMESPACE_BEGIN

bool GradientFilter::Execute() {

    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) return false;
        if (curIndex == -1 && name == "") return false;
        if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(name);
        if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) return false;

        int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        if (dim != 1) { return false; }
        return true;
    };

    SetOutput(input);

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            surface_Mesh = DynamicCast<SurfaceMesh>(input);
            if (!CheckType()) return false;
            return ComputeGradientWithSurfaceMesh(surface_Mesh, attributeSet, curIndex);
        } break;
        case IG_VOLUME_MESH: {
            return false;
            // volume_Mesh = DynamicCast<VolumeMesh>(input);
            // if (!CheckType()) return false;
            // return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);

        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            surface_Mesh = mesh->TransferToSurfaceMesh();
            volume_Mesh = mesh->TransferToVolumeMesh();

            if (surface_Mesh) {
                if (!CheckType()) return false;
                return ComputeGradientWithSurfaceMesh(surface_Mesh, attributeSet, curIndex);
            }

            if (volume_Mesh) {
                return false;
                // if (!CheckType()) return false;
                // return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);
            }
        } break;
        default:
            return false;
    }

    return true;
}

std::array<float, 3> GradientFilter::ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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
}

std::array<float, 3> GradientFilter::ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    switch (cell->GetCellType()) {
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
            return ComputePointGradient(cell, data, dim);
        case IG_TETRA:
        case IG_HEXAHEDRON:
        case IG_POLYHEDRON:
            return ComputeHexPointGradient(cell, data, dim);
    }

    return ComputePointGradient(cell, data, dim);
}

std::array<float, 3> GradientFilter::ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    switch (cell->GetCellType()) {
        case IG_TETRA: // 纯四面体
            return ComputeTetCellGradient(type, cell, data, dim);
        case IG_HEXAHEDRON: // 纯六面体
            return ComputeHexCellGradient(type, cell, data, dim);
        default: // 其他
            return ComputePolyCellGradient(type, cell, data, dim);
    }
}

bool GradientFilter::ComputeGradientWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                    int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    FloatArray::Pointer gradient = FloatArray::New();
    gradient->SetDimension(3);
    gradient->Reserve(NumCells);
    gradient->SetName("gradient");
    attributeSet->AddScalar(IG_CELL, gradient);
    std::vector<std::array<double, 3>> gradient_values(NumCells);

    int progress = 0;
    int block = NumCells / 100;
    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        auto cell = Mesh->GetFace(cellId);

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

                float a0 = Data->GetValue(pid0);
                float a1 = Data->GetValue(pid1);
                float a2 = Data->GetValue(pid2);

                float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
                float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
                float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

                grad.x.gx = gx;
                grad.x.gy = gy;
                grad.x.gz = gz;
                break;
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

                float a0_1 = Data->GetValue(pid0);
                float a1_1 = Data->GetValue(pid1);
                float a2_1 = Data->GetValue(pid2);

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

                float a0_2 = Data->GetValue(pid0);
                float a2_2 = Data->GetValue(pid2);
                float a3_2 = Data->GetValue(pid3);

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
                    centerValue += Data->GetValue(cell->GetPointId(i));
                }

                center[0] /= numPoints;
                center[1] /= numPoints;
                center[2] /= numPoints;
                centerValue /= numPoints;

                std::array<float, 3> gradientApprox = {0.0f, 0.0f, 0.0f};
                for (int i = 0; i < numPoints; ++i) {
                    auto p = cell->GetPoint(i);
                    std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
                    float valDiff = Data->GetValue(cell->GetPointId(i)) - centerValue;

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

        gradient_values[cellId][0] = grad.x.gx;
        gradient_values[cellId][1] = grad.x.gy;
        gradient_values[cellId][2] = grad.x.gz;
    }
        // auto grad = ComputeVectorGradByPlane(cell, Data);

        //
        // double omega_x = grad.x.gx;
        // double omega_y = grad.x.gy;
        // double omega_z = grad.x.gz;
        // gradient_values[cellId][0] = omega_x;
        // gradient_values[cellId][1] = omega_y;
        // gradient_values[cellId][2] = omega_z;

        // gradient->AddElement3(omega_x, omega_y, omega_z);
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

    std::vector<double> xs(NumCells), ys(NumCells), zs(NumCells);
    for (int i = 0; i < NumCells; ++i) {
        xs[i] = gradient_values[i][0];
        ys[i] = gradient_values[i][1];
        zs[i] = gradient_values[i][2];
    }
    clamp_by_quantile(xs, 0.000, 1);
    clamp_by_quantile(ys, 0.000, 1);
    clamp_by_quantile(zs, 0.000, 1);

    for (int i = 0; i < NumCells; ++i) {
        gradient->AddElement3(xs[i], ys[i], zs[i]);
    }

    UpdateProgress(1.0);
    return true;
    // double mean_x = 0.0, mean_y = 0.0, mean_z = 0.0;
    // for (int i = 0; i < NumCells; ++i) {
    //     mean_x += gradient_values[i][0];
    //     mean_y += gradient_values[i][1];
    //     mean_z += gradient_values[i][2];
    // }
    // mean_x /= NumCells;
    // mean_y /= NumCells;
    // mean_z /= NumCells;
    //
    // double std_x = 0.0, std_y = 0.0, std_z = 0.0;
    // for (int i = 0; i < NumCells; ++i) {
    //     std_x += (gradient_values[i][0] - mean_x) * (gradient_values[i][0] - mean_x);
    //     std_y += (gradient_values[i][1] - mean_y) * (gradient_values[i][1] - mean_y);
    //     std_z += (gradient_values[i][2] - mean_z) * (gradient_values[i][2] - mean_z);
    // }
    // std_x = std::sqrt(std_x / NumCells);
    // std_y = std::sqrt(std_y / NumCells);
    // std_z = std::sqrt(std_z / NumCells);
    //
    // double threshold_x = 5.0 * std_x;
    // double threshold_y = 5.0 * std_y;
    // double threshold_z = 5.0 * std_z;
    //
    // for (int i = 0; i < NumCells; ++i) {
    //     double omega_x = gradient_values[i][0];
    //     double omega_y = gradient_values[i][1];
    //     double omega_z = gradient_values[i][2];
    //
    //     if (std::abs(omega_x - mean_x) > threshold_x) {
    //         omega_x = mean_x + (omega_x - mean_x) / std::abs(omega_x - mean_x) * threshold_x;
    //     }
    //
    //     if (std::abs(omega_y - mean_y) > threshold_y) {
    //         omega_y = mean_y + (omega_y - mean_y) / std::abs(omega_y - mean_y) * threshold_y;
    //     }
    //
    //     if (std::abs(omega_z - mean_z) > threshold_z) {
    //         omega_z = mean_z + (omega_z - mean_z) / std::abs(omega_z - mean_z) * threshold_z;
    //     }
    //
    //     gradient->AddElement3(omega_x, omega_y, omega_z);
    // }
    //
    // return true;
}

bool GradientFilter::ComputeGradientWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                   int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfVolumes();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    std::vector<std::array<float, 3>> gradients_x(NumPoints, {0, 0, 0});

    int progress = 0;
    int block = NumCells / 90;
    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        auto cell = volume_Mesh->GetVolume(cellId);

        auto grad_x = ComputePointGradientWithVolumeMesh(cell, Data, 0);

        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
            igIndex pid = cell->GetPointId(i);
            for (int d = 0; d < 3; d++) { gradients_x[pid][d] += grad_x[d]; }
        }
    }

    FloatArray::Pointer gradient = FloatArray::New();
    gradient->SetDimension(3);
    gradient->Reserve(NumPoints);
    gradient->SetName("gradient");
    attributeSet->AddScalar(IG_POINT, gradient);

    ResetProgress();
    progress = 0;
    block = NumPoints / 100;
    for (int i = 0; i < NumPoints; ++i) {
        if (i > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        float omega_x = gradients_x[i][0];
        float omega_y = gradients_x[i][1];
        float omega_z = gradients_x[i][2];
        gradient->AddElement3(omega_x, omega_y, omega_z);
    }
    return true;
}


GradientFilter::VectorGrad GradientFilter::ComputeVectorGradByPlane(Cell* cell, ArrayObject* data) {
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


bool GradientFilter::ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
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

bool GradientFilter::ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
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

bool GradientFilter::ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
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

bool GradientFilter::ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
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

std::array<float, 3> GradientFilter::ComputePointGradientWithSurfaceMesh(Cell* cell, ArrayObject::Pointer data,
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

std::array<float, 3> GradientFilter::ComputePointGradientWithVolumeMesh(Cell* cell, ArrayObject::Pointer data,
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

std::array<float, 3> GradientFilter::ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

ArrayObject::Pointer GradientFilter::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
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

float GradientFilter::ComputeSurfaceArea(Cell* cell) {
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

float GradientFilter::ComputeTriangleArea(Cell* cell) {
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

float GradientFilter::ComputeVolumeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float GradientFilter::ComputeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float GradientFilter::ComputeTetVolume(Cell* cell) {
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

bool GradientFilter::InverseMatrix4x4(const float in[4][4], float out[4][4]) {
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

GradientFilter::GradientFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

std::array<float, 3> GradientFilter::ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> GradientFilter::ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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