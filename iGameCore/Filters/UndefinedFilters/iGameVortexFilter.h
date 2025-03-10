#ifndef VortexFilter_h
#define VortexFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN

class VortexFilter : public Filter {
public:
    I_OBJECT(VortexFilter);
    static Pointer New() { return new VortexFilter; }
    bool Execute() override {

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

        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH: {
                surface_Mesh = DynamicCast<SurfaceMesh>(input);
                if (!CheckType()) return false;

                auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
                if (attachmentType == 0) ComputePointVorticityForMesh(surface_Mesh, attributeSet, curIndex);
                else if (attachmentType == 1)
                    ComputeCellVorticityForMesh(surface_Mesh, attributeSet, curIndex);
            } break;
            case IG_VOLUME_MESH: {
                volume_Mesh = DynamicCast<VolumeMesh>(input);
                if (!CheckType()) return false;

                auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
                if (attachmentType == 0) ComputePointVorticityForVol(volume_Mesh, attributeSet, curIndex);
                else if (attachmentType == 1)
                    ComputeCellVorticityForVol(volume_Mesh, attributeSet, curIndex);
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(input);
                surface_Mesh = mesh->TransferToSurfaceMesh();
                volume_Mesh = mesh->TransferToVolumeMesh();


                if (surface_Mesh) {
                    if (!CheckType()) return false;
                    auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
                    if (attachmentType == 0) ComputePointVorticityForMesh(surface_Mesh, attributeSet, curIndex);
                    else if (attachmentType == 1)
                        ComputeCellVorticityForMesh(surface_Mesh, attributeSet, curIndex);
                }

                if (volume_Mesh) {
                    if (!CheckType()) return false;
                    auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
                    if (attachmentType == 0) ComputePointVorticityForVol(volume_Mesh, attributeSet, curIndex);
                    else if (attachmentType == 1)
                        ComputeCellVorticityForVol(volume_Mesh, attributeSet, curIndex);
                }
            } break;
            default:
                return false;
        }

        if (volume_Mesh) {}
        if (surface_Mesh) {}

        return true;
    }

    float ComputeCellVolume(Cell* cell) {
        switch (cell->GetCellType()) {
            case IG_TETRA:
                //return ComputeTetVolume(cell);
            case IG_HEXAHEDRON:
                //return ComputeHexVolume(cell);
            default:
                return 1.0f;
        }
    }
    std::array<float, 3> ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
        switch (cell->GetCellType()) {
            case IG_TETRA: // 纯四面体
                return ComputeTetPointGradient(cell, data, dim);
            case IG_HEXAHEDRON: // 纯六面体
                return ComputeHexPointGradient(cell, data, dim);
            default: // 其他
                return ComputePolyPointGradient(cell, data, dim);
        }
    }

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
        switch (cell->GetCellType()) {
            case IG_TETRA: // 纯四面体
                return ComputeTetCellGradient(type, cell, data, dim);
            case IG_HEXAHEDRON: // 纯六面体
                return ComputeHexCellGradient(type, cell, data, dim);
            default: // 其他
                return ComputePolyCellGradient(type, cell, data, dim);
        }
    }

    // // ===========================表面网格===========================
    bool ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex) {

        int PointNum = surface_Mesh->GetNumberOfPoints();
        int numCells = surface_Mesh->GetNumberOfFaces();


        std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
        std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
        std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
        std::vector<float> volumes(PointNum, 0.0f);

        auto data = attributeSet->GetAttribute(curIndex).pointer;

        for (int cellId = 0; cellId < numCells; ++cellId) {
            auto cell = surface_Mesh->GetFace(cellId);
            float cellVolume = ComputeCellVolume(cell);
            auto grad_x = ComputePointGradient(cell, data, 0);
            auto grad_y = ComputePointGradient(cell, data, 1);
            auto grad_z = ComputePointGradient(cell, data, 2);

            for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
                igIndex pid = cell->GetPointId(i);
                for (int d = 0; d < 3; d++) {
                    gradients_x[pid][d] += grad_x[d] * cellVolume;
                    gradients_y[pid][d] += grad_y[d] * cellVolume;
                    gradients_z[pid][d] += grad_z[d] * cellVolume;
                }
                volumes[pid] += cellVolume;
            }
        }

        for (int i = 0; i < PointNum; ++i)
            //if (volumes[i] > 1e-6f)
            for (int d = 0; d < 3; d++) {
                gradients_x[i][d] /= volumes[i];
                gradients_y[i][d] /= volumes[i];
                gradients_z[i][d] /= volumes[i];
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
            //            if (mag > 1e-6f)
            vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
            //            else
            //                vorticities->AddElement3(0, 0, 0);
        }
        return true;
    }

    // =========================== 表面网格 Cell ===========================
    bool ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex) {

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

    // ===========================体网格===========================
    bool ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex) {

        int PointNum = volume_Mesh->GetNumberOfPoints();
        int numCells = volume_Mesh->GetNumberOfVolumes();
        auto data = attributeSet->GetAttribute(curIndex).pointer;

        std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
        std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
        std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
        std::vector<float> volumes(PointNum, 0.0f);

        for (int cellId = 0; cellId < numCells; ++cellId) {
            auto cell = volume_Mesh->GetVolume(cellId);
            float cellVolume = ComputeCellVolume(cell);
            auto grad_x = ComputePointGradient(cell, data, 0);
            auto grad_y = ComputePointGradient(cell, data, 1);
            auto grad_z = ComputePointGradient(cell, data, 2);

            for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
                igIndex pid = cell->GetPointId(i);
                for (int d = 0; d < 3; d++) {
                    gradients_x[pid][d] += grad_x[d] * cellVolume;
                    gradients_y[pid][d] += grad_y[d] * cellVolume;
                    gradients_z[pid][d] += grad_z[d] * cellVolume;
                }
                volumes[pid] += cellVolume;
            }
        }

        for (int i = 0; i < PointNum; ++i)
            //if (volumes[i] > 1e-5f)
            for (int d = 0; d < 3; d++) {
                gradients_x[i][d] /= volumes[i];
                gradients_y[i][d] /= volumes[i];
                gradients_z[i][d] /= volumes[i];
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

    // ===========================体网格 Cell  ===========================
    bool ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex) {
        int numCells = volume_Mesh->GetNumberOfVolumes();
        auto data = attributeSet->GetAttribute(curIndex).pointer;

        FloatArray::Pointer cellVorticities = FloatArray::New();
        cellVorticities->SetDimension(3);
        cellVorticities->Reserve(numCells);
        cellVorticities->SetName("vorticities");
        attributeSet->AddScalar(IG_CELL, cellVorticities);

        for (int cellId = 0; cellId < numCells; ++cellId) {
            auto cell = volume_Mesh->GetVolume(cellId);

            // 0:surface_Mesh
            // 1:volume_Mesh

            auto grad_x = ComputeCellGradient(1, cell, data, 0);
            auto grad_y = ComputeCellGradient(1, cell, data, 1);
            auto grad_z = ComputeCellGradient(1, cell, data, 2);
            // 计算涡度
            float omega_x = grad_z[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
            float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
            float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y

            float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
            //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
            cellVorticities->AddElement3(omega_x, omega_y, omega_z);
            //else
            //    vorticities->AddElement3(0, 0, 0);
        }
        return true;
    }

    // 四面体线性插值 point
    std::array<float, 3> ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
        float centerValue = 0.0f;

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
        for (int d = 0; d < 3; d++) gradient[d] /= 4.0f;

        return gradient;
    }

    // 四面体线性插值 cell
    std::array<float, 3> ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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


    float ComputeTetVolume(Cell* cell) {
        auto p0 = cell->GetPoint(0);
        auto p1 = cell->GetPoint(1);
        auto p2 = cell->GetPoint(2);
        auto p3 = cell->GetPoint(3);

        std::array<float, 3> a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
        std::array<float, 3> b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
        std::array<float, 3> c = {p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]};

        std::array<float, 3> cross_bc = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2],
                                         b[0] * c[1] - b[1] * c[0]};

        float dot_a = a[0] * cross_bc[0] + a[1] * cross_bc[1] + a[2] * cross_bc[2];
        return std::abs(dot_a) / 6.0f;
    }


    // 六面体中心差分
    std::array<float, 3> ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
        float centerValue = 0.0f;

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
        for (int d = 0; d < 3; d++) gradient[d] /= 8.0f;

        return gradient;
    }

    // 六面体中心差分 cell
    std::array<float, 3> ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 多面体中心差分
    std::array<float, 3> ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 多面体中心差分 cell
    std::array<float, 3> ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
        float centerValue = 0.0f;
        int num = 0;
        if (type == 1) num = cell->GetNumberOfPoints();
        else if (type == 0)
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

    bool InverseMatrix4x4(const float in[4][4], float out[4][4]) {
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


protected:
    VortexFilter()
    //输入输出个数
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~VortexFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    int curDim{-1};
};
IGAME_NAMESPACE_END
#endif

// #ifndef VortexFilter_h
// #define VortexFilter_h
//
// #include "Eigen/Dense"
// #include "Eigen/Eigenvalues"
// #include "iGameFilter.h"
// #include "iGamePointSet.h"
// #include "iGameSurfaceMesh.h"
// #include "iGameUnstructuredMesh.h"
// #include "iGameVolumeMesh.h"
// #include <cmath>
//
//
// IGAME_NAMESPACE_BEGIN
//
// class VortexFilter : public Filter {
// public:
//     I_OBJECT(VortexFilter);
//     static Pointer New() { return new VortexFilter; }
//     bool Execute() override {
//
//         auto input = GetInput(0);
//         if (input == nullptr) return false;
//
//         auto CheckType = [&]() -> bool {
//             attributeSet = input->GetAttributeSet();
//             if (!attributeSet) return false;
//             curIndex = input->GetAttributeIndex();
//             curDim = input->GetAttributeDimension();
//             if (curIndex < 0) return false;
//
//             int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
//             if (dim != 3) { return false; }
//             return true;
//         };
//
//         switch (input->GetDataObjectType()) {
//             case IG_SURFACE_MESH: {
//                 surface_Mesh = DynamicCast<SurfaceMesh>(input);
//                 if (!CheckType()) return false;
//
//                 ComputeVorticityForMesh(surface_Mesh, attributeSet, curIndex);
//
//             } break;
//             case IG_VOLUME_MESH: {
//                 volume_Mesh = DynamicCast<VolumeMesh>(input);
//                 ComputeVorticityForVol(volume_Mesh, attributeSet, curIndex);
//
//             } break;
//             case IG_UNSTRUCTURED_MESH: {
//                 auto mesh = DynamicCast<UnstructuredMesh>(input);
//                 surface_Mesh = mesh->TransferToSurfaceMesh();
//                 volume_Mesh = mesh->TransferToVolumeMesh();
//
//                 if (surface_Mesh) {
//                     if (!CheckType()) return false;
//
//                     ComputeVorticityForMesh(surface_Mesh, attributeSet, curIndex);
//                 }
//
//                 if (volume_Mesh) {
//                     if (!CheckType()) return false;
//
//                     //FloatArray::Pointer vorticities = FloatArray::New();
//                     //vorticities->SetDimension(3);
//                     //vorticities->SetName("vorticities");
//                     //input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                     //std::cout << "add vorticities\n";
//
//                     return ComputeVorticityForVol(volume_Mesh, attributeSet, curIndex);
//
//                     //// because surface_Mesh is mesh's DisplayObject
//                     //if (!mesh->GetDisplayObject()) { mesh->ConvertToDrawableData(); }
//
//                     //surface_Mesh = DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
//                     //if (!surface_Mesh) return false;
//
//                     //if (!CheckType()) return false;
//
//                     //FloatArray::Pointer vorticities = FloatArray::New();
//                     //vorticities->SetDimension(3);
//                     //vorticities->SetName("vorticities");
//                     //input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                 }
//             } break;
//             default:
//                 return false;
//         }
//
//         if (volume_Mesh) {}
//
//         if (surface_Mesh) {
//             //            attributeSet = surface_Mesh->GetAttributeSet();
//             //            if (attributeSet == nullptr) return false;
//             //
//             //            auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
//             //
//             //            int FaceNum = surface_Mesh->GetNumberOfFaces();
//             //            int PointNum = surface_Mesh->GetNumberOfPoints();
//             //            Points::Pointer Points = surface_Mesh->GetPoints();
//             //            surface_Mesh->RequestEditStatus();
//             //            if (PointNum != 0 && attachmentType == 0) {
//             //                return GetPointVortex(0, Points, PointNum);
//             //            } else if (FaceNum != 0 && attachmentType == 1)
//             //                return GetOtherVortex(0, FaceNum);
//         }
//
//         return true;
//     }
//
//     // =========================================================修改=========================================================
//
//     float ComputeCellVolume(Cell* cell) {
//         switch (cell->GetCellType()) {
//             case IG_TETRA:
//                 //return ComputeTetVolume(cell);
//             case IG_HEXAHEDRON:
//                 //return ComputeHexVolume(cell);
//             default:
//                 return 1.0f;
//         }
//     }
//     std::array<float, 3> ComputeCellGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//         switch (cell->GetCellType()) {
//             case IG_TETRA: // 纯四面体
//                 return ComputeTetGradient(cell, data, dim);
//             case IG_HEXAHEDRON: // 纯六面体
//                 return ComputeHexGradient(cell, data, dim);
//             default:
//                 return ComputePolyGradient(cell, data, dim);
//                 ;
//         }
//     }
//     // // ===========================表面网格===========================
//     bool ComputeVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex) {
//
//         int PointNum = surface_Mesh->GetNumberOfPoints();
//         int numCells = surface_Mesh->GetNumberOfFaces();
//
//
//         std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
//         std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
//         std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
//         std::vector<float> volumes(PointNum, 0.0f);
//
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//
//         for (int cellId = 0; cellId < numCells; ++cellId) {
//             auto cell = surface_Mesh->GetFace(cellId);
//             float cellVolume = ComputeCellVolume(cell);
//             auto grad_x = ComputeCellGradient(cell, data, 0);
//             auto grad_y = ComputeCellGradient(cell, data, 1);
//             auto grad_z = ComputeCellGradient(cell, data, 2);
//
//             for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//                 igIndex pid = cell->GetPointId(i);
//                 for (int d = 0; d < 3; d++) {
//                     gradients_x[pid][d] += grad_x[d] * cellVolume;
//                     gradients_y[pid][d] += grad_y[d] * cellVolume;
//                     gradients_z[pid][d] += grad_z[d] * cellVolume;
//                 }
//                 volumes[pid] += cellVolume;
//             }
//         }
//
//         for (int i = 0; i < PointNum; ++i)
//             //if (volumes[i] > 1e-6f)
//             for (int d = 0; d < 3; d++) {
//                 gradients_x[i][d] /= volumes[i];
//                 gradients_y[i][d] /= volumes[i];
//                 gradients_z[i][d] /= volumes[i];
//             }
//
//         FloatArray::Pointer vorticities = FloatArray::New();
//         vorticities->SetDimension(3);
//         vorticities->Reserve(PointNum);
//         vorticities->SetName("vorticities");
//         attributeSet->AddScalar(IG_POINT, vorticities);
//
//         for (int i = 0; i < PointNum; ++i) {
//             float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
//             float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
//             float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y
//
//             float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
//             if (mag > 1e-6f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
//             else
//                 vorticities->AddElement3(0, 0, 0);
//         }
//         return true;
//     }
//
//     // ===========================体网格===========================
//     bool ComputeVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex) {
//
//         int PointNum = volume_Mesh->GetNumberOfPoints();
//         int numCells = volume_Mesh->GetNumberOfVolumes();
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//
//         std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
//         std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
//         std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
//         std::vector<float> volumes(PointNum, 0.0f);
//
//
//         for (int cellId = 0; cellId < numCells; ++cellId) {
//             auto cell = volume_Mesh->GetVolume(cellId);
//             float cellVolume = ComputeCellVolume(cell);
//             auto grad_x = ComputeCellGradient(cell, data, 0);
//             auto grad_y = ComputeCellGradient(cell, data, 1);
//             auto grad_z = ComputeCellGradient(cell, data, 2);
//
//             for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//                 igIndex pid = cell->GetPointId(i);
//                 for (int d = 0; d < 3; d++) {
//                     gradients_x[pid][d] += grad_x[d] * cellVolume;
//                     gradients_y[pid][d] += grad_y[d] * cellVolume;
//                     gradients_z[pid][d] += grad_z[d] * cellVolume;
//                 }
//                 volumes[pid] += cellVolume;
//             }
//         }
//
//         for (int i = 0; i < PointNum; ++i)
//             //if (volumes[i] > 1e-5f)
//             for (int d = 0; d < 3; d++) {
//                 gradients_x[i][d] /= volumes[i];
//                 gradients_y[i][d] /= volumes[i];
//                 gradients_z[i][d] /= volumes[i];
//             }
//
//         FloatArray::Pointer vorticities = FloatArray::New();
//         vorticities->SetDimension(3);
//         vorticities->Reserve(PointNum);
//         vorticities->SetName("vorticities");
//         attributeSet->AddScalar(IG_POINT, vorticities);
//
//         //FloatArray::Pointer gradient = FloatArray::New();
//         //gradient->SetDimension(3);
//         //gradient->Reserve(PointNum);
//         //gradient->SetName("gradient");
//         //attributeSet->AddScalar(IG_POINT, gradient);
//         //for (int i = 0; i < PointNum; ++i) {
//         //    gradient->AddElement3(gradients_x[i][0], gradients_x[i][1], gradients_x[i][2]);
//         //}
//
//         for (int i = 0; i < PointNum; ++i) {
//             float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
//             float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
//             float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y
//
//             float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
//             //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
//             vorticities->AddElement3(omega_x, omega_y, omega_z);
//             //else
//             //    vorticities->AddElement3(0, 0, 0);
//         }
//         return true;
//     }
//
//     // 四面体线性插值
//     std::array<float, 3> ComputeTetGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//         std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//         float centerValue = 0.0f;
//
//         for (int i = 0; i < 4; i++) {
//             auto p = cell->GetPoint(i);
//             center[0] += p[0];
//             center[1] += p[1];
//             center[2] += p[2];
//             centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//         }
//         for (int d = 0; d < 3; d++) center[d] /= 4.0f;
//         centerValue /= 4.0f;
//
//         std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//         for (int i = 0; i < 4; ++i) {
//             auto p = cell->GetPoint(i);
//             std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//             float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//             for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//         }
//         for (int d = 0; d < 3; d++) gradient[d] /= 4.0f;
//
//         return gradient;
//     }
//
//     float ComputeTetVolume(Cell* cell) {
//         auto p0 = cell->GetPoint(0);
//         auto p1 = cell->GetPoint(1);
//         auto p2 = cell->GetPoint(2);
//         auto p3 = cell->GetPoint(3);
//
//         std::array<float, 3> a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
//         std::array<float, 3> b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
//         std::array<float, 3> c = {p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]};
//
//         std::array<float, 3> cross_bc = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2],
//                                          b[0] * c[1] - b[1] * c[0]};
//
//         float dot_a = a[0] * cross_bc[0] + a[1] * cross_bc[1] + a[2] * cross_bc[2];
//         return std::abs(dot_a) / 6.0f;
//     }
//
//
//     // 六面体中心差分
//     std::array<float, 3> ComputeHexGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//         std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//         float centerValue = 0.0f;
//
//         for (int i = 0; i < 8; i++) {
//             auto p = cell->GetPoint(i);
//             center[0] += p[0];
//             center[1] += p[1];
//             center[2] += p[2];
//             centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//         }
//         for (int d = 0; d < 3; d++) center[d] /= 8.0f;
//         centerValue /= 8.0f;
//
//         std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//         for (int i = 0; i < 8; ++i) {
//             auto p = cell->GetPoint(i);
//             std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//             float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//             for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//         }
//         for (int d = 0; d < 3; d++) gradient[d] /= 8.0f;
//
//         return gradient;
//     }
//
//     // 多面体中心差分
//     std::array<float, 3> ComputePolyGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//         std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//         float centerValue = 0.0f;
//
//         for (int i = 0; i < cell->GetNumberOfPoints(); i++) {
//             auto p = cell->GetPoint(i);
//             center[0] += p[0];
//             center[1] += p[1];
//             center[2] += p[2];
//             centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//         }
//         for (int d = 0; d < 3; d++) center[d] /= cell->GetNumberOfPoints();
//         centerValue /= cell->GetNumberOfPoints();
//
//         std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//         for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//             auto p = cell->GetPoint(i);
//             std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//             float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//             for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//         }
//         for (int d = 0; d < 3; d++) gradient[d] /= cell->GetNumberOfPoints();
//
//         return gradient;
//     }
//
//     bool InverseMatrix4x4(const float in[4][4], float out[4][4]) {
//         float aug[4][8] = {0};
//         for (int i = 0; i < 4; ++i) {
//             for (int j = 0; j < 4; ++j) {
//                 aug[i][j] = in[i][j];
//                 aug[i][j + 4] = (i == j) ? 1.0f : 0.0f;
//             }
//         }
//
//         for (int col = 0; col < 4; ++col) {
//             int max_row = col;
//             for (int i = col + 1; i < 4; ++i) {
//                 if (std::abs(aug[i][col]) > std::abs(aug[max_row][col])) { max_row = i; }
//             }
//
//             // 如果最大主元接近零，矩阵奇异
//             if (std::abs(aug[max_row][col]) < 1e-12f) { return false; }
//
//             if (max_row != col) {
//                 for (int j = col; j < 8; ++j) { std::swap(aug[col][j], aug[max_row][j]); }
//             }
//
//             float pivot = aug[col][col];
//             for (int j = col; j < 8; ++j) { aug[col][j] /= pivot; }
//
//             for (int i = 0; i < 4; ++i) {
//                 if (i != col && std::abs(aug[i][col]) > 1e-12f) {
//                     float factor = aug[i][col];
//                     for (int j = col; j < 8; ++j) { aug[i][j] -= factor * aug[col][j]; }
//                 }
//             }
//         }
//
//         for (int i = 0; i < 4; ++i) {
//             for (int j = 0; j < 4; ++j) { out[i][j] = aug[i][j + 4]; }
//         }
//         return true;
//     }
//
//
//     // =========================================================修改=========================================================
//
//
// protected:
//     VortexFilter()
//     //输入输出个数
//     {
//         SetNumberOfInputs(1);
//         SetNumberOfOutputs(1);
//     }
//     ~VortexFilter() override = default;
//
//     VolumeMesh::Pointer volume_Mesh{};
//     SurfaceMesh::Pointer surface_Mesh{};
//     AttributeSet* attributeSet{nullptr};
//
//     int curIndex{-1};
//     int curDim{-1};
// };
//
// IGAME_NAMESPACE_END
// #endif


// #ifndef VortexFilter_h
// #define VortexFilter_h
//
// #include "Eigen/Dense"
// #include "Eigen/Eigenvalues"
// #include "iGameFilter.h"
// #include "iGamePointSet.h"
// #include "iGameSurfaceMesh.h"
// #include "iGameUnstructuredMesh.h"
// #include "iGameVolumeMesh.h"
// #include <cmath>
//
//
// IGAME_NAMESPACE_BEGIN
//
// //现在默认取第一个数组
// class VortexFilter : public Filter {
// public:
//     I_OBJECT(VortexFilter);
//     static Pointer New() { return new VortexFilter; }
//     bool Execute() override {
//
//         auto input = GetInput(0);
//         if (input == nullptr) return false;
//
//         auto CheckType = [&]() -> bool {
//             attributeSet = input->GetAttributeSet();
//             if (!attributeSet) return false;
//             curIndex = input->GetAttributeIndex();
//             curDim = input->GetAttributeDimension();
//             if (curIndex < 0) return false;
//
//             int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
//             if (dim != 3) { return false; }
//             return true;
//         };
//
//         switch (input->GetDataObjectType()) {
//             case IG_SURFACE_MESH: {
//                 surface_Mesh = DynamicCast<SurfaceMesh>(input);
//
//                 if (!CheckType()) return false;
//
//             } break;
//             case IG_VOLUME_MESH: {
//                 volume_Mesh = DynamicCast<VolumeMesh>(input);
//                 if (volume_Mesh) {
//                     surface_Mesh = DynamicCast<SurfaceMesh>(volume_Mesh->GetDisplayObject());
//                     if (!surface_Mesh) return false;
//
//                     if (!CheckType()) return false;
//
//                     FloatArray::Pointer vorticities = FloatArray::New();
//                     vorticities->SetDimension(3);
//                     vorticities->SetName("vorticities");
//                     input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                 }
//             } break;
//             case IG_UNSTRUCTURED_MESH: {
//                 auto mesh = DynamicCast<UnstructuredMesh>(input);
//                 surface_Mesh = mesh->TransferToSurfaceMesh();
//                 volume_Mesh = mesh->TransferToVolumeMesh();
//
//                 if (surface_Mesh) {
//                     if (!CheckType()) return false;
//                 }
//
//                 if (volume_Mesh) {
//                     // because surface_Mesh is mesh's DisplayObject
//                     if (!mesh->GetDisplayObject()) { mesh->ConvertToDrawableData(); }
//
//                     surface_Mesh = DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
//                     if (!surface_Mesh) return false;
//
//                     if (!CheckType()) return false;
//
//                     FloatArray::Pointer vorticities = FloatArray::New();
//                     vorticities->SetDimension(3);
//                     vorticities->SetName("vorticities");
//                     input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                 }
//             } break;
//             default:
//                 return false;
//         }
//
//         if (volume_Mesh) {
//             //surface_Mesh =
//             //        DynamicCast<SurfaceMesh>(volume_Mesh->GetDisplayObject());
//             //if (!surface_Mesh) return false;
//
//             //FloatArray::Pointer vorticities = FloatArray::New();
//             //vorticities->SetDimension(3);
//             //vorticities->SetName("vorticities");
//             //volume_Mesh->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//
//             //// 测试时默认取第一个数组
//             //auto attachmentType = attributeSet->GetAttribute(0).attachmentType;
//
//             //int VolumeNum = volume_Mesh->GetNumberOfVolumes();
//             //int PointNum = volume_Mesh->GetNumberOfPoints();
//             //Points::Pointer Points = volume_Mesh->GetPoints();
//             //volume_Mesh->RequestEditStatus();
//             //if (PointNum != 0 && attachmentType == 0)
//             //    GetPointVortex(1, Points, PointNum);
//             //else if (VolumeNum != 0 && attachmentType == 0)
//             //    GetOtherVortex(1, VolumeNum);
//         }
//
//         if (surface_Mesh) {
//             attributeSet = surface_Mesh->GetAttributeSet();
//             if (attributeSet == nullptr) return false;
//
//             auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
//
//             int FaceNum = surface_Mesh->GetNumberOfFaces();
//             int PointNum = surface_Mesh->GetNumberOfPoints();
//             Points::Pointer Points = surface_Mesh->GetPoints();
//             surface_Mesh->RequestEditStatus();
//             if (PointNum != 0 && attachmentType == 0) {
//                 return GetPointVortex(0, Points, PointNum);
//             } else if (FaceNum != 0 && attachmentType == 1)
//                 return GetOtherVortex(0, FaceNum);
//         }
//
//         return false;
//     }
//
//     bool GetPointVortex(int type, Points::Pointer Points, int PointNum) {
//
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//
//         int dimension = data->GetDimension();
//         // 必须为三维向量
//         if (dimension != 3) return false;
//
//         std::cout << "[Debug  ] " << "compute vortex" << '\n';
//
//         FloatArray::Pointer vorticities = FloatArray::New();
//         vorticities->SetDimension(3);
//         vorticities->Reserve(PointNum);
//         vorticities->SetName("vorticities");
//         attributeSet->AddScalar(IG_POINT, vorticities);
//
//         // 分别获取三个维度的梯度
//         std::vector<std::array<float, 3>> gradient_1 = GetPointGradient(type, Points, PointNum, 0);
//         std::vector<std::array<float, 3>> gradient_2 = GetPointGradient(type, Points, PointNum, 1);
//         std::vector<std::array<float, 3>> gradient_3 = GetPointGradient(type, Points, PointNum, 2);
//
//         // 计算涡旋
//         for (igIndex idx = 0; idx < PointNum; ++idx) {
//             const auto& grad_x = gradient_1[idx];
//             const auto& grad_y = gradient_2[idx];
//             const auto& grad_z = gradient_3[idx];
//
//             //float omega_x = grad_x[0];
//             float omega_x = grad_z[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
//             float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
//             float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y
//
//             //auto scalar = sqrt(omega_x * omega_x + omega_y * omega_y +
//             //                   omega_z * omega_z);
//             float scalar = 1;
//             if (scalar > 1e-6) {
//                 vorticities->AddElement3(omega_x / scalar, omega_y / scalar, omega_z / scalar);
//             } else
//                 vorticities->AddElement3(0, 0, 0);
//         }
//
//         //GetInput(0)
//         //->GetAttributeSet()
//         //->GetAttribute("vorticities")
//         //.updateAllDataRange();
//
//         //attributeSet->GetAttribute("vorticities").updateAllDataRange();
//
//         return true;
//     }
//
//     bool GetOtherVortex(int type, int Num) {
//
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//         int dimension = data->GetDimension();
//         // 必须为三维向量
//         if (dimension != 3) return false;
//
//         std::cout << "[Debug  ] " << "compute vortex" << '\n';
//
//         FloatArray::Pointer vorticities = FloatArray::New();
//         vorticities->SetDimension(3);
//         vorticities->Reserve(Num);
//         vorticities->SetName("vorticities");
//         attributeSet->AddScalar(IG_POINT, vorticities);
//
//         // 分别获取三个维度的梯度
//         std::vector<std::array<float, 3>> gradient_1 = GetOtherGradient(type, Num, 0);
//         std::vector<std::array<float, 3>> gradient_2 = GetOtherGradient(type, Num, 1);
//         std::vector<std::array<float, 3>> gradient_3 = GetOtherGradient(type, Num, 2);
//
//         // 计算涡旋
//         for (igIndex idx = 0; idx < Num; ++idx) {
//             const auto& grad_x = gradient_1[idx];
//             const auto& grad_y = gradient_2[idx];
//             const auto& grad_z = gradient_3[idx];
//
//             float omega_x = grad_x[1] - grad_y[2]; // ∂vz/∂y - ∂vy/∂z
//             float omega_y = grad_x[2] - grad_z[0]; // ∂vx/∂z - ∂vz/∂x
//             float omega_z = grad_y[0] - grad_x[1]; // ∂vy/∂x - ∂vx/∂y
//
//             auto scalar = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
//
//             //            vorticities->AddElement3(omega_x, omega_y, omega_z);
//             if (scalar > 1e-6) {
//                 vorticities->AddElement3(omega_x / scalar, omega_y / scalar, omega_z / scalar);
//             } else
//                 vorticities->AddElement3(0, 0, 0);
//             //if (type == 0) {
//             //    auto arr = surface_Mesh->GetMetadata()->GetStringArray(
//             //            ATTRIBUTE_NAME_ARRAY);
//             //    arr->AddElement("vorticities");
//             //} else if (type == 1) {
//             //    auto arr = volume_Mesh->GetMetadata()->GetStringArray(
//             //            ATTRIBUTE_NAME_ARRAY);
//             //    arr->AddElement("vorticities");
//             //}
//         }
//
//         attributeSet->GetAttribute("vorticities").UpdateAllDataRange();
//
//         return true;
//         //    bool GetPointVortex_ivd(int type, Points::Pointer Points, int PointNum){
//         //
//         //        PropertySet* m_PropertySet;
//         //        if (type == 0 )
//         //            m_PropertySet = surface_Mesh->GetPropertySet();
//         //        else if ( type == 1 )
//         //            m_PropertySet = volume_Mesh->GetPropertySet();
//         //
//         //        auto data = m_PropertySet->GetProperty(1).pointer;
//         //        int dimension = data->GetElementSize();
//         //        if(dimension != 3)
//         //            return false;
//         //
//         //        std::cout << "[Debug  ] " << "compute vortex" << '\n';
//         //
//         //        FloatArray::Pointer vorticities_ivd = FloatArray::New();
//         //        vorticities_ivd->SetElementSize(3);
//         //        vorticities_ivd->Reserve(PointNum);
//         //        m_PropertySet->AddScalar(IG_POINT,vorticities_ivd);
//         //
//         //        std::vector<std::array<float, 3>> vorticity(PointNum, {0.0f, 0.0f, 0.0f});
//         //        std::vector scalar(PointNum,0.0f);
//         //
//         //        // 计算涡旋
//         //        for (igIndex idx = 0; idx < PointNum; ++idx) {
//         //            auto v1 = Points->GetPoint(idx);
//         //            vorticity[idx][0] = data->GetValue(dimension * idx + 1) - data->GetValue(dimension * idx + 2);
//         //            vorticity[idx][1] = data->GetValue(dimension * idx + 2) - data->GetValue(dimension * idx);
//         //            vorticity[idx][0] = data->GetValue(dimension * idx) - data->GetValue(dimension * idx + 1);
//         //            scalar[idx] = sqrt(vorticity[idx][0] * vorticity[idx][0] + vorticity[idx][1] * vorticity[idx][1]+vorticity[idx][2]*vorticity[idx][2]);
//         //        }
//         //
//         //        // ivd
//         //        float max_scalar = *std::max_element(scalar.begin(), scalar.end());
//         //        float min_scalar = *std::min_element(scalar.begin(), scalar.end());
//         //        std::vector<float> normalized_scalar(scalar.size());
//         //        float total_ivd = 0.0f, ivd = 0.0f;
//         //        for (size_t i = 0; i < scalar.size(); ++i) {
//         //            if (max_scalar - min_scalar != 0) {
//         //                normalized_scalar[i] = (scalar[i] - min_scalar) / (max_scalar - min_scalar);
//         //                total_ivd+= normalized_scalar[i];
//         //            } else {
//         //                normalized_scalar[i] = 0.0;
//         //            }
//         //        }
//         //        if(total_ivd>0)
//         //            ivd = total_ivd / scalar.size();
//         //
//         //        for (igIndex idx = 0; idx < PointNum; idx++) {
//         //            if ( (abs(normalized_scalar[idx]) - ivd) >= 0.00 )
//         //                vorticities_ivd->AddElement3(vorticity[idx][0],vorticity[idx][1],vorticity[idx][2]);
//         //            else vorticities_ivd->AddElement3(0,0,0);
//         //        }
//         //
//         //        if (type == 0 ) {
//         //            auto arr = surface_Mesh->GetMetadata()->GetStringArray(ATTRIBUTE_NAME_ARRAY);
//         //            arr->AddElement("vorticities_ivd");
//         //        } else if ( type == 1 )
//         //        {
//         //            auto arr = volume_Mesh->GetMetadata()->GetStringArray(ATTRIBUTE_NAME_ARRAY);
//         //            arr->AddElement("vorticities_ivd");
//         //        }
//         //
//         //        return true;
//         //    }
//     }
//     std::vector<std::array<float, 3>> GetPointGradient(int type, Points::Pointer Points, int PointNum, int dim) {
//
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//         int dimension = data->GetDimension();
//
//         std::vector<std::array<float, 3>> gradient(PointNum, {0.0f, 0.0f, 0.0f});
//         std::vector<float> sumWeights(PointNum, 0.0f);
//
//         igIndex neighborVerts[256]{};
//         // 计算点的梯度
//         for (igIndex idx = 0; idx < PointNum; ++idx) {
//             int NeighborNum;
//             // 获取邻接顶点
//             if (type == 1) NeighborNum = volume_Mesh->GetPointToOneRingPoints(idx, neighborVerts);
//             else if (type == 0)
//                 NeighborNum = surface_Mesh->GetPointToOneRingPoints(idx, neighborVerts);
//
//             auto v1 = Points->GetPoint(idx);
//
//             for (int m = 0; m < NeighborNum; m++) {
//                 Vector<float, 3> v2;
//                 if (type == 1) v2 = volume_Mesh->GetPoint(neighborVerts[m]);
//                 else if (type == 0)
//                     v2 = surface_Mesh->GetPoint(neighborVerts[m]);
//
//                 float x = v1[0] - v2[0];
//                 float y = v1[1] - v2[1];
//                 float z = v1[2] - v2[2];
//
//                 // 先默认读第一维
//                 float value =
//                         data->GetValue(dimension * idx + dim) - data->GetValue(dimension * neighborVerts[m] + dim);
//
//                 float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
//                 sumWeights[idx] += weight;
//                 gradient[idx][0] += x * weight * value;
//                 gradient[idx][1] += y * weight * value;
//                 gradient[idx][2] += z * weight * value;
//
//                 //gradient[idx][0] += value;
//                 //gradient[idx][1] += value;
//                 //gradient[idx][2] += value;
//             }
//             if (sumWeights[idx] > 0) {
//                 gradient[idx][0] /= sumWeights[idx];
//                 gradient[idx][1] /= sumWeights[idx];
//                 gradient[idx][2] /= sumWeights[idx];
//             }
//         }
//         return gradient;
//     }
//
//     std::array<float, 3> GetPosition_volume(Volume* v, int num) {
//         std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
//         for (igIndex idx = 0; idx < num; idx++) {
//             position[0] += v->GetPoint(idx)[0];
//             position[1] += v->GetPoint(idx)[1];
//             position[2] += v->GetPoint(idx)[2];
//         }
//
//         for (igIndex i = 0; i < 3; i++) {
//             if (position[i] != 0) position[i] /= num;
//         }
//         return position;
//     }
//     std::array<float, 3> GetPosition_face(Face* f, int num) {
//         std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
//         for (igIndex idx = 0; idx < num; idx++) {
//             position[0] += f->GetPoint(idx)[0];
//             position[1] += f->GetPoint(idx)[1];
//             position[2] += f->GetPoint(idx)[2];
//         }
//
//         for (igIndex i = 0; i < 3; i++) {
//             if (position[i] != 0) position[i] /= num;
//         }
//         return position;
//     }
//
//     std::vector<std::array<float, 3>> GetOtherGradient(int type, int Num, int dim) {
//
//         auto data = attributeSet->GetAttribute(curIndex).pointer;
//         int dimension = data->GetDimension();
//
//         std::vector<std::array<float, 3>> gradient(Num, {0.0f, 0.0f, 0.0f});
//         std::vector<float> sumWeights(Num, 0.0f);
//
//         igIndex neighborVerts[256]{};
//         // 计算点的梯度
//         for (igIndex idx = 0; idx < Num; ++idx) {
//
//             int NeighborNum;
//             // 获取邻接顶点
//             if (type == 1)
//                 // neighbors:volumeIds
//                 NeighborNum = volume_Mesh->GetVolumeToNeighborVolumesWithFace(idx, neighborVerts);
//
//             else if (type == 0)
//                 // neighbors:faceIds
//                 NeighborNum = surface_Mesh->GetFaceToNeighborFaces(idx, neighborVerts);
//
//             for (int m = 0; m < NeighborNum; m++) {
//                 float x, y, z;
//                 if (type == 1) {
//                     igIndex* volumeIds;
//                     auto v1 = volume_Mesh->GetVolume(idx);
//                     auto v2 = volume_Mesh->GetVolume(neighborVerts[m]);
//                     auto size_v1 = v1->GetNumberOfPoints();
//                     auto size_v2 = v2->GetNumberOfPoints();
//
//                     std::array<float, 3> v1_position = GetPosition_volume(v1, size_v1);
//                     std::array<float, 3> v2_position = GetPosition_volume(v2, size_v2);
//
//                     x = v1_position[0] - v2_position[0];
//                     y = v1_position[1] - v2_position[1];
//                     z = v1_position[2] - v2_position[2];
//
//                 } else if (type == 0) {
//                     auto v1 = surface_Mesh->GetFace(idx);
//                     auto v2 = surface_Mesh->GetFace(neighborVerts[m]);
//                     auto size_v1 = v1->GetNumberOfPoints();
//                     auto size_v2 = v2->GetNumberOfPoints();
//
//                     std::array<float, 3> v1_position = GetPosition_face(v1, size_v1);
//                     std::array<float, 3> v2_position = GetPosition_face(v2, size_v2);
//
//                     x = v1_position[0] - v2_position[0];
//                     y = v1_position[1] - v2_position[1];
//                     z = v1_position[2] - v2_position[2];
//                 }
//                 // 标量计算时就算是三维数据也默认取第一维
//                 double value =
//                         data->GetValue(idx * dimension + dim) - data->GetValue(neighborVerts[m] * dimension + dim);
//
//                 float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
//                 sumWeights[idx] += weight;
//                 gradient[idx][0] += x * weight * value;
//                 gradient[idx][1] += y * weight * value;
//                 gradient[idx][2] += z * weight * value;
//             }
//             if (sumWeights[idx] > 0) {
//                 gradient[idx][0] /= sumWeights[idx];
//                 gradient[idx][1] /= sumWeights[idx];
//                 gradient[idx][2] /= sumWeights[idx];
//             }
//         }
//
//         return gradient;
//     }
//
//
// protected:
//     VortexFilter()
//     //输入输出个数
//     {
//         SetNumberOfInputs(1);
//         SetNumberOfOutputs(1);
//     }
//     ~VortexFilter() override = default;
//
//     VolumeMesh::Pointer volume_Mesh{};
//     SurfaceMesh::Pointer surface_Mesh{};
//     AttributeSet* attributeSet{nullptr};
//
//     int curIndex{-1};
//     int curDim{-1};
// };
//
// IGAME_NAMESPACE_END
// #endif
