#include "iGameVortexFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include <algorithm>
#include <cmath>
IGAME_NAMESPACE_BEGIN
bool VortexFilter::Execute()  {

    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (!attributeSet) return false;
        curIndex = input->GetAttributeIndex();
        curDim = input->GetAttributeDimension();
        if (curIndex < 0) return false;

        int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        if (dim != 3) { return false; }
        return true;
    };

    SetOutput(input);

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            surface_Mesh = DynamicCast<SurfaceMesh>(input);
            if (!CheckType()) return false;

            return ComputeVorticityWithSurfaceMesh2(surface_Mesh, attributeSet, curIndex);
        } break;
        case IG_VOLUME_MESH: {
            volume_Mesh = DynamicCast<VolumeMesh>(input);
            if (!CheckType()) return false;
            return ComputeVorticityWithVolumeMesh(volume_Mesh, attributeSet, curIndex);

        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);

            surface_Mesh = mesh->TransferToSurfaceMesh();

            if (surface_Mesh) {
                if (!CheckType()) return false;
                return ComputeVorticityWithSurfaceMesh2(surface_Mesh, attributeSet, curIndex);
            }

            if (mesh) {
                if (!CheckType()) return false;
                return ComputeVorticityWithUnstructuredMesh(mesh, attributeSet, curIndex);
            }

        } break;
        default:
            return false;
    }

    return true;
}

float VortexFilter::ComputeCellVolume(Cell* cell) {
    switch (cell->GetCellType()) {
        case IG_TRIANGLE: {
            double area = 0;
            Vector3d n1 = cell->GetPoint(0) - cell->GetPoint(1);
            Vector3d n2 = cell->GetPoint(0) - cell->GetPoint(2);
            area = n1.cross(n2).length() / 2;
            return area;
        }
        case IG_TETRA:
            return ComputeTetVolume(cell);
        case IG_HEXAHEDRON:
            //return ComputeHexVolume(cell);
        default:
            return 1.0f;
    }
}

std::array<float, 3> VortexFilter::ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    switch (cell->GetCellType()) {
        case IG_TETRA: // 纯四面体
            return ComputeTetCellGradient(type, cell, data, dim);
        case IG_HEXAHEDRON: // 纯六面体
            return ComputeHexCellGradient(type, cell, data, dim);
        default: // 其他
            return ComputePolyCellGradient(type, cell, data, dim);
    }
}

bool VortexFilter::ComputeVorticityWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                   int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    //FloatArray::Pointer vorticities = FloatArray::New();
    //vorticities->SetDimension(3);
    //vorticities->Reserve(NumPoints);
    //vorticities->SetName("vorticities");
    //attributeSet->AddScalar(IG_POINT, vorticities);

    std::vector<std::array<float, 3>> gradients_x(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(NumPoints, {0, 0, 0});

    //for (int cellId = 0; cellId < NumCells; ++cellId) {
    //    auto cell = Mesh->GetFace(cellId);
    //    auto grad_x = ComputePointGradientWithSurfaceMesh(cell, Data, 0);
    //    auto grad_y = ComputePointGradientWithSurfaceMesh(cell, Data, 1);
    //    auto grad_z = ComputePointGradientWithSurfaceMesh(cell, Data, 2);

    //    for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
    //        igIndex pid = cell->GetPointId(i);
    //        for (int d = 0; d < 3; d++) {
    //            gradients_x[pid][d] += grad_x[d];
    //            gradients_y[pid][d] += grad_y[d];
    //            gradients_z[pid][d] += grad_z[d];
    //        }
    //    }
    //}

    //for (int i = 0; i < NumPoints; ++i) {
    //    double omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
    //    double omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
    //    double omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y

    //    double mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
    //    vorticities->AddElement3(omega_x, omega_y, omega_z);
    //}

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(NumCells);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, vorticities);

    for (int cellId = 0; cellId < NumCells; ++cellId) {
        auto cell = Mesh->GetFace(cellId);
        auto grad_x = ComputePointGradientWithSurfaceMesh(cell, Data, 0);
        auto grad_y = ComputePointGradientWithSurfaceMesh(cell, Data, 1);
        auto grad_z = ComputePointGradientWithSurfaceMesh(cell, Data, 2);

        double omega_x = grad_z[1] - grad_y[2];         // ∂vz/∂y - ∂vy/∂z
        double omega_y = grad_x[2] - grad_z[0];         // ∂vx/∂z - ∂vz/∂x
        double omega_z = grad_y[0] - grad_x[1];         // ∂vy/∂x - ∂vx/∂y

        vorticities->AddElement3(omega_x, omega_y, omega_z);
    }

    return true;
}

bool VortexFilter::ComputeVorticityWithSurfaceMesh2(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                    int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    std::vector<double> omega_x_values(NumCells);
    std::vector<double> omega_y_values(NumCells);
    std::vector<double> omega_z_values(NumCells);

    //FloatArray::Pointer vorticities = FloatArray::New();
    //vorticities->SetDimension(3);
    //vorticities->Reserve(NumPoints);
    //vorticities->SetName("vorticities");
    //attributeSet->AddScalar(IG_POINT, vorticities);

    //std::vector<VectorGrad> Gradients;

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(NumCells);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, vorticities);

    int progress = 0;
    int block = NumCells / 100;
    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        auto cell = Mesh->GetFace(cellId);
        auto grad = ComputeVectorGradByPlane(cell, Data);
        //auto center = ComputeCenter(cell);

        double omega_x = grad.z.gy - grad.y.gz; // ∂vz/∂y - ∂vy/∂z
        double omega_y = grad.x.gz - grad.z.gx; // ∂vx/∂z - ∂vz/∂x
        double omega_z = grad.y.gx - grad.x.gy; // ∂vy/∂x - ∂vx/∂y

        omega_x_values[cellId] = omega_x;
        omega_y_values[cellId] = omega_y;
        omega_z_values[cellId] = omega_z;

        //float va1 = grad.x.gx* center[0] + grad.x.gy* center[1] + grad.x.gz* center[2] + grad.x.gw;
        //float va2 = grad.y.gx* center[0] + grad.y.gy* center[1] + grad.y.gz* center[2] + grad.y.gw;
        //float va3 = grad.z.gx* center[0] + grad.z.gy* center[1] + grad.z.gz* center[2] + grad.z.gw;
        vorticities->AddElement3(omega_x, omega_y, omega_z);
        //vorticities->AddElement3(va1, va2, va3);
    }
    auto clamp_by_quantile = [](std::vector<double>& v, double qlo, double qhi) {
        if (v.empty()) return;
        std::vector<double> tmp = v;
        auto nth_q = [&](double q) {
            size_t idx = size_t(std::clamp(q, 0.0, 1.0) * (tmp.size() - 1));
            std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
            return tmp[idx];
        };
        double lo = nth_q(qlo);
        double hi = nth_q(qhi);
        if (lo > hi) std::swap(lo, hi);
        for (double& x : v) {
            if (x < lo) x = lo;
            else if (x > hi) x = hi;
        }
    };

    clamp_by_quantile(omega_x_values, 0.0001, 0.9999);
    clamp_by_quantile(omega_y_values, 0.0001, 0.9999);
    clamp_by_quantile(omega_z_values, 0.0001, 0.9999);

    for (int i = 0; i < NumCells; ++i) {
        vorticities->AddElement3(omega_x_values[i],
                                 omega_y_values[i],
                                 omega_z_values[i]);
    }

    UpdateProgress(1.0);
    // return true;

    //for (int cellId = 0; cellId < NumCells; ++cellId) {
    //    auto cell = Mesh->GetFace(cellId);
    //    auto grad_x = ComputePointGradientWithSurfaceMesh(cell, Data, 0);
    //    auto grad_y = ComputePointGradientWithSurfaceMesh(cell, Data, 1);
    //    auto grad_z = ComputePointGradientWithSurfaceMesh(cell, Data, 2);

    //    for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
    //        igIndex pid = cell->GetPointId(i);
    //        for (int d = 0; d < 3; d++) {
    //            gradients_x[pid][d] += grad_x[d];
    //            gradients_y[pid][d] += grad_y[d];
    //            gradients_z[pid][d] += grad_z[d];
    //        }
    //    }
    //}

    //for (int i = 0; i < NumPoints; ++i) {
    //    double omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
    //    double omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
    //    double omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y

    //    double mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
    //    vorticities->AddElement3(omega_x, omega_y, omega_z);
    //}

    return true;
}

bool VortexFilter::ComputeVorticityWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                  int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfVolumes();

    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetVolumes(), Data, NumPoints);
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(NumCells);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, vorticities);

    std::vector<std::array<float, 3>> gradients_x(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(NumPoints, {0, 0, 0});

    std::vector<double> omega_x_values(NumCells);
    std::vector<double> omega_y_values(NumCells);
    std::vector<double> omega_z_values(NumCells);

    int progress = 0;
    int block = NumCells / 100;

    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }
        // std::cout<<"11111111111"<<std::endl;
        auto cell = Mesh->GetVolume(cellId);

        // std::cout<<"1122222222"<<std::endl;

        VectorGrad grad;
        switch (cell->GetCellType()) {
            case IG_TETRA: {
                grad = ComputeVectorGradByTetra(cell, Data);
                break;
            }
            case IG_HEXAHEDRON: {
                grad = ComputeVectorGradByHex(cell, Data);
                break;
            }
            case IG_PYRAMID:
            case IG_PRISM: {
                grad = ComputeVectorGradByPolyhedron(cell, Data);
                break;
            }
            case IG_POLYHEDRON: {
                grad = ComputeVectorGradByPolyhedron(cell, Data);
                break;
            }
            default: {
                grad = ComputeVectorGradByPlane(cell, Data);
                break;
            }
        }

        // auto grad_x = ComputePointGradientWithVolumeMesh(cell, Data, 0);
        // auto grad_y = ComputePointGradientWithVolumeMesh(cell, Data, 1);
        // auto grad_z = ComputePointGradientWithVolumeMesh(cell, Data, 2);

        double omega_x = grad.z.gy - grad.y.gz; // ∂vz/∂y - ∂vy/∂z
        double omega_y = grad.x.gz - grad.z.gx; // ∂vx/∂z - ∂vz/∂x
        double omega_z = grad.y.gx - grad.x.gy; // ∂vy/∂x - ∂vx/∂y

        omega_x_values[cellId] = omega_x;
        omega_y_values[cellId] = omega_y;
        omega_z_values[cellId] = omega_z;

        // vorticities->AddElement3(omega_x, omega_y, omega_z);

        // for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
        //     igIndex pid = cell->GetPointId(i);
        //     for (int d = 0; d < 3; d++) {
        //         gradients_x[pid][d] += grad_x[d];
        //         gradients_y[pid][d] += grad_y[d];
        //         gradients_z[pid][d] += grad_z[d];
        //     }
        // }
    }
    // std::cout<<"4"<<std::endl;
    // double mean_omega_x = 0.0, mean_omega_y = 0.0, mean_omega_z = 0.0;
    // for (int i = 0; i < NumCells; ++i) {
    //     mean_omega_x += omega_x_values[i];
    //     mean_omega_y += omega_y_values[i];
    //     mean_omega_z += omega_z_values[i];
    // }
    // mean_omega_x /= NumCells;
    // mean_omega_y /= NumCells;
    // mean_omega_z /= NumCells;
    //
    // double std_omega_x = 0.0, std_omega_y = 0.0, std_omega_z = 0.0;
    // for (int i = 0; i < NumCells; ++i) {
    //     std_omega_x += (omega_x_values[i] - mean_omega_x) * (omega_x_values[i] - mean_omega_x);
    //     std_omega_y += (omega_y_values[i] - mean_omega_y) * (omega_y_values[i] - mean_omega_y);
    //     std_omega_z += (omega_z_values[i] - mean_omega_z) * (omega_z_values[i] - mean_omega_z);
    // }
    // std_omega_x = std::sqrt(std_omega_x / NumCells);
    // std_omega_y = std::sqrt(std_omega_y / NumCells);
    // std_omega_z = std::sqrt(std_omega_z / NumCells);
    //
    // double threshold_omega_x = 5.0 * std_omega_x;
    // double threshold_omega_y = 5.0 * std_omega_y;
    // double threshold_omega_z = 5.0 * std_omega_z;
    //
    // for (int i = 0; i < NumCells; ++i) {
    //     double omega_x = omega_x_values[i];
    //     double omega_y = omega_y_values[i];
    //     double omega_z = omega_z_values[i];
    //
    //     if (std::abs(omega_x - mean_omega_x) > threshold_omega_x) {
    //         omega_x = mean_omega_x + (omega_x - mean_omega_x) / std::abs(omega_x - mean_omega_x) * threshold_omega_x;
    //     }
    //
    //     if (std::abs(omega_y - mean_omega_y) > threshold_omega_y) {
    //         omega_y = mean_omega_y + (omega_y - mean_omega_y) / std::abs(omega_y - mean_omega_y) * threshold_omega_y;
    //     }
    //
    //     if (std::abs(omega_z - mean_omega_z) > threshold_omega_z) {
    //         omega_z = mean_omega_z + (omega_z - mean_omega_z) / std::abs(omega_z - mean_omega_z) * threshold_omega_z;
    //     }
    //
    //     vorticities->AddElement3(omega_x, omega_y, omega_z);
    // }
    auto clamp_by_quantile = [](std::vector<double>& v, double qlo, double qhi) {
        if (v.empty()) return;
        std::vector<double> tmp = v;
        auto nth_q = [&](double q) {
            size_t idx = size_t(std::clamp(q, 0.0, 1.0) * (tmp.size() - 1));
            std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
            return tmp[idx];
        };
        double lo = nth_q(qlo);
        double hi = nth_q(qhi);
        if (lo > hi) std::swap(lo, hi);
        for (double& x : v) {
            if (x < lo) x = lo;
            else if (x > hi) x = hi;
        }
    };

    clamp_by_quantile(omega_x_values, 0.000005, 0.999995);
    clamp_by_quantile(omega_y_values, 0.000005, 0.999995);
    clamp_by_quantile(omega_z_values, 0.000005, 0.999995);

    for (int i = 0; i < NumCells; ++i) {
        vorticities->AddElement3(omega_x_values[i],
                                 omega_y_values[i],
                                 omega_z_values[i]);
    }

    UpdateProgress(1.0);
    return true;

    // FloatArray::Pointer vorticities = FloatArray::New();
    // vorticities->SetDimension(3);
    // vorticities->Reserve(NumPoints);
    // vorticities->SetName("vorticities");
    // attributeSet->AddScalar(IG_POINT, vorticities);
    //
    // ResetProgress();
    // progress = 0;
    // block = NumPoints / 100;
    // for (int i = 0; i < NumPoints; ++i) {
    //     if (i > block * progress) {
    //         progress++;
    //         UpdateProgress(progress * 0.01);
    //     }
    //
    //     float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
    //     float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
    //     float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y
    //
    //     float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
    //     vorticities->AddElement3(omega_x, omega_y, omega_z);
    // }
    return true;
}

bool VortexFilter::ComputeVorticityWithUnstructuredMesh(UnstructuredMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfCells();

    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetCells(), Data, NumPoints);
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(NumCells);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, vorticities);

    std::vector<std::array<float, 3>> gradients_x(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(NumPoints, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(NumPoints, {0, 0, 0});

    std::vector<double> omega_x_values(NumCells);
    std::vector<double> omega_y_values(NumCells);
    std::vector<double> omega_z_values(NumCells);

    int progress = 0;
    int block = NumCells / 100;

    for (int cellId = 0; cellId < NumCells; ++cellId) {
        if (cellId > block * progress) {
            progress++;
            UpdateProgress(progress * 0.01);
        }

        auto cell = Mesh->GetCell(cellId);

        VectorGrad grad;
        switch (cell->GetCellType()) {
            case IG_TETRA: {

                grad = ComputeVectorGradByTetra(cell, Data);
                break;
            }
            case IG_HEXAHEDRON: {

                grad = ComputeVectorGradByHex(cell, Data);
                break;
            }
            case IG_PYRAMID:
            case IG_PRISM: {

                grad = ComputeVectorGradByPolyhedron(cell, Data);
                break;
            }
            case IG_POLYHEDRON: {

                grad = ComputeVectorGradByPolyhedron(cell, Data);
                break;
            }
            default: {

                grad = ComputeVectorGradByPlane(cell, Data);
                break;
            }
        }

        // auto grad_x = ComputePointGradientWithVolumeMesh(cell, Data, 0);
        // auto grad_y = ComputePointGradientWithVolumeMesh(cell, Data, 1);
        // auto grad_z = ComputePointGradientWithVolumeMesh(cell, Data, 2);

        double omega_x = grad.z.gy - grad.y.gz; // ∂vz/∂y - ∂vy/∂z
        double omega_y = grad.x.gz - grad.z.gx; // ∂vx/∂z - ∂vz/∂x
        double omega_z = grad.y.gx - grad.x.gy; // ∂vy/∂x - ∂vx/∂y

        omega_x_values[cellId] = omega_x;
        omega_y_values[cellId] = omega_y;
        omega_z_values[cellId] = omega_z;

        // vorticities->AddElement3(omega_x, omega_y, omega_z);

        // for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
        //     igIndex pid = cell->GetPointId(i);
        //     for (int d = 0; d < 3; d++) {
        //         gradients_x[pid][d] += grad_x[d];
        //         gradients_y[pid][d] += grad_y[d];
        //         gradients_z[pid][d] += grad_z[d];
        //     }
        // }
    }
    auto clamp_by_quantile = [](std::vector<double>& v, double qlo, double qhi) {
        if (v.empty()) return;
        std::vector<double> tmp = v;
        auto nth_q = [&](double q) {
            size_t idx = size_t(std::clamp(q, 0.0, 1.0) * (tmp.size() - 1));
            std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
            return tmp[idx];
        };
        double lo = nth_q(qlo);
        double hi = nth_q(qhi);
        if (lo > hi) std::swap(lo, hi);
        for (double& x : v) {
            if (x < lo) x = lo;
            else if (x > hi) x = hi;
        }
    };

    clamp_by_quantile(omega_x_values, 0.000005, 0.999995);
    clamp_by_quantile(omega_y_values, 0.000005, 0.999995);
    clamp_by_quantile(omega_z_values, 0.000005, 0.999995);

    for (int i = 0; i < NumCells; ++i) {
        vorticities->AddElement3(omega_x_values[i],
                                 omega_y_values[i],
                                 omega_z_values[i]);
    }

    UpdateProgress(1.0);
    return true;
}

bool VortexFilter::ComputeVorticityWithVolumeMesh2(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                   int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    int NumCells = Mesh->GetNumberOfFaces();
    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        Data = AttributeCell2Point(Mesh->GetFaces(), Data, NumPoints);
    }

    FloatArray::Pointer vorticities = FloatArray::New();
    vorticities->SetDimension(3);
    vorticities->Reserve(NumCells);
    vorticities->SetName("vorticities");
    attributeSet->AddScalar(IG_CELL, vorticities);

    for (int cellId = 0; cellId < NumCells; ++cellId) {
        auto cell = Mesh->GetVolume(cellId);
        auto grad = ComputeVectorGradByTetra(cell, Data);

        double omega_x = grad.z.gy - grad.y.gz; // ∂vz/∂y - ∂vy/∂z
        double omega_y = grad.x.gz - grad.z.gx; // ∂vx/∂z - ∂vz/∂x
        double omega_z = grad.y.gx - grad.x.gy; // ∂vy/∂x - ∂vx/∂y

        vorticities->AddElement3(omega_x, omega_y, omega_z);
    }

    return true;
}

VortexFilter::VectorGrad VortexFilter::ComputeVectorGradByPlane(Cell* cell, ArrayObject* data) {
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

        float a0 = data->GetValue(pid0 * 3), a1 = data->GetValue(pid1 * 3), a2 = data->GetValue(pid2 * 3);

        float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
        float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
        float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

        Grad.x.gx += w * gx;
        Grad.x.gy += w * gy;
        Grad.x.gz += w * gz;

        a0 = data->GetValue(pid0 * 3 + 1), a1 = data->GetValue(pid1 * 3 + 1), a2 = data->GetValue(pid2 * 3 + 1);

        gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
        gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
        gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

        Grad.y.gx += w * gx;
        Grad.y.gy += w * gy;
        Grad.y.gz += w * gz;

        a0 = data->GetValue(pid0 * 3 + 2), a1 = data->GetValue(pid1 * 3 + 2), a2 = data->GetValue(pid2 * 3 + 2);

        gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
        gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
        gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);

        Grad.z.gx += w * gx;
        Grad.z.gy += w * gy;
        Grad.z.gz += w * gz;
    }

    return Grad;
}

Vector3f VortexFilter::ComputeCenter(Cell* cell) {
    Vector3f c(0,0,0);
    for (int i = 0; i < cell->GetNumberOfPoints(); i++) {
        c += cell->GetPoint(i);
    }
    c /= cell->GetNumberOfPoints();
    return c;
 }

bool VortexFilter::ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
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

VortexFilter::VectorGrad VortexFilter::ComputeVectorGradByTetra(Cell* cell, ArrayObject* data) {

    Eigen::Matrix3f m = Eigen::Matrix3f::Zero();
    //float m[3][3];
    float derivs[12];
    InterpolationDerivs(derivs);

    //memset(m, 0, 9 * sizeof(float));

    for (int j = 0; j < 4; j++) {
        auto& x = cell->GetPoint(j);
        for (int i = 0; i < 3; i++) {
            m(0, i) += x[i] * derivs[j];
            m(1, i) += x[i] * derivs[4 + j];
            m(2, i) += x[i] * derivs[8 + j];
        }
    }

    m = m.inverse();

    Vector3f sum;
    for (int k = 0; k < 3; k++)
    {
        sum[0] = sum[1] = sum[2] = 0.0;
        for (int i = 0; i < 4; i++)
        {
            float value = data->GetValue(3 * i + k);
            sum[0] += derivs[i] * value;
            sum[1] += derivs[4 + i] * value;
            sum[2] += derivs[8 + i] * value;
        }

        for (int j = 0; j < 3; j++) {
            derivs[3 * k + j] = sum[0] * m(j, 0) + sum[1] * m(j, 1) + sum[2] * m(j, 2);
        }
    }
    VectorGrad Grad;
    Grad.x.gx = derivs[0];
    Grad.x.gy = derivs[1];
    Grad.x.gz = derivs[2];
    Grad.y.gx = derivs[3];
    Grad.y.gy = derivs[4];
    Grad.y.gz = derivs[5];
    Grad.z.gx = derivs[6];
    Grad.z.gy = derivs[7];
    Grad.z.gz = derivs[8];
    return Grad;
}

VortexFilter::VectorGrad VortexFilter::ComputeVectorGradByHex(Cell* cell, ArrayObject* data) {

    Vector3f center(0, 0, 0);
    Vector3f vel_center(0, 0, 0);
    int numPoints = cell->GetNumberOfPoints();

    for (int i = 0; i < numPoints; ++i) {
        auto p = cell->GetPoint(i);
        center += p;
        vel_center[0] += data->GetValue(cell->GetPointId(i) * 3 + 0);
        vel_center[1] += data->GetValue(cell->GetPointId(i) * 3 + 1);
        vel_center[2] += data->GetValue(cell->GetPointId(i) * 3 + 2);
    }
    center /= static_cast<float>(numPoints);
    vel_center /= static_cast<float>(numPoints);

    // 构建位置矩阵A和速度差矩阵B
    Eigen::MatrixXf A(numPoints, 3);
    Eigen::MatrixXf B(numPoints, 3);
    for (int i = 0; i < numPoints; ++i) {
        auto p = cell->GetPoint(i);
        Vector3f diff = p - center;
        A(i, 0) = diff[0]; A(i, 1) = diff[1]; A(i, 2) = diff[2];

        B(i, 0) = data->GetValue(cell->GetPointId(i) * 3 + 0) - vel_center[0];
        B(i, 1) = data->GetValue(cell->GetPointId(i) * 3 + 1) - vel_center[1];
        B(i, 2) = data->GetValue(cell->GetPointId(i) * 3 + 2) - vel_center[2];
    }

    Eigen::MatrixXf G = (A.transpose() * A).ldlt().solve(A.transpose() * B);

    VectorGrad Grad;
    Grad.x.gx = G(0, 0);
    Grad.x.gy = G(1, 0);
    Grad.x.gz = G(2, 0);

    Grad.y.gx = G(0, 1);
    Grad.y.gy = G(1, 1);
    Grad.y.gz = G(2, 1);

    Grad.z.gx = G(0, 2);
    Grad.z.gy = G(1, 2);
    Grad.z.gz = G(2, 2);

    return Grad;
}

VortexFilter::VectorGrad VortexFilter::ComputeVectorGradByPolyhedron(Cell* cell, ArrayObject* data) {

    std::unordered_set<igIndex> st;

    Vector3f center(0, 0, 0);
    Vector3f vel_center(0, 0, 0);
    int numPoints = cell->GetNumberOfPoints();
    //std::cout << cell  << " " <<  cell->m_PointIds->GetNumberOfIds()  << " " << cell->m_Points->GetNumberOfPoints() << std::endl;
    std::vector<int> isExisted(numPoints, 0);

    int count = 0;
    for (int i = 0; i < numPoints; ++i) {
        if (st.count(cell->GetPointId(i))) continue;
        st.insert(cell->GetPointId(i));

        isExisted[i] = 1;
        count++;
        auto p = cell->GetPoint(i);
        center += p;
        vel_center[0] += data->GetValue(cell->GetPointId(i) * 3 + 0);
        vel_center[1] += data->GetValue(cell->GetPointId(i) * 3 + 1);
        vel_center[2] += data->GetValue(cell->GetPointId(i) * 3 + 2);
    }
    center /= static_cast<float>(count);
    vel_center /= static_cast<float>(count);

    Eigen::MatrixXf A(count, 3);
    Eigen::MatrixXf B(count, 3);
    int i = 0;
    for (int j = 0; j < numPoints; ++j) {
        if (!isExisted[j]) continue;

        auto p = cell->GetPoint(i);
        Vector3f diff = p - center;
        A(i, 0) = diff[0]; A(i, 1) = diff[1]; A(i, 2) = diff[2];

        B(i, 0) = data->GetValue(cell->GetPointId(i) * 3 + 0) - vel_center[0];
        B(i, 1) = data->GetValue(cell->GetPointId(i) * 3 + 1) - vel_center[1];
        B(i, 2) = data->GetValue(cell->GetPointId(i) * 3 + 2) - vel_center[2];
        i++;
    }

    Eigen::MatrixXf G = (A.transpose() * A + Eigen::MatrixXf::Identity(3, 3) * 1e-8).ldlt().solve(A.transpose() * B);
    VectorGrad Grad;
    Grad.x.gx = G(0, 0);
    Grad.x.gy = G(1, 0);
    Grad.x.gz = G(2, 0);

    Grad.y.gx = G(0, 1);
    Grad.y.gy = G(1, 1);
    Grad.y.gz = G(2, 1);

    Grad.z.gx = G(0, 2);
    Grad.z.gy = G(1, 2);
    Grad.z.gz = G(2, 2);

    return Grad;
}

bool VortexFilter::ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet,
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

bool VortexFilter::ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
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

bool VortexFilter::ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
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

std::array<float, 3> VortexFilter::ComputePointGradientWithSurfaceMesh(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 计算梯度，并按面积归一化
    for (int i = 0; i < NumPoints; ++i) {
        auto p = cell->GetPoint(i);
        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;

        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
    }

    // 归一化梯度，使其与单元大小无关
    for (int d = 0; d < NumPoints; d++) gradient[d] /= length;

    return gradient;
}

std::array<float, 3> VortexFilter::ComputePointGradientWithVolumeMesh(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;

    float avgEdgeLength = ComputeVolumeAverageEdgeLength(cell);

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

    for (int d = 0; d < 3; d++) gradient[d] /= avgEdgeLength;

    return gradient;
}

std::array<float, 3> VortexFilter::ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

ArrayObject::Pointer VortexFilter::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray, size_t PointNum) { 
    int dim = OriArray->GetDimension();

    auto NewArray = FloatArray::New();
    NewArray->SetName(OriArray->GetName());
    NewArray->SetDimension(dim);
    NewArray->Reserve(PointNum);

    float scalar[16]{0}, temp[16]{0};
    for (int i = 0; i < PointNum; ++i) { 
        NewArray->AddElement(scalar);
    }

    std::vector<int> PointAdjNum(PointNum, 0);

    igIndex cell[IGAME_CELL_MAX_SIZE];
    
    for (int i = 0; i < Cell->GetNumberOfCells(); ++i) 
    { 
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

float VortexFilter::ComputeSurfaceArea(Cell* cell) {
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

float VortexFilter::ComputeTriangleArea(Cell* cell) {
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

float VortexFilter::ComputeVolumeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float VortexFilter::ComputeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

float VortexFilter::ComputeTetVolume(Cell* cell) {
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

bool VortexFilter::InverseMatrix4x4(const float in[4][4], float out[4][4]) {
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

VortexFilter::VortexFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

std::array<float, 3> VortexFilter::ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexFilter::ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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