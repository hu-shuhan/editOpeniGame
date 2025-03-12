#include "iGameVortexFilter.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
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
    int dimension = OriArray->GetDimension();

    auto NewArray = FloatArray::New();
    NewArray->SetName(OriArray->GetName());
    NewArray->SetDimension(dimension);
    NewArray->Reserve(PointNum);

    std::vector<int> PointAdjNum(PointNum, 0);

    igIndex cell[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < Cell->GetNumberOfCells(); ++i) 
    { 
        int size = Cell->GetCellIds(i, cell);
        for (int j = 0; j < size; ++j)
        {

        }
    }
    return nullptr;
    //for (int id = 0; id < PointNum; id++) {
    //    double new_values[3] = {0, 0, 0};
    //    int NeiNum = adj[id].size();
    //    for (int j = 0; j < NeiNum; j++) {
    //        Ori_Array->GetElement(adj[id][j], values);
    //        for (int k = 0; k < dimension; k++) { new_values[k] += values[k]; }
    //    }
    //    if (NeiNum) {
    //        for (int k = 0; k < dimension; k++) { new_values[k] /= NeiNum; }
    //    }
    //    Array->AddElement(new_values);
    //}
    //for (int id = PointNum; id < New_Points->GetNumberOfPoints(); id++) {
    //    double new_values[3] = {0, 0, 0};
    //    int NeiNum = adj[id].size();
    //    for (int j = 0; j < NeiNum; j++) {
    //        Array->GetElement(adj[id][j], values);
    //        for (int k = 0; k < dimension; k++) { new_values[k] += values[k]; }
    //    }
    //    if (NeiNum) {
    //        for (int k = 0; k < dimension; k++) { new_values[k] /= NeiNum; }
    //    }
    //    Array->AddElement(new_values);
    //}
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

float VortexFilter::ComputeAverageEdgeLength(Cell* cell) {
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

    return totalLength / numEdges;
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

IGAME_NAMESPACE_END