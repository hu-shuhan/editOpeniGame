//#ifndef VortexFilter_h
//#define VortexFilter_h
//
//#include "Eigen/Dense"
//#include "Eigen/Eigenvalues"
//#include "iGameFilter.h"
//#include "iGamePointSet.h"
//#include "iGameSurfaceMesh.h"
//#include "iGameUnstructuredMesh.h"
//#include "iGameVolumeMesh.h"
//#include <cmath>
//
//
//IGAME_NAMESPACE_BEGIN
//
//class VortexFilter : public Filter {
//public:
//    I_OBJECT(VortexFilter);
//    static Pointer New() { return new VortexFilter; }
//    bool Execute() override {
//
//        auto input = GetInput(0);
//        if (input == nullptr) return false;
//
//        auto CheckType = [&]() -> bool {
//            attributeSet = input->GetAttributeSet();
//            if (!attributeSet) return false;
//            curIndex = input->GetAttributeIndex();
//            curDim = input->GetAttributeDimension();
//            if (curIndex < 0) return false;
//
//            int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
//            if (dim != 3) { return false; }
//            return true;
//        };
//
//        switch (input->GetDataObjectType()) {
//            case IG_SURFACE_MESH: {
//                surface_Mesh = DynamicCast<SurfaceMesh>(input);
//                if (!CheckType()) return false;
//
//                ComputeVorticityForMesh(surface_Mesh, attributeSet, curIndex);
//
//            } break;
//            case IG_VOLUME_MESH: {
//                volume_Mesh = DynamicCast<VolumeMesh>(input);
//                ComputeVorticityForVol(volume_Mesh, attributeSet, curIndex);
//
//            } break;
//            case IG_UNSTRUCTURED_MESH: {
//                auto mesh = DynamicCast<UnstructuredMesh>(input);
//                surface_Mesh = mesh->TransferToSurfaceMesh();
//                volume_Mesh = mesh->TransferToVolumeMesh();
//
//                if (surface_Mesh) {
//                    if (!CheckType()) return false;
//
//                    ComputeVorticityForMesh(surface_Mesh, attributeSet, curIndex);
//                }
//
//                if (volume_Mesh) {
//                    if (!CheckType()) return false;
//
//                    //FloatArray::Pointer vorticities = FloatArray::New();
//                    //vorticities->SetDimension(3);
//                    //vorticities->SetName("vorticities");
//                    //input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                    //std::cout << "add vorticities\n";
//
//                    return ComputeVorticityForVol(volume_Mesh, attributeSet, curIndex);
//
//                    //// because surface_Mesh is mesh's DisplayObject
//                    //if (!mesh->GetDisplayObject()) { mesh->ConvertToDrawableData(); }
//
//                    //surface_Mesh = DynamicCast<SurfaceMesh>(mesh->GetDisplayObject());
//                    //if (!surface_Mesh) return false;
//
//                    //if (!CheckType()) return false;
//
//                    //FloatArray::Pointer vorticities = FloatArray::New();
//                    //vorticities->SetDimension(3);
//                    //vorticities->SetName("vorticities");
//                    //input->GetAttributeSet()->AddScalar(IG_POINT, vorticities);
//                }
//            } break;
//            default:
//                return false;
//        }
//
//        if (volume_Mesh) {}
//
//        if (surface_Mesh) {
//            //            attributeSet = surface_Mesh->GetAttributeSet();
//            //            if (attributeSet == nullptr) return false;
//            //
//            //            auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
//            //
//            //            int FaceNum = surface_Mesh->GetNumberOfFaces();
//            //            int PointNum = surface_Mesh->GetNumberOfPoints();
//            //            Points::Pointer Points = surface_Mesh->GetPoints();
//            //            surface_Mesh->RequestEditStatus();
//            //            if (PointNum != 0 && attachmentType == 0) {
//            //                return GetPointVortex(0, Points, PointNum);
//            //            } else if (FaceNum != 0 && attachmentType == 1)
//            //                return GetOtherVortex(0, FaceNum);
//        }
//
//        return true;
//    }
//
//    // =========================================================修改=========================================================
//
//    float ComputeCellVolume(Cell* cell) {
//        switch (cell->GetCellType()) {
//            case IG_TETRA:
//                //return ComputeTetVolume(cell);
//            case IG_HEXAHEDRON:
//                //return ComputeHexVolume(cell);
//            default:
//                return 1.0f;
//        }
//    }
//    std::array<float, 3> ComputeCellGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//        switch (cell->GetCellType()) {
//            case IG_TETRA: // 纯四面体
//                return ComputeTetGradient(cell, data, dim);
//            case IG_HEXAHEDRON: // 纯六面体
//                return ComputeHexGradient(cell, data, dim);
//            default:
//                return ComputePolyGradient(cell, data, dim);;
//        }
//    }
//    // // ===========================表面网格===========================
//    bool ComputeVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex) {
//
//        int PointNum = surface_Mesh->GetNumberOfPoints();
//        int numCells = surface_Mesh->GetNumberOfFaces();
//
//
//        std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
//        std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
//        std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
//        std::vector<float> volumes(PointNum, 0.0f);
//
//        auto data = attributeSet->GetAttribute(curIndex).pointer;
//
//        for (int cellId = 0; cellId < numCells; ++cellId) {
//            auto cell = surface_Mesh->GetFace(cellId);
//            float cellVolume = ComputeCellVolume(cell);
//            auto grad_x = ComputeCellGradient(cell, data, 0);
//            auto grad_y = ComputeCellGradient(cell, data, 1);
//            auto grad_z = ComputeCellGradient(cell, data, 2);
//
//            for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//                igIndex pid = cell->GetPointId(i);
//                for (int d = 0; d < 3; d++) {
//                    gradients_x[pid][d] += grad_x[d] * cellVolume;
//                    gradients_y[pid][d] += grad_y[d] * cellVolume;
//                    gradients_z[pid][d] += grad_z[d] * cellVolume;
//                }
//                volumes[pid] += cellVolume;
//            }
//        }
//
//        for (int i = 0; i < PointNum; ++i)
//            //if (volumes[i] > 1e-6f)
//                for (int d = 0; d < 3; d++) {
//                    gradients_x[i][d] /= volumes[i];
//                    gradients_y[i][d] /= volumes[i];
//                    gradients_z[i][d] /= volumes[i];
//                }
//
//        FloatArray::Pointer vorticities = FloatArray::New();
//        vorticities->SetDimension(3);
//        vorticities->Reserve(PointNum);
//        vorticities->SetName("vorticities");
//        attributeSet->AddScalar(IG_POINT, vorticities);
//
//        for (int i = 0; i < PointNum; ++i) {
//            float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
//            float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
//            float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y
//
//            float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
//            if (mag > 1e-6f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
//            else
//                vorticities->AddElement3(0, 0, 0);
//        }
//        return true;
//    }
//
//    // ===========================体网格===========================
//    bool ComputeVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex) {
//
//        int PointNum = volume_Mesh->GetNumberOfPoints();
//        int numCells = volume_Mesh->GetNumberOfVolumes();
//        auto data = attributeSet->GetAttribute(curIndex).pointer;
//
//        std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
//        std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
//        std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
//        std::vector<float> volumes(PointNum, 0.0f);
//        
//
//
//        for (int cellId = 0; cellId < numCells; ++cellId) {
//            auto cell = volume_Mesh->GetVolume(cellId);
//            float cellVolume = ComputeCellVolume(cell);
//            auto grad_x = ComputeCellGradient(cell, data, 0);
//            auto grad_y = ComputeCellGradient(cell, data, 1);
//            auto grad_z = ComputeCellGradient(cell, data, 2);
//
//            for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//                igIndex pid = cell->GetPointId(i);
//                for (int d = 0; d < 3; d++) {
//                    gradients_x[pid][d] += grad_x[d] * cellVolume;
//                    gradients_y[pid][d] += grad_y[d] * cellVolume;
//                    gradients_z[pid][d] += grad_z[d] * cellVolume;
//                }
//                volumes[pid] += cellVolume;
//            }
//        }
//
//        for (int i = 0; i < PointNum; ++i)
//            //if (volumes[i] > 1e-5f)
//                for (int d = 0; d < 3; d++) {
//                    gradients_x[i][d] /= volumes[i];
//                    gradients_y[i][d] /= volumes[i];
//                    gradients_z[i][d] /= volumes[i];
//                }
//
//        FloatArray::Pointer vorticities = FloatArray::New();
//        vorticities->SetDimension(3);
//        vorticities->Reserve(PointNum);
//        vorticities->SetName("vorticities");
//        attributeSet->AddScalar(IG_POINT, vorticities);
//
//        //FloatArray::Pointer gradient = FloatArray::New();
//        //gradient->SetDimension(3);
//        //gradient->Reserve(PointNum);
//        //gradient->SetName("gradient");
//        //attributeSet->AddScalar(IG_POINT, gradient);
//        //for (int i = 0; i < PointNum; ++i) {
//        //    gradient->AddElement3(gradients_x[i][0], gradients_x[i][1], gradients_x[i][2]);
//        //}
//
//        for (int i = 0; i < PointNum; ++i) {
//            float omega_x = gradients_z[i][1] - gradients_y[i][2]; // ∂vz/∂y - ∂vy/∂z
//            float omega_y = gradients_x[i][2] - gradients_z[i][0]; // ∂vx/∂z - ∂vz/∂x
//            float omega_z = gradients_y[i][0] - gradients_x[i][1]; // ∂vy/∂x - ∂vx/∂y
//
//            float mag = sqrt(omega_x * omega_x + omega_y * omega_y + omega_z * omega_z);
//            //if (mag > 1e-5f) vorticities->AddElement3(omega_x / mag, omega_y / mag, omega_z / mag);
//            vorticities->AddElement3(omega_x, omega_y, omega_z);
//            //else
//            //    vorticities->AddElement3(0, 0, 0);
//        }
//        return true;
//    }
//
//    // 四面体线性插值
//    std::array<float, 3> ComputeTetGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//        float centerValue = 0.0f;
//
//        for (int i = 0; i < 4; i++) {
//            auto p = cell->GetPoint(i);
//            center[0] += p[0];
//            center[1] += p[1];
//            center[2] += p[2];
//            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//        }
//        for (int d = 0; d < 3; d++) center[d] /= 4.0f;
//        centerValue /= 4.0f;
//
//        std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//        for (int i = 0; i < 4; ++i) {
//            auto p = cell->GetPoint(i);
//            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//        }
//        for (int d = 0; d < 3; d++) gradient[d] /= 4.0f;
//
//        return gradient;
//    }
//
//    float ComputeTetVolume(Cell* cell) {
//        auto p0 = cell->GetPoint(0);
//        auto p1 = cell->GetPoint(1);
//        auto p2 = cell->GetPoint(2);
//        auto p3 = cell->GetPoint(3);
//
//        std::array<float, 3> a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
//        std::array<float, 3> b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
//        std::array<float, 3> c = {p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]};
//
//        std::array<float, 3> cross_bc = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2],
//                                         b[0] * c[1] - b[1] * c[0]};
//
//        float dot_a = a[0] * cross_bc[0] + a[1] * cross_bc[1] + a[2] * cross_bc[2];
//        return std::abs(dot_a) / 6.0f;
//    }
//
//
//    // 六面体中心差分
//    std::array<float, 3> ComputeHexGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//        float centerValue = 0.0f;
//
//        for (int i = 0; i < 8; i++) {
//            auto p = cell->GetPoint(i);
//            center[0] += p[0];
//            center[1] += p[1];
//            center[2] += p[2];
//            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//        }
//        for (int d = 0; d < 3; d++) center[d] /= 8.0f;
//        centerValue /= 8.0f;
//
//        std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//        for (int i = 0; i < 8; ++i) {
//            auto p = cell->GetPoint(i);
//            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//        }
//        for (int d = 0; d < 3; d++) gradient[d] /= 8.0f;
//
//        return gradient;
//    }
//
//    // 多面体中心差分
//    std::array<float, 3> ComputePolyGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//        std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//        float centerValue = 0.0f;
//
//        for (int i = 0; i < cell->GetNumberOfPoints(); i++) {
//            auto p = cell->GetPoint(i);
//            center[0] += p[0];
//            center[1] += p[1];
//            center[2] += p[2];
//            centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//        }
//        for (int d = 0; d < 3; d++) center[d] /= cell->GetNumberOfPoints();
//        centerValue /= cell->GetNumberOfPoints();
//
//        std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//            auto p = cell->GetPoint(i);
//            std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//            float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//            for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//        }
//        for (int d = 0; d < 3; d++) gradient[d] /= cell->GetNumberOfPoints();
//
//        return gradient;
//    }
//
//    bool InverseMatrix4x4(const float in[4][4], float out[4][4]) {
//        float aug[4][8] = {0};
//        for (int i = 0; i < 4; ++i) {
//            for (int j = 0; j < 4; ++j) {
//                aug[i][j] = in[i][j];
//                aug[i][j + 4] = (i == j) ? 1.0f : 0.0f;
//            }
//        }
//
//        for (int col = 0; col < 4; ++col) {
//            int max_row = col;
//            for (int i = col + 1; i < 4; ++i) {
//                if (std::abs(aug[i][col]) > std::abs(aug[max_row][col])) { max_row = i; }
//            }
//
//            // 如果最大主元接近零，矩阵奇异
//            if (std::abs(aug[max_row][col]) < 1e-12f) { return false; }
//
//            if (max_row != col) {
//                for (int j = col; j < 8; ++j) { std::swap(aug[col][j], aug[max_row][j]); }
//            }
//
//            float pivot = aug[col][col];
//            for (int j = col; j < 8; ++j) { aug[col][j] /= pivot; }
//
//            for (int i = 0; i < 4; ++i) {
//                if (i != col && std::abs(aug[i][col]) > 1e-12f) {
//                    float factor = aug[i][col];
//                    for (int j = col; j < 8; ++j) { aug[i][j] -= factor * aug[col][j]; }
//                }
//            }
//        }
//
//        for (int i = 0; i < 4; ++i) {
//            for (int j = 0; j < 4; ++j) { out[i][j] = aug[i][j + 4]; }
//        }
//        return true;
//    }
//
//
//    // =========================================================修改=========================================================
//
//
//protected:
//    VortexFilter()
//    //输入输出个数
//    {
//        SetNumberOfInputs(1);
//        SetNumberOfOutputs(1);
//    }
//    ~VortexFilter() override = default;
//
//    VolumeMesh::Pointer volume_Mesh{};
//    SurfaceMesh::Pointer surface_Mesh{};
//    AttributeSet* attributeSet{nullptr};
//
//    int curIndex{-1};
//    int curDim{-1};
//};
//
//IGAME_NAMESPACE_END
//#endif


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
    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
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

    float ComputeTriangleArea(Cell* cell) {
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

    // 三角形
    std::array<float, 3> ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 四边形
    std::array<float, 3> ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 多边形
    std::array<float, 3> ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    // 四面体线性插值 point
    std::array<float, 3> ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

    float ComputeAverageEdgeLength(Cell* cell) {
        float totalLength = 0.0f;
        int numEdges = 6; // 四面体有6条边
        int edgePairs[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};

        for (int i = 0; i < 6; i++) {
            auto p1 = cell->GetPoint(edgePairs[i][0]);
            auto p2 = cell->GetPoint(edgePairs[i][1]);

            float dx = p1[0] - p2[0];
            float dy = p1[1] - p2[1];
            float dz = p1[2] - p2[2];

            totalLength += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        for (int d = 0; d < 3; d++) center[d] /= 4.0f;
        centerValue /= 4.0f;

        return totalLength / numEdges;
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