#include "iGameVortexDetection.h"
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "StreamView/iGameStreamTracer.h"
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
#include <memory>
#include <algorithm>
#include <cstdint>

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
#include <ATen/cuda/CUDAContext.h>
#include <torch/script.h>
#include <cuda_runtime.h>
#include <torch/torch.h>
#include <ATen/ATen.h>
#include <c10/core/ScalarType.h>
#include <c10/core/DeviceType.h>
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
//
// #include <ATen/cuda/CUDAContext.h>
// #include <torch/script.h>
// #include <cuda_runtime.h>
// #include <torch/torch.h>
// #include <ATEN/ATEN.h>
// #include <c10/core/ScalarType.h>
// #include <c10/core/DeviceType.h>
// using namespace torch::nn::functional;
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

template <typename Scalar = float, typename Index = int32_t, int Dim = 3>
struct KDTree
{
    static_assert(Dim > 0, "Dim must be positive");
    static_assert(std::is_floating_point<Scalar>::value, "Scalar must be floating point");
    static_assert(std::is_integral<Index>::value, "Index must be integral");

    struct PointCloud
    {
        std::vector<std::array<Scalar, Dim>> pts;

        inline size_t kdtree_get_point_count() const { return pts.size(); }

        inline Scalar kdtree_get_pt(const size_t idx, const size_t dim) const
        {
            return pts[idx][dim];
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& bb) const
        {
            if (pts.empty()) return false;
            std::array<Scalar, Dim> lo = pts[0];
            std::array<Scalar, Dim> hi = pts[0];
            for (const auto& p : pts)
            {
                for (int d = 0; d < Dim; ++d)
                {
                    lo[d] = std::min(lo[d], p[d]);
                    hi[d] = std::max(hi[d], p[d]);
                }
            }
            for (int d = 0; d < Dim; ++d)
            {
                bb[d].low  = lo[d];
                bb[d].high = hi[d];
            }
            return true;
        }
    };

    using Adaptor = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<Scalar, PointCloud>,
        PointCloud,
        Dim,
        Index // 索引类型
    >;

    PointCloud                       cloud_;
    std::unique_ptr<Adaptor>         index_;
    size_t                           leaf_max_size_ = 32;

    KDTree() = default;
    template <typename Derived>
    explicit KDTree(const Eigen::MatrixBase<Derived>& points,
                    size_t leaf_max_size = 32)
    {
        set_points(points);
        build(leaf_max_size);
    }

    template <typename Derived>
    void set_points(const Eigen::MatrixBase<Derived>& points)
    {
        static_assert(Derived::ColsAtCompileTime == Dim || Derived::ColsAtCompileTime == Eigen::Dynamic,
                      "Input matrix must have Dim columns");
        const auto rows = static_cast<size_t>(points.rows());
        cloud_.pts.assign(rows, std::array<Scalar, Dim>{});

        for (size_t i = 0; i < rows; ++i)
        {
            for (int d = 0; d < Dim; ++d)
            {
                cloud_.pts[i][d] = static_cast<Scalar>(points(static_cast<Eigen::Index>(i), d));
            }
        }
    }

    void build(size_t leaf_max_size = 32)
    {
        leaf_max_size_ = leaf_max_size;
        index_ = std::make_unique<Adaptor>(
            Dim, cloud_,
            nanoflann::KDTreeSingleIndexAdaptorParams(static_cast<int>(leaf_max_size_))
        );
        index_->buildIndex();
    }

    template <typename QDerived>
    void query(const Eigen::MatrixBase<QDerived>& q, int k,
               std::vector<Index>& result,
               std::vector<Scalar>& distances,
               bool sorted = true) const
    {
        if (!index_ || cloud_.pts.empty() || k <= 0)
        {
            result.clear(); distances.clear(); return;
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

    template <int K, typename QDerived>
    void queryFixedK(const Eigen::MatrixBase<QDerived>& q,
                     std::array<Index, K>& idx_out,
                     std::array<Scalar, K>& dist2_out,
                     bool sorted = true) const
    {
        static_assert(K > 0, "K must be positive");
        if (!index_ || cloud_.pts.empty())
        {
            for (int i = 0; i < K; ++i) { idx_out[i] = Index(-1); dist2_out[i] = Scalar(0); }
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
// struct KDTree {
//     struct PointCloud {
//         std::vector<std::vector<double>> pts;
//
//         inline size_t kdtree_get_point_count() const { return pts.size(); }
//         inline double kdtree_get_pt(size_t idx, size_t dim) const { return pts[idx][dim]; }
//         template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
//     };
//
//     using NanoFLANNIndex =
//         nanoflann::KDTreeSingleIndexAdaptor<
//             nanoflann::L2_Simple_Adaptor<double, PointCloud>,
//             PointCloud,
//             3 // 维度
//         >;
//
//     PointCloud point_cloud;
//     std::unique_ptr<NanoFLANNIndex> index;
//
//     explicit KDTree(const Eigen::MatrixXd& points) {
//         const size_t rows = static_cast<size_t>(points.rows());
//         point_cloud.pts.assign(rows, std::vector<double>(3));
//         for (size_t i = 0; i < rows; ++i) {
//             point_cloud.pts[i][0] = points(static_cast<Eigen::Index>(i), 0);
//             point_cloud.pts[i][1] = points(static_cast<Eigen::Index>(i), 1);
//             point_cloud.pts[i][2] = points(static_cast<Eigen::Index>(i), 2);
//         }
//         index = std::make_unique<NanoFLANNIndex>(
//             3, point_cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
//         index->buildIndex();
//     }
//
//     void query(const Eigen::VectorXd& q, int k,
//                std::vector<int>& result, std::vector<double>& distances) const {
//         if (point_cloud.pts.empty() || k <= 0) {
//             result.clear(); distances.clear(); return;
//         }
//
//         double query_pt[3] = { q[0], q[1], q[2] };
//
//         using IdxT = nanoflann::KNNResultSet<double>::IndexType;
//         std::vector<IdxT> idx_tmp(static_cast<size_t>(k));
//         distances.resize(static_cast<size_t>(k));
//
//         nanoflann::KNNResultSet<double> rs(static_cast<size_t>(k));
//         rs.init(idx_tmp.data(), distances.data());
//         index->findNeighbors(rs, &query_pt[0], nanoflann::SearchParameters());
//
//         const size_t n = rs.size();
//         idx_tmp.resize(n);
//         distances.resize(n);
//
//         result.resize(n);
//         for (size_t i = 0; i < n; ++i) result[i] = static_cast<int>(idx_tmp[i]);
//     }
// };

struct BlockInfo {
    Vector3f minP, maxP;
    Vector3f step;
    int nx, ny, nz;
    std::vector<int64_t> prediction;
};

bool VortexDetection::DetectionVortexWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes,
                                                     int Index, std::string name) {
    std::cout<<"VortexDetection::DetectionVortex must in VolumeMesh!"<<std::endl;
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
    // float maxLen = std::max({range[0], range[1], range[2]});
    // int split = 6;
    // double min_effective_edge = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 20) * 2;
    // std::cout<<"min_effective_edge = "<<min_effective_edge<<std::endl;
    // int targetPoints = int(maxLen / (min_effective_edge + 1e-8)) + 1;
    // targetPoints = targetPoints / split;
    // std::cout<<"targetPoints = "<<targetPoints<<std::endl;

    int split = 6;
    const double Lp = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 60.0);
    const double Lv = std::cbrt(compute_percentile_cell_volume(gridPoints, gridCells, 75.0));
    const double L  = std::max(Lp, 0.5 * Lv);
    const double bbox_diag = double(range.norm());
    const double alpha   = 1.3;
    const double h_raw   = alpha * L;
    const double h_floor = bbox_diag * 1e-3;
    const double h_ceil  = bbox_diag * 0.08;
    double h = std::min(std::max(h_raw, h_floor), h_ceil);

    auto vol_tet4 = [](const Vector3f& a, const Vector3f& b,
                       const Vector3f& c, const Vector3f& d) -> double
    {
        const double bax = double(b[0]) - double(a[0]);
        const double bay = double(b[1]) - double(a[1]);
        const double baz = double(b[2]) - double(a[2]);
        const double cax = double(c[0]) - double(a[0]);
        const double cay = double(c[1]) - double(a[1]);
        const double caz = double(c[2]) - double(a[2]);
        const double dax = double(d[0]) - double(a[0]);
        const double day = double(d[1]) - double(a[1]);
        const double daz = double(d[2]) - double(a[2]);
        const double det = bax*(cay*daz - caz*day)
                         - bay*(cax*daz - caz*dax)
                         + baz*(cax*day - cay*dax);
        return std::abs(det) * (1.0/6.0);
    };

    // auto sum_cell_volume_parallel = [&](const std::vector<Vector3f>& P,
    //                                     const std::vector<Volume*>&  C) -> double
    // {
    //     double sumV = 0.0;
    //     #pragma omp parallel
    //     {
    //         std::vector<double> el;
    //         el.reserve(64);
    //         double local_sum = 0.0;
    //         #pragma omp for schedule(guided)
    //         for (int i = 0; i < (int)C.size(); ++i) {
    //             Volume* cell = C[i];
    //             const int n = cell->GetNumberOfPoints();
    //             if (n < 4) continue;
    //             if (n == 4) {
    //                 const int id0 = cell->GetPointId(0);
    //                 const int id1 = cell->GetPointId(1);
    //                 const int id2 = cell->GetPointId(2);
    //                 const int id3 = cell->GetPointId(3);
    //                 local_sum += vol_tet4(P[id0], P[id1], P[id2], P[id3]);
    //             } else {
    //                 el.clear();
    //                 el.reserve((size_t)n * (n - 1) / 2);
    //                 for (int j = 0; j < n; ++j) {
    //                     const int vj = cell->GetPointId(j);
    //                     const auto& pj = P[vj];
    //                     for (int k = j + 1; k < n; ++k) {
    //                         const int vk = cell->GetPointId(k);
    //                         const auto& pk = P[vk];
    //                         const double dx = double(pj[0]) - double(pk[0]);
    //                         const double dy = double(pj[1]) - double(pk[1]);
    //                         const double dz = double(pj[2]) - double(pk[2]);
    //                         const double len = std::sqrt(dx*dx + dy*dy + dz*dz);
    //                         if (len > 1e-12 && std::isfinite(len)) el.push_back(len);
    //                     }
    //                 }
    //                 if (!el.empty()) {
    //                     const size_t mid = el.size() / 2;
    //                     std::nth_element(el.begin(), el.begin() + mid, el.end());
    //                     const double Lm = el[mid];
    //                     local_sum += Lm * Lm * Lm;
    //                 }
    //             }
    //         }
    //         #pragma omp atomic
    //         sumV += local_sum;
    //     }
    //     return sumV;
    // };
    // const double V_occ  = sum_cell_volume_parallel(gridPoints, gridCells);
    // const double V_bbox = std::max(1e-18,
    //                         double(range[0]) * double(range[1]) * double(range[2]));
    // const double f = std::clamp(V_occ / V_bbox, 1e-6, 1.0);
    // const double sub_len_x = double(range[0]) / double(split);
    // const double sub_len_y = double(range[1]) / double(split);
    // const double sub_len_z = double(range[2]) / double(split);
    // const double V_sub     = sub_len_x * sub_len_y * sub_len_z;
    // const long long V_TARGET = 96LL * 96LL * 96LL;
    // const double h_occ = std::cbrt(std::max(1e-18, (V_sub * f) / double(V_TARGET)));
    // const double c_occ = 1.5;
    // h = std::min(h, c_occ * h_occ);
    //
    // auto compute_n = [&](double len) { return int(std::ceil(len / h)) + 1; };
    // int nx = compute_n(sub_len_x);
    // int ny = compute_n(sub_len_y);
    // int nz = compute_n(sub_len_z);
    // const int N_MIN = 12, N_MAX = 200;
    // nx = std::max(N_MIN, std::min(nx, N_MAX));
    // ny = std::max(N_MIN, std::min(ny, N_MAX));
    // nz = std::max(N_MIN, std::min(nz, N_MAX));
    //
    // const long long VOX_CAP = 96LL * 96LL * 96LL;
    // long long vox = 1LL * nx * ny * nz;
    // if (vox > VOX_CAP) {
    //     const double s = std::cbrt(double(VOX_CAP) / double(vox));
    //     nx = std::max(N_MIN, int(nx * s));
    //     ny = std::max(N_MIN, int(ny * s));
    //     nz = std::max(N_MIN, int(nz * s));
    // }
    //
    // std::cout << "per-block resolution: " << nx << " x " << ny << " x " << nz << std::endl;
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
        for (int j = 0; j < n && vc < VMAX; j += stride) {
            vids[vc++] = cell->GetPointId(j);
        }
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
                const double len = std::sqrt(dx*dx + dy*dy + dz*dz);
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
    auto sum_cell_volume_parallel_fast = [&](const std::vector<Vector3f>& P,
                                             const std::vector<Volume*>&  C) -> double
    {
        double sumV = 0.0;
        #pragma omp parallel for schedule(dynamic,1) reduction(+:sumV)
        for (int i = 0; i < (int)C.size(); ++i) {
            Volume* cell = C[i];
            if (!cell) continue;
            sumV += approx_cell_volume_by_bounded_edges(P, cell);
        }
        return sumV;
    };
    const double V_occ  = sum_cell_volume_parallel_fast(gridPoints, gridCells);
    const double V_bbox = std::max(1e-18, double(range[0]) * double(range[1]) * double(range[2]));
    const double f      = std::clamp(V_occ / V_bbox, 1e-6, 1.0);

    const double sub_len_x = double(range[0]) / double(split);
    const double sub_len_y = double(range[1]) / double(split);
    const double sub_len_z = double(range[2]) / double(split);
    const double V_sub     = sub_len_x * sub_len_y * sub_len_z;

    constexpr long long V_TARGET = 96LL * 96LL * 96LL;
    const double h_occ = std::cbrt(std::max(1e-18, (V_sub * f) / double(V_TARGET)));
    const double c_occ = 1.5;
    h = std::min(h, c_occ * h_occ);

    auto compute_n = [&](double len) { return int(std::ceil(len / h)) + 1; };
    int nx = compute_n(sub_len_x);
    int ny = compute_n(sub_len_y);
    int nz = compute_n(sub_len_z);

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

    // int split = 6;
    // double Lp = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 60);
    // double Lv = cbrt(compute_percentile_cell_volume(gridPoints, gridCells, 75));
    // double L = std::max(Lp, 0.5 * Lv);
    // double bbox_diag = range.norm();
    //
    // double alpha = 1.3;
    // double h_raw = alpha * L;
    // double h_floor = bbox_diag * 1e-3;
    // double h_ceil = bbox_diag * 0.08;
    // double h = std::min(std::max(h_raw, h_floor), h_ceil);
    //
    // auto sum_cell_volume = [&](const std::vector<Vector3f>& P,
    //                        const std::vector<Volume*>& C)->double {
    //     double sumV = 0.0;
    //     for (auto cell : C) {
    //         int n = cell->GetNumberOfPoints();
    //         if (n < 4) continue;
    //         std::vector<int> vids(n);
    //         for (int j = 0; j < n; ++j) vids[j] = cell->GetPointId(j);
    //         if (n == 4) {
    //             auto vol_tet = [&](const Vector3f& a,const Vector3f& b,
    //                                const Vector3f& c,const Vector3f& d)->double{
    //                 double bax=b[0]-a[0], bay=b[1]-a[1], baz=b[2]-a[2];
    //                 double cax=c[0]-a[0], cay=c[1]-a[1], caz=c[2]-a[2];
    //                 double dax=d[0]-a[0], day=d[1]-a[1], daz=d[2]-a[2];
    //                 double det = bax*(cay*daz - caz*day)
    //                            - bay*(cax*daz - caz*dax)
    //                            + baz*(cax*day - cay*dax);
    //                 return std::abs(det)/6.0;
    //             };
    //             sumV += vol_tet(P[vids[0]], P[vids[1]], P[vids[2]], P[vids[3]]);
    //         } else {
    //             std::vector<double> el; el.reserve(n*(n-1)/2);
    //             for (int j=0;j<n;++j) for (int k=j+1;k<n;++k){
    //                 double len = (P[vids[j]] - P[vids[k]]).norm();
    //                 if (len>1e-12 && std::isfinite(len)) el.push_back(len);
    //             }
    //             if (!el.empty()){
    //                 size_t mid = el.size()/2;
    //                 std::nth_element(el.begin(), el.begin()+mid, el.end());
    //                 double Lm = el[mid];
    //                 sumV += Lm*Lm*Lm;
    //             }
    //         }
    //     }
    //     return sumV;
    // };
    //
    // double V_occ  = sum_cell_volume(gridPoints, gridCells);
    // double V_bbox = std::max(1e-18, double(range[0])*double(range[1])*double(range[2]));
    // double f = std::clamp(V_occ / V_bbox, 1e-6, 1.0);
    //
    // double sub_len_x = range[0] / split;
    // double sub_len_y = range[1] / split;
    // double sub_len_z = range[2] / split;
    // double V_sub = sub_len_x * sub_len_y * sub_len_z;
    // const long long V_TARGET = 96LL*96LL*96LL;
    // double h_occ = std::cbrt(std::max(1e-18, (V_sub * f) / double(V_TARGET)));
    //
    // double c_occ = 1.5;
    // h = std::min(h, c_occ * h_occ);
    //
    // auto compute_n = [&](double len) { return int(std::ceil(len / h)) + 1; };
    // int nx = compute_n(sub_len_x);
    // int ny = compute_n(sub_len_y);
    // int nz = compute_n(sub_len_z);
    // int N_MIN = 12, N_MAX = 200;
    // long long VOX_CAP = 96LL*96LL*96LL;
    // nx = std::max(N_MIN, std::min(nx, N_MAX));
    // ny = std::max(N_MIN, std::min(ny, N_MAX));
    // nz = std::max(N_MIN, std::min(nz, N_MAX));
    // long long vox = 1LL * nx * ny * nz;
    // if (vox > VOX_CAP) {
    //     double s = std::cbrt(double(VOX_CAP)/double(vox));
    //     nx = std::max(N_MIN, int(nx*s));
    //     ny = std::max(N_MIN, int(ny*s));
    //     nz = std::max(N_MIN, int(nz*s));
    // }
    // std::cout << "per-block resolution: " << nx << " x " << ny << " x " << nz << std::endl;

    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    for (auto& t : all_grid_tensors) {
        if (device.is_cuda() && t.device().is_cpu()) {
            t = t.to(device, /*non_blocking=*/true);
        }
    }
    torch::Tensor mean_t = torch::tensor({mean[0], mean[1], mean[2]}, torch::dtype(torch::kFloat32).device(device))
                               .view({1, 1, 1, 3});
    torch::Tensor std_t  = torch::tensor({std[0],  std[1],  std[2]},  torch::dtype(torch::kFloat32).device(device))
                               .view({1, 1, 1, 3});

    static std::counting_semaphore<> infer_slots(25);
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
    return std::make_tuple(result_volume_11, global_step, std::move(predict_vals));
}

torch::Tensor VortexDetection::gaussian_weights(const torch::Tensor& dists, float sigma) {
    return torch::exp(-0.5 * torch::pow(dists / sigma, 2));
}

#endif
IGAME_NAMESPACE_END
