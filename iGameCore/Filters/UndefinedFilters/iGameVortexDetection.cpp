#include "iGameVortexDetection.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameStreamTracer.h"
#include "iGameSurfaceMesh.h"
#include "iGameThreadPool.h"
#include "iGameUnstructuredMesh.h"
#include <Eigen/Dense>
#include <cmath>
#include <filesystem>
#include <omp.h>
#include <semaphore>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN
bool VortexDetection::Execute() {
#if defined(LibTorch_ENABLE)

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
        name = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetName();
        return true;
    };

    SetOutput(input);

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            surface_Mesh = DynamicCast<SurfaceMesh>(input);
            if (!CheckType()) return false;
            return DetectionVortexWithSurfaceMesh(surface_Mesh, attributeSet, curIndex, name);
        } break;
        case IG_VOLUME_MESH: {
            volume_Mesh = DynamicCast<VolumeMesh>(input);
            if (!CheckType()) return false;
            return DetectionVortexWithVolumeMesh(volume_Mesh, attributeSet, curIndex, name);
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            surface_Mesh = mesh->TransferToSurfaceMesh();
            volume_Mesh = mesh->TransferToVolumeMesh();

            if (surface_Mesh) {
                if (!CheckType()) return false;
                return DetectionVortexWithSurfaceMesh(surface_Mesh, attributeSet, curIndex, name);
            }

            if (volume_Mesh) {
                if (!CheckType()) return false;
                return DetectionVortexWithVolumeMesh(volume_Mesh, attributeSet, curIndex, name);
            }
        } break;
        default:
            return false;
    }
#else
    std::cout << "LibTorch is not enabled." << std::endl;
#endif
    return true;
}
VortexDetection::VortexDetection() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}


#if defined(LibTorch_ENABLE)

struct KDTree {
    struct Node {
        int idx = -1;
        int left = -1, right = -1;
        uint8_t axis = 0;
        double split = 0.0;
    };

    const Eigen::MatrixXd& pts;
    std::vector<Node> nodes;
    int root = -1;

    explicit KDTree(const Eigen::MatrixXd& points) : pts(points) {
        std::vector<int> idxs(pts.rows());
        std::iota(idxs.begin(), idxs.end(), 0);
        nodes.reserve(pts.rows());
        root = build(idxs, 0, (int) idxs.size(), /*axis=*/0);
    }

    void query(const Eigen::VectorXd& query_point, int k, std::vector<int>& result,
               std::vector<double>& distances) const {
        if (pts.rows() == 0 || k <= 0) {
            result.clear();
            distances.clear();
            return;
        }
        using Item = std::pair<double, int>; // (squared_dist, idx)
        auto cmp = [](const Item& a, const Item& b) { return a.first < b.first; };
        std::priority_queue<Item, std::vector<Item>, decltype(cmp)> heap(cmp);

        std::function<void(int)> dfs = [&](int ni) {
            if (ni == -1) return;
            const Node& nd = nodes[ni];
            const int pi = nd.idx;

            double d2 = sqDist(query_point, pi);
            if ((int) heap.size() < k) heap.emplace(d2, pi);
            else if (d2 < heap.top().first) {
                heap.pop();
                heap.emplace(d2, pi);
            }

            int ax = nd.axis;
            double diff = query_point[ax] - nd.split;
            int first = (diff < 0.0) ? nd.left : nd.right;
            int second = (diff < 0.0) ? nd.right : nd.left;

            dfs(first);

            double worst = (heap.size() < (size_t) k) ? std::numeric_limits<double>::infinity() : heap.top().first;
            if (diff * diff <= worst) dfs(second);
        };

        dfs(root);

        int m = (int) heap.size();
        result.resize(m);
        distances.resize(m);
        for (int i = m - 1; i >= 0; --i) {
            result[i] = heap.top().second;
            distances[i] = std::sqrt(heap.top().first);
            heap.pop();
        }
    }

private:
    int build(std::vector<int>& idxs, int l, int r, int axis) {
        if (l >= r) return -1;
        int m = (l + r) / 2;

        std::nth_element(idxs.begin() + l, idxs.begin() + m, idxs.begin() + r,
                         [&](int a, int b) { return pts(a, axis) < pts(b, axis); });

        int cur = (int) nodes.size();
        nodes.push_back({});
        nodes[cur].idx = idxs[m];
        nodes[cur].axis = (uint8_t) axis;
        nodes[cur].split = pts(idxs[m], axis);

        int next_axis = (axis + 1) % 3;
        nodes[cur].left = build(idxs, l, m, next_axis);
        nodes[cur].right = build(idxs, m + 1, r, next_axis);
        return cur;
    }

    inline double sqDist(const Eigen::VectorXd& q, int pi) const {
        double dx = q[0] - pts(pi, 0);
        double dy = q[1] - pts(pi, 1);
        double dz = q[2] - pts(pi, 2);
        return dx * dx + dy * dy + dz * dz;
    }
};

struct BlockInfo {
    Vector3f minP, maxP;
    Vector3f step;
    int nx, ny, nz;
    std::vector<int64_t> prediction;
};

bool VortexDetection::DetectionVortexWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                     int Index, std::string name) {

    return true;
}

bool VortexDetection::DetectionVortexWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                    int Index, std::string name) {

    //std::cout << "Hardware supports: " << std::thread::hardware_concurrency() << " threads.\n";

    auto t0 = std::chrono::high_resolution_clock::now();

    int NumPoints = Mesh->GetNumberOfPoints();

    ArrayObject::Pointer Data = Attributes->GetAttribute(Index).pointer;

    ArrayObject::Pointer velocityData = Attributes->GetAttribute(Index).pointer;
    std::vector<Vector3f> gridPoints;
    std::vector<Vector3f> gridVelocities;

    for (int i = 0; i < NumPoints; ++i) {
        Vector3f pt = Mesh->GetPoint(i);

        float vel_1 = velocityData->GetValue(i * 3 + 0);
        float vel_2 = velocityData->GetValue(i * 3 + 1);
        float vel_3 = velocityData->GetValue(i * 3 + 2);

        Vector3f vel(vel_1, vel_2, vel_3);

        gridPoints.push_back(pt);
        gridVelocities.push_back(vel);
    }

    int NumCells = Mesh->GetNumberOfVolumes();
    std::vector<Volume*> gridCells;
    gridCells.reserve(NumCells);

    for (int i = 0; i < NumCells; i++) {
        auto ct = Mesh->GetVolume(i);
        gridCells.push_back(ct);
    }

    auto boundBox = Mesh->GetBoundingBox();
    Vector3f maxPosition(boundBox.max);
    Vector3f minPosition(boundBox.min);

    Vector3f range = maxPosition - minPosition;
    float maxLen = std::max({range[0], range[1], range[2]});

    int split = 3;

    double min_effective_edge = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 4.0);
    int targetPoints = int(maxLen / (min_effective_edge + 1e-8)) + 1;

    targetPoints = targetPoints / split / 4;

    std::string model_path = "../../../iGameCore/Filters/UndefinedFilters/model_1x64x64x64_0810_cpu.pt";

    std::tuple<torch::Tensor, Eigen::Vector3f> result =
            process_blocks(gridPoints, gridVelocities, minPosition, maxPosition, model_path, targetPoints, split);

    torch::Tensor result_volume_11 = std::get<0>(result);
    Eigen::Vector3f global_step = std::get<1>(result);

    Eigen::Vector3f eigen_min(minPosition[0], minPosition[1], minPosition[2]);

    std::vector<Eigen::Vector3f> eigenPoints;
    eigenPoints.reserve(gridPoints.size());

    for (const auto& p: gridPoints) { eigenPoints.emplace_back(p[0], p[1], p[2]); }

    torch::Tensor smooth_vals = knn_smooth_labels(result_volume_11, eigen_min, global_step, eigenPoints, 16);

    std::vector<float> Predict(NumPoints, 0.0f);

    FloatArray::Pointer vortexs = FloatArray::New();
    vortexs->SetDimension(1);
    vortexs->Reserve(NumPoints);
    vortexs->SetName("vortexPredict");
    attributeSet->AddScalar(IG_POINT, vortexs);

    for (int i = 0; i < NumPoints; ++i) {
        float value = smooth_vals[i].item<float>();
        vortexs->AddValue(value);
        Predict[i] = value;
        //predictions.push_back(value);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "[VortexDetection::Execute] Total time = " << elapsed << " s" << std::endl;

    EvaluatePredictMetrics(Mesh, Attributes, Index, Predict);

    return true;
}


void VortexDetection::EvaluatePredictMetrics(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index,
                                             const std::vector<float>& Predict) {
    std::vector<float> Q = ComputePointQForVol(Mesh, Attributes, Index);
    if (Q.size() != Predict.size()) { return; }

    const size_t N = Q.size();
    size_t count = 0;

    for (size_t i = 0; i < N; ++i) {
        int gt = (Q[i] > 0.0f) ? 1 : 0;
        int pred = (Predict[i] > 0.01f) ? 1 : 0;
        if ((pred == 1 && gt == 1) || (pred == 0 && gt == 0)) ++count;
    }

    const double eps = 1e-12;
    double accuracy = static_cast<double>(count) / std::max<size_t>(1, N);
    std::cout << "[VortexDetection::Execute] Accuracy  = " << accuracy << "\n";
}

std::vector<float> VortexDetection::ComputePointQForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
                                                        int curIndex) {
    int PointNum = volume_Mesh->GetNumberOfPoints();
    int numCells = volume_Mesh->GetNumberOfVolumes();
    ArrayObject::Pointer data = attributeSet->GetAttribute(curIndex).pointer;
    if (attributeSet->GetAttribute(curIndex).attachmentType == IG_CELL) {
        data = AttributeCell2Point(volume_Mesh->GetCells(), data, PointNum);
    }


    std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
    std::vector<float> volumes(PointNum, 0.0f);
    std::vector<int> deg(PointNum, 0);

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
            deg[pid]++;
        }
    }
    for (int i = 0; i < PointNum; ++i) {
        if (deg[i] > 0) {
            const float inv = 1.0f / static_cast<float>(deg[i]);
            for (int d = 0; d < 3; ++d) {
                gradients_x[i][d] *= inv;
                gradients_y[i][d] *= inv;
                gradients_z[i][d] *= inv;
            }
        }
    }

    std::vector<float> Q(PointNum, 0.0f);
    //FloatArray::Pointer QCri = FloatArray::New();
    //QCri->SetDimension(1);
    //QCri->Reserve(PointNum);
    //QCri->SetName("QCriteria");
    //attributeSet->AddScalar(IG_POINT, QCri);

    for (int i = 0; i < PointNum; ++i) {

        const float ux = gradients_x[i][0], uy = gradients_x[i][1], uz = gradients_x[i][2];
        const float vx = gradients_y[i][0], vy = gradients_y[i][1], vz = gradients_y[i][2];
        const float wx = gradients_z[i][0], wy = gradients_z[i][1], wz = gradients_z[i][2];

        const float omega_x = wy - vz; // ∂w/∂y - ∂v/∂z
        const float omega_y = uz - wx; // ∂u/∂z - ∂w/∂x
        const float omega_z = vx - uy; // ∂v/∂x - ∂u/∂y

        // S = 0.5 (J + J^T)
        const float Sxx = ux, Syy = vy, Szz = wz;
        const float Sxy = 0.5f * (uy + vx);
        const float Sxz = 0.5f * (uz + wx);
        const float Syz = 0.5f * (vz + wy);

        const float Oxy = 0.5f * (uy - vx);
        const float Oxz = 0.5f * (uz - wx);
        const float Oyz = 0.5f * (vz - wy);

        const float S2 = (Sxx * Sxx + Syy * Syy + Szz * Szz) + 2.0f * (Sxy * Sxy + Sxz * Sxz + Syz * Syz);
        const float O2 = 2.0f * (Oxy * Oxy + Oxz * Oxz + Oyz * Oyz);

        const float Qval = 0.5f * (O2 - S2);
        Q[i] = (Qval > 0.0025f) ? 1.0f : 0.0f;
        //QCri->AddValue(Q[i]);
    }
    return Q;
}

std::array<float, 3> VortexDetection::ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
    if (type == 1) {
        switch (cell->GetCellType()) {
            case IG_TETRA: // 纯四面体
                return ComputeTetPointGradient(cell, data, dim);
            case IG_HEXAHEDRON: // 纯六面体
                return ComputeHexPointGradient(cell, data, dim);
            default: // 其他
                return ComputePolyPointGradient(cell, data, dim);
        }
    }
}

ArrayObject::Pointer VortexDetection::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
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

std::array<float, 3> VortexDetection::ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
    float centerValue = 0.0f;
    float tetVolume = ComputeTetVolume(cell);
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

std::array<float, 3> VortexDetection::ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

std::array<float, 3> VortexDetection::ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
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

float VortexDetection::ComputeTetVolume(Cell* cell) {
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

float VortexDetection::ComputeAverageEdgeLength(Cell* cell) {
    int num = cell->GetNumberOfEdges();
    float totalLength = 0.0f;
    for (int i = 0; i < num; ++i) {
        auto* e = cell->GetEdge(i);
        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
    }
    return totalLength / num;
}

torch::Tensor VortexDetection::sigmoid(const torch::Tensor& x) { return 1.0 / (1.0 + (-x).exp()); }

torch::Tensor VortexDetection::padTensor(const torch::Tensor& tensor, int pad_z, int pad_y, int pad_x) {
    std::vector<int64_t> padding = {0, 0, 0, pad_x, 0, pad_y, 0, pad_z};
    return torch::constant_pad_nd(tensor, padding, 0);
}

Vector3f VortexDetection::nearestVector(const Vector3f& pos, const std::vector<Vector3f>& gridPoints,
                                        const std::vector<Vector3f>& gridVelocities, bool& inside) {
    float minDist = std::numeric_limits<float>::max();
    Vector3f nearestVel = {0, 0, 0};
    inside = false;

    for (size_t i = 0; i < gridPoints.size(); ++i) {
        float dist = (gridPoints[i] - pos).squaredNorm();
        if (dist < minDist) {
            minDist = dist;
            nearestVel = gridVelocities[i];
            inside = true;
        }
    }

    return nearestVel;
}

std::vector<torch::Tensor> VortexDetection::extractPatches(const torch::Tensor& tensor, int patch_size, int stride) {
    std::vector<torch::Tensor> patches;
    int z_size = tensor.size(0);
    int y_size = tensor.size(1);
    int x_size = tensor.size(2);
    int num_features = tensor.size(3);

    for (int z = 0; z <= z_size - patch_size; z += stride)
        for (int y = 0; y <= y_size - patch_size; y += stride)
            for (int x = 0; x <= x_size - patch_size; x += stride) {
                torch::Tensor patch =
                        tensor.slice(0, z, z + patch_size).slice(1, y, y + patch_size).slice(2, x, x + patch_size);
                patch = patch.permute({3, 0, 1, 2}).unsqueeze(0); // [1, 3, D, H, W]
                patches.push_back(patch);
            }
    return patches;
}

//std::vector<int64_t> VortexDetection::runPrediction(const std::vector<Vector3f>& points,
//                                                    const std::vector<Vector3f>& velocities,
//                                                    const Vector3f& minPosition, const Vector3f& maxPosition, int nx,
//                                                    int ny, int nz) {
//    const int patch_size = 64;
//    const int stride = 32;
//
//    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
//    //torch::Device device(torch::kCPU);
//
//    // 构造 3D Tensor
//
//    std::cout << nx << " " << ny << " " << nz << std::endl;
//    // torch::Tensor tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
//    torch::Tensor tensor = torch::zeros({nz, ny, nx, 3});
//
//    Vector3f range = maxPosition - minPosition;
//    if (range[0] == 0 || range[1] == 0 || range[2] == 0) {
//        std::cout << "range is zero" << std::endl;
//        // return;
//    }
//
//    for (size_t i = 0; i < points.size(); ++i) {
//        const auto& pos = points[i];
//        const auto& vel = velocities[i];
//        int x = static_cast<int>((pos[0] - minPosition[0]) / range[0] * (nx - 1));
//        int y = static_cast<int>((pos[1] - minPosition[1]) / range[1] * (ny - 1));
//        int z = static_cast<int>((pos[2] - minPosition[2]) / range[2] * (nz - 1));
//        if (x >= 0 && x < nx && y >= 0 && y < ny && z >= 0 && z < nz) {
//            // tensor.index({z, y, x, 0}) = vel[0];
//            // tensor.index({z, y, x, 1}) = vel[1];
//            // tensor.index({z, y, x, 2}) = vel[2];
//            tensor[z][y][x][0] = vel[0];
//            tensor[z][y][x][1] = vel[1];
//            tensor[z][y][x][2] = vel[2];
//        }
//    }
//
//    // Padding
//    int pad_z = (std::ceil(static_cast<float>(nz) / patch_size) * patch_size) - nz;
//    int pad_y = (std::ceil(static_cast<float>(ny) / patch_size) * patch_size) - ny;
//    int pad_x = (std::ceil(static_cast<float>(nx) / patch_size) * patch_size) - nx;
//    torch::Tensor padded = padTensor(tensor, pad_z, pad_y, pad_x);
//
//    std::vector<torch::Tensor> patches = extractPatches(padded, patch_size, stride);
//
//    std::cout << "model_path" << std::endl;
//    std::string model_path = "../../../iGameCore/Filters/UndefinedFilters/model_1x64x64x64_0810.pt";
//    std::filesystem::path model_path_act = std::filesystem::absolute(model_path);
//    torch::jit::script::Module model;
//
//    try {
//        model = torch::jit::load(model_path_act.string());
//        std::cout << "Model loaded successfully." << std::endl;
//    } catch (const c10::Error& e) { std::cerr << "Error loading the model." << std::endl; }
//
//    model.to(device);
//    // model.to(torch::kCPU);
//
//    std::cout << "model.eval()" << std::endl;
//    model.eval();
//
//    std::vector<torch::Tensor> predictions;
//    for (const auto& patch: patches) {
//        torch::Tensor input = patch.to(device);
//        torch::Tensor pred = model.forward({input}).toTensor();
//
//        // softmax
//        torch::Tensor prob = torch::nn::functional::softmax(pred, /*dim=*/1);
//        predictions.push_back(prob);
//    }
//
//    int padded_nz = nz + pad_z;
//    int padded_ny = ny + pad_y;
//    int padded_nx = nx + pad_x;
//    int num_classes = predictions[0].size(1);
//    torch::Tensor prediction_full = torch::zeros({1, num_classes, padded_nz, padded_ny, padded_nx});
//
//    int patch_idx = 0;
//    for (int pz = 0; pz < padded_nz / patch_size; ++pz) {
//        for (int py = 0; py < padded_ny / patch_size; ++py) {
//            for (int px = 0; px < padded_nx / patch_size; ++px) {
//                int start_z = pz * patch_size;
//                int start_y = py * patch_size;
//                int start_x = px * patch_size;
//                torch::Tensor pred = predictions[patch_idx].squeeze(0);
//                prediction_full.slice(2, start_z, start_z + patch_size)
//                        .slice(3, start_y, start_y + patch_size)
//                        .slice(4, start_x, start_x + patch_size) = pred;
//                patch_idx++;
//            }
//        }
//    }
//
//    torch::Tensor prediction_original = prediction_full.slice(2, 0, nz).slice(3, 0, ny).slice(4, 0, nx);
//
//    torch::Tensor prediction_class = torch::argmax(prediction_original, 1); // shape: [1, D, H, W]
//    torch::Tensor flat = prediction_class.flatten();                        // shape: [N]
//
//    std::vector<int64_t> result(flat.numel());
//    std::memcpy(result.data(), flat.data_ptr<int64_t>(), sizeof(int64_t) * flat.numel());
//
//    return result;
//}


torch::Tensor VortexDetection::gaussian_kernel1d(float sigma, int radius) {
    int size = 2 * radius + 1;
    torch::TensorOptions options = torch::TensorOptions().dtype(torch::kFloat32);
    torch::Tensor kernel = torch::empty({size}, options);

    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        int x = i - radius;
        float v = std::exp(-(x * x) / (2 * sigma * sigma));
        kernel[i] = v;
        sum += v;
    }
    kernel /= sum; // 归一化
    return kernel;
}

torch::Tensor VortexDetection::gaussian_kernel3d(float sigma, int radius) {
    auto kx = gaussian_kernel1d(sigma, radius);
    auto ky = gaussian_kernel1d(sigma, radius);
    auto kz = gaussian_kernel1d(sigma, radius);

    auto k3 = kx.view({-1, 1, 1}) * ky.view({1, -1, 1}) * kz.view({1, 1, -1});
    k3 = k3 / k3.sum();
    return k3.view({1, 1, 2 * radius + 1, 2 * radius + 1, 2 * radius + 1});
}

torch::Tensor VortexDetection::gaussian_filter3d(torch::Tensor input, float sigma, int radius) {
    if (radius < 0) { radius = static_cast<int>(std::round(4.0f * sigma)); }

    auto kernel = gaussian_kernel3d(sigma, radius);
    int C = input.size(1);

    if (input.dim() == 3) {
        input = input.unsqueeze(0).unsqueeze(0); // [1,1,D,H,W]
        C = 1;
    } else if (input.dim() == 4) {           // [D,H,W,C]
        input = input.permute({3, 0, 1, 2}); // [C,D,H,W]
        input = input.unsqueeze(0);          // [1,C,D,H,W]
        C = input.size(1);
    } else {
        TORCH_CHECK(false, "Unsupported input dim: ", input.dim());
    }

    kernel = kernel.expand({C, 1, kernel.size(2), kernel.size(3), kernel.size(4)});
    auto conv_opts = torch::nn::functional::Conv3dFuncOptions().stride(1).padding(radius).groups(C);

    auto output = torch::nn::functional::conv3d(input, kernel, conv_opts); // [1,C,D,H,W]
    output = output.squeeze(0);

    if (C == 1) {
        output = output.squeeze(0); // [D,H,W]
    } else {
        output = output.permute({1, 2, 3, 0}); // [D,H,W,C]
    }

    return output;
}

double VortexDetection::compute_percentile_edge_length_from_cells(const std::vector<Vector3f>& points,
                                                                  const std::vector<Volume*>& cells,
                                                                  double percentile) {
    std::set<std::pair<int, int>> seen_edges;
    std::vector<double> edge_lengths;

    for (auto cell: cells) {
        int num_pts = cell->GetNumberOfPoints();
        for (int j = 0; j < num_pts; ++j) {
            for (int k = j + 1; k < num_pts; ++k) {
                int id1 = cell->GetPointId(j);
                int id2 = cell->GetPointId(k);
                if (id1 > id2) std::swap(id1, id2);

                auto edge_key = std::make_pair(id1, id2);
                if (seen_edges.find(edge_key) != seen_edges.end()) continue;
                seen_edges.insert(edge_key);

                double length = (points[id1] - points[id2]).norm();
                if (length > 1e-6) edge_lengths.push_back(length);
            }
        }
    }

    if (edge_lengths.empty()) return 0.01;
    std::sort(edge_lengths.begin(), edge_lengths.end());

    double rank = percentile / 100.0 * (edge_lengths.size() - 1);
    size_t low_idx = static_cast<size_t>(std::floor(rank));
    size_t high_idx = static_cast<size_t>(std::ceil(rank));
    double t = rank - low_idx;
    double percentile_val = edge_lengths[low_idx] * (1.0 - t) + edge_lengths[high_idx] * t;

    return percentile_val;
}

torch::Tensor VortexDetection::_hann3d(int patch_size) {
    torch::Tensor wz = torch::hann_window(patch_size, /*periodic=*/true).to(torch::kFloat32);
    torch::Tensor wy = torch::hann_window(patch_size, /*periodic=*/true).to(torch::kFloat32);
    torch::Tensor wx = torch::hann_window(patch_size, /*periodic=*/true).to(torch::kFloat32);

    torch::Tensor w3 = wz.view({patch_size, 1, 1}) * wy.view({1, patch_size, 1}) * wx.view({1, 1, patch_size});

    w3 = w3 / (w3.max() + 1e-8);
    w3 = w3.unsqueeze(0).unsqueeze(0);
    return w3;
}

std::tuple<torch::Tensor, int, int, int> VortexDetection::pad_tensor(const torch::Tensor& grid_tensor, int patch_size) {
    auto sizes = grid_tensor.sizes();
    int D = sizes[0];
    int H = sizes[1];
    int W = sizes[2];
    int C = sizes[3];

    int pad_D = (patch_size - D % patch_size) % patch_size;
    int pad_H = (patch_size - H % patch_size) % patch_size;
    int pad_W = (patch_size - W % patch_size) % patch_size;

    int Dp = D + pad_D, Hp = H + pad_H, Wp = W + pad_W;
    torch::Tensor padded = torch::zeros({Dp, Hp, Wp, C}, grid_tensor.options());
    padded.slice(0, 0, D).slice(1, 0, H).slice(2, 0, W).copy_(grid_tensor);

    /*torch::Tensor padded = torch::nn::functional::pad(
            grid_tensor, torch::nn::functional::PadFuncOptions({0, 0, 0, pad_W, 0, pad_H, 0, pad_D})
                                 .mode(torch::kConstant)
                                 .value(0));*/
    auto padded_sizes = padded.sizes();
    return {padded, pad_D, pad_H, pad_W};
}

std::vector<torch::Tensor> VortexDetection::extract_patches(const torch::Tensor& padded, int patch_size, int stride) {
    std::vector<torch::Tensor> patches;

    auto sizes = padded.sizes();
    int D = sizes[0];
    int H = sizes[1];
    int W = sizes[2];
    int C = sizes[3];

    for (int z = 0; z <= D - patch_size; z += stride) {
        for (int y = 0; y <= H - patch_size; y += stride) {
            for (int x = 0; x <= W - patch_size; x += stride) {
                torch::Tensor patch = padded.slice(0, z, z + patch_size)    // Slice depth dimension (D)
                                              .slice(1, y, y + patch_size)  // Slice height dimension (H)
                                              .slice(2, x, x + patch_size); // Slice width dimension (W)

                patch = patch.permute({3, 0, 1, 2}).unsqueeze(0); // [1, C, D, H, W]
                patches.push_back(patch);
            }
        }
    }
    return patches;
}


torch::Tensor VortexDetection::run_prediction_on_block(const torch::Tensor& grid_tensor, const std::string& model_path,
                                                       const torch::jit::script::Module& model) {
    int patch_size = 64;
    int stride = 32;
    //torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    torch::Device device(torch::kCPU);

    auto [padded, pad_D, apad_H, pad_W] = pad_tensor(grid_tensor, patch_size);
    std::vector<torch::Tensor> patches = extract_patches(padded, patch_size, stride);

    //model.to(device);
    //model.eval();
    auto padded_sizes = padded.sizes();
    int padded_D = padded_sizes[0];
    int padded_H = padded_sizes[1];
    int padded_W = padded_sizes[2];

    torch::Tensor prob_full;

    if (device.type() == torch::kCUDA) {
        prob_full = torch::zeros({1, 1, padded_D, padded_H, padded_W}, torch::kCUDA);
    } else {
        prob_full = torch::zeros({1, 1, padded_D, padded_H, padded_W}, torch::kCPU);
    }
    torch::Tensor w_full = torch::zeros_like(prob_full);
    torch::Tensor w_patch = _hann3d(patch_size);
    auto model_const = std::make_shared<const torch::jit::script::Module>(std::move(model));

    int patch_idx = 0;

    {
        torch::NoGradGuard no_grad;
        for (int z = 0; z < padded_D - patch_size + 1; z += stride) {
            for (int y = 0; y < padded_H - patch_size + 1; y += stride) {
                for (int x = 0; x < padded_W - patch_size + 1; x += stride) {
                    torch::Tensor patch; // [1, C, 64, 64, 64]
                    if (device.type() == torch::kCUDA) {
                        patch = patches[patch_idx].to(torch::kCUDA);
                    } else {
                        patch = patches[patch_idx].to(torch::kCPU);
                    }
                    /*c10::IValue out = model.forward({patch});
                    torch::Tensor logits = out.toTensor();*/
                    torch::Tensor logits =
                            const_cast<torch::jit::script::Module&>(*model_const).forward({patch}).toTensor();
                    //torch::Tensor logits = model.forward({patch}).toTuple()->elements()[0].toTensor(); // [1, 2, 64, 64, 64]
                    torch::Tensor prob;
                    if (logits.size(1) == 2) {
                        prob = torch::sigmoid(logits.slice(1, 1, 2)); // 取前景通道 [1, 1, 64, 64, 64]
                    } else {
                        prob = torch::sigmoid(logits);
                    }

                    torch::Tensor prob_w = prob * w_patch;

                    prob_full.slice(2, z, z + patch_size).slice(3, y, y + patch_size).slice(4, x, x + patch_size) +=
                            prob_w;
                    w_full.slice(2, z, z + patch_size).slice(3, y, y + patch_size).slice(4, x, x + patch_size) +=
                            w_patch;

                    patch_idx += 1;
                }
            }
        }
    }
    prob_full = prob_full / (w_full + 1e-8);

    auto grid_tensor_sizes = grid_tensor.sizes();
    int D = grid_tensor_sizes[0];
    int H = grid_tensor_sizes[1];
    int W = grid_tensor_sizes[2];

    torch::Tensor prob_cropped = prob_full.slice(2, 0, D).slice(3, 0, H).slice(4, 0, W).squeeze(0).squeeze(0);
    return prob_cropped;
}

//std::tuple<torch::Tensor, Eigen::Vector3f>
//VortexDetection::process_blocks_pre(const std::vector<Vector3f>& gridPoints, const std::vector<Vector3f>& gridVelocities,
//                                const Vector3f& min_pos, const Vector3f& max_pos, const std::string& model_path,
//                                int target_points, int split) {
//
//    torch::jit::script::Module model;
//
//    try {
//        model = torch::jit::load(model_path);
//        std::cout << "Model loaded successfully." << std::endl;
//    } catch (const c10::Error& e) { std::cerr << "Error loading the model." << e.what() << std::endl; }
//    Eigen::Vector3f min_pos_eigen(min_pos[0], min_pos[1], min_pos[2]);
//    Eigen::Vector3f max_pos_eigen(max_pos[0], max_pos[1], max_pos[2]);
//
//    Eigen::Vector3f range_vec = max_pos_eigen - min_pos_eigen;
//    Eigen::Vector3f block_size = range_vec / split;
//    //Eigen::Vector3f range_vec = max_pos - min_pos;
//    //Eigen::Vector3f block_size = range_vec / split;
//    // 初始化 KD-Tree
//    Eigen::MatrixXd points(gridPoints.size(), 3);
//    for (size_t i = 0; i < gridPoints.size(); ++i) {
//        points(i, 0) = gridPoints[i][0];
//        points(i, 1) = gridPoints[i][1];
//        points(i, 2) = gridPoints[i][2];
//    }
//
//    KDTree tree(points);
//
//    Eigen::MatrixXd velocities(gridVelocities.size(), 3);
//    for (size_t i = 0; i < gridVelocities.size(); ++i) {
//        velocities(i, 0) = gridVelocities[i][0];
//        velocities(i, 1) = gridVelocities[i][1];
//        velocities(i, 2) = gridVelocities[i][2];
//    }
//
//    Vector3f mean = {-1.572247e-04, -4.576315e-04, -2.9615819e-10};
//    Vector3f std = {2.6299512e-02, 2.8212167e-02, 1.9456959e-08};
//    //std::vector<torch::Tensor> all_results_1;
//
//    //for (int bz = 0; bz < split; ++bz) {
//    //    for (int by = 0; by < split; ++by) {
//    //        for (int bx = 0; bx < split; ++bx) {
//    //            Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
//    //            sub_min = sub_min + min_pos_eigen;
//    //            Eigen::Vector3f sub_max = sub_min + block_size;
//    //            Eigen::Vector3f sub_range = sub_max - sub_min;
//    //            float max_len = sub_range.maxCoeff();
//    //            float uniform_step = max_len / (target_points - 1);
//    //            int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
//    //            int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
//    //            int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
//    //            Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
//    //            //std::cout << "nx: " << nx << " ny: " << ny << " nz: " << nz << std::endl;
//    //            Eigen::Vector3f step = sub_range.cwiseQuotient(now);
//    //            torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
//    //            int K = 16; // Number of nearest neighbors
//    //            double eps = 1e-6;
//    //            std::vector<double> weights(K, 0.0);
//    //            std::vector<int> idxs(K, 0);
//    //            std::vector<double> dists(K, 0.0);
//    //            for (int i = 0; i < nx; ++i) {
//    //                //std::cout << "i" << i << std::endl;
//    //                for (int j = 0; j < ny; ++j) {
//    //                    for (int k = 0; k < nz; ++k) {
//    //                        Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
//    //                        Eigen::VectorXd pos_vec(3);
//    //                        pos_vec << pos[0], pos[1], pos[2];
//    //                        tree.query(pos_vec, K, idxs, dists);
//    //                        torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
//    //                        if (K == 1) {
//    //                            weighted_sum = torch::tensor(
//    //                                    {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
//    //                                    torch::kFloat32);
//    //                        } else {
//    //                            double dist_sum = 0.0;
//    //                            for (int l = 0; l < K; ++l) {
//    //                                double dist = dists[l];
//    //                                weights[l] = 1.0 / (dist * dist + eps);
//    //                                dist_sum += weights[l];
//    //                            }
//    //                            for (int l = 0; l < K; ++l) { weights[l] /= dist_sum; }
//    //                            for (int l = 0; l < K; ++l) {
//    //                                if (idxs[l] >= 0 && idxs[l] < velocities.rows()) {
//    //                                    weighted_sum += weights[l] *
//    //                                                    torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
//    //                                                                   velocities(idxs[l], 2)},
//    //                                                                  torch::kFloat32);
//    //                                } else {
//    //                                    std::cerr << "Index out of bounds: " << idxs[l] << std::endl;
//    //                                }
//    //                            }
//    //                        }
//    //                        grid_tensor[k][j][i] = weighted_sum;
//    //                    }
//    //                }
//    //            }
//    //            torch::Tensor arr = grid_tensor.clone();
//    //            for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
//    //            arr = sigmoid(arr);
//    //            std::cout << "model_prediction " << std::endl;
//    //            torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path);
//    //        }
//    //    }
//    //}
//
//    const int total_blocks = split * split * split;
//    std::vector<torch::Tensor> all_results_1(total_blocks);
//
//
//
//    /*int max_threads = (int) std::thread::hardware_concurrency();
//    int workers = std::min(total_blocks, max_threads);
//    static std::counting_semaphore<> infer_slots(workers);
//    at::set_num_threads(1);
//    at::set_num_interop_threads(1);
//    omp_set_num_threads(1);
//    std::atomic<int> next_block{0};*/
//    //auto worker_task = [&]() {
//    //    //static thread_local bool printed = false;
//    //    //if (!printed) {
//    //    //    printed = true;
//    //    //    std::cout << "[Thread Start] ID = " << std::this_thread::get_id() << std::endl;
//    //    //}
//    //    while (true) {
//    //        int id = next_block.fetch_add(1, std::memory_order_relaxed);
//    //        if (id >= total_blocks) break;
//    //        int bz = id / (split * split);
//    //        int by = (id / split) % split;
//    //        int bx = id % split;
//    //        Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
//    //        sub_min = sub_min + min_pos_eigen;
//    //        Eigen::Vector3f sub_max = sub_min + block_size;
//    //        Eigen::Vector3f sub_range = sub_max - sub_min;
//    //        float max_len = sub_range.maxCoeff();
//    //        float uniform_step = max_len / (target_points - 1);
//    //        int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
//    //        int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
//    //        int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
//    //        Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
//    //        Eigen::Vector3f step = sub_range.cwiseQuotient(now);
//    //        torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
//    //        const int K = 16;
//    //        const double eps = 1e-6;
//    //        std::vector<double> weights(K, 0.0);
//    //        std::vector<int> idxs(K, 0);
//    //        std::vector<double> dists(K, 0.0);
//    //        std::cout << "nx: " << nx << " ny: " << ny << " nz: " << nz << std::endl;
//    //        for (int i = 0; i < nx; ++i) {
//    //            for (int j = 0; j < ny; ++j) {
//    //                for (int k = 0; k < nz; ++k) {
//    //                    Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
//    //                    Eigen::VectorXd pos_vec(3);
//    //                    pos_vec << pos[0], pos[1], pos[2];
//    //                    tree.query(pos_vec, K, idxs, dists);
//    //                    torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
//    //                    if (K == 1) {
//    //                        weighted_sum = torch::tensor(
//    //                                {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
//    //                                torch::kFloat32);
//    //                    } else {
//    //                        double dist_sum = 0.0;
//    //                        for (int l = 0; l < K; ++l) {
//    //                            double dist = dists[l];
//    //                            weights[l] = 1.0 / (dist * dist + eps);
//    //                            dist_sum += weights[l];
//    //                        }
//    //                        for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
//    //                        for (int l = 0; l < K; ++l) {
//    //                            weighted_sum +=
//    //                                    weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
//    //                                                                velocities(idxs[l], 2)},
//    //                                                               torch::kFloat32);
//    //                        }
//    //                    }
//    //                    grid_tensor[k][j][i] = weighted_sum;
//    //                }
//    //            }
//    //        }
//    //        torch::Tensor arr = grid_tensor.clone();
//    //        for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
//    //        arr = sigmoid(arr);
//    //        infer_slots.acquire();
//    //        torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
//    //        infer_slots.release();
//    //        all_results_1[id] = std::move(pred_block_1);
//    //        std::cout << "end.  idx = " << id<< std::endl;
//    //    }
//    //};
//    //std::vector<std::thread> threads;
//    //threads.reserve(workers);
//    //for (int t = 0; t < workers; ++t) threads.emplace_back(worker_task);
//    //for (auto& t: threads) t.join();
//    //std::cout << "[All worker threads finished!]" << std::endl;
//
//
//    const double eps = 1e-6;
//    int max_threads = (int) std::thread::hardware_concurrency();
//    int workers = std::min(total_blocks, max_threads);
//    static std::counting_semaphore<> infer_slots(workers);
//
//    int progress = 0;
//    std::mutex progress_mutex;
//
//    auto process_blocks_range = [&](int begin, int end) {
//        static thread_local bool printed = false;
//        for (int id = begin; id < end; ++id) {
//            int bz = id / (split * split);
//            int by = (id / split) % split;
//            int bx = id % split;
//            Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
//            sub_min = sub_min + min_pos_eigen;
//            Eigen::Vector3f sub_max = sub_min + block_size;
//            Eigen::Vector3f sub_range = sub_max - sub_min;
//            float max_len = sub_range.maxCoeff();
//            float uniform_step = max_len / (target_points - 1);
//            int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
//            int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
//            int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
//            Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
//            Eigen::Vector3f step = sub_range.cwiseQuotient(now);
//            torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
//            /*const int K = 16;*/
//            const int K = 32;
//            std::vector<double> weights(K, 0.0);
//            std::vector<int> idxs(K, 0);
//            std::vector<double> dists(K, 0.0);
//            static std::atomic<int> test_counter{0};
//            for (int i = 0; i < nx; ++i) {
//                for (int j = 0; j < ny; ++j) {
//                    for (int k = 0; k < nz; ++k) {
//                        Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
//
//                        Eigen::VectorXd pos_vec(3);
//                        pos_vec << pos[0], pos[1], pos[2];
//                        tree.query(pos_vec, K, idxs, dists);
//                        torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
//                        if (K == 1) {
//                            weighted_sum = torch::tensor(
//                                    {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
//                                    torch::kFloat32);
//                        } else {
//                            double dist_sum = 0.0;
//                            for (int l = 0; l < K; ++l) {
//                                double dist = dists[l];
//                                weights[l] = 1.0 / (dist * dist + eps);
//                                dist_sum += weights[l];
//                            }
//                            for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
//                            for (int l = 0; l < K; ++l) {
//                                if (idxs[l] >= 0 && idxs[l] < velocities.rows()) {
//                                    weighted_sum +=
//                                            weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
//                                                                        velocities(idxs[l], 2)},
//                                                                       torch::kFloat32);
//                                }
//                            }
//                        }
//                        grid_tensor[k][j][i] = weighted_sum;
//                    }
//                }
//            }
//            torch::Tensor arr = grid_tensor.clone();
//            for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
//            arr = sigmoid(arr);
//            infer_slots.acquire();
//            torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
//            infer_slots.release();
//            all_results_1[id] = std::move(pred_block_1);
//
//        }
//    };
//
//    ThreadPool::parallelFor(0, total_blocks, process_blocks_range, workers);
//    //ThreadPool::parallelFor(0, total_blocks, process_blocks_range);
//
//    /*int max_threads = (int) std::thread::hardware_concurrency();
//    int workers = std::min(total_blocks, max_threads);
//    static std::counting_semaphore<> infer_slots(workers);*/
//
//
//    int nz_sub = all_results_1[0].size(0);
//    int ny_sub = all_results_1[0].size(1);
//    int nx_sub = all_results_1[0].size(2);
//
//    torch::Tensor result_volume_1 = torch::zeros({split * nz_sub, split * ny_sub, split * nx_sub}, torch::kFloat32);
//
//    int idx = 0;
//    for (int bz = 0; bz < split; ++bz) {
//        for (int by = 0; by < split; ++by) {
//            for (int bx = 0; bx < split; ++bx) {
//
//                torch::Tensor cur = all_results_1[idx];
//                auto sz = cur.sizes();
//                if (sz[0] != nz_sub || sz[1] != ny_sub || sz[2] != nx_sub) {
//                    cur = torch::nn::functional::pad(cur, torch::nn::functional::PadFuncOptions({
//                                                                  0, nx_sub - sz[2], // W
//                                                                  0, ny_sub - sz[1], // H
//                                                                  0, nz_sub - sz[0]  // D
//                                                          }));
//                }
//
//                int z_start = bz * nz_sub;
//                int y_start = by * ny_sub;
//                int x_start = bx * nx_sub;
//                result_volume_1.slice(0, z_start, z_start + nz_sub)
//                        .slice(1, y_start, y_start + ny_sub)
//                        .slice(2, x_start, x_start + nx_sub)
//                        .copy_(all_results_1[idx]);
//                ++idx;
//            }
//        }
//    }
//
//    // 计算全局步长
//    auto sizes = result_volume_1.sizes();
//    float depth = static_cast<float>(sizes[0]);
//    float height = static_cast<float>(sizes[1]);
//    float width = static_cast<float>(sizes[2]);
//
//    Eigen::Vector3f volume_size(width, height, depth);
//    Eigen::Vector3f global_step = range_vec.cwiseQuotient(volume_size);
//
//    torch::Tensor result_volume_11 = gaussian_filter3d(result_volume_1, 3, -1);
//    return std::make_tuple(result_volume_11, global_step);
//}

std::tuple<torch::Tensor, Eigen::Vector3f>
VortexDetection::process_blocks(const std::vector<Vector3f>& gridPoints, const std::vector<Vector3f>& gridVelocities,
                                const Vector3f& min_pos, const Vector3f& max_pos, const std::string& model_path,
                                int target_points, int split) {

    torch::jit::script::Module model;

    try {
        model = torch::jit::load(model_path);
        std::cout << "Model loaded successfully." << std::endl;
    } catch (const c10::Error& e) { std::cerr << "Error loading the model." << e.what() << std::endl; }
    Eigen::Vector3f min_pos_eigen(min_pos[0], min_pos[1], min_pos[2]);
    Eigen::Vector3f max_pos_eigen(max_pos[0], max_pos[1], max_pos[2]);

    Eigen::Vector3f range_vec = max_pos_eigen - min_pos_eigen;
    Eigen::Vector3f block_size = range_vec / split;
    //Eigen::Vector3f range_vec = max_pos - min_pos;
    //Eigen::Vector3f block_size = range_vec / split;
    // 初始化 KD-Tree
    Eigen::MatrixXd points(gridPoints.size(), 3);
    for (size_t i = 0; i < gridPoints.size(); ++i) {
        points(i, 0) = gridPoints[i][0];
        points(i, 1) = gridPoints[i][1];
        points(i, 2) = gridPoints[i][2];
    }

    KDTree tree(points);

    Eigen::MatrixXd velocities(gridVelocities.size(), 3);
    for (size_t i = 0; i < gridVelocities.size(); ++i) {
        velocities(i, 0) = gridVelocities[i][0];
        velocities(i, 1) = gridVelocities[i][1];
        velocities(i, 2) = gridVelocities[i][2];
    }

    Vector3f mean = {-1.572247e-04, -4.576315e-04, -2.9615819e-10};
    Vector3f std = {2.6299512e-02, 2.8212167e-02, 1.9456959e-08};

    const int total_blocks = split * split * split;
    std::vector<torch::Tensor> all_results_1(total_blocks);


    int max_threads = (int) std::thread::hardware_concurrency();
    int workers = std::min(total_blocks, max_threads);
    static std::counting_semaphore<> infer_slots(workers);
    at::set_num_threads(1);
    at::set_num_interop_threads(1);
    omp_set_num_threads(1);
    std::atomic<int> next_block{0};
    auto worker_task = [&]() {
        while (true) {
            int id = next_block.fetch_add(1, std::memory_order_relaxed);
            if (id >= total_blocks) break;
            int bz = id / (split * split);
            int by = (id / split) % split;
            int bx = id % split;
            Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
            sub_min = sub_min + min_pos_eigen;
            Eigen::Vector3f sub_max = sub_min + block_size;
            Eigen::Vector3f sub_range = sub_max - sub_min;
            float max_len = sub_range.maxCoeff();
            float uniform_step = max_len / (target_points - 1);
            int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
            int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
            int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
            Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
            Eigen::Vector3f step = sub_range.cwiseQuotient(now);
            torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
            const int K = 32;
            const double eps = 1e-6;
            std::vector<double> weights(K, 0.0);
            std::vector<int> idxs(K, 0);
            std::vector<double> dists(K, 0.0);
            //std::cout << "nx: " << nx << " ny: " << ny << " nz: " << nz << std::endl;
            for (int i = 0; i < nx; ++i) {
                for (int j = 0; j < ny; ++j) {
                    for (int k = 0; k < nz; ++k) {
                        Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
                        Eigen::VectorXd pos_vec(3);
                        pos_vec << pos[0], pos[1], pos[2];
                        tree.query(pos_vec, K, idxs, dists);
                        torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
                        if (K == 1) {
                            weighted_sum = torch::tensor(
                                    {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
                                    torch::kFloat32);
                        } else {
                            double dist_sum = 0.0;
                            for (int l = 0; l < K; ++l) {
                                double dist = dists[l];
                                weights[l] = 1.0 / (dist * dist + eps);
                                dist_sum += weights[l];
                            }
                            for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
                            for (int l = 0; l < K; ++l) {
                                weighted_sum +=
                                        weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
                                                                    velocities(idxs[l], 2)},
                                                                   torch::kFloat32);
                            }
                        }
                        grid_tensor[k][j][i] = weighted_sum;
                    }
                }
            }
            torch::Tensor arr = grid_tensor.clone();
            for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
            arr = sigmoid(arr);
            infer_slots.acquire();
            torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
            infer_slots.release();
            all_results_1[id] = std::move(pred_block_1);
            //std::cout << "end.  idx = " << id<< std::endl;
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(workers);
    for (int t = 0; t < workers; ++t) threads.emplace_back(worker_task);
    for (auto& t: threads) t.join();
    //std::cout << "[All worker threads finished!]" << std::endl;


    //const double eps = 1e-6;
    //int max_threads = (int) std::thread::hardware_concurrency();
    //int workers = std::min(total_blocks, max_threads);
    //static std::counting_semaphore<> infer_slots(workers);

    //int progress = 0;
    //std::mutex progress_mutex;

    //auto process_blocks_range = [&](int begin, int end) {
    //    static thread_local bool printed = false;
    //    for (int id = begin; id < end; ++id) {
    //        int bz = id / (split * split);
    //        int by = (id / split) % split;
    //        int bx = id % split;
    //        Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
    //        sub_min = sub_min + min_pos_eigen;
    //        Eigen::Vector3f sub_max = sub_min + block_size;
    //        Eigen::Vector3f sub_range = sub_max - sub_min;
    //        float max_len = sub_range.maxCoeff();
    //        float uniform_step = max_len / (target_points - 1);
    //        int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
    //        int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
    //        int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
    //        Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
    //        Eigen::Vector3f step = sub_range.cwiseQuotient(now);
    //        torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
    //        /*const int K = 16;*/
    //        const int K = 32;
    //        std::vector<double> weights(K, 0.0);
    //        std::vector<int> idxs(K, 0);
    //        std::vector<double> dists(K, 0.0);
    //        static std::atomic<int> test_counter{0};
    //        for (int i = 0; i < nx; ++i) {
    //            for (int j = 0; j < ny; ++j) {
    //                for (int k = 0; k < nz; ++k) {
    //                    Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);

    //                    Eigen::VectorXd pos_vec(3);
    //                    pos_vec << pos[0], pos[1], pos[2];
    //                    tree.query(pos_vec, K, idxs, dists);
    //                    torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
    //                    if (K == 1) {
    //                        weighted_sum = torch::tensor(
    //                                {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
    //                                torch::kFloat32);
    //                    } else {
    //                        double dist_sum = 0.0;
    //                        for (int l = 0; l < K; ++l) {
    //                            double dist = dists[l];
    //                            weights[l] = 1.0 / (dist * dist + eps);
    //                            dist_sum += weights[l];
    //                        }
    //                        for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
    //                        for (int l = 0; l < K; ++l) {
    //                            if (idxs[l] >= 0 && idxs[l] < velocities.rows()) {
    //                                weighted_sum +=
    //                                        weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
    //                                                                    velocities(idxs[l], 2)},
    //                                                                   torch::kFloat32);
    //                            }
    //                        }
    //                    }
    //                    grid_tensor[k][j][i] = weighted_sum;
    //                }
    //            }
    //        }
    //        torch::Tensor arr = grid_tensor.clone();
    //        for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
    //        arr = sigmoid(arr);
    //        infer_slots.acquire();
    //        torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
    //        infer_slots.release();
    //        all_results_1[id] = std::move(pred_block_1);
    //    }
    //};

    //ThreadPool::parallelFor(0, total_blocks, process_blocks_range, workers);
    //ThreadPool::parallelFor(0, total_blocks, process_blocks_range);

    /*int max_threads = (int) std::thread::hardware_concurrency();
    int workers = std::min(total_blocks, max_threads);
    static std::counting_semaphore<> infer_slots(workers);*/


    int nz_sub = all_results_1[0].size(0);
    int ny_sub = all_results_1[0].size(1);
    int nx_sub = all_results_1[0].size(2);

    torch::Tensor result_volume_1 = torch::zeros({split * nz_sub, split * ny_sub, split * nx_sub}, torch::kFloat32);

    int idx = 0;
    for (int bz = 0; bz < split; ++bz) {
        for (int by = 0; by < split; ++by) {
            for (int bx = 0; bx < split; ++bx) {

                torch::Tensor cur = all_results_1[idx];
                auto sz = cur.sizes();
                if (sz[0] != nz_sub || sz[1] != ny_sub || sz[2] != nx_sub) {
                    cur = torch::nn::functional::pad(cur, torch::nn::functional::PadFuncOptions({
                                                                  0, nx_sub - sz[2], // W
                                                                  0, ny_sub - sz[1], // H
                                                                  0, nz_sub - sz[0]  // D
                                                          }));
                }

                int z_start = bz * nz_sub;
                int y_start = by * ny_sub;
                int x_start = bx * nx_sub;
                result_volume_1.slice(0, z_start, z_start + nz_sub)
                        .slice(1, y_start, y_start + ny_sub)
                        .slice(2, x_start, x_start + nx_sub)
                        .copy_(all_results_1[idx]);
                ++idx;
            }
        }
    }

    // 计算全局步长
    auto sizes = result_volume_1.sizes();
    float depth = static_cast<float>(sizes[0]);
    float height = static_cast<float>(sizes[1]);
    float width = static_cast<float>(sizes[2]);

    Eigen::Vector3f volume_size(width, height, depth);
    Eigen::Vector3f global_step = range_vec.cwiseQuotient(volume_size);

    torch::Tensor result_volume_11 = gaussian_filter3d(result_volume_1, 3, -1);
    return std::make_tuple(result_volume_11, global_step);
}

torch::Tensor VortexDetection::gaussian_weights(const torch::Tensor& dists, float sigma) {
    return torch::exp(-0.5 * torch::pow(dists / sigma, 2));
}

torch::Tensor VortexDetection::knn_smooth_labels(const torch::Tensor& prob_vol_1, // [nz, ny, nx]
                                                 const Eigen::Vector3f& min_pos, const Eigen::Vector3f& global_step,
                                                 const std::vector<Eigen::Vector3f>& query_points, // 要查询的点
                                                 int k) {

    int nz = prob_vol_1.size(0);
    int ny = prob_vol_1.size(1);
    int nx = prob_vol_1.size(2);

    std::vector<Eigen::Vector3f> grid_coords;
    grid_coords.reserve(nx * ny * nz);

    for (int z = 0; z < nz; z++) {
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                Eigen::Vector3f pos = min_pos + Eigen::Vector3f(x, y, z).cwiseProduct(global_step);
                grid_coords.push_back(pos);
            }
        }
    }

    Eigen::MatrixXd coords_mat(grid_coords.size(), 3);
    for (int i = 0; i < grid_coords.size(); i++) { coords_mat.row(i) = grid_coords[i].cast<double>().transpose(); }

    KDTree tree(coords_mat);

    torch::Tensor flat_prob = prob_vol_1.flatten();

    int N = query_points.size();
    torch::Tensor smooth_1 = torch::zeros({N}, torch::kFloat32);

    ResetProgress();
    int progress = 0;
    int block = std::max(1, N / 100);

    for (int i = 0; i < N; i++) {
        std::vector<int> idxs;
        std::vector<double> dists;
        Eigen::VectorXd query = query_points[i].cast<double>();

        tree.query(query, k, idxs, dists);

        torch::Tensor dists_t =
                torch::from_blob(dists.data(), {(int) idxs.size()}, torch::kFloat64).to(torch::kFloat32).clone();

        torch::Tensor weights = torch::exp(-0.5 * torch::pow(dists_t / 1.5, 2));
        weights = weights / (torch::sum(weights) + 1e-8);

        //torch::Tensor weights = gaussian_weights(dists_t);

        torch::Tensor idxs_t =
                torch::from_blob(idxs.data(), {(int) idxs.size()}, torch::kInt32).to(torch::kLong).clone();
        torch::Tensor neighbors = flat_prob.index_select(0, idxs_t);

        //float val = torch::sum(weights * neighbors).item<float>() / torch::sum(weights).item<float>();
        float val = torch::sum(weights * neighbors).item<float>();
        smooth_1[i] = val;
        progress++;
        if (progress % block == 0 || i == N - 1) { UpdateProgress(progress / N); }
    }

    return smooth_1;
}

#endif
IGAME_NAMESPACE_END
