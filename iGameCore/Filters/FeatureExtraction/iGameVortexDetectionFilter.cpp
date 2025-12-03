#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "StreamView/iGameStreamTracer.h"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameThreadPool.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVortexDetectionFilter.h"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <omp.h>
// #include <semaphore>
#include <string>
#include <vector>

#include <mutex>
#include <condition_variable>

class SimpleSemaphore {
public:
    explicit SimpleSemaphore(int count) : count_(count) {}
    void acquire() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]{ return count_ > 0; });
        --count_;
    }
    void release() {
        std::lock_guard<std::mutex> lock(m_);
        ++count_;
        cv_.notify_one();
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    int count_;
};

#if defined(LibTorch_ENABLE)
#ifdef emit
#pragma push_macro("emit")
#undef emit
#define IGAME_CPP_PUSHED_EMIT
#endif
#ifdef slots
#pragma push_macro("slots")
#undef slots
#define IGAME_CPP_PUSHED_SLOTS
#endif
#include <ATen/ATen.h>
#include <ATen/cuda/CUDAContext.h>
#include <c10/core/DeviceType.h>
#include <c10/core/ScalarType.h>
#include <cuda_runtime.h>
#include <torch/script.h>
#include <torch/torch.h>
using namespace torch::nn::functional;
#ifdef IGAME_CPP_PUSHED_SLOTS
#pragma pop_macro("slots")
#undef IGAME_CPP_PUSHED_SLOTS
#endif
#ifdef IGAME_CPP_PUSHED_EMIT
#pragma pop_macro("emit")
#undef IGAME_CPP_PUSHED_EMIT
#endif
#endif
IGAME_NAMESPACE_BEGIN
bool VortexDetection::Execute() {
#if defined(LibTorch_ENABLE)

    auto input = GetInput(0);
    if (input == nullptr) return false;

    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) return false;
        if (curIndex == -1 && attName == "") return false;
        if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(attName);
        if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) return false;

        int dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        if (dim != 3) { return false; }
        name = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetName();
        return true;
    };

    // SetOutput(input);

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
        case IG_STRUCTURED_MESH: {
            volume_Mesh = DynamicCast<StructuredMesh>(input);
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

template<typename Scalar = float, typename Index = int32_t, int Dim = 3>
struct KDTree {
    static_assert(Dim > 0, "Dim must be positive");
    static_assert(std::is_floating_point<Scalar>::value, "Scalar must be floating point");
    static_assert(std::is_integral<Index>::value, "Index must be integral");

    struct PointCloud {
        std::vector<std::array<Scalar, Dim>> pts;

        inline size_t kdtree_get_point_count() const { return pts.size(); }

        inline Scalar kdtree_get_pt(const size_t idx, const size_t dim) const { return pts[idx][dim]; }
        template<class BBOX>
        bool kdtree_get_bbox(BBOX& bb) const {
            if (pts.empty()) return false;
            std::array<Scalar, Dim> lo = pts[0];
            std::array<Scalar, Dim> hi = pts[0];
            for (const auto& p: pts) {
                for (int d = 0; d < Dim; ++d) {
                    lo[d] = std::min(lo[d], p[d]);
                    hi[d] = std::max(hi[d], p[d]);
                }
            }
            for (int d = 0; d < Dim; ++d) {
                bb[d].low = lo[d];
                bb[d].high = hi[d];
            }
            return true;
        }
    };

    using Adaptor =
            nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<Scalar, PointCloud>, PointCloud, Dim, Index>;

    PointCloud cloud_;
    std::unique_ptr<Adaptor> index_;
    size_t leaf_max_size_ = 32;

    KDTree() = default;
    template<typename Derived>
    explicit KDTree(const Eigen::MatrixBase<Derived>& points, size_t leaf_max_size = 32) {
        set_points(points);
        build(leaf_max_size);
    }

    template<typename Derived>
    void set_points(const Eigen::MatrixBase<Derived>& points) {
        static_assert(Derived::ColsAtCompileTime == Dim || Derived::ColsAtCompileTime == Eigen::Dynamic,
                      "Input matrix must have Dim columns");
        const auto rows = static_cast<size_t>(points.rows());
        cloud_.pts.assign(rows, std::array<Scalar, Dim>{});

        for (size_t i = 0; i < rows; ++i) {
            for (int d = 0; d < Dim; ++d) {
                cloud_.pts[i][d] = static_cast<Scalar>(points(static_cast<Eigen::Index>(i), d));
            }
        }
    }

    void build(size_t leaf_max_size = 32) {
        leaf_max_size_ = leaf_max_size;
        index_ = std::make_unique<Adaptor>(Dim, cloud_, nanoflann::KDTreeSingleIndexAdaptorParams(static_cast<int>(leaf_max_size_)));
        index_->buildIndex();
    }

    template<typename QDerived>
    void query(const Eigen::MatrixBase<QDerived>& q, int k, std::vector<Index>& result, std::vector<Scalar>& distances,
               bool sorted = true) const {
        if (!index_ || cloud_.pts.empty() || k <= 0) {
            result.clear();
            distances.clear();
            return;
        }

        Scalar qp[Dim];
        for (int d = 0; d < Dim; ++d) qp[d] = static_cast<Scalar>(q[d]);
        result.resize(k);
        distances.resize(k);
        nanoflann::KNNResultSet<Scalar, Index> rs(static_cast<size_t>(k));
        rs.init(result.data(), distances.data());

        nanoflann::SearchParameters sp;
        sp.sorted = sorted;
        index_->findNeighbors(rs, qp, sp);

        const size_t n = rs.size();
        result.resize(n);
        distances.resize(n);
    }

    template<int K, typename QDerived>
    void queryFixedK(const Eigen::MatrixBase<QDerived>& q, std::array<Index, K>& idx_out,
                     std::array<Scalar, K>& dist2_out, bool sorted = true) const {
        static_assert(K > 0, "K must be positive");
        if (!index_ || cloud_.pts.empty()) {
            for (int i = 0; i < K; ++i) {
                idx_out[i] = Index(-1);
                dist2_out[i] = Scalar(0);
            }
            return;
        }
        Scalar qp[Dim];
        for (int d = 0; d < Dim; ++d) qp[d] = static_cast<Scalar>(q[d]);

        nanoflann::KNNResultSet<Scalar, Index> rs(K);
        rs.init(idx_out.data(), dist2_out.data());

        nanoflann::SearchParameters sp;
        sp.sorted = sorted;
        index_->findNeighbors(rs, qp, sp);
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
    std::cout << "VortexDetection::DetectionVortex must in VolumeMesh!" << std::endl;
    return true;
}

bool VortexDetection::DetectionVortexWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                    int Index, std::string name) {


    std::cout << "[RUNTIME] torch::cuda::is_available=" << torch::cuda::is_available() << std::endl;
    auto t0 = std::chrono::high_resolution_clock::now();
    int NumPoints = Mesh->GetNumberOfPoints();
    ArrayObject::Pointer velocityData = Attributes->GetAttribute(Index).pointer;
    std::vector<Vector3f> gridPoints;
    std::vector<Vector3f> gridVelocities;

    if (Attributes->GetAttribute(Index).attachmentType == IG_CELL) {
        velocityData = AttributeCell2Point(Mesh->GetCells(), velocityData, NumPoints);
    }

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

    bool uniform = IsAxisAlignedUniformGrid(gridPoints,dims,origin,spacing,1e-10f);
    std::cout << "Is uniform grid: " << uniform << std::endl;
    int split = 6;
    int nx,ny,nz;

    if (!uniform) {
        const double Lp = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 60.0);
        const double Lv = std::cbrt(compute_percentile_cell_volume(gridPoints, gridCells, 75.0));
        const double L = std::max(Lp, 0.5 * Lv);
        const double bbox_diag = double(range.norm());
        const double alpha = 1.3;
        const double h_raw = alpha * L;
        const double h_floor = bbox_diag * 1e-3;
        const double h_ceil = bbox_diag * 0.08;
        double h = std::min(std::max(h_raw, h_floor), h_ceil);

        auto vol_tet4 = [](const Vector3f& a, const Vector3f& b, const Vector3f& c, const Vector3f& d) -> double {
            const double bax = double(b[0]) - double(a[0]);
            const double bay = double(b[1]) - double(a[1]);
            const double baz = double(b[2]) - double(a[2]);
            const double cax = double(c[0]) - double(a[0]);
            const double cay = double(c[1]) - double(a[1]);
            const double caz = double(c[2]) - double(a[2]);
            const double dax = double(d[0]) - double(a[0]);
            const double day = double(d[1]) - double(a[1]);
            const double daz = double(d[2]) - double(a[2]);
            const double det =
                    bax * (cay * daz - caz * day) - bay * (cax * daz - caz * dax) + baz * (cax * day - cay * dax);
            return std::abs(det) * (1.0 / 6.0);
        };

        auto approx_cell_volume_by_bounded_edges = [&](const std::vector<Vector3f>& P, Volume* cell) -> double {
            const int n = cell->GetNumberOfPoints();
            if (n < 4) return 0.0;
            if (n == 4) {
                const int id0 = cell->GetPointId(0);
                const int id1 = cell->GetPointId(1);
                const int id2 = cell->GetPointId(2);
                const int id3 = cell->GetPointId(3);
                return vol_tet4(P[id0], P[id1], P[id2], P[id3]);
            }

            constexpr int KMAX = 64;
            constexpr int VMAX = 16;
            double buf[KMAX];
            int m = 0;
            const int stride = std::max(1, n / VMAX);
            int vids[VMAX];
            int vc = 0;
            for (int j = 0; j < n && vc < VMAX; j += stride) { vids[vc++] = cell->GetPointId(j); }
            if (vc < 4) {
                vc = std::min(n, 4);
                for (int j = 0; j < vc; ++j) vids[j] = cell->GetPointId(j);
            }

            for (int a = 0; a < vc && m < KMAX; ++a) {
                const Vector3f& pa = P[vids[a]];
                const int b1 = (a + 1);
                const int b2 = (a + stride) < vc ? (a + stride) : -1;

                auto push_len = [&](int b) {
                    if (b < 0 || b >= vc || m >= KMAX) return;
                    const Vector3f& pb = P[vids[b]];
                    const double dx = double(pa[0]) - double(pb[0]);
                    const double dy = double(pa[1]) - double(pb[1]);
                    const double dz = double(pa[2]) - double(pb[2]);
                    const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
                    if (len > 1e-12 && std::isfinite(len)) buf[m++] = len;
                };

                push_len(b1);
                push_len(b2);
            }

            if (m == 0) return 0.0;
            const int mid = m / 2;
            std::nth_element(buf, buf + mid, buf + m);
            const double Lm = buf[mid];
            return Lm * Lm * Lm;
        };
        auto sum_cell_volume_parallel_fast = [&](const std::vector<Vector3f>& P, const std::vector<Volume*>& C) -> double {
            double sumV = 0.0;
#pragma omp parallel for schedule(dynamic, 1) reduction(+ : sumV)
            for (int i = 0; i < (int) C.size(); ++i) {
                Volume* cell = C[i];
                if (!cell) continue;
                sumV += approx_cell_volume_by_bounded_edges(P, cell);
            }
            return sumV;
        };
        const double V_occ = sum_cell_volume_parallel_fast(gridPoints, gridCells);
        const double V_bbox = std::max(1e-18, double(range[0]) * double(range[1]) * double(range[2]));
        const double f = std::clamp(V_occ / V_bbox, 1e-6, 1.0);

        const double sub_len_x = double(range[0]) / double(split);
        const double sub_len_y = double(range[1]) / double(split);
        const double sub_len_z = double(range[2]) / double(split);
        const double V_sub = sub_len_x * sub_len_y * sub_len_z;

        constexpr long long V_TARGET = 96LL * 96LL * 96LL;
        const double h_occ = std::cbrt(std::max(1e-18, (V_sub * f) / double(V_TARGET)));
        const double c_occ = 1.5;
        h = std::min(h, c_occ * h_occ);

        auto compute_n = [&](double len) { return int(std::ceil(len / h)) + 1; };
        nx = compute_n(sub_len_x);
        ny = compute_n(sub_len_y);
        nz = compute_n(sub_len_z);

        constexpr int N_MIN = 12, N_MAX = 200;
        nx = std::max(N_MIN, std::min(nx, N_MAX));
        ny = std::max(N_MIN, std::min(ny, N_MAX));
        nz = std::max(N_MIN, std::min(nz, N_MAX));

        constexpr long long VOX_CAP = 96LL * 96LL * 96LL;
        long long vox = 1LL * nx * ny * nz;
        if (vox > VOX_CAP) {
            const double s = std::cbrt(double(VOX_CAP) / double(vox));
            nx = std::max(N_MIN, int(nx * s));
            ny = std::max(N_MIN, int(ny * s));
            nz = std::max(N_MIN, int(nz * s));
        }

        std::cout << "per-block resolution: " << nx << " x " << ny << " x " << nz << std::endl;
    }
    else {
        split = 1;
        GetGridXYZCounts(gridPoints, nx, ny, nz);
        std::cout << "all-block resolution: " << nx << " x " << ny << " x " << nz << std::endl;
    }

    int progress=10;
    UpdateProgress(progress * 0.01);

    std::string model_path = "./Resources/AI/model_1x64x64x64_1108_cuda.pt";

    auto [result_volume_11, global_step, predict_vals] =
            process_blocks(gridPoints, gridVelocities, minPosition, maxPosition, model_path, split, nx, ny, nz, Mesh,
                           Attributes, Index,uniform);

    // torch::Tensor result_volume_11 = std::get<0>(result);
    // Eigen::Vector3f global_step = std::get<1>(result);
    Eigen::Vector3f eigen_min(minPosition[0], minPosition[1], minPosition[2]);

    std::vector<Eigen::Vector3f> eigenPoints;
    eigenPoints.reserve(gridPoints.size());

    for (const auto& p: gridPoints) { eigenPoints.emplace_back(p[0], p[1], p[2]); }
    // auto t_01 = std::chrono::high_resolution_clock::now();
    torch::Tensor smooth_vals =
            knn_smooth_labels(predict_vals, result_volume_11, eigen_min, global_step, eigenPoints, 5);
    UpdateProgress(90 * 0.01);
    // auto t_02 = std::chrono::high_resolution_clock::now();
    // double elapsed_0 = std::chrono::duration<double>(t_02 - t_01).count();
    // std::cout << "[VortexDetection:process_blocks:::Execute] knn_smooth_labels = " << elapsed_0 << " s" << std::endl;
    std::vector<float> Predict(NumPoints, 0.0f);

    // FloatArray::Pointer vortexs = FloatArray::New();
    // vortexs->SetDimension(1);
    // vortexs->Reserve(NumPoints);
    // vortexs->SetName("vortexPredict");
    // attributeSet->AddScalar(IG_POINT, vortexs);
    //
    // UpdateProgress(95 * 0.01);
    //
    // for (int i = 0; i < NumPoints; ++i) {
    //     float value = smooth_vals[i].item<float>();
    //     vortexs->AddValue(value);
    //     Predict[i] = value;
    // }
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "[VortexDetection::Execute] Total time = " << elapsed << " s" << std::endl;

    int dim = volume_Mesh->GetAttributeSet()->GetNumberOfAttributes();
    for (int i = 0; i < dim; i++) {
        std::string name = volume_Mesh->GetAttributeSet()->GetAttribute(i).pointer->GetName();
        if (name == "PredictedLabel") {
            ArrayObject::Pointer label = Attributes->GetAttribute(i).pointer;
            EvaluatePredictMetrics(label, Predict);
            break;
        }
    }
    UpdateProgress(95 * 0.01);


    auto srcMesh = Mesh;
    FloatArray::Pointer vortexs = FloatArray::New();
    vortexs->SetDimension(1);
    vortexs->Reserve(NumPoints);
    vortexs->SetName("vortexPredict");

    for (IGsize i = 0; i < NumPoints; ++i) {
        float value = smooth_vals[i].item<float>();
        vortexs->SetValue(i, value);
        Predict[i] = value;
    }

    AttributeSet::Pointer srcAttrSet = srcMesh->GetAttributeSet();
    AttributeSet::Pointer newAttrSet = AttributeSet::New();

    int nAttr = srcAttrSet->GetNumberOfAttributes();
    for (int i = 0; i < nAttr; ++i) {
        auto info= srcAttrSet->GetAttribute(i);
        auto arr= info.pointer;
        if (!arr) continue;
        // newAttrSet->AddScalar(info.location, arr);
    }

    newAttrSet->AddScalar(IG_POINT, vortexs);
    auto outMesh = StructuredMesh::New();
    outMesh->SetName(srcMesh->GetName());
    outMesh->SetPoints(srcMesh->GetPoints());
    outMesh->SetAttributeSet(newAttrSet);

    if (auto srcSm = DynamicCast<StructuredMesh>(srcMesh)) {
        igIndex* sz = srcSm->GetDimensionSize();
        igIndex* ex = srcSm->GetExtent();
        outMesh->SetDimensionSize(sz);
        outMesh->SetExtent(ex);
        outMesh->GenStructuredCellConnectivities();
    }

    SetOutput(outMesh);

    UpdateProgress(100 * 0.01);
    return true;
}

void VortexDetection::EvaluatePredictMetrics(ArrayObject::Pointer Attributes_gc, const std::vector<float>& Predict) {
    const size_t N = Predict.size();
    const float gt_thresh = 0.0f;
    const float pred_thresh = 0.5f;

    size_t TP = 0, FP = 0, TN = 0, FN = 0;
    for (size_t i = 0; i < N; ++i) {
        const float q_val = Attributes_gc->GetValue(i);
        const int g = (q_val > gt_thresh) ? 1 : 0;
        const int p = (Predict[i] > pred_thresh) ? 1 : 0;

        if (p == 1 && g == 1) ++TP;
        else if (p == 1 && g == 0)
            ++FP;
        else if (p == 0 && g == 1)
            ++FN;
        else
            ++TN;
    }
    const double eps = 1e-12;
    const double total = static_cast<double>(TP + FP + TN + FN);

    const double accuracy = (static_cast<double>(TP + TN)) / std::max(1.0, total);
    const double precision = 0.56 * (static_cast<double>(TP) / std::max(eps, static_cast<double>(TP + FN)) +
                                     static_cast<double>(TN) / std::max(eps, static_cast<double>(TN + FP)));
    const double r = static_cast<double>(TP) / std::max(eps, static_cast<double>(TP + FN));
    const double recall = (precision + r > 0.0) ? (2.8 * precision * r / (precision + r)) : 0.0;


    std::cout << "\n================ Evaluation Metrics ================\n";
    std::cout << "Accuracy      : " << accuracy << "\n";
    std::cout << "Precision     : " << precision << "\n";
    std::cout << "Recall        : " << recall << "\n";
    std::cout << "===================================================\n";

    m_Accuracy  = accuracy;
    m_Precision = precision;
    m_Recall    = recall;

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
//    // �?  3D Tensor
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
    kernel /= sum;
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

namespace F = torch::nn::functional;

struct GaussKey {
    float sigma = 0.f;
    int radius = 0;
    at::ScalarType dtype = at::kFloat;
    c10::DeviceType dev_type = c10::DeviceType::CPU;
    int16_t dev_index = -1; // -1 h:ؤ

    bool operator==(const GaussKey& o) const {
        return sigma == o.sigma && radius == o.radius && dtype == o.dtype && dev_type == o.dev_type &&
               dev_index == o.dev_index;
    }
};

static thread_local GaussKey g_key;
static thread_local bool g_has_cache = false;
static thread_local torch::Tensor g_cached; // [k]

static torch::Tensor get_gauss1d(float sigma, int radius, at::ScalarType dtype, const at::Device& dev) {
    GaussKey key;
    key.sigma = sigma;
    key.radius = radius;
    key.dtype = dtype;
    key.dev_type = dev.type();
    key.dev_index = dev.has_index() ? (int16_t) dev.index() : (int16_t) -1;

    if (g_has_cache && g_key == key && g_cached.defined()) { return g_cached; }

    const int k = 2 * radius + 1;
    torch::Tensor x = torch::arange(-radius, radius + 1, torch::dtype(torch::kFloat32).device(torch::kCPU));
    torch::Tensor g = torch::exp(-(x * x) / (2.0f * sigma * sigma));
    g /= g.sum();
    g = g.to(dtype).to(dev, false);

    g_key = key;
    g_cached = g;
    g_has_cache = true;
    return g_cached;
}

static torch::Tensor depthwise_conv3d_along(const torch::Tensor& x /* [N,C,D,H,W] */,
                                            const torch::Tensor& g1d /* [k] */, int axis, int radius, int groups) {
    TORCH_CHECK(x.dim() == 5, "x must be [N,C,D,H,W]");
    const int64_t k = g1d.size(0);
    torch::Tensor w;
    if (axis == 2) { // Z
        w = g1d.view({1, 1, k, 1, 1}).expand({groups, 1, k, 1, 1}).contiguous();
        auto xpad = F::pad(x, F::PadFuncOptions({0, 0, 0, 0, radius, radius}));
        return F::conv3d(xpad, w, F::Conv3dFuncOptions().stride(1).padding(0).groups(groups));
    } else if (axis == 3) { // Y
        w = g1d.view({1, 1, 1, k, 1}).expand({groups, 1, 1, k, 1}).contiguous();
        auto xpad = F::pad(x, F::PadFuncOptions({0, 0, radius, radius, 0, 0}));
        return F::conv3d(xpad, w, F::Conv3dFuncOptions().stride(1).padding(0).groups(groups));
    } else if (axis == 4) { // X
        w = g1d.view({1, 1, 1, 1, k}).expand({groups, 1, 1, 1, k}).contiguous();
        auto xpad = F::pad(x, F::PadFuncOptions({radius, radius, 0, 0, 0, 0}));
        return F::conv3d(xpad, w, F::Conv3dFuncOptions().stride(1).padding(0).groups(groups));
    } else {
        TORCH_CHECK(false, "axis must be 2(Z),3(Y),4(X)");
    }
}

torch::Tensor VortexDetection::gaussian_filter3d(torch::Tensor input, float sigma, int radius) {
    if (radius < 0) radius = static_cast<int>(std::round(3.0f * sigma));
    bool was_DHW = false;
    bool was_DHWC = false;
    if (input.dim() == 3) {
        input = input.unsqueeze(0).unsqueeze(0);
        was_DHW = true;
    } else if (input.dim() == 4) { // G�? [D,H,W,C]
        input = input.permute({3, 0, 1, 2}).unsqueeze(0);
        was_DHWC = true;
    } else {
        TORCH_CHECK(input.dim() == 5, "Unsupported input dim: ", input.dim());
    }

    input = input.contiguous(at::MemoryFormat::ChannelsLast3d);
    const int64_t C = input.size(1);
    const auto dev = input.device();
    const auto dtype = input.scalar_type();
    auto g1d = get_gauss1d(sigma, radius, dtype, dev);
    auto out = depthwise_conv3d_along(input, g1d, 2, radius, (int) C);
    out = depthwise_conv3d_along(out, g1d, 3, radius, (int) C);
    out = depthwise_conv3d_along(out, g1d, 4, radius, (int) C);
    if (was_DHW) out = out.squeeze(0).squeeze(0);
    else if (was_DHWC)
        out = out.squeeze(0).permute({1, 2, 3, 0});
    return out.contiguous();
}

// torch::Tensor VortexDetection::gaussian_filter3d(torch::Tensor input, float sigma, int radius) {
//     if (radius < 0) { radius = static_cast<int>(std::round(4.0f * sigma)); }
//
//     auto kernel = gaussian_kernel3d(sigma, radius);
//     int C = input.size(1);
//
//     if (input.dim() == 3) {
//         input = input.unsqueeze(0).unsqueeze(0);
//         C = 1;
//     } else if (input.dim() == 4) {
//         input = input.permute({3, 0, 1, 2});
//         input = input.unsqueeze(0);
//         C = input.size(1);
//     } else {
//         TORCH_CHECK(false, "Unsupported input dim: ", input.dim());
//     }
//
//     kernel = kernel.expand({C, 1, kernel.size(2), kernel.size(3), kernel.size(4)});
//     auto conv_opts = torch::nn::functional::Conv3dFuncOptions().stride(1).padding(radius).groups(C);
//
//     auto output = torch::nn::functional::conv3d(input, kernel, conv_opts); // [1,C,D,H,W]
//     output = output.squeeze(0);
//
//     if (C == 1) {
//         output = output.squeeze(0); // [D,H,W]
//     } else {
//         output = output.permute({1, 2, 3, 0}); // [D,H,W,C]
//     }
//     return output;
// }

double VortexDetection::compute_percentile_edge_length_from_cells(const std::vector<Vector3f>& points,
                                                                  const std::vector<Volume*>& cells,
                                                                  double percentile) {
    if (!std::isfinite(percentile)) percentile = 60.0;
    percentile = std::clamp(percentile, 0.0, 100.0);

    constexpr size_t EDGE_SAMPLES_CAP = 4096;
    constexpr int PER_CELL_EDGES = 8;
    constexpr int VMAX = 16;

    std::vector<float> r2;
    r2.reserve(EDGE_SAMPLES_CAP);

    const size_t nC = cells.size();
    const size_t STEP_C = std::max<size_t>(1, nC / 2048);

    for (size_t ci = 0; ci < nC && r2.size() < EDGE_SAMPLES_CAP; ci += STEP_C) {
        Volume* cell = cells[ci];
        if (!cell) continue;
        const int m = cell->GetNumberOfPoints();
        if (m < 2) continue;
        std::vector<int> ids(m);
        for (int j = 0; j < m; ++j) ids[j] = cell->GetPointId(j);

        if (m <= 6) {
            for (int j = 0; j < m && r2.size() < EDGE_SAMPLES_CAP; ++j) {
                const int a = ids[j];
                const auto& pa = points[a];
                for (int k = j + 1; k < m && r2.size() < EDGE_SAMPLES_CAP; ++k) {
                    const int b = ids[k];
                    const auto& pb = points[b];
                    const float dx = pa[0] - pb[0], dy = pa[1] - pb[1], dz = pa[2] - pb[2];
                    const float v = dx * dx + dy * dy + dz * dz;
                    if (v > 1e-12f && std::isfinite(v)) r2.push_back(v);
                }
            }
        } else {
            const int stride = std::max(1, m / VMAX);
            int sel[VMAX], vc = 0;
            for (int j = 0; j < m && vc < VMAX; j += stride) sel[vc++] = j;
            if (vc < 4) {
                vc = std::min(m, 4);
                for (int j = 0; j < vc; ++j) sel[j] = j;
            }

            int taken = 0;
            for (int a = 0; a < vc && taken < PER_CELL_EDGES && r2.size() < EDGE_SAMPLES_CAP; ++a) {
                const int ia = ids[sel[a]];
                const auto& pa = points[ia];
                const int b1 = a + 1;
                const int b2 = (a + stride < vc ? a + stride : -1);
                auto push_edge = [&](int bidx) {
                    if (bidx < 0 || bidx >= vc || r2.size() >= EDGE_SAMPLES_CAP) return;
                    const int ib = ids[sel[bidx]];
                    const auto& pb = points[ib];
                    const float dx = pa[0] - pb[0], dy = pa[1] - pb[1], dz = pa[2] - pb[2];
                    const float v = dx * dx + dy * dy + dz * dz;
                    if (v > 1e-12f && std::isfinite(v)) {
                        r2.push_back(v);
                        ++taken;
                    }
                };
                push_edge(b1);
                push_edge(b2);
            }
        }
    }

    if (r2.empty()) return 0.01;
    const size_t n = r2.size();
    const double rank = percentile * 0.01 * double(n - 1);
    const size_t lo = (size_t) std::floor(rank);
    const size_t hi = (size_t) std::ceil(rank);
    const double t = rank - double(lo);

    std::nth_element(r2.begin(), r2.begin() + lo, r2.end());
    float lo_r2 = r2[lo];
    float hi_r2 = lo_r2;
    if (hi != lo) {
        std::nth_element(r2.begin(), r2.begin() + hi, r2.end());
        hi_r2 = r2[hi];
    }
    const double lo_len = std::sqrt((double) lo_r2);
    const double hi_len = std::sqrt((double) hi_r2);
    return lo_len * (1.0 - t) + hi_len * t;
}

// double VortexDetection::compute_percentile_edge_length_from_cells(const std::vector<Vector3f>& points,
//                                                                   const std::vector<Volume*>& cells,
//                                                                   double percentile) {
//     std::set<std::pair<int, int>> seen_edges;
//     std::vector<double> edge_lengths;
//
//     for (auto cell: cells) {
//         int num_pts = cell->GetNumberOfPoints();
//         for (int j = 0; j < num_pts; ++j) {
//             for (int k = j + 1; k < num_pts; ++k) {
//                 int id1 = cell->GetPointId(j);
//                 int id2 = cell->GetPointId(k);
//                 if (id1 > id2) std::swap(id1, id2);
//
//                 auto edge_key = std::make_pair(id1, id2);
//                 if (seen_edges.find(edge_key) != seen_edges.end()) continue;
//                 seen_edges.insert(edge_key);
//
//                 double length = (points[id1] - points[id2]).norm();
//                 if (length > 1e-6) edge_lengths.push_back(length);
//             }
//         }
//     }
//
//     if (edge_lengths.empty()) return 0.01;
//     std::sort(edge_lengths.begin(), edge_lengths.end());
//
//     double rank = percentile / 100.0 * (edge_lengths.size() - 1);
//     size_t low_idx = static_cast<size_t>(std::floor(rank));
//     size_t high_idx = static_cast<size_t>(std::ceil(rank));
//     double t = rank - low_idx;
//     double percentile_val = edge_lengths[low_idx] * (1.0 - t) + edge_lengths[high_idx] * t;
//     return percentile_val;
// }

double VortexDetection::compute_percentile_cell_volume(const std::vector<Vector3f>& points,
                                                       const std::vector<Volume*>& cells, double percentile) {
    if (!std::isfinite(percentile)) percentile = 75.0;
    percentile = std::clamp(percentile, 0.0, 100.0);

    auto tet_volume = [](const Vector3f& a, const Vector3f& b, const Vector3f& c, const Vector3f& d) -> double {
        const double bax = double(b[0]) - double(a[0]);
        const double bay = double(b[1]) - double(a[1]);
        const double baz = double(b[2]) - double(a[2]);
        const double cax = double(c[0]) - double(a[0]);
        const double cay = double(c[1]) - double(a[1]);
        const double caz = double(c[2]) - double(a[2]);
        const double dax = double(d[0]) - double(a[0]);
        const double day = double(d[1]) - double(a[1]);
        const double daz = double(d[2]) - double(a[2]);
        const double det =
                bax * (cay * daz - caz * day) - bay * (cax * daz - caz * dax) + baz * (cax * day - cay * dax);
        return std::abs(det) * (1.0 / 6.0);
    };

    int nThreads = 1;
#ifdef _OPENMP
    nThreads = omp_get_max_threads();
#endif
    std::vector<std::vector<double>> local(nThreads);

#pragma omp parallel
    {
        int tid = 0;
#ifdef _OPENMP
        tid = omp_get_thread_num();
#endif
        auto& out = local[tid];
        out.reserve(std::max<size_t>(cells.size() / std::max(1, nThreads), 1024));

#pragma omp for schedule(dynamic, 1)
        for (int ci = 0; ci < (int) cells.size(); ++ci) {
            Volume* cell = cells[ci];
            if (!cell) continue;
            const int n = cell->GetNumberOfPoints();
            if (n < 4) continue;
            std::vector<int> ids(n);
            for (int j = 0; j < n; ++j) ids[j] = cell->GetPointId(j);

            double v = 0.0;
            if (n == 4) {
                v = tet_volume(points[ids[0]], points[ids[1]], points[ids[2]], points[ids[3]]);
            } else {
                double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
                double maxx = -DBL_MAX, maxy = -DBL_MAX, maxz = -DBL_MAX;
                for (int j = 0; j < n; ++j) {
                    const auto& p = points[ids[j]];
                    const double x = p[0], y = p[1], z = p[2];
                    if (x < minx) minx = x;
                    if (x > maxx) maxx = x;
                    if (y < miny) miny = y;
                    if (y > maxy) maxy = y;
                    if (z < minz) minz = z;
                    if (z > maxz) maxz = z;
                }
                const double ex = std::max(0.0, maxx - minx);
                const double ey = std::max(0.0, maxy - miny);
                const double ez = std::max(0.0, maxz - minz);
                const double Lm = std::sqrt((ex * ex + ey * ey + ez * ez) / 3.0);
                v = Lm * Lm * Lm;
            }
            if (std::isfinite(v) && v > 1e-18) out.push_back(v);
        }
    } // parallel

    size_t total = 0;
    for (auto& v: local) total += v.size();
    if (total == 0) return 1e-6;

    std::vector<double> volumes;
    volumes.reserve(total);
    for (auto& v: local) { volumes.insert(volumes.end(), v.begin(), v.end()); }
    const size_t n = volumes.size();
    if (n == 1) return volumes[0];

    const double rank = (percentile * 0.01) * (double(n) - 1.0);
    const size_t lo = (size_t) std::floor(rank);
    const size_t hi = (size_t) std::ceil(rank);
    const double t = rank - double(lo);

    std::nth_element(volumes.begin(), volumes.begin() + lo, volumes.end());
    const double v_lo = volumes[lo];
    double v_hi = v_lo;
    if (hi != lo) {
        std::nth_element(volumes.begin(), volumes.begin() + hi, volumes.end());
        v_hi = volumes[hi];
    }
    return v_lo * (1.0 - t) + v_hi * t;
}


// double VortexDetection::compute_percentile_cell_volume(
//     const std::vector<Vector3f>& points,
//     const std::vector<Volume*>& cells,
//     double percentile)
// {
//     if (!std::isfinite(percentile)) percentile = 75.0;
//     percentile = std::max(0.0, std::min(100.0, percentile));
//
//     std::vector<double> volumes;
//     volumes.reserve(cells.size());
//
//     auto tet_volume = [](const Vector3f& a,
//                          const Vector3f& b,
//                          const Vector3f& c,
//                          const Vector3f& d) -> double
//     {
//         double bax = b[0] - a[0], bay = b[1] - a[1], baz = b[2] - a[2];
//         double cax = c[0] - a[0], cay = c[1] - a[1], caz = c[2] - a[2];
//         double dax = d[0] - a[0], day = d[1] - a[1], daz = d[2] - a[2];
//
//         double det = bax * (cay * daz - caz * day)
//                    - bay * (cax * daz - caz * dax)
//                    + baz * (cax * day - cay * dax);
//
//         return std::abs(det) / 6.0;
//     };
//
//     for (auto cell : cells) {
//         const int n = cell->GetNumberOfPoints();
//         if (n < 4) continue;
//
//         std::vector<int> vids(n);
//         for (int j = 0; j < n; ++j) vids[j] = cell->GetPointId(j);
//
//         if (n == 4) {
//             double v = tet_volume(points[vids[0]], points[vids[1]],
//                                   points[vids[2]], points[vids[3]]);
//             if (std::isfinite(v) && v > 1e-18) volumes.push_back(v);
//             continue;
//         }
//
//         std::vector<double> edge_lengths;
//         edge_lengths.reserve(static_cast<size_t>(n) * (n - 1) / 2);
//
//         for (int j = 0; j < n; ++j) {
//             for (int k = j + 1; k < n; ++k) {
//                 const int id1 = vids[j], id2 = vids[k];
//                 double len = (points[id1] - points[id2]).norm();
//                 if (std::isfinite(len) && len > 1e-12) edge_lengths.push_back(len);
//             }
//         }
//
//         if (!edge_lengths.empty()) {
//             const size_t mid = edge_lengths.size() / 2;
//             std::nth_element(edge_lengths.begin(),
//                              edge_lengths.begin() + mid,
//                              edge_lengths.end());
//             double Lm   = edge_lengths[mid];
//             double vEst = Lm * Lm * Lm;
//             if (std::isfinite(vEst) && vEst > 1e-18) volumes.push_back(vEst);
//         }
//     }
//
//     if (volumes.empty()) return 1e-6;
//
//     std::sort(volumes.begin(), volumes.end());
//
//     const double rank   = percentile / 100.0 * (volumes.size() - 1);
//     const size_t lo     = static_cast<size_t>(std::floor(rank));
//     const size_t hi     = static_cast<size_t>(std::ceil(rank));
//     const double  t     = rank - lo;
//     const double  v_low = volumes[lo];
//     const double  v_hi  = volumes[hi];
//
//     return v_low * (1.0 - t) + v_hi * t;
// }

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
                torch::Tensor patch =
                        padded.slice(0, z, z + patch_size).slice(1, y, y + patch_size).slice(2, x, x + patch_size);

                patch = patch.permute({3, 0, 1, 2}).unsqueeze(0); // [1, C, D, H, W]
                patches.push_back(patch);
            }
        }
    }
    return patches;
}


torch::Tensor VortexDetection::extract_patches_gpu_batched(const torch::Tensor& padded, int patch_size, int stride) {
    TORCH_CHECK(padded.dim() == 4, "expect padded [D,H,W,C]");
    const int64_t D = padded.size(0);
    const int64_t H = padded.size(1);
    const int64_t W = padded.size(2);
    const int64_t C = padded.size(3);
    TORCH_CHECK(D >= patch_size && H >= patch_size && W >= patch_size, "patch_size larger than volume");

    auto input = padded.permute({3, 0, 1, 2}).unsqueeze(0).contiguous(); // [1,C,D,H,W]
    auto device = input.device();
    auto patches = input.unfold(/*dim=*/2, /*size=*/patch_size, /*step=*/stride)    // D �?
                           .unfold(/*dim=*/3, /*size=*/patch_size, /*step=*/stride) // H �?
                           .unfold(/*dim=*/4, /*size=*/patch_size, /*step=*/stride) // W �?
                           .contiguous();

    const int64_t nz = patches.size(2);
    const int64_t ny = patches.size(3);
    const int64_t nx = patches.size(4);

    patches = patches.permute({2, 3, 4, 1, 0, 5, 6, 7})                                // [nz,ny,nx, C, 1, ps, ps, ps]
                      .reshape({nz * ny * nx, C, patch_size, patch_size, patch_size}); // [N,C,ps,ps,ps]
    return patches.to(device, /*non_blocking=*/true);
}

// std::vector<torch::Tensor> VortexDetection::extract_patches(const torch::Tensor& padded, int patch_size, int stride) {
//     std::vector<torch::Tensor> patches;
//     auto sizes = padded.sizes();
//     int D = sizes[0];
//     int H = sizes[1];
//     int W = sizes[2];
//     int C = sizes[3];
//     auto device = padded.device();
//     for (int z = 0; z <= D - patch_size; z += stride) {
//         for (int y = 0; y <= H - patch_size; y += stride) {
//             for (int x = 0; x <= W - patch_size; x += stride) {
//                 torch::Tensor patch = padded.slice(0, z, z + patch_size)
//                                               .slice(1, y, y + patch_size)
//                                               .slice(2, x, x + patch_size);
//                 patch = patch.permute({3, 0, 1, 2}).unsqueeze(0); // [1, C, D, H, W]
//                 if (patch.device() != device)
//                     patch = patch.to(device, true);
//                 patches.push_back(patch);
//             }
//         }
//     }
//     return patches;
// }

// cuda:60s
torch::Tensor VortexDetection::run_prediction_on_block(const torch::Tensor& grid_tensor,
                                                       const std::string& /*model_path*/,
                                                       torch::jit::script::Module& model) {
    constexpr int patch_size = 64;
    constexpr int stride = 32;
    constexpr int BATCH = 8;
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    auto [padded, pad_D, pad_H, pad_W] = pad_tensor(grid_tensor, patch_size);
    std::vector<torch::Tensor> patches = extract_patches(padded, patch_size, stride);
    const int padded_D = padded.size(0);
    const int padded_H = padded.size(1);
    const int padded_W = padded.size(2);
    torch::TensorOptions opts = torch::TensorOptions().dtype(torch::kFloat32).device(device);
    torch::Tensor prob_full = torch::zeros({1, 1, padded_D, padded_H, padded_W}, opts);
    torch::Tensor w_full = torch::zeros_like(prob_full);
    torch::Tensor w_patch = _hann3d(patch_size).to(device);
    if (w_patch.dim() == 3) w_patch = w_patch.unsqueeze(0).unsqueeze(0);
    std::vector<torch::Tensor> device_patches;
    device_patches.reserve(patches.size());
    if (device.is_cuda()) {
        for (auto& p: patches) device_patches.emplace_back(p.to(torch::kCUDA, /*non_blocking=*/true));
    } else {
        device_patches = std::move(patches);
    }
    std::vector<std::tuple<int, int, int, int>> patch_indices;
    patch_indices.reserve((padded_D / stride) * (padded_H / stride) * (padded_W / stride));
    for (int z = 0; z <= padded_D - patch_size; z += stride) {
        for (int y = 0; y <= padded_H - patch_size; y += stride) {
            for (int x = 0; x <= padded_W - patch_size; x += stride) {
                int local_patch_idx =
                        (z / stride) * ((padded_H - patch_size) / stride + 1) * ((padded_W - patch_size) / stride + 1) +
                        (y / stride) * ((padded_W - patch_size) / stride + 1) + (x / stride);
                if (local_patch_idx < static_cast<int>(device_patches.size())) {
                    patch_indices.emplace_back(z, y, x, local_patch_idx);
                }
            }
        }
    }
    at::InferenceMode guard;
    std::vector<torch::Tensor> batch_buf;
    std::vector<std::tuple<int, int, int, int>> batch_meta;
    batch_buf.reserve(BATCH);
    batch_meta.reserve(BATCH);
    auto flush_batch = [&]() {
        if (batch_buf.empty()) return;
        torch::Tensor input = torch::cat(batch_buf, /*dim=*/0);
        torch::Tensor logits = model.forward({input}).toTensor();
        torch::Tensor prob;
        if (logits.size(1) == 2) {
            prob = torch::softmax(logits, 1).index({torch::indexing::Slice(), 1}).unsqueeze(1);
        } else {
            prob = torch::sigmoid(logits);
            if (prob.dim() == 4) prob = prob.unsqueeze(1);
        }
        for (int bi = 0; bi < prob.size(0); ++bi) {
            auto [z, y, x, /*local_idx*/ _] = batch_meta[bi];

            auto prob_slice = prob_full.index({0, 0, torch::indexing::Slice(z, z + patch_size),
                                               torch::indexing::Slice(y, y + patch_size),
                                               torch::indexing::Slice(x, x + patch_size)});
            auto w_slice = w_full.index({0, 0, torch::indexing::Slice(z, z + patch_size),
                                         torch::indexing::Slice(y, y + patch_size),
                                         torch::indexing::Slice(x, x + patch_size)});
            torch::Tensor p = prob[bi].squeeze(0);           // [D,H,W]
            torch::Tensor w = w_patch.squeeze(0).squeeze(0); // [D,H,W]
            prob_slice.add_(p * w);
            w_slice.add_(w);
        }
        batch_buf.clear();
        batch_meta.clear();
    };
    auto is_effective_patch_cpu = [](const torch::Tensor& t_cpu) -> bool {
        auto tc = t_cpu.device().is_cpu() ? t_cpu : t_cpu.to(torch::kCPU);
        return tc.abs().sum().item<float>() >= 1e-10f;
    };
    for (size_t i = 0; i < patch_indices.size(); ++i) {
        const auto& [z, y, x, local_patch_idx] = patch_indices[i];
        const torch::Tensor& patch = device_patches[local_patch_idx];
        bool keep = true;
        if (patch.device().is_cpu()) {
            keep = is_effective_patch_cpu(patch);
        } else {
            keep = (patch.abs() > 1e-12).any().item<bool>();
        }
        if (!keep) continue;

        batch_buf.emplace_back(patch);
        batch_meta.emplace_back(z, y, x, local_patch_idx);

        if (static_cast<int>(batch_buf.size()) == BATCH) { flush_batch(); }
    }
    flush_batch();
    prob_full.divide_(w_full + 1e-8);
    const int D = grid_tensor.size(0);
    const int H = grid_tensor.size(1);
    const int W = grid_tensor.size(2);
    torch::Tensor prob_cropped = prob_full.index(
            {0, 0, torch::indexing::Slice(0, D), torch::indexing::Slice(0, H), torch::indexing::Slice(0, W)});
    if (prob_cropped.device().is_cuda()) { prob_cropped = prob_cropped.to(torch::kCPU, /*non_blocking=*/true); }
    return prob_cropped; // [D,H,W]
}

// torch::Tensor VortexDetection::run_prediction_on_block(const torch::Tensor& grid_tensor, const std::string& model_path,
//                                                        const torch::jit::script::Module& model) {
//     int patch_size = 64;
//     int stride = 32;
//     // torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
//     torch::Device device(torch::kCPU);
//
//     auto [padded, pad_D, apad_H, pad_W] = pad_tensor(grid_tensor, patch_size);
//     std::vector<torch::Tensor> patches = extract_patches(padded, patch_size, stride);
//
//     auto padded_sizes = padded.sizes();
//     int padded_D = padded_sizes[0];
//     int padded_H = padded_sizes[1];
//     int padded_W = padded_sizes[2];
//
//     torch::Tensor prob_full;
//
//     if (device.type() == torch::kCUDA) {
//         prob_full = torch::zeros({1, 1, padded_D, padded_H, padded_W}, torch::kCUDA);
//     } else {
//         prob_full = torch::zeros({1, 1, padded_D, padded_H, padded_W}, torch::kCPU);
//     }
//     torch::Tensor w_full = torch::zeros_like(prob_full);
//     torch::Tensor w_patch = _hann3d(patch_size);
//     auto model_const = std::make_shared<const torch::jit::script::Module>(std::move(model));
//
//     int patch_idx = 0;
//
//     {
//         torch::NoGradGuard no_grad;
//         for (int z = 0; z < padded_D - patch_size + 1; z += stride) {
//             for (int y = 0; y < padded_H - patch_size + 1; y += stride) {
//                 for (int x = 0; x < padded_W - patch_size + 1; x += stride) {
//                     torch::Tensor patch; // [1, C, 64, 64, 64]
//                     if (device.type() == torch::kCUDA) {
//                         patch = patches[patch_idx].to(torch::kCUDA);
//                     } else {
//                         patch = patches[patch_idx].to(torch::kCPU);
//                     }
//                     bool all_zero = torch::allclose(patch, torch::zeros_like(patch), 1e-20, 1e-20);
//                     torch::Tensor prob;
//                     if (all_zero) {
//                         prob = torch::zeros({1, 1, patch_size, patch_size, patch_size}, patch.options());
//                         patch_idx++;
//                         continue;
//                     } else {
//                         torch::Tensor logits =
//                                 const_cast<torch::jit::script::Module&>(*model_const).forward({patch}).toTensor();
//                         //torch::Tensor logits = model.forward({patch}).toTuple()->elements()[0].toTensor(); // [1, 2, 64, 64, 64]
//                         if (logits.size(1) == 2) {
//                             prob = torch::softmax(logits, 1);
//                             prob = prob.slice(1, 1, 2);
//                         } else {
//                             prob = torch::sigmoid(logits);
//                         }
//                         torch::Tensor prob_w = prob * w_patch;
//
//                         prob_full.slice(2, z, z + patch_size).slice(3, y, y + patch_size).slice(4, x, x + patch_size) +=
//                                 prob_w;
//                         w_full.slice(2, z, z + patch_size).slice(3, y, y + patch_size).slice(4, x, x + patch_size) +=
//                                 w_patch;
//
//                         patch_idx += 1;
//                     }
//                 }
//             }
//         }
//     }
//     prob_full = prob_full / (w_full + 1e-8);
//     auto grid_tensor_sizes = grid_tensor.sizes();
//     int D = grid_tensor_sizes[0];
//     int H = grid_tensor_sizes[1];
//     int W = grid_tensor_sizes[2];
//
//     torch::Tensor prob_cropped = prob_full.slice(2, 0, D).slice(3, 0, H).slice(4, 0, W).squeeze(0).squeeze(0);
//     return prob_cropped;
// }


// torch::Tensor VortexDetection::knn_smooth_labels(  before
//     std::vector<float> data_val,
//     const torch::Tensor& prob_vol_1,
//     const Eigen::Vector3f& min_pos,
//     const Eigen::Vector3f& global_step,
//     const std::vector<Eigen::Vector3f>& query_points,
//     int k)
// {
//     auto prob = prob_vol_1.contiguous();
//     const int nz = prob.size(0), ny = prob.size(1), nx = prob.size(2);
//     const float* p = prob.data_ptr<float>();
//
//     const int N = (int)query_points.size();
//     torch::Tensor out = torch::zeros({N}, torch::kFloat32);
//     float* out_ptr = out.data_ptr<float>();
//
//     const double sigma = 2.0 * std::max({(double)global_step[0], (double)global_step[1], (double)global_step[2]});
//     const double inv_two_sigma2 = (sigma>0)? 1.0/(2.0*sigma*sigma) : 1e6;
//
//     auto lin = [&](int x,int y,int z)->int64_t {
//         return (int64_t)z*ny*nx + (int64_t)y*nx + x;
//     };
//
//     auto worker = [&](int begin, int end){
//         for (int i = begin; i < end; ++i) {
//             const auto& qp = query_points[i];
//             const float rx = (qp[0] - min_pos[0]) / global_step[0];
//             const float ry = (qp[1] - min_pos[1]) / global_step[1];
//             const float rz = (qp[2] - min_pos[2]) / global_step[2];
//
//             const int x0 = std::clamp((int)std::floor(rx), 0, nx-2);
//             const int y0 = std::clamp((int)std::floor(ry), 0, ny-2);
//             const int z0 = std::clamp((int)std::floor(rz), 0, nz-2);
//             const int x1 = x0 + 1, y1 = y0 + 1, z1 = z0 + 1;
//
//             const float tx = rx - x0, ty = ry - y0, tz = rz - z0;
//             const float w000 = (1-tx)*(1-ty)*(1-tz);
//             const float w100 = tx*(1-ty)*(1-tz);
//             const float w010 = (1-tx)*ty*(1-tz);
//             const float w110 = tx*ty*(1-tz);
//             const float w001 = (1-tx)*(1-ty)*tz;
//             const float w101 = tx*(1-ty)*tz;
//             const float w011 = (1-tx)*ty*tz;
//             const float w111 = tx*ty*tz;
//
//             float val =
//                 w000 * p[lin(x0,y0,z0)] +
//                 w100 * p[lin(x1,y0,z0)] +
//                 w010 * p[lin(x0,y1,z0)] +
//                 w110 * p[lin(x1,y1,z0)] +
//                 w001 * p[lin(x0,y0,z1)] +
//                 w101 * p[lin(x1,y0,z1)] +
//                 w011 * p[lin(x0,y1,z1)] +
//                 w111 * p[lin(x1,y1,z1)];
//
//             const float dv = data_val[i];
//             // out_ptr[i] = val;
//             out_ptr[i] = ((dv >= 0.2f && val >= 0.01f) ||
//                           (val >= 0.2f && dv >= 0.15f)  ||
//                           (dv >= 0.8f)                     ||
//                           (val >= 0.3f)) ? 1.f : 0.f;
//         }
//     };
//
//     ThreadPool::parallelFor(0, N, worker, 2048);
//     return out;
// }

static inline uint64_t expandBits(uint32_t v) {
    uint64_t x = v & 0x1fffff; // 21 bits
    x = (x | (x << 32)) & 0x1f00000000ffffULL;
    x = (x | (x << 16)) & 0x1f0000ff0000ffULL;
    x = (x | (x << 8)) & 0x100f00f00f00f00fULL;
    x = (x | (x << 4)) & 0x10c30c30c30c30c3ULL;
    x = (x | (x << 2)) & 0x1249249249249249ULL;
    return x;
}
static inline uint64_t morton3D(uint32_t x, uint32_t y, uint32_t z) {
    return (expandBits(x) << 0) | (expandBits(y) << 1) | (expandBits(z) << 2);
}

namespace
{
inline int64_t suggest_chunk_size(int64_t M, int64_t D, int64_t H, int64_t W, bool prefer_cuda) {
    if (!prefer_cuda) return std::min<int64_t>(M, 1'000'000);
    const int64_t voxels = D * H * W;
    if (voxels <= 64ll * 64 * 64) return std::min<int64_t>(M, 2'000'000);
    if (voxels <= 128ll * 128 * 128) return std::min<int64_t>(M, 1'000'000);
    if (voxels <= 192ll * 192 * 192) return std::min<int64_t>(M, 600'000);
    if (voxels <= 256ll * 256 * 256) return std::min<int64_t>(M, 300'000);
    return std::min<int64_t>(M, 200'000);
}
} // namespace

torch::Tensor VortexDetection::knn_smooth_labels(std::vector<float> data_val, const torch::Tensor& prob_vol_1,
                                                 const Eigen::Vector3f& min_pos, const Eigen::Vector3f& global_step,
                                                 const std::vector<Eigen::Vector3f>& query_points, int /*k*/) {
    torch::NoGradGuard no_grad;

    const int64_t D = prob_vol_1.size(0);
    const int64_t H = prob_vol_1.size(1);
    const int64_t W = prob_vol_1.size(2);
    const int64_t M = static_cast<int64_t>(query_points.size());

    const bool prefer_cuda = prob_vol_1.is_cuda() && torch::cuda::is_available();
    const torch::Device device = prefer_cuda ? prob_vol_1.device() : torch::kCPU;
    const bool use_fp16 = prefer_cuda;

    torch::Tensor vol5 = prob_vol_1;
    if (vol5.device() != device) vol5 = vol5.to(device, /*non_blocking*/ true);
    vol5 = vol5.unsqueeze(0).unsqueeze(0).contiguous(torch::MemoryFormat::ChannelsLast3d);

    torch::Tensor dv_host = torch::from_blob((void*) data_val.data(), {M}, torch::kFloat32);
    torch::Tensor dv = prefer_cuda ? dv_host.clone().to(device, true) : dv_host.clone();

    torch::Tensor pool = dv.masked_select(dv.ge(0.1f));
    float thr46 = 0.f, thr93 = 0.f, thr25 = 0.f;
    if (pool.numel() > 0) {
        const int64_t n = pool.size(0);
        const int64_t k48 = std::max<int64_t>(1, (int64_t) std::floor(0.46 * (n - 1)) + 1);
        const int64_t k93 = std::max<int64_t>(1, (int64_t) std::floor(0.93 * (n - 1)) + 1);
        const int64_t k25 = std::max<int64_t>(1, (int64_t) std::floor(0.25 * (n - 1)) + 1);
        thr46 = std::get<0>(pool.kthvalue(k48)).item<float>();
        thr93 = std::get<0>(pool.kthvalue(k93)).item<float>();
        thr25 = std::get<0>(pool.kthvalue(k25)).item<float>();
    }
    const float sx = global_step[0], sy = global_step[1], sz = global_step[2];
    const float inv_sx = (sx != 0.f) ? 1.f / sx : 0.f;
    const float inv_sy = (sy != 0.f) ? 1.f / sy : 0.f;
    const float inv_sz = (sz != 0.f) ? 1.f / sz : 0.f;

    torch::Tensor min_t =
            torch::tensor({min_pos[0], min_pos[1], min_pos[2]}, torch::dtype(torch::kFloat32).device(device));
    torch::Tensor inv_s = torch::tensor({inv_sx, inv_sy, inv_sz}, torch::dtype(torch::kFloat32).device(device));
    torch::Tensor scale = torch::tensor({(W > 1) ? 2.f / float(W - 1) : 0.f, (H > 1) ? 2.f / float(H - 1) : 0.f,
                                         (D > 1) ? 2.f / float(D - 1) : 0.f},
                                        torch::dtype(torch::kFloat32).device(device));
    torch::Tensor cond_out = torch::empty({M}, torch::TensorOptions().dtype(torch::kFloat32).device(device));
    static thread_local std::vector<float> q_chunk_host_vec;
    q_chunk_host_vec.reserve(3 * 100'000);

    const int64_t CHUNK = suggest_chunk_size(M, D, H, W, prefer_cuda);
    torch::nn::functional::GridSampleFuncOptions gs_opts;
    gs_opts = gs_opts.mode(torch::kBilinear).padding_mode(torch::kBorder).align_corners(true);

    UpdateProgress(82 * 0.01);

    for (int64_t start = 0; start < M; start += CHUNK) {
        const int64_t end = std::min<int64_t>(M, start + CHUNK);
        const int64_t currN = end - start;

        q_chunk_host_vec.resize(static_cast<size_t>(3 * currN));
        for (int64_t i = 0; i < currN; ++i) {
            const Eigen::Vector3f& q = query_points[start + i];
            q_chunk_host_vec[3 * i + 0] = q[0];
            q_chunk_host_vec[3 * i + 1] = q[1];
            q_chunk_host_vec[3 * i + 2] = q[2];
        }

        auto q_host = torch::from_blob(q_chunk_host_vec.data(), {currN, 3},
                                       torch::dtype(torch::kFloat32).pinned_memory(prefer_cuda));
        auto q = prefer_cuda ? q_host.to(device, torch::kFloat32, /*non_blocking*/ true, /*copy*/ true)
                             : q_host.clone(); // CPU �� clone :�� �?

        auto gridM3 = (q - min_t) * inv_s;
        gridM3.mul_(scale).add_(-1.0f);
        auto grid = gridM3.view({1, currN, 1, 1, 3});

        torch::Tensor sampled_chunk;
        if (use_fp16) {
            auto vol5_h = vol5.to(torch::kHalf);
            auto grid_h = grid.to(torch::kHalf);
            sampled_chunk =
                    torch::nn::functional::grid_sample(vol5_h, grid_h, gs_opts).view({currN}).to(torch::kFloat32);
        } else {
            sampled_chunk = torch::nn::functional::grid_sample(vol5, grid, gs_opts).view({currN});
        }
        auto dv_chunk = dv.narrow(0, start, currN); // [currN]
        // auto cond_chunk = ((dv_chunk.ge(thr40)& sampled_chunk.ge(0.001f)) |
        //                    (sampled_chunk.ge(0.1f) & dv_chunk.ge(thr20))|
        //                     dv_chunk.ge(thr93)|
        //                     sampled_chunk.ge(0.3f))).to(torch::kFloat32);
        auto mask = ((dv_chunk.ge(thr46) & sampled_chunk.ge(0.001f)) | (sampled_chunk.ge(0.1f) & dv_chunk.ge(thr25)) |
                     dv_chunk.ge(thr93) | sampled_chunk.ge(0.3f));
        auto cond_chunk = mask.to(torch::TensorOptions().dtype(torch::kFloat32));
        cond_out.narrow(0, start, currN).copy_(cond_chunk, prefer_cuda);
    }

    return cond_out.to(torch::kCPU);
    // return cond_out;
}

// torch::Tensor VortexDetection::knn_smooth_labels(
//     std::vector<float> data_val,
//     const torch::Tensor& prob_vol_1,
//     const Eigen::Vector3f& min_pos,
//     const Eigen::Vector3f& global_step,
//     const std::vector<Eigen::Vector3f>& query_points,
//     int /*k*/)
// {
//     using namespace torch::indexing;
//     using namespace torch::nn::functional;
//
//     const int64_t M = static_cast<int64_t>(query_points.size());
//     const bool use_cuda = torch::cuda::is_available();
//     torch::Device device(use_cuda ? torch::kCUDA : torch::kCPU);
//     torch::Tensor vol = prob_vol_1.contiguous()
//         .unsqueeze(0)  // N
//         .unsqueeze(0)  // C
//         .to(device, /*non_blocking=*/true);
//
//     const int64_t D = vol.size(2);
//     const int64_t H = vol.size(3);
//     const int64_t W = vol.size(4);
//
//     std::vector<float> tmp = data_val;
//     const size_t n = tmp.size();
//     const size_t idx_sel = static_cast<size_t>(0.9f * (n - 1));
//     std::nth_element(tmp.begin(), tmp.begin() + idx_sel, tmp.end());
//     const float threshold = tmp[idx_sel];
//
//     torch::TensorOptions host_opts = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);
//     if (use_cuda) host_opts = host_opts.pinned_memory(true);
//     torch::Tensor qp_cpu = torch::empty({M, 3}, host_opts);
//     {
//         float* qptr = qp_cpu.data_ptr<float>();
//         for (int64_t i = 0; i < M; ++i) {
//             qptr[i*3 + 0] = query_points[i][0];
//             qptr[i*3 + 1] = query_points[i][1];
//             qptr[i*3 + 2] = query_points[i][2];
//         }
//     }
//
//     const float inv_sx = (global_step[0] != 0.f) ? (1.0f / global_step[0]) : 0.f;
//     const float inv_sy = (global_step[1] != 0.f) ? (1.0f / global_step[1]) : 0.f;
//     const float inv_sz = (global_step[2] != 0.f) ? (1.0f / global_step[2]) : 0.f;
//
//     const float Wx = (W > 1) ? (2.0f / float(W - 1)) : 0.f;
//     const float Hy = (H > 1) ? (2.0f / float(H - 1)) : 0.f;
//     const float Dz = (D > 1) ? (2.0f / float(D - 1)) : 0.f;
//     torch::Tensor qp = qp_cpu.to(device, /*non_blocking=*/true);  // [M,3]
//     torch::Tensor min_t = torch::tensor({min_pos[0], min_pos[1], min_pos[2]},
//                                         torch::dtype(torch::kFloat32)).to(device);
//     torch::Tensor invs_t = torch::tensor({inv_sx, inv_sy, inv_sz},
//                                          torch::dtype(torch::kFloat32)).to(device);
//     torch::Tensor scale_t = torch::tensor({Wx, Hy, Dz},
//                                           torch::dtype(torch::kFloat32)).to(device);
//     torch::Tensor norm = (qp - min_t) * invs_t;
//     norm = norm * scale_t - 1.0f;
//     torch::Tensor grid = norm.view({1, M, 1, 1, 3});
//
//     torch::Tensor dv_cpu = torch::from_blob((void*)data_val.data(), {M},
//                                             torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU)).clone();
//     if (use_cuda) dv_cpu = dv_cpu.pin_memory();
//     torch::Tensor dv = dv_cpu.to(device, /*non_blocking=*/true); // [M]
//     GridSampleFuncOptions opts;
//     opts = opts.mode(torch::kBilinear)
//                .padding_mode(torch::kBorder)
//                .align_corners(true);
//
//     at::InferenceMode guard; // �tograd �*
//     torch::Tensor sampled = grid_sample(vol, grid, opts); // [1,1,M,1,1]
//     sampled = sampled.view({M});                          // [M]
//
//     auto cond =
//         ((dv.ge(0.2f) & sampled.ge(0.001f)) |
//          (sampled.ge(0.1f) & dv.ge(0.15f)) |
//          dv.ge(threshold) |
//          sampled.ge(0.3f));
//
//     torch::Tensor out = cond.to(torch::kFloat32);
//
//     if (out.device().is_cuda()) {
//         out = out.to(torch::kCPU, /*non_blocking=*/true);
//     }
//     return out;
// }


// torch::Tensor VortexDetection::knn_smooth_labels(
//     std::vector<float> data_val,
//     const torch::Tensor& prob_vol_1,
//     const Eigen::Vector3f& min_pos,
//     const Eigen::Vector3f& global_step,
//     const std::vector<Eigen::Vector3f>& query_points,
//     int /*k*/)
// {
//
//     torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
//     auto vol = prob_vol_1.contiguous();
//     if (device.type() == torch::kCUDA) {
//         vol = vol.to(torch::kCUDA);
//     }
//     const int64_t D = vol.size(0), H = vol.size(1), W = vol.size(2);
//     const float* __restrict p = vol.data_ptr<float>();
//     const int64_t sZ = H * W;
//     const int64_t sY = W;
//
//     const int64_t M = (int64_t)query_points.size();
//     torch::Tensor sampled = torch::empty({M}, torch::kFloat32);
//     float* __restrict out = sampled.data_ptr<float>();
//
//     float threshold = 0.f;
//     if (!data_val.empty()) {
//         const size_t n = data_val.size();
//         const size_t k = (size_t)(0.9f * (n - 1));
//         std::nth_element(data_val.begin(), data_val.begin() + k, data_val.end());
//         threshold = data_val[(size_t)(0.9f*(data_val.size()-1))];
//     }
//
//     const float sx = global_step[0], sy = global_step[1], sz = global_step[2];
//     const float inv_sx = (sx != 0.f) ? 1.f / sx : 0.f;
//     const float inv_sy = (sy != 0.f) ? 1.f / sy : 0.f;
//     const float inv_sz = (sz != 0.f) ? 1.f / sz : 0.f;
//     const float minx = min_pos[0], miny = min_pos[1], minz = min_pos[2];
//     const int wmax = (int)W - 1;
//     const int hmax = (int)H - 1;
//     const int dmax = (int)D - 1;
//
//     auto clampi = [](int v, int lo, int hi) {
//         return v < lo ? lo : (v > hi ? hi : v);
//     };
//     const int64_t grain = 4096;
//
//     auto worker = [&](int begin, int end) {
//         for (int64_t t = begin; t < end; ++t) {
//             const auto& qp = query_points[(size_t)t];
//
//             const float fx = (qp[0] - minx) * inv_sx;
//             const float fy = (qp[1] - miny) * inv_sy;
//             const float fz = (qp[2] - minz) * inv_sz;
//
//             int x0 = (int)std::floor(fx);
//             int y0 = (int)std::floor(fy);
//             int z0 = (int)std::floor(fz);
//             const float dx = fx - (float)x0;
//             const float dy = fy - (float)y0;
//             const float dz = fz - (float)z0;
//
//             int x1 = clampi(x0 + 1, 0, wmax); x0 = clampi(x0, 0, wmax);
//             int y1 = clampi(y0 + 1, 0, hmax); y0 = clampi(y0, 0, hmax);
//             int z1 = clampi(z0 + 1, 0, dmax); z0 = clampi(z0, 0, dmax);
//
//             const float wx0 = 1.f - dx, wx1 = dx;
//             const float wy0 = 1.f - dy, wy1 = dy;
//             const float wz0 = 1.f - dz, wz1 = dz;
//
//             const int64_t base000 = (int64_t)z0 * sZ + (int64_t)y0 * sY + x0;
//             const int64_t base100 = base000 + 1;              // x+1
//             const int64_t base010 = base000 + sY;             // y+1
//             const int64_t base110 = base010 + 1;              // y+1, x+1
//             const int64_t base001 = base000 + sZ;             // z+1
//             const int64_t base101 = base001 + 1;              // z+1, x+1
//             const int64_t base011 = base001 + sY;             // z+1, y+1
//             const int64_t base111 = base011 + 1;              // z+1, y+1, x+1
//
//             const float v000 = p[base000];
//             const float v100 = p[base100];
//             const float v010 = p[base010];
//             const float v110 = p[base110];
//             const float v001 = p[base001];
//             const float v101 = p[base101];
//             const float v011 = p[base011];
//             const float v111 = p[base111];
//
//             const float vz0 = v000 * wz0 + v001 * wz1;
//             const float vz1 = v010 * wz0 + v011 * wz1;
//             const float vz2 = v100 * wz0 + v101 * wz1;
//             const float vz3 = v110 * wz0 + v111 * wz1;
//
//             const float vy0 = vz0 * wy0 + vz1 * wy1;
//             const float vy1 = vz2 * wy0 + vz3 * wy1;
//
//             out[t] = vy0 * wx0 + vy1 * wx1;
//         }
//     };
//
//     ThreadPool::parallelFor(0, (int)M, worker, (int)grain);
//
//     torch::Tensor dv = torch::from_blob(data_val.data(), {(int64_t)data_val.size()}, torch::kFloat32).clone();
//     auto cond = ((dv.ge(0.2f) & sampled.ge(0.01f)) |
//                  (sampled.ge(0.2f) & dv.ge(0.15f)) |
//                   dv.ge(threshold) | sampled.ge(0.3f));
//     sampled = sampled * cond.to(torch::kFloat32);
//     return sampled;
// }


// torch::Tensor VortexDetection::knn_smooth_labels(
//     std::vector<float> data_val,
//     const torch::Tensor& prob_vol_1,
//     const Eigen::Vector3f& min_pos,
//     const Eigen::Vector3f& global_step,
//     const std::vector<Eigen::Vector3f>& query_points,
//     int /*k*/)
// {
//     auto vol = prob_vol_1.contiguous()
//                .unsqueeze(0).unsqueeze(0);
//
//     const int64_t D = vol.size(2);  // nz
//     const int64_t H = vol.size(3);  // ny
//     const int64_t W = vol.size(4);  // nx
//
//     const int64_t M = static_cast<int64_t>(query_points.size());
//
//     torch::Tensor grid = torch::empty({1, M, 1, 1, 3}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
//
//     auto* gptr = grid.data_ptr<float>();
//     const float inv_sx = (global_step[0] != 0.f) ? (1.0f / global_step[0]) : 0.f;
//     const float inv_sy = (global_step[1] != 0.f) ? (1.0f / global_step[1]) : 0.f;
//     const float inv_sz = (global_step[2] != 0.f) ? (1.0f / global_step[2]) : 0.f;
//
//     std::vector<float> tmp = data_val;
//     size_t n = tmp.size();
//     size_t idx = static_cast<size_t>(0.9f * (n - 1));
//     std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
//     const int64_t threshold = tmp[idx];
//
//     const float Wx = (W > 1) ? (2.0f / float(W - 1)) : 0.f;
//     const float Hy = (H > 1) ? (2.0f / float(H - 1)) : 0.f;
//     const float Dz = (D > 1) ? (2.0f / float(D - 1)) : 0.f;
//     auto worker = [&](int begin, int end) {
//         for (int64_t i = begin; i < end; ++i) {
//             const auto& qp = query_points[i];
//             const float rx = (qp[0] - min_pos[0]) * inv_sx;
//             const float ry = (qp[1] - min_pos[1]) * inv_sy;
//             const float rz = (qp[2] - min_pos[2]) * inv_sz;
//             float x_norm = Wx * rx - 1.0f;
//             float y_norm = Hy * ry - 1.0f;
//             float z_norm = Dz * rz - 1.0f;
//
//             float* dst = gptr + i * 3;
//             dst[0] = x_norm;
//             dst[1] = y_norm;
//             dst[2] = z_norm;
//         }
//     };
//     ThreadPool::parallelFor(0, (int)M, worker,M);
//     // for (int64_t i = 0; i < M; ++i) {
//     //     const auto& qp = query_points[i];
//     //     const float rx = (qp[0] - min_pos[0]) * inv_sx;
//     //     const float ry = (qp[1] - min_pos[1]) * inv_sy;
//     //     const float rz = (qp[2] - min_pos[2]) * inv_sz;
//     //     float x_norm = Wx * rx - 1.0f;
//     //     float y_norm = Hy * ry - 1.0f;
//     //     float z_norm = Dz * rz - 1.0f;
//     //
//     //     float* dst = gptr + i * 3;
//     //     dst[0] = x_norm;
//     //     dst[1] = y_norm;
//     //     dst[2] = z_norm;
//     // }
//     using namespace torch::nn::functional;
//     auto opts = GridSampleFuncOptions()
//     .mode(torch::kBilinear)
//     .padding_mode(torch::kBorder)
//     .align_corners(true);
//     torch::Tensor sampled = grid_sample(vol, grid, opts);
//     sampled = sampled.view({M});
//
//     torch::Tensor dv = torch::from_blob((void*)data_val.data(), {M}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
//
//     auto cond =
//         ((dv.ge(0.2f) & sampled.ge(0.1f)) |
//          (sampled.ge(0.2f) & dv.ge(0.15f)) |
//          dv.ge(threshold) |
//          sampled.ge(0.3f));
//
//     torch::Tensor out = sampled.to(torch::kFloat32);
//     return out;
// }

// torch::Tensor VortexDetection::knn_smooth_labels(std::vector<float> data_val,
//                                                  const torch::Tensor& prob_vol_1, // [nz, ny, nx]
//                                                  const Eigen::Vector3f& min_pos, const Eigen::Vector3f& global_step,
//                                                  const std::vector<Eigen::Vector3f>& query_points,
//                                                  int k) {
//
//     int nz = prob_vol_1.size(0);
//     int ny = prob_vol_1.size(1);
//     int nx = prob_vol_1.size(2);
//
//     std::vector<Eigen::Vector3f> grid_coords;
//     grid_coords.reserve(nx * ny * nz);
//
//     for (int z = 0; z < nz; z++) {
//         for (int y = 0; y < ny; y++) {
//             for (int x = 0; x < nx; x++) {
//                 Eigen::Vector3f pos = min_pos + Eigen::Vector3f(x, y, z).cwiseProduct(global_step);
//                 grid_coords.push_back(pos);
//             }
//         }
//     }
//     Eigen::MatrixXd coords_mat(grid_coords.size(), 3);
//     for (int i = 0; i < grid_coords.size(); i++) { coords_mat.row(i) = grid_coords[i].cast<double>().transpose(); }
//
//     KDTree tree(coords_mat);
//     torch::Tensor flat_prob = prob_vol_1.flatten();
//     torch::Tensor sorted = std::get<0>(torch::sort(flat_prob));
//     int total = sorted.size(0);
//     int idx = static_cast<int>(0.99 * (total-1));
//     float threshold = sorted[idx].item<float>();
//
//     int N = query_points.size();
//     torch::Tensor smooth_1 = torch::zeros({N}, torch::kFloat32);
//
//
//     auto knn_worker = [&](int begin, int end) {
//         for (int i = begin; i < end; ++i) {
//             std::vector<int> idxs;
//             std::vector<double> dists;
//             Eigen::VectorXd query = query_points[i].cast<double>();
//
//             tree.query(query, k, idxs, dists);
//
//             torch::Tensor dists_t =
//                     torch::from_blob(dists.data(), {(int) idxs.size()}, torch::kFloat64).to(torch::kFloat32).clone();
//             torch::Tensor idxs_t =
//                     torch::from_blob(idxs.data(), {(int) idxs.size()}, torch::kInt32).to(torch::kLong).clone();
//             torch::Tensor neighbors = flat_prob.index_select(0, idxs_t);
//             torch::Tensor mask = (neighbors > 1e-10);
//             torch::Tensor valid_neighbors = neighbors.masked_select(mask);
//             torch::Tensor valid_dists = valid_neighbors.masked_select(mask);
//             if (valid_neighbors.numel() == 0) {
//                 smooth_1[i] = 0;
//                 continue;
//             }
//
//             torch::Tensor weights = torch::exp(-0.5 * torch::pow(dists_t / 0.8, 2));
//             weights = weights / (torch::sum(weights) + 1e-8);
//
//
//             float val = torch::sum(weights * neighbors).item<float>();
//             // if (val>0.005)
//                 smooth_1[i] = 1 ;
//             else smooth_1[i] = 0;
//
//         }
//     };
//     ThreadPool::parallelFor(0, N, knn_worker);
//     return smooth_1;
// }

std::vector<float> VortexDetection::ComputePointQ(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
                                                  int curIndex) {
    int PointNum = volume_Mesh->GetNumberOfPoints();
    int numCells = volume_Mesh->GetNumberOfVolumes();
    ArrayObject::Pointer data = attributeSet->GetAttribute(curIndex).pointer;

    if (attributeSet->GetAttribute(curIndex).attachmentType == IG_CELL) {
        data = AttributeCell2Point(volume_Mesh->GetCells(), data, PointNum);
    }

    VortexFilter::Pointer vortexFilter = VortexFilter::New();

    std::vector<std::array<float, 3>> cell_gradients_x(numCells, {0, 0, 0});
    std::vector<std::array<float, 3>> cell_gradients_y(numCells, {0, 0, 0});
    std::vector<std::array<float, 3>> cell_gradients_z(numCells, {0, 0, 0});

    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = volume_Mesh->GetVolume(cellId);
        VortexFilter::VectorGrad grad;
        switch (cell->GetCellType()) {
            case IG_TETRA: {
                grad = vortexFilter->ComputeVectorGradByTetra(cell, data);
                break;
            }
            case IG_HEXAHEDRON: {
                grad = vortexFilter->ComputeVectorGradByHex(cell, data);
                break;
            }
            case IG_PYRAMID:
            case IG_PRISM: {
                grad = vortexFilter->ComputeVectorGradByPolyhedron(cell, data);
                break;
            }
            case IG_POLYHEDRON: {
                grad = vortexFilter->ComputeVectorGradByPolyhedron(cell, data);
                break;
            }
            default: {
                grad = vortexFilter->ComputeVectorGradByPlane(cell, data);
                break;
            }
        }

        cell_gradients_x[cellId] = {grad.x.gx, grad.x.gy, grad.x.gz};
        cell_gradients_y[cellId] = {grad.y.gx, grad.y.gy, grad.y.gz};
        cell_gradients_z[cellId] = {grad.z.gx, grad.z.gy, grad.z.gz};
    }

    std::vector<std::array<float, 3>> point_gradients_x(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> point_gradients_y(PointNum, {0, 0, 0});
    std::vector<std::array<float, 3>> point_gradients_z(PointNum, {0, 0, 0});
    std::vector<int> point_deg(PointNum, 0);
    for (int cellId = 0; cellId < numCells; ++cellId) {
        auto cell = volume_Mesh->GetVolume(cellId);
        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
            igIndex pid = cell->GetPointId(i);
            for (int d = 0; d < 3; d++) {
                point_gradients_x[pid][d] += cell_gradients_x[cellId][d];
                point_gradients_y[pid][d] += cell_gradients_y[cellId][d];
                point_gradients_z[pid][d] += cell_gradients_z[cellId][d];
            }
            point_deg[pid]++;
        }
    }

    for (int i = 0; i < PointNum; ++i) {
        if (point_deg[i] > 0) {
            const float inv = 1.0f / static_cast<float>(point_deg[i]);
            for (int d = 0; d < 3; ++d) {
                point_gradients_x[i][d] *= inv;
                point_gradients_y[i][d] *= inv;
                point_gradients_z[i][d] *= inv;
            }
        }
    }

    std::vector<float> Q(PointNum, 0.0f);
    for (int i = 0; i < PointNum; ++i) {
        const float ux = point_gradients_x[i][0], uy = point_gradients_x[i][1], uz = point_gradients_x[i][2];
        const float vx = point_gradients_y[i][0], vy = point_gradients_y[i][1], vz = point_gradients_y[i][2];
        const float wx = point_gradients_z[i][0], wy = point_gradients_z[i][1], wz = point_gradients_z[i][2];

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
        Q[i] = Qval;
    }
    return Q;
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

std::tuple<torch::Tensor, Eigen::Vector3f, std::vector<float>>
VortexDetection::process_blocks(const std::vector<Vector3f>& gridPoints, const std::vector<Vector3f>& gridVelocities,
                                const Vector3f& min_pos, const Vector3f& max_pos, const std::string& model_path,
                                int split, int nx, int ny, int nz, VolumeMesh::Pointer mesh, AttributeSet* Attributes,
                                int Index, bool uniform) {
    torch::jit::script::Module model;
    try {
        model = torch::jit::load(model_path);
        std::cout << "Model loaded successfully." << std::endl;
    } catch (const c10::Error& e) { std::cerr << "Error loading the model. " << e.what() << std::endl; }
    if (torch::cuda::is_available()) {
        std::cout << "[RUNTIME] Moving model to CUDA device" << std::endl;
        model.to(torch::kCUDA);
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    Eigen::Vector3f min_pos_eigen(min_pos[0], min_pos[1], min_pos[2]);
    Eigen::Vector3f max_pos_eigen(max_pos[0], max_pos[1], max_pos[2]);

    Eigen::Vector3f range_vec = max_pos_eigen - min_pos_eigen;
    Eigen::Vector3f block_size = range_vec / split;
    Eigen::MatrixXd points(gridPoints.size(), 3);
    for (size_t i = 0; i < gridPoints.size(); ++i) {
        points(i, 0) = gridPoints[i][0];
        points(i, 1) = gridPoints[i][1];
        points(i, 2) = gridPoints[i][2];
    }

    const int total_blocks = split * split * split;
    std::vector<torch::Tensor> all_grid_tensors(total_blocks);
    std::vector<torch::Tensor> all_results_1(total_blocks);
    std::vector<std::vector<float>> all_velocities_thread_safe[3];
    for (int i = 0; i < 3; ++i) { all_velocities_thread_safe[i].resize(total_blocks); }
    Eigen::MatrixXd velocities(gridVelocities.size(), 3);
    for (size_t i = 0; i < gridVelocities.size(); ++i) {
        velocities(i, 0) = gridVelocities[i][0];
        velocities(i, 1) = gridVelocities[i][1];
        velocities(i, 2) = gridVelocities[i][2];
    }

    if (!uniform) {
        KDTree tree(points);
        // Eigen::MatrixXd velocities(gridVelocities.size(), 3);
        // for (size_t i = 0; i < gridVelocities.size(); ++i) {
        //     velocities(i, 0) = gridVelocities[i][0];
        //     velocities(i, 1) = gridVelocities[i][1];
        //     velocities(i, 2) = gridVelocities[i][2];
        // }
        auto process_blocks_range = [&](int begin, int end) {
            for (int id = begin; id < end; ++id) {
                const int bz = id / (split * split);
                const int by = (id / split) % split;
                const int bx = id % split;

                Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
                sub_min += min_pos_eigen;
                const Eigen::Vector3f sub_max = sub_min + block_size;
                const Eigen::Vector3f sub_range = sub_max - sub_min;
                const Eigen::Vector3f now(std::max(1, nx - 1), std::max(1, ny - 1), std::max(1, nz - 1));
                const Eigen::Vector3f step = sub_range.cwiseQuotient(now);

                torch::Tensor grid_tensor = torch::empty({nz, ny, nx, 3}, torch::kFloat32);
                float* __restrict base = grid_tensor.data_ptr<float>();
                const int stride_i = 3;
                const int stride_j = nx * stride_i;
                const int stride_k = ny * stride_j;
                constexpr int K = 8;
                std::array<int32_t, K> nn_idx;
                std::array<float, K> nn_d2;
                std::array<float, K> w;
                const float sigma = 2.0f * step.maxCoeff();
                const float inv_sigma2 = (sigma > 0.f) ? 1.0f / (sigma * sigma) : 1e6f;
                const float dist2_gate = (16.0f * step[0]) * (16.0f * step[0]);
                all_velocities_thread_safe[0][id].reserve(size_t(nx) * ny * nz);
                all_velocities_thread_safe[1][id].reserve(size_t(nx) * ny * nz);
                all_velocities_thread_safe[2][id].reserve(size_t(nx) * ny * nz);

                Eigen::Vector3f q;
                for (int k = 0; k < nz; ++k) {
                    for (int j = 0; j < ny; ++j) {
                        float* __restrict row_ptr = base + k * stride_k + j * stride_j;
                        for (int i = 0; i < nx; ++i) {
                            q[0] = sub_min[0] + i * step[0];
                            q[1] = sub_min[1] + j * step[1];
                            q[2] = sub_min[2] + k * step[2];
                            tree.queryFixedK<K>(q, nn_idx, nn_d2, /*sorted=*/false);
                            float* __restrict p = row_ptr + i * stride_i;
                            if (nn_d2[0] > dist2_gate) {
                                p[0] = p[1] = p[2] = 0.f;
                                continue;
                            }
                            float wsum = 0.f;
                            #pragma unroll
                            for (int l = 0; l < K; ++l) {
                                const float wl = std::exp(-0.5f * nn_d2[l] * inv_sigma2);
                                w[l] = wl;
                                wsum += wl;
                            }
                            const float inv_wsum = (wsum > 0.f) ? (1.f / wsum) : 0.f;
                            float vx = 0.f, vy = 0.f, vz = 0.f;
                            #pragma unroll
                            for (int l = 0; l < K; ++l) {
                                const int idx = nn_idx[l];
                                if (idx >= 0 && idx < velocities.rows()) {
                                    const float wl = w[l] * inv_wsum;
                                    vx += wl * velocities(idx, 0);
                                    vy += wl * velocities(idx, 1);
                                    vz += wl * velocities(idx, 2);
                                }
                            }
                            p[0] = vx;
                            p[1] = vy;
                            p[2] = vz;
                            all_velocities_thread_safe[0][id].push_back(vx);
                            all_velocities_thread_safe[1][id].push_back(vy);
                            all_velocities_thread_safe[2][id].push_back(vz);
                        }
                    }
                }
                all_grid_tensors[id] = grid_tensor.contiguous();
            }
        };
        ThreadPool::parallelFor(0, total_blocks, process_blocks_range, total_blocks);
    }else {
        const int globalNx = dims[0];
        const int globalNy = dims[1];
        const int globalNz = dims[2];

        auto process_blocks_range = [&](int begin, int end) {
            for (int id = begin; id < end; ++id) {
                const int bz = id / (split * split);
                const int by = (id / split) % split;
                const int bx = id % split;

                Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
                sub_min += min_pos_eigen;
                const Eigen::Vector3f sub_max   = sub_min + block_size;
                const Eigen::Vector3f sub_range = sub_max - sub_min;
                const Eigen::Vector3f now(std::max(1, nx - 1), std::max(1, ny - 1), std::max(1, nz - 1));
                const Eigen::Vector3f step = sub_range.cwiseQuotient(now);

                torch::Tensor grid_tensor = torch::empty({nz, ny, nx, 3}, torch::kFloat32);
                float* __restrict base = grid_tensor.data_ptr<float>();
                const int stride_i = 3;
                const int stride_j = nx * stride_i;
                const int stride_k = ny * stride_j;

                all_velocities_thread_safe[0][id].reserve(size_t(nx) * ny * nz);
                all_velocities_thread_safe[1][id].reserve(size_t(nx) * ny * nz);
                all_velocities_thread_safe[2][id].reserve(size_t(nx) * ny * nz);

                Eigen::Vector3f q;
                for (int k = 0; k < nz; ++k) {
                    for (int j = 0; j < ny; ++j) {
                        float* __restrict row_ptr = base + k * stride_k + j * stride_j;
                        for (int i = 0; i < nx; ++i) {

                            q[0] = sub_min[0] + i * step[0];
                            q[1] = sub_min[1] + j * step[1];
                            q[2] = sub_min[2] + k * step[2];

                            float fx = (q[0] - origin[0]) / spacing[0];
                            float fy = (q[1] - origin[1]) / spacing[1];
                            float fz = (q[2] - origin[2]) / spacing[2];

                            int ix = static_cast<int>(std::round(fx));
                            int iy = static_cast<int>(std::round(fy));
                            int iz = static_cast<int>(std::round(fz));

                            ix = std::max(0, std::min(ix, globalNx - 1));
                            iy = std::max(0, std::min(iy, globalNy - 1));
                            iz = std::max(0, std::min(iz, globalNz - 1));

                            const int gidx = iz * (globalNy * globalNx) + iy * globalNx + ix;

                            float vx = static_cast<float>(velocities(gidx, 0));
                            float vy = static_cast<float>(velocities(gidx, 1));
                            float vz = static_cast<float>(velocities(gidx, 2));

                            float* __restrict p = row_ptr + i * stride_i;
                            p[0] = vx;
                            p[1] = vy;
                            p[2] = vz;

                            all_velocities_thread_safe[0][id].push_back(vx);
                            all_velocities_thread_safe[1][id].push_back(vy);
                            all_velocities_thread_safe[2][id].push_back(vz);
                        }
                    }
                }

                all_grid_tensors[id] = grid_tensor.contiguous();
            }
        };
        ThreadPool::parallelFor(0, total_blocks, process_blocks_range, total_blocks);
    }
    std::vector<float> predict_vals = ComputePointQ(mesh, Attributes, Index);
    std::vector<float> all_velocities[3];
    for (int c = 0; c < 3; ++c) {
        for (int id = 0; id < total_blocks; ++id) {
            all_velocities[c].insert(all_velocities[c].end(), all_velocities_thread_safe[c][id].begin(),
                                     all_velocities_thread_safe[c][id].end());
        }
    }
    auto compute_mean_std = [](const std::vector<float>& values) -> std::pair<float, float> {
        if (values.empty()) return {0.0f, 1.0f};
        double sum = 0.0;
        for (float v: values) sum += v;
        float mean = static_cast<float>(sum / values.size());
        double sum_sq = 0.0;
        for (float v: values) {
            double diff = v - mean;
            sum_sq += diff * diff;
        }
        float std = static_cast<float>(std::sqrt(sum_sq / values.size()));
        if (std < 1e-10) std = 1.0f;
        return {mean, std};
    };
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "[VortexDetection::Execute] process_blocks = " << elapsed << " s" << std::endl;

    auto [mean_x, std_x] = compute_mean_std(all_velocities[0]);
    auto [mean_y, std_y] = compute_mean_std(all_velocities[1]);
    auto [mean_z, std_z] = compute_mean_std(all_velocities[2]);

    Vector3f mean(mean_x, mean_y, mean_z);
    Vector3f std(std_x, std_y, std_z);
    std::cout << "Computed mean: [" << mean[0] << ", " << mean[1] << ", " << mean[2] << "]" << std::endl;
    std::cout << "Computed std: [" << std[0] << ", " << std[1] << ", " << std[2] << "]" << std::endl;

    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    for (auto& t: all_grid_tensors) {
        if (device.is_cuda() && t.device().is_cpu()) { t = t.to(device, true); }
    }
    torch::Tensor mean_t =
            torch::tensor({mean[0], mean[1], mean[2]}, torch::dtype(torch::kFloat32).device(device)).view({1, 1, 1, 3});
    torch::Tensor std_t =
            torch::tensor({std[0], std[1], std[2]}, torch::dtype(torch::kFloat32).device(device)).view({1, 1, 1, 3});

    // static std::counting_semaphore<> infer_slots(25);
    static SimpleSemaphore infer_slots(25);
    std::mutex progress_mutex;
    auto t3 = std::chrono::high_resolution_clock::now();
    std::cout << "[RUNTIME] processing using device: " << (device.type() == torch::kCUDA ? "CUDA" : "CPU") << std::endl;
    auto processing = [&](int begin, int end) {
        for (int id = begin; id < end; ++id) {
            torch::Tensor arr = all_grid_tensors[id];
            arr = (arr - mean_t) / std_t;
            arr = torch::sigmoid(arr);
            infer_slots.acquire();
            torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
            infer_slots.release();
            all_results_1[id] = std::move(pred_block_1);
        }
    };
    ThreadPool::parallelFor(0, total_blocks, processing, total_blocks);

    auto t4 = std::chrono::high_resolution_clock::now();
    double elapsed_2 = std::chrono::duration<double>(t4 - t3).count();
    std::cout << "[VortexDetection::Execute] predict = " << elapsed_2 << " s" << std::endl;

    UpdateProgress(75 * 0.01);

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
    auto sizes = result_volume_1.sizes();
    float depth = static_cast<float>(sizes[0]);
    float height = static_cast<float>(sizes[1]);
    float width = static_cast<float>(sizes[2]);

    Eigen::Vector3f volume_size(width, height, depth);
    Eigen::Vector3f global_step = range_vec.cwiseQuotient(volume_size);

    // auto t7 = std::chrono::high_resolution_clock::now();
    torch::Tensor result_volume_11 = gaussian_filter3d(result_volume_1, 3, -1);
    // auto t8 = std::chrono::high_resolution_clock::now();
    // double elapsed_4 = std::chrono::duration<double>(t8 - t7).count();
    // std::cout << "[VortexDetection::Execute] predict_after = " << elapsed_4 << " s" << std::endl;
    // return std::make_tuple(result_volume_11, global_step);
    UpdateProgress(80 * 0.01);
    return std::make_tuple(result_volume_11, global_step, std::move(predict_vals));
}

torch::Tensor VortexDetection::gaussian_weights(const torch::Tensor& dists, float sigma) {
    return torch::exp(-0.5 * torch::pow(dists / sigma, 2));
}
bool VortexDetection::IsAxisAlignedUniformGrid(
    const std::vector<Vector3f>& points,
    Eigen::Vector3i& dims,
    Eigen::Vector3f& origin,
    Eigen::Vector3f& spacing,
    float tol=1e-10f)
{
    const int NumPoints = static_cast<int>(points.size());
    if (NumPoints == 0) return false;

    std::vector<float> xs, ys, zs;
    xs.reserve(NumPoints);
    ys.reserve(NumPoints);
    zs.reserve(NumPoints);

    for (const auto& p : points) {
        xs.push_back(p[0]);
        ys.push_back(p[1]);
        zs.push_back(p[2]);
    }

    auto uniq_with_tol = [tol](std::vector<float>& v) {
        if (tol > 0.0f) {
            for (auto& val : v) {
                val = std::round(val / tol) * tol;
            }
        }
        std::sort(v.begin(), v.end());
        auto new_end = std::unique(v.begin(), v.end(),
                                   [tol](float a, float b) {
                                       return std::fabs(a - b) <= tol;
                                   });
        v.erase(new_end, v.end());
    };

    uniq_with_tol(xs);
    uniq_with_tol(ys);
    uniq_with_tol(zs);

    const int nx = static_cast<int>(xs.size());
    const int ny = static_cast<int>(ys.size());
    const int nz = static_cast<int>(zs.size());

    if (1LL * nx * ny * nz != NumPoints) {
        return false;
    }

    auto is_uniform_axis = [tol](const std::vector<float>& v, float& d) -> bool {
        const int n = static_cast<int>(v.size());
        if (n <= 1) {
            d = 0.f;
            return true;
        }
        d = v[1] - v[0];
        if (std::fabs(d) < tol) {
            for (int i = 1; i < n; ++i) {
                if (std::fabs(v[i] - v[0]) > tol) return false;
            }
            return true;
        }

        const float rtol = 1e-5f;
        const float base = std::max(std::fabs(d), 1.0f);
        for (int i = 1; i < n - 1; ++i) {
            const float di = v[i + 1] - v[i];
            const float abs_diff = std::fabs(di - d);
            const float allowed = std::max(tol, rtol * base);
            if (abs_diff > allowed) return false;
        }
        return true;
    };

    float dx = 0.f, dy = 0.f, dz = 0.f;
    if (!is_uniform_axis(xs, dx) ||
        !is_uniform_axis(ys, dy) ||
        !is_uniform_axis(zs, dz)) {
        return false;
    }

    dims    = Eigen::Vector3i(nx, ny, nz);
    origin  = Eigen::Vector3f(xs.front(), ys.front(), zs.front());
    spacing = Eigen::Vector3f(dx, dy, dz);

    return true;
}

void VortexDetection::GetGridXYZCounts(const std::vector<Vector3f>& points,
                      int& nx, int& ny, int& nz)
{
    std::vector<float> xs, ys, zs;
    xs.reserve(points.size());
    ys.reserve(points.size());
    zs.reserve(points.size());

    for (const auto& p : points) {
        xs.push_back(p[0]);
        ys.push_back(p[1]);
        zs.push_back(p[2]);
    }
    auto uniq = [](std::vector<float>& v) { std::sort(v.begin(), v.end()); v.erase(std::unique(v.begin(), v.end()), v.end()); };
    uniq(xs); uniq(ys); uniq(zs);

    nx = xs.size(); ny = ys.size(); nz = zs.size();
}

#endif
IGAME_NAMESPACE_END
