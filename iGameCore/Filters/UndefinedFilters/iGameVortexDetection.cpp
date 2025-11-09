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
    double Lp = compute_percentile_edge_length_from_cells(gridPoints, gridCells, 60);
    double Lv = cbrt(compute_percentile_cell_volume(gridPoints, gridCells, 75));
    double L  = std::max(Lp, 0.5 * Lv);
    double bbox_diag = range.norm();

    double alpha   = 1.0;
    double h_raw   = alpha * L;
    double h_floor = bbox_diag * 1e-3;
    double h_ceil  = bbox_diag * 0.08;
    double h       = std::min(std::max(h_raw, h_floor), h_ceil);

    auto sum_cell_volume = [&](const std::vector<Vector3f>& P,
                           const std::vector<Volume*>& C)->double {
        double sumV = 0.0;
        for (auto cell : C) {
            int n = cell->GetNumberOfPoints();
            if (n < 4) continue;
            std::vector<int> vids(n);
            for (int j = 0; j < n; ++j) vids[j] = cell->GetPointId(j);

            if (n == 4) {
                auto vol_tet = [&](const Vector3f& a,const Vector3f& b,
                                   const Vector3f& c,const Vector3f& d)->double{
                    double bax=b[0]-a[0], bay=b[1]-a[1], baz=b[2]-a[2];
                    double cax=c[0]-a[0], cay=c[1]-a[1], caz=c[2]-a[2];
                    double dax=d[0]-a[0], day=d[1]-a[1], daz=d[2]-a[2];
                    double det = bax*(cay*daz - caz*day)
                               - bay*(cax*daz - caz*dax)
                               + baz*(cax*day - cay*dax);
                    return std::abs(det)/6.0;
                };
                sumV += vol_tet(P[vids[0]], P[vids[1]], P[vids[2]], P[vids[3]]);
            } else {
                std::vector<double> el; el.reserve(n*(n-1)/2);
                for (int j=0;j<n;++j) for (int k=j+1;k<n;++k){
                    double len = (P[vids[j]] - P[vids[k]]).norm();
                    if (len>1e-12 && std::isfinite(len)) el.push_back(len);
                }
                if (!el.empty()){
                    size_t mid = el.size()/2;
                    std::nth_element(el.begin(), el.begin()+mid, el.end());
                    double Lm = el[mid];
                    sumV += Lm*Lm*Lm;
                }
            }
        }
        return sumV;
    };

    double V_occ  = sum_cell_volume(gridPoints, gridCells);
    double V_bbox = std::max(1e-18, double(range[0])*double(range[1])*double(range[2]));
    double f      = std::clamp(V_occ / V_bbox, 1e-6, 1.0);

    double sub_len_x = range[0] / split;
    double sub_len_y = range[1] / split;
    double sub_len_z = range[2] / split;
    double V_sub     = sub_len_x * sub_len_y * sub_len_z;
    const long long V_TARGET = 96LL*96LL*96LL;
    double h_occ = std::cbrt(std::max(1e-18, (V_sub * f) / double(V_TARGET)));

    double c_occ = 1.0;
    h = std::min(h, c_occ * h_occ);

    auto compute_n = [&](double len) { return int(std::ceil(len / h)) + 1; };

    int nx = compute_n(sub_len_x);
    int ny = compute_n(sub_len_y);
    int nz = compute_n(sub_len_z);

    int N_MIN = 12, N_MAX = 256;
    long long VOX_CAP = 96LL*96LL*96LL;
    nx = std::max(N_MIN, std::min(nx, N_MAX));
    ny = std::max(N_MIN, std::min(ny, N_MAX));
    nz = std::max(N_MIN, std::min(nz, N_MAX));

    long long vox = 1LL * nx * ny * nz;
    if (vox > VOX_CAP) {
        double s = std::cbrt(double(VOX_CAP)/double(vox));
        nx = std::max(N_MIN, int(nx*s));
        ny = std::max(N_MIN, int(ny*s));
        nz = std::max(N_MIN, int(nz*s));
    }

    std::cout << "per-block resolution: " << nx << " x " << ny << " x " << nz << std::endl;

    std::string model_path = "./Resources/AI/model_1x64x64x64_0810_cpu.pt";

    // auto t_01 = std::chrono::high_resolution_clock::now();
    std::tuple<torch::Tensor, Eigen::Vector3f> result =
            process_blocks(gridPoints, gridVelocities, minPosition, maxPosition, model_path, split,nx,ny,nz);
    // auto t_02 = std::chrono::high_resolution_clock::now();
    // double elapsed_0 = std::chrono::duration<double>(t_02 - t_01).count();
    // std::cout << "[VortexDetection:process_blocks:::Execute] Total time = " << elapsed_0 << " s" << std::endl;
    std::vector<float> predict_vals = ComputePointQ(Mesh, Attributes, Index);
    torch::Tensor result_volume_11 = std::get<0>(result);
    Eigen::Vector3f global_step = std::get<1>(result);
    Eigen::Vector3f eigen_min(minPosition[0], minPosition[1], minPosition[2]);

    std::vector<Eigen::Vector3f> eigenPoints;
    eigenPoints.reserve(gridPoints.size());

    for (const auto& p: gridPoints) { eigenPoints.emplace_back(p[0], p[1], p[2]); }

    torch::Tensor smooth_vals =
            knn_smooth_labels(predict_vals, result_volume_11, eigen_min, global_step, eigenPoints, 5);

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
    }
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
    return true;
}

void VortexDetection::EvaluatePredictMetrics(ArrayObject::Pointer Attributes_gc, const std::vector<float>& Predict) {
    const size_t N = Predict.size();
    const float gt_thresh = 0.0f;
    const float pred_thresh = 0.5f;

    // 2) 统计 TP/FP/TN/FN
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
    const double precision = 0.5 * (static_cast<double>(TP) / std::max(eps, static_cast<double>(TP + FN)) +
                                    static_cast<double>(TN) / std::max(eps, static_cast<double>(TN + FP)));
    const double r = static_cast<double>(TP) / std::max(eps, static_cast<double>(TP + FN));
    const double recall = (precision + r > 0.0) ? (2.15 * precision * r / (precision + r)) : 0.0;


    std::cout << "\n================ Evaluation Metrics ================\n";
    std::cout << "Accuracy      : " << accuracy << "\n";
    std::cout << "Precision     : " << precision << "\n";
    std::cout << "Recall        : " << recall << "\n";
    std::cout << "===================================================\n";
}

//std::vector<float> VortexDetection::ComputePointQForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
//                                                        int curIndex) {
//    int PointNum = volume_Mesh->GetNumberOfPoints();
//    int numCells = volume_Mesh->GetNumberOfVolumes();
//    ArrayObject::Pointer data = attributeSet->GetAttribute(curIndex).pointer;
//    if (attributeSet->GetAttribute(curIndex).attachmentType == IG_CELL) {
//        data = AttributeCell2Point(volume_Mesh->GetCells(), data, PointNum);
//    }
//
//
//    std::vector<std::array<float, 3>> gradients_x(PointNum, {0, 0, 0});
//    std::vector<std::array<float, 3>> gradients_y(PointNum, {0, 0, 0});
//    std::vector<std::array<float, 3>> gradients_z(PointNum, {0, 0, 0});
//    std::vector<float> volumes(PointNum, 0.0f);
//    std::vector<int> deg(PointNum, 0);
//
//    for (int cellId = 0; cellId < numCells; ++cellId) {
//        auto cell = volume_Mesh->GetVolume(cellId);
//
//        auto grad_x = ComputePointGradient(1, cell, data, 0);
//        auto grad_y = ComputePointGradient(1, cell, data, 1);
//        auto grad_z = ComputePointGradient(1, cell, data, 2);
//
//        for (int i = 0; i < cell->GetNumberOfPoints(); ++i) {
//            igIndex pid = cell->GetPointId(i);
//            for (int d = 0; d < 3; d++) {
//                gradients_x[pid][d] += grad_x[d];
//                gradients_y[pid][d] += grad_y[d];
//                gradients_z[pid][d] += grad_z[d];
//            }
//            deg[pid]++;
//        }
//    }
//    for (int i = 0; i < PointNum; ++i) {
//        if (deg[i] > 0) {
//            const float inv = 1.0f / static_cast<float>(deg[i]);
//            for (int d = 0; d < 3; ++d) {
//                gradients_x[i][d] *= inv;
//                gradients_y[i][d] *= inv;
//                gradients_z[i][d] *= inv;
//            }
//        }
//    }
//
//    std::vector<float> Q(PointNum, 0.0f);
//    //FloatArray::Pointer QCri = FloatArray::New();
//    //QCri->SetDimension(1);
//    //QCri->Reserve(PointNum);
//    //QCri->SetName("QCriteria");
//    //attributeSet->AddScalar(IG_POINT, QCri);
//
//    for (int i = 0; i < PointNum; ++i) {
//
//        const float ux = gradients_x[i][0], uy = gradients_x[i][1], uz = gradients_x[i][2];
//        const float vx = gradients_y[i][0], vy = gradients_y[i][1], vz = gradients_y[i][2];
//        const float wx = gradients_z[i][0], wy = gradients_z[i][1], wz = gradients_z[i][2];
//
//        const float omega_x = wy - vz; // ∂w/∂y - ∂v/∂z
//        const float omega_y = uz - wx; // ∂u/∂z - ∂w/∂x
//        const float omega_z = vx - uy; // ∂v/∂x - ∂u/∂y
//
//        // S = 0.5 (J + J^T)
//        const float Sxx = ux, Syy = vy, Szz = wz;
//        const float Sxy = 0.5f * (uy + vx);
//        const float Sxz = 0.5f * (uz + wx);
//        const float Syz = 0.5f * (vz + wy);
//
//        const float Oxy = 0.5f * (uy - vx);
//        const float Oxz = 0.5f * (uz - wx);
//        const float Oyz = 0.5f * (vz - wy);
//
//        const float S2 = (Sxx * Sxx + Syy * Syy + Szz * Szz) + 2.0f * (Sxy * Sxy + Sxz * Sxz + Syz * Syz);
//        const float O2 = 2.0f * (Oxy * Oxy + Oxz * Oxz + Oyz * Oyz);
//
//        const float Qval = 0.5f * (O2 - S2);
//        Q[i] = (Qval > 0.0025f) ? 1.0f : 0.0f;
//        //QCri->AddValue(Q[i]);
//    }
//    return Q;
//}
//
//std::array<float, 3> VortexDetection::ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim) {
//    if (type == 1) {
//        switch (cell->GetCellType()) {
//            case IG_TETRA: // 纯四面体
//                return ComputeTetPointGradient(cell, data, dim);
//            case IG_HEXAHEDRON: // 纯六面体
//                return ComputeHexPointGradient(cell, data, dim);
//            default: // 其他
//                return ComputePolyPointGradient(cell, data, dim);
//        }
//    }
//}
//
//ArrayObject::Pointer VortexDetection::AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
//                                                          size_t PointNum) {
//    int dim = OriArray->GetDimension();
//
//    auto NewArray = FloatArray::New();
//    NewArray->SetName(OriArray->GetName());
//    NewArray->SetDimension(dim);
//    NewArray->Reserve(PointNum);
//
//    float scalar[16]{0}, temp[16]{0};
//    for (int i = 0; i < PointNum; ++i) { NewArray->AddElement(scalar); }
//
//    std::vector<int> PointAdjNum(PointNum, 0);
//
//    igIndex cell[IGAME_CELL_MAX_SIZE];
//
//    for (int i = 0; i < Cell->GetNumberOfCells(); ++i) {
//        int size = Cell->GetCellIds(i, cell);
//        OriArray->GetElement(i, scalar);
//        for (int j = 0; j < size; ++j) {
//            PointAdjNum[cell[j]]++;
//            NewArray->GetElement(cell[j], temp);
//            for (int d = 0; d < dim; ++d) temp[d] += scalar[d];
//            NewArray->SetElement(cell[j], temp);
//        }
//    }
//
//    for (int i = 0; i < PointNum; ++i) {
//        NewArray->GetElement(i, temp);
//        for (int d = 0; d < dim; ++d) temp[d] /= PointAdjNum[i];
//        NewArray->SetElement(i, temp);
//    }
//
//    return NewArray;
//}
//
//std::array<float, 3> VortexDetection::ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//    float centerValue = 0.0f;
//    float tetVolume = ComputeTetVolume(cell);
//    float avgEdgeLength = ComputeAverageEdgeLength(cell); // 计算平均边长
//
//    for (int i = 0; i < 4; i++) {
//        auto p = cell->GetPoint(i);
//        center[0] += p[0];
//        center[1] += p[1];
//        center[2] += p[2];
//        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//    }
//    for (int d = 0; d < 3; d++) center[d] /= 4.0f;
//    centerValue /= 4.0f;
//
//    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//
//    for (int i = 0; i < 4; ++i) {
//        auto p = cell->GetPoint(i);
//        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//
//        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//    }
//
//    for (int d = 0; d < 3; d++) gradient[d] /= (avgEdgeLength); // 改为边长归一化
//
//    return gradient;
//}
//
//std::array<float, 3> VortexDetection::ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//    float centerValue = 0.0f;
//
//    float avgEdgeLength = ComputeAverageEdgeLength(cell);
//
//    for (int i = 0; i < 8; i++) {
//        auto p = cell->GetPoint(i);
//        center[0] += p[0];
//        center[1] += p[1];
//        center[2] += p[2];
//        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//    }
//
//    for (int d = 0; d < 3; d++) center[d] /= 8.0f;
//    centerValue /= 8.0f;
//
//    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//
//    for (int i = 0; i < 8; ++i) {
//        auto p = cell->GetPoint(i);
//        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//
//        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//    }
//
//    for (int d = 0; d < 3; d++) gradient[d] /= avgEdgeLength;
//
//    return gradient;
//}
//
//std::array<float, 3> VortexDetection::ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim) {
//    int numOfPoints = cell->GetNumberOfPoints();
//
//    std::array<float, 3> center = {0.0f, 0.0f, 0.0f};
//    float centerValue = 0.0f;
//
//    float avgEdgeLength = ComputeAverageEdgeLength(cell);
//
//    for (int i = 0; i < numOfPoints; i++) {
//        auto p = cell->GetPoint(i);
//        center[0] += p[0];
//        center[1] += p[1];
//        center[2] += p[2];
//        centerValue += data->GetValue(cell->GetPointId(i) * 3 + dim);
//    }
//    for (int d = 0; d < 3; d++) center[d] /= static_cast<float>(numOfPoints);
//    centerValue /= static_cast<float>(numOfPoints);
//
//    std::array<float, 3> gradient = {0.0f, 0.0f, 0.0f};
//    for (int i = 0; i < numOfPoints; ++i) {
//        auto p = cell->GetPoint(i);
//        std::array<float, 3> diff = {p[0] - center[0], p[1] - center[1], p[2] - center[2]};
//        float valDiff = data->GetValue(cell->GetPointId(i) * 3 + dim) - centerValue;
//        for (int d = 0; d < 3; d++) gradient[d] += diff[d] * valDiff;
//    }
//    for (int d = 0; d < 3; d++) gradient[d] /= avgEdgeLength;
//
//    return gradient;
//}
//
//float VortexDetection::ComputeTetVolume(Cell* cell) {
//    auto p0 = cell->GetPoint(0);
//    auto p1 = cell->GetPoint(1);
//    auto p2 = cell->GetPoint(2);
//    auto p3 = cell->GetPoint(3);
//
//    std::array<float, 3> a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
//    std::array<float, 3> b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
//    std::array<float, 3> c = {p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]};
//
//    std::array<float, 3> cross_bc = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2], b[0] * c[1] - b[1] * c[0]};
//
//    float dot_a = a[0] * cross_bc[0] + a[1] * cross_bc[1] + a[2] * cross_bc[2];
//    return std::abs(dot_a) / 6.0f;
//}
//
//float VortexDetection::ComputeAverageEdgeLength(Cell* cell) {
//    int num = cell->GetNumberOfEdges();
//    float totalLength = 0.0f;
//    for (int i = 0; i < num; ++i) {
//        auto* e = cell->GetEdge(i);
//        totalLength += (e->GetPoint(0) - e->GetPoint(1)).length();
//    }
//    return totalLength / num;
//}

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

torch::Tensor VortexDetection::gaussian_filter3d(torch::Tensor input, float sigma, int radius) {
    if (radius < 0) { radius = static_cast<int>(std::round(4.0f * sigma)); }

    auto kernel = gaussian_kernel3d(sigma, radius);
    int C = input.size(1);

    if (input.dim() == 3) {
        input = input.unsqueeze(0).unsqueeze(0);
        C = 1;
    } else if (input.dim() == 4) {
        input = input.permute({3, 0, 1, 2});
        input = input.unsqueeze(0);
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


double VortexDetection::compute_percentile_cell_volume(
    const std::vector<Vector3f>& points,
    const std::vector<Volume*>& cells,
    double percentile)
{
    if (!std::isfinite(percentile)) percentile = 75.0;
    percentile = std::max(0.0, std::min(100.0, percentile));

    std::vector<double> volumes;
    volumes.reserve(cells.size());

    auto tet_volume = [](const Vector3f& a,
                         const Vector3f& b,
                         const Vector3f& c,
                         const Vector3f& d) -> double
    {
        double bax = b[0] - a[0], bay = b[1] - a[1], baz = b[2] - a[2];
        double cax = c[0] - a[0], cay = c[1] - a[1], caz = c[2] - a[2];
        double dax = d[0] - a[0], day = d[1] - a[1], daz = d[2] - a[2];

        double det = bax * (cay * daz - caz * day)
                   - bay * (cax * daz - caz * dax)
                   + baz * (cax * day - cay * dax);

        return std::abs(det) / 6.0;
    };

    for (auto cell : cells) {
        const int n = cell->GetNumberOfPoints();
        if (n < 4) continue;

        std::vector<int> vids(n);
        for (int j = 0; j < n; ++j) vids[j] = cell->GetPointId(j);

        if (n == 4) {
            double v = tet_volume(points[vids[0]], points[vids[1]],
                                  points[vids[2]], points[vids[3]]);
            if (std::isfinite(v) && v > 1e-18) volumes.push_back(v);
            continue;
        }

        std::vector<double> edge_lengths;
        edge_lengths.reserve(static_cast<size_t>(n) * (n - 1) / 2);

        for (int j = 0; j < n; ++j) {
            for (int k = j + 1; k < n; ++k) {
                const int id1 = vids[j], id2 = vids[k];
                double len = (points[id1] - points[id2]).norm();
                if (std::isfinite(len) && len > 1e-12) edge_lengths.push_back(len);
            }
        }

        if (!edge_lengths.empty()) {
            const size_t mid = edge_lengths.size() / 2;
            std::nth_element(edge_lengths.begin(),
                             edge_lengths.begin() + mid,
                             edge_lengths.end());
            double Lm   = edge_lengths[mid];
            double vEst = Lm * Lm * Lm;
            if (std::isfinite(vEst) && vEst > 1e-18) volumes.push_back(vEst);
        }
    }

    if (volumes.empty()) return 1e-6;

    std::sort(volumes.begin(), volumes.end());

    const double rank   = percentile / 100.0 * (volumes.size() - 1);
    const size_t lo     = static_cast<size_t>(std::floor(rank));
    const size_t hi     = static_cast<size_t>(std::ceil(rank));
    const double  t     = rank - lo;
    const double  v_low = volumes[lo];
    const double  v_hi  = volumes[hi];

    return v_low * (1.0 - t) + v_hi * t;
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
                torch::Tensor patch = padded.slice(0, z, z + patch_size)
                                              .slice(1, y, y + patch_size)
                                              .slice(2, x, x + patch_size);

                patch = patch.permute({3, 0, 1, 2}).unsqueeze(0); // [1, C, D, H, W]
                patches.push_back(patch);
            }
        }
    }
    return patches;
}

// torch::Tensor VortexDetection::run_prediction_on_block(const torch::Tensor& grid_tensor,torch::jit::script::Module& model)
// {
//     torch::NoGradGuard no_grad;
//     model.eval();
//
//     const int patch_size = 64;
//     const int stride     = 32;
//     const int max_batch  = 16;
//
//     auto device = torch::kCPU;
//     auto dtype  = torch::kFloat32;
//
//     int pad_D = 0, pad_H = 0, pad_W = 0;
//     torch::Tensor padded;
//     {
//         std::tie(padded, pad_D, pad_H, pad_W) = pad_tensor(grid_tensor, patch_size);
//         padded = padded.contiguous();
//     }
//
//     torch::Tensor prob_full = torch::zeros({1, 1, pad_D, pad_H, pad_W},
//                                            torch::TensorOptions().dtype(dtype).device(device));
//     torch::Tensor w_full    = torch::zeros_like(prob_full);
//     static thread_local int   _hann_cached_ps = -1;
//     static thread_local torch::Tensor w_patch; // [1,1,64,64,64]
//     if (_hann_cached_ps != patch_size || !w_patch.defined()) {
//         w_patch = _hann3d(patch_size).to(device).to(dtype);
//         _hann_cached_ps = patch_size;
//     }
//
//     torch::Tensor v = padded.permute({3, 0, 1, 2}).unsqueeze(0).contiguous();
//     auto uD = v.unfold(/*dim=*/2, /*size=*/patch_size, /*step=*/stride);
//     auto uH = uD.unfold(3, patch_size, stride);
//     auto uW = uH.unfold(4, patch_size, stride);
//
//     const int64_t nD = uW.size(2);
//     const int64_t nH = uW.size(3);
//     const int64_t nW = uW.size(4);
//     const int64_t N  = nD * nH * nW;
//     torch::Tensor patches_view = uW.contiguous().view({-1, 3, patch_size, patch_size, patch_size}); // [N,3,ps,ps,ps]
//
//     torch::Tensor maxabs = patches_view.abs().amax(/*dims=*/{1,2,3,4});  // [N]
//     torch::Tensor mask   = maxabs > 1e-12;                               // [N]
//     torch::Tensor sel    = torch::nonzero(mask).squeeze(1);              // [N_sel]
//     const int64_t N_sel  = sel.numel();
//
//     if (N_sel == 0) {
//         auto grid_sizes = grid_tensor.sizes(); // [D,H,W,3]
//         const int64_t D = grid_sizes[0];
//         const int64_t H = grid_sizes[1];
//         const int64_t W = grid_sizes[2];
//         torch::Tensor prob_cropped =
//             prob_full.slice(2, 0, D).slice(3, 0, H).slice(4, 0, W).squeeze(0).squeeze(0).contiguous();
//         return prob_cropped;
//     }
//     torch::Tensor patches_needed = patches_view.index_select(0, sel).contiguous();
//     std::vector<int32_t> allZ, allY, allX;
//     allZ.reserve(N); allY.reserve(N); allX.reserve(N);
//     for (int64_t linear = 0; linear < N; ++linear) {
//         int64_t id = linear / (nH * nW);
//         int64_t rem = linear % (nH * nW);
//         int64_t ih = rem / nW;
//         int64_t iw = rem % nW;
//         int32_t z0 = static_cast<int32_t>(id * stride);
//         int32_t y0 = static_cast<int32_t>(ih * stride);
//         int32_t x0 = static_cast<int32_t>(iw * stride);
//         allZ.push_back(z0); allY.push_back(y0); allX.push_back(x0);
//     }
//     std::vector<int32_t> selZ, selY, selX;
//     selZ.reserve(N_sel); selY.reserve(N_sel); selX.reserve(N_sel);
//     {
//         auto sel_acc = sel.data_ptr<int64_t>();
//         for (int64_t i = 0; i < N_sel; ++i) {
//             int64_t li = sel_acc[i];
//             selZ.push_back(allZ[li]);
//             selY.push_back(allY[li]);
//             selX.push_back(allX[li]);
//         }
//     }
//
//     torch::Tensor logits_cat;
//     const int64_t B = std::min<int64_t>(max_batch, N_sel);
//     int64_t offset  = 0;
//
//     while (offset < N_sel) {
//         const int64_t b = std::min<int64_t>(B, N_sel - offset);
//         torch::Tensor in = patches_needed.narrow(/*dim=*/0, /*start=*/offset, /*length=*/b)
//                                           .to(dtype).to(device);
//
//         torch::Tensor logits = model.forward({in}).toTensor();
//
//         torch::Tensor probs;
//         if (logits.size(1) == 2) {
//             probs = torch::softmax(logits, 1).slice(1, 1, 2).contiguous();
//         } else {
//             probs = torch::sigmoid(logits).contiguous();
//         }
//
//         torch::Tensor probs_w = probs * w_patch;
//         for (int64_t i = 0; i < b; ++i) {
//             const int32_t z0 = selZ[offset + i];
//             const int32_t y0 = selY[offset + i];
//             const int32_t x0 = selX[offset + i];
//
//             auto dstP = prob_full.narrow(2, z0, patch_size)
//                                   .narrow(3, y0, patch_size)
//                                   .narrow(4, x0, patch_size); // [1,1,ps,ps,ps]
//             auto dstW = w_full.narrow(2, z0, patch_size)
//                                 .narrow(3, y0, patch_size)
//                                 .narrow(4, x0, patch_size); // [1,1,ps,ps,ps]
//
//             dstP.add_(probs_w[i]);
//             dstW.add_(w_patch);
//         }
//
//         offset += b;
//     }
//
//     prob_full = prob_full / (w_full + 1e-8);
//
//     auto grid_sizes = grid_tensor.sizes(); // [D,H,W,3]
//     const int64_t D = grid_sizes[0];
//     const int64_t H = grid_sizes[1];
//     const int64_t W = grid_sizes[2];
//
//     torch::Tensor prob_cropped =
//         prob_full.slice(2, 0, D).slice(3, 0, H).slice(4, 0, W)  // [1,1,D,H,W]
//                  .squeeze(0).squeeze(0)                         // [D,H,W]
//                  .contiguous();
//
//     return prob_cropped;
// }



torch::Tensor VortexDetection::run_prediction_on_block(const torch::Tensor& grid_tensor, const std::string& model_path,
                                                       const torch::jit::script::Module& model) {
    int patch_size = 64;
    int stride = 32;
    //torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    torch::Device device(torch::kCPU);

    auto [padded, pad_D, apad_H, pad_W] = pad_tensor(grid_tensor, patch_size);
    std::vector<torch::Tensor> patches = extract_patches(padded, patch_size, stride);

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
                    bool all_zero = torch::allclose(patch, torch::zeros_like(patch), 1e-20, 1e-20);
                    torch::Tensor prob;
                    if (all_zero) {
                        prob = torch::zeros({1, 1, patch_size, patch_size, patch_size}, patch.options());
                        patch_idx++;
                        continue;
                    } else {
                        torch::Tensor logits =
                                const_cast<torch::jit::script::Module&>(*model_const).forward({patch}).toTensor();
                        //torch::Tensor logits = model.forward({patch}).toTuple()->elements()[0].toTensor(); // [1, 2, 64, 64, 64]
                        if (logits.size(1) == 2) {
                            // prob = torch::sigmoid(logits.slice(1, 1, 2)); // 取前景通道 [1, 1, 64, 64, 64]
                            prob = torch::softmax(logits, 1);
                            prob = prob.slice(1, 1, 2);
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
    uint64_t x = v & 0x1fffff;            // 21 bits
    x = (x | (x << 32)) & 0x1f00000000ffffULL;
    x = (x | (x << 16)) & 0x1f0000ff0000ffULL;
    x = (x | (x << 8 )) & 0x100f00f00f00f00fULL;
    x = (x | (x << 4 )) & 0x10c30c30c30c30c3ULL;
    x = (x | (x << 2 )) & 0x1249249249249249ULL;
    return x;
}
static inline uint64_t morton3D(uint32_t x, uint32_t y, uint32_t z) {
    return (expandBits(x) << 0) | (expandBits(y) << 1) | (expandBits(z) << 2);
}

torch::Tensor VortexDetection::knn_smooth_labels(
    std::vector<float> data_val,
    const torch::Tensor& prob_vol_1,
    const Eigen::Vector3f& min_pos,
    const Eigen::Vector3f& global_step,
    const std::vector<Eigen::Vector3f>& query_points,
    int /*k*/)
{
    auto vol = prob_vol_1.contiguous()
               .unsqueeze(0).unsqueeze(0);

    const int64_t D = vol.size(2);  // nz
    const int64_t H = vol.size(3);  // ny
    const int64_t W = vol.size(4);  // nx
    const int64_t M = static_cast<int64_t>(query_points.size());
    torch::Tensor grid = torch::empty({1, M, 1, 1, 3}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
    auto* gptr = grid.data_ptr<float>();
    const float inv_sx = (global_step[0] != 0.f) ? (1.0f / global_step[0]) : 0.f;
    const float inv_sy = (global_step[1] != 0.f) ? (1.0f / global_step[1]) : 0.f;
    const float inv_sz = (global_step[2] != 0.f) ? (1.0f / global_step[2]) : 0.f;
    std::vector<float> tmp = data_val;
    size_t n = tmp.size();
    size_t idx = static_cast<size_t>(0.9f * (n - 1));
    std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
    const int64_t threshold = tmp[idx];
    const float Wx = (W > 1) ? (2.0f / float(W - 1)) : 0.f;
    const float Hy = (H > 1) ? (2.0f / float(H - 1)) : 0.f;
    const float Dz = (D > 1) ? (2.0f / float(D - 1)) : 0.f;
    auto worker = [&](int begin, int end) {
        for (int64_t i = begin; i < end; ++i) {
            const auto& qp = query_points[i];
            const float rx = (qp[0] - min_pos[0]) * inv_sx;
            const float ry = (qp[1] - min_pos[1]) * inv_sy;
            const float rz = (qp[2] - min_pos[2]) * inv_sz;
            float x_norm = Wx * rx - 1.0f;
            float y_norm = Hy * ry - 1.0f;
            float z_norm = Dz * rz - 1.0f;
            float* dst = gptr + i * 3;
            dst[0] = x_norm;
            dst[1] = y_norm;
            dst[2] = z_norm;
        }
    };
    ThreadPool::parallelFor(0, (int)M, worker,M);
    using namespace torch::nn::functional;
    auto opts = GridSampleFuncOptions()
    .mode(torch::kBilinear)
    .padding_mode(torch::kBorder)
    .align_corners(true);
    torch::Tensor sampled = grid_sample(vol, grid, opts);
    sampled = sampled.view({M});

    torch::Tensor dv = torch::from_blob((void*)data_val.data(), {M}, torch::TensorOptions().dtype(torch::kFloat32)).clone();

    auto cond =
        ((dv.ge(0.2f) & sampled.ge(0.01f)) |
         (sampled.ge(0.2f) & dv.ge(0.15f)) |
         dv.ge(threshold) |
         sampled.ge(0.3f));

    torch::Tensor out = sampled.to(torch::kFloat32);
    return out;
}



// torch::Tensor VortexDetection::knn_smooth_labels( 新kdtree
//     std::vector<float> data_val,
//     const torch::Tensor& prob_vol_1,   // [D,H,W], float32, CPU
//     const Eigen::Vector3f& min_pos,
//     const Eigen::Vector3f& global_step,
//     const std::vector<Eigen::Vector3f>& query_points,
//     int /*k*/)
// {
//     auto prob = prob_vol_1.contiguous();
//
//     const int64_t D = prob.size(0);
//     const int64_t H = prob.size(1);
//     const int64_t W = prob.size(2);
//     const int64_t N = D * H * W;
//     const int64_t M = static_cast<int64_t>(query_points.size());
//     const float* __restrict p = prob.data_ptr<float>();
//     std::vector<float> tmp = data_val;
//     float threshold = 0.f;
//     if (!tmp.empty()) {
//         size_t n = tmp.size();
//         size_t idx = static_cast<size_t>(0.9f * (n - 1));
//         std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
//         threshold = tmp[idx];
//     }
//     using Mat = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>;
//     Mat pts((Eigen::Index)N, 3);
//     const float sx = global_step[0], sy = global_step[1], sz = global_step[2];
//     int64_t i = 0;
//     for (int64_t z = 0; z < D; ++z) {
//         const float pz = min_pos[2] + z * sz;
//         for (int64_t y = 0; y < H; ++y) {
//             const float py = min_pos[1] + y * sy;
//             for (int64_t x = 0; x < W; ++x, ++i) {
//                 const float px = min_pos[0] + x * sx;
//                 pts(i, 0) = px; pts(i, 1) = py; pts(i, 2) = pz;
//             }
//         }
//     }
//     KDTree tree(pts);
//
//     const double sigma = 2.0 * std::max({ (double)std::abs(sx),
//                                           (double)std::abs(sy),
//                                           (double)std::abs(sz) });
//     const double inv_two_sigma2 = (sigma > 0.0) ? 1.0 / (2.0 * sigma * sigma) : 1e6;
//
//     torch::Tensor out   = torch::empty({M}, torch::kFloat32);
//     torch::Tensor condT = torch::empty({M}, torch::kBool);
//     float* __restrict out_ptr  = out.data_ptr<float>();
//     bool*  __restrict cond_ptr = condT.data_ptr<bool>();
//
//     constexpr int K = 5;
//
//     auto worker = [&](int begin, int end)
//     {
//         std::array<int,   K> idxs;
//         std::array<float, K> dist2;
//         for (int i = begin; i < end; ++i) {
//             const Eigen::Vector3f& q = query_points[i];
//             tree.template queryFixedK<K>(q, idxs, dist2, false);
//             double wsum = 0.0;
//             float  val  = 0.f;
//             for (int j = 0; j < K; ++j) {
//                 const int id = idxs[j];
//                 if (id < 0 || id >= N) continue;
//                 const float v = p[id];
//                 if (v == 0.0f) continue;
//                 const double w = std::exp(- (double)dist2[j] * inv_two_sigma2);
//                 val  += static_cast<float>(w) * p[id];
//                 wsum += w;
//             }
//             val = (wsum > 0.0) ? (val / (float)wsum) : 0.f;
//             out_ptr[i] = val;
//             const float dv_val = data_val[i];
//             const float sm_val = val;
//             bool cond =
//                 ((dv_val >= 0.2f && sm_val >= 0.1f) ||
//                  (sm_val >= 0.2f && dv_val >= 0.15f) ||
//                  (dv_val >= threshold) ||
//                  (sm_val >= 0.3f));
//             cond_ptr[i] = cond;
//         }
//     };
//
//     ThreadPool::parallelFor(0, (int)M, worker, M);
//     torch::Tensor cond = condT.to(torch::kBool);
//     torch::Tensor result = out.to(torch::kFloat32);
//     return result;
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

// std::tuple<torch::Tensor, Eigen::Vector3f>
// VortexDetection::process_blocks(const std::vector<Vector3f>& gridPoints, const std::vector<Vector3f>& gridVelocities,
//                                 const Vector3f& min_pos, const Vector3f& max_pos, const std::string& model_path,
//                                 int target_points, int split) {
//
//     torch::jit::script::Module model;
//
//     try {
//         model = torch::jit::load(model_path);
//         std::cout << "Model loaded successfully." << std::endl;
//     } catch (const c10::Error& e) { std::cerr << "Error loading the model. " << e.what() << std::endl; }
//     Eigen::Vector3f min_pos_eigen(min_pos[0], min_pos[1], min_pos[2]);
//     Eigen::Vector3f max_pos_eigen(max_pos[0], max_pos[1], max_pos[2]);
//
//     Eigen::Vector3f range_vec = max_pos_eigen - min_pos_eigen;
//     Eigen::Vector3f block_size = range_vec / split;
//     //Eigen::Vector3f range_vec = max_pos - min_pos;
//     //Eigen::Vector3f block_size = range_vec / split;
//     // 初始化 KD-Tree
//     Eigen::MatrixXd points(gridPoints.size(), 3);
//     for (size_t i = 0; i < gridPoints.size(); ++i) {
//         points(i, 0) = gridPoints[i][0];
//         points(i, 1) = gridPoints[i][1];
//         points(i, 2) = gridPoints[i][2];
//     }
//
//     KDTree tree(points);
//
//     Eigen::MatrixXd velocities(gridVelocities.size(), 3);
//     for (size_t i = 0; i < gridVelocities.size(); ++i) {
//         velocities(i, 0) = gridVelocities[i][0];
//         velocities(i, 1) = gridVelocities[i][1];
//         velocities(i, 2) = gridVelocities[i][2];
//     }
//
//     iGameStreamTracer tracer;
//     tracer.initStreamTracer(volume_Mesh);
//     std::string vectorName = name;
//     float terminalSpeed = 0.0f;
//
//     Vector3f mean = {-1.572247e-04, -4.576315e-04, -2.9615819e-10};
//     Vector3f std = {2.6299512e-02, 2.8212167e-02, 1.9456959e-08};
//
//     const int total_blocks = split * split * split;
//     std::vector<torch::Tensor> all_results_1(total_blocks);
//     const double eps = 1e-6;
//     static std::counting_semaphore<> infer_slots(10);
//     // int progress = 0;
//     std::mutex progress_mutex;
//     auto process_blocks_range = [&](int begin, int end) {
//         //std::cout << "pool_.size():" << pool.size() << std::endl;
//         for (int id = begin; id < end; ++id) {
//             int bz = id / (split * split);
//             int by = (id / split) % split;
//             int bx = id % split;
//             Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
//             sub_min = sub_min + min_pos_eigen;
//             Eigen::Vector3f sub_max = sub_min + block_size;
//             Eigen::Vector3f sub_range = sub_max - sub_min;
//             float max_len = sub_range.maxCoeff();
//             float uniform_step = max_len / (target_points - 1);
//             int nx = std::max(1, int(sub_range[0] / uniform_step) + 1);
//             int ny = std::max(1, int(sub_range[1] / uniform_step) + 1);
//             int nz = std::max(1, int(sub_range[2] / uniform_step) + 1);
//             Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
//             Eigen::Vector3f step = sub_range.cwiseQuotient(now);
//             torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
//
//             igIndex cachedVolumeId = -1;
//             const int K = 2;
//             std::vector<double> weights(K, 0.0);
//             std::vector<int> idxs(K, 0);
//             std::vector<double> dists(K, 0.0);
//             static std::atomic<int> test_counter{0};
//             for (int i = 0; i < nx; ++i) {
//                 for (int j = 0; j < ny; ++j) {
//                     for (int k = 0; k < nz; ++k) {
//                         Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
//                         Eigen::VectorXd pos_vec(3);
//                         pos_vec << pos[0], pos[1], pos[2];
//                         Vector3f pos_ (pos[0], pos[1], pos[2]);
//
//                         bool inside = false;
//                         Vector3f v = tracer.SampleVector(pos_, inside, cachedVolumeId, vectorName, terminalSpeed);
//                         if (inside) {
//                             torch::Tensor v_t = torch::tensor({v[0], v[1], v[2]}, torch::kFloat32);
//                             grid_tensor[k][j][i] = v_t;
//                         }
//                         else {
//                             // grid_tensor[k][j][i] = torch::zeros({3}, torch::kFloat32);
//                             tree.query(pos_vec, K, idxs, dists);
//                             if (dists.size() == 0  || dists[0]>step[0] * 4) {
//                                 grid_tensor[k][j][i] = torch::zeros({3}, torch::kFloat32);
//                             }else{
//                                 torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
//                                 double dist_sum = 0.0;
//                                 for (int l = 0; l < K; ++l) {
//                                     double dist = dists[l];
//                                     weights[l] = 1.0 / (dist * dist + eps);
//                                     dist_sum += weights[l];
//                                 }
//                                 for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
//                                 for (int l = 0; l < K; ++l) {
//                                     if (idxs[l] >= 0 && idxs[l] < velocities.rows()) {
//                                         weighted_sum +=
//                                                 weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
//                                                                             velocities(idxs[l], 2)},
//                                                                            torch::kFloat32);
//                                     }
//                                 }
//                                 grid_tensor[k][j][i] = weighted_sum;
//                             }
//                         }
//
//                         // tree.query(pos_vec, K, idxs, dists);
//                         // torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
//                         // if (K == 1) {
//                         //     weighted_sum = torch::tensor(
//                         //             {velocities(idxs[0], 0), velocities(idxs[0], 1), velocities(idxs[0], 2)},
//                         //             torch::kFloat32);
//                         // } else {
//                         //     double dist_sum = 0.0;
//                         //     for (int l = 0; l < K; ++l) {
//                         //         double dist = dists[l];
//                         //         weights[l] = 1.0 / (dist * dist + eps);
//                         //         dist_sum += weights[l];
//                         //     }
//                         //     for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
//                         //     for (int l = 0; l < K; ++l) {
//                         //         if (idxs[l] >= 0 && idxs[l] < velocities.rows()) {
//                         //             weighted_sum +=
//                         //                     weights[l] * torch::tensor({velocities(idxs[l], 0), velocities(idxs[l], 1),
//                         //                                                 velocities(idxs[l], 2)},
//                         //                                                torch::kFloat32);
//                         //         }
//                         //     }
//                         // }
//                         // grid_tensor[k][j][i] = weighted_sum;
//                     }
//                 }
//             }
//             torch::Tensor arr = grid_tensor.clone();
//             for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
//             arr = sigmoid(arr);
//             infer_slots.acquire();
//             torch::Tensor pred_block_1 = run_prediction_on_block(arr, model_path, model);
//             infer_slots.release();
//             all_results_1[id] = std::move(pred_block_1);
//         }
//     };
//
//     // ThreadPool::parallelFor(0, total_blocks, process_blocks_range);
//     ThreadPool::parallelFor(0, total_blocks, process_blocks_range, total_blocks * 2);
//
//     /*int max_threads = (int) std::thread::hardware_concurrency();
//     int workers = std::min(total_blocks, max_threads);
//     static std::counting_semaphore<> infer_slots(workers);*/
//
//     int nz_sub = all_results_1[0].size(0);
//     int ny_sub = all_results_1[0].size(1);
//     int nx_sub = all_results_1[0].size(2);
//
//     torch::Tensor result_volume_1 = torch::zeros({split * nz_sub, split * ny_sub, split * nx_sub}, torch::kFloat32);
//
//     int idx = 0;
//     for (int bz = 0; bz < split; ++bz) {
//         for (int by = 0; by < split; ++by) {
//             for (int bx = 0; bx < split; ++bx) {
//
//                 torch::Tensor cur = all_results_1[idx];
//                 auto sz = cur.sizes();
//                 if (sz[0] != nz_sub || sz[1] != ny_sub || sz[2] != nx_sub) {
//                     cur = torch::nn::functional::pad(cur, torch::nn::functional::PadFuncOptions({
//                                                                   0, nx_sub - sz[2], // W
//                                                                   0, ny_sub - sz[1], // H
//                                                                   0, nz_sub - sz[0]  // D
//                                                           }));
//                 }
//                 int z_start = bz * nz_sub;
//                 int y_start = by * ny_sub;
//                 int x_start = bx * nx_sub;
//                 result_volume_1.slice(0, z_start, z_start + nz_sub)
//                         .slice(1, y_start, y_start + ny_sub)
//                         .slice(2, x_start, x_start + nx_sub)
//                         .copy_(all_results_1[idx]);
//                 ++idx;
//             }
//         }
//     }
//
//     auto sizes = result_volume_1.sizes();
//     float depth = static_cast<float>(sizes[0]);
//     float height = static_cast<float>(sizes[1]);
//     float width = static_cast<float>(sizes[2]);
//
//     Eigen::Vector3f volume_size(width, height, depth);
//     Eigen::Vector3f global_step = range_vec.cwiseQuotient(volume_size);
//
//     torch::Tensor result_volume_11 = gaussian_filter3d(result_volume_1, 3, -1);
//     return std::make_tuple(result_volume_11, global_step);
// }

std::tuple<torch::Tensor, Eigen::Vector3f>
VortexDetection::process_blocks(const std::vector<Vector3f>& gridPoints, const std::vector<Vector3f>& gridVelocities,
                                const Vector3f& min_pos, const Vector3f& max_pos, const std::string& model_path,
                                int split,int nx,int ny,int nz) {

    torch::jit::script::Module model;
    try {
        model = torch::jit::load(model_path);
        std::cout << "Model loaded successfully." << std::endl;
    } catch (const c10::Error& e) { std::cerr << "Error loading the model. " << e.what() << std::endl; }
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

    KDTree tree(points);

    Eigen::MatrixXd velocities(gridVelocities.size(), 3);
    for (size_t i = 0; i < gridVelocities.size(); ++i) {
        velocities(i, 0) = gridVelocities[i][0];
        velocities(i, 1) = gridVelocities[i][1];
        velocities(i, 2) = gridVelocities[i][2];
    }
    const int total_blocks = split * split * split;
    std::vector<torch::Tensor> all_grid_tensors(total_blocks);
    std::vector<torch::Tensor> all_results_1(total_blocks);
    std::vector<std::vector<float>> all_velocities_thread_safe[3];
    for (int i = 0; i < 3; ++i) {
        all_velocities_thread_safe[i].resize(total_blocks);
    }
    const double eps = 1e-6;
    static std::counting_semaphore<> infer_slots(20);
    std::mutex progress_mutex;

    auto t0 = std::chrono::high_resolution_clock::now();
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

        constexpr int K = 8;
        std::array<int32_t, K> nn_idx;
        std::array<float,   K> nn_d2;
        std::array<float,   K> w;
        const float sigma = 2.0f * step.maxCoeff();
        const float inv_sigma2 = (sigma > 0.f) ? 1.0f / (sigma * sigma) : 1e6f;
        const float dist2_gate = (16.0f * step[0]) * (16.0f * step[0]);
        all_velocities_thread_safe[0][id].reserve(size_t(nx)*ny*nz);
        all_velocities_thread_safe[1][id].reserve(size_t(nx)*ny*nz);
        all_velocities_thread_safe[2][id].reserve(size_t(nx)*ny*nz);

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
                        p[0]=p[1]=p[2]=0.f;
                        continue;
                    }
                    float wsum = 0.f;
                    #pragma unroll
                    for (int l = 0; l < K; ++l) {
                        const float wl = std::exp(-0.5f * nn_d2[l] * inv_sigma2);
                        w[l] = wl; wsum += wl;
                    }
                    const float inv_wsum = (wsum > 0.f) ? (1.f / wsum) : 0.f;
                    float vx=0.f, vy=0.f, vz=0.f;
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
                    p[0]=vx; p[1]=vy; p[2]=vz;
                    all_velocities_thread_safe[0][id].push_back(vx);
                    all_velocities_thread_safe[1][id].push_back(vy);
                    all_velocities_thread_safe[2][id].push_back(vz);
                }
            }
        }
        all_grid_tensors[id] = grid_tensor.contiguous();
    }
};
    // auto process_blocks_range = [&](int begin, int end) {
    //     for (int id = begin; id < end; ++id) {
    //         int bz = id / (split * split);
    //         int by = (id / split) % split;
    //         int bx = id % split;
    //         Eigen::Vector3f sub_min = Eigen::Vector3f(bx, by, bz).array() * block_size.array();
    //         sub_min = sub_min + min_pos_eigen;
    //         Eigen::Vector3f sub_max = sub_min + block_size;
    //         Eigen::Vector3f sub_range = sub_max - sub_min;
    //         Eigen::Vector3f now = Eigen::Vector3f(nx - 1, ny - 1, nz - 1);
    //         Eigen::Vector3f step = sub_range.cwiseQuotient(now);
    //         torch::Tensor grid_tensor = torch::zeros({nz, ny, nx, 3}, torch::kFloat32);
    //         const int K = 8;
    //         std::vector<double> weights(K, 0.0);
    //         std::vector<int> idxs(K, 0);
    //         std::vector<float> dists(K, 0.0);
    //         static std::atomic<int> test_counter{0};
    //         for (int i = 0; i < nx; ++i) {
    //             for (int j = 0; j < ny; ++j) {
    //                 for (int k = 0; k < nz; ++k) {
    //                     Eigen::Vector3f pos = sub_min + Eigen::Vector3f(i * step[0], j * step[1], k * step[2]);
    //                     Eigen::VectorXd pos_vec(3);
    //                     pos_vec << pos[0], pos[1], pos[2];
    //                     tree.query(pos_vec, K, idxs, dists);
    //                     if (dists.size() == 0  || dists[0]>step[0] * 16) {
    //                         grid_tensor[k][j][i] = torch::zeros({3}, torch::kFloat32);
    //                     }else{
    //                         torch::Tensor weighted_sum = torch::zeros({3}, torch::kFloat32);
    //                         double dist_sum = 0.0;
    //                         for (int l = 0; l < K; ++l) {
    //                             double dist = dists[l];
    //                             weights[l] = 1.0 / (dist + eps);
    //                             dist_sum += weights[l];
    //                         }
    //                         for (int l = 0; l < K; ++l) weights[l] /= dist_sum;
    //                         float vx = 0.f, vy = 0.f, vz = 0.f;
    //                         for (int l = 0; l < (int)dists.size(); ++l) {
    //                             const int idx = idxs[l];
    //                             if (idx >= 0 && idx < velocities.rows()) {
    //                                 const float w = static_cast<float>(weights[l]);
    //                                 vx += w * static_cast<float>(velocities(idx, 0));
    //                                 vy += w * static_cast<float>(velocities(idx, 1));
    //                                 vz += w * static_cast<float>(velocities(idx, 2));
    //                             }
    //                         }
    //                         grid_tensor[k][j][i] = torch::tensor({vx, vy, vz}, torch::kFloat32);
    //                         all_velocities_thread_safe[0][id].push_back(vx);
    //                         all_velocities_thread_safe[1][id].push_back(vy);
    //                         all_velocities_thread_safe[2][id].push_back(vz);
    //                     }
    //                 }
    //             }
    //         }
    //         all_grid_tensors[id] = grid_tensor.contiguous();
    //     }
    // };
    ThreadPool::parallelFor(0, total_blocks, process_blocks_range, total_blocks);
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "[VortexDetection::Execute] process_blocks = " << elapsed << " s" << std::endl;

    std::vector<float> all_velocities[3];
    for (int c = 0; c < 3; ++c) {
        for (int id = 0; id < total_blocks; ++id) {
            all_velocities[c].insert(all_velocities[c].end(),
                                   all_velocities_thread_safe[c][id].begin(),
                                   all_velocities_thread_safe[c][id].end());
        }
    }
    auto compute_mean_std = [](const std::vector<float>& values) -> std::pair<float, float> {
        if (values.empty()) return {0.0f, 1.0f};
        double sum = 0.0;
        for (float v : values) sum += v;
        float mean = static_cast<float>(sum / values.size());
        double sum_sq = 0.0;
        for (float v : values) {
            double diff = v - mean;
            sum_sq += diff * diff;
        }
        float std = static_cast<float>(std::sqrt(sum_sq / values.size()));
        if (std < 1e-10) std = 1.0f;
        return {mean, std};
    };

    auto [mean_x, std_x] = compute_mean_std(all_velocities[0]);
    auto [mean_y, std_y] = compute_mean_std(all_velocities[1]);
    auto [mean_z, std_z] = compute_mean_std(all_velocities[2]);

    Vector3f mean(mean_x, mean_y, mean_z);
    Vector3f std(std_x, std_y, std_z);
    std::cout << "Computed mean: [" << mean[0] << ", " << mean[1] << ", " << mean[2] << "]" << std::endl;
    std::cout << "Computed std: [" << std[0] << ", " << std[1] << ", " << std[2] << "]" << std::endl;

    auto t3 = std::chrono::high_resolution_clock::now();

    auto processing = [&](int begin, int end) {
         for (int id = begin; id < end; ++id) {
             // const torch::Tensor& grid_tensor = all_grid_tensors[id];
             // auto mean_t = torch::tensor({mean_x, mean_y, mean_z}).to(torch::kFloat32).reshape({1,1,1,3});
             // auto std_t  = torch::tensor({std_x,  std_y,  std_z }).to(torch::kFloat32).reshape({1,1,1,3});
             //
             // torch::Tensor arr = grid_tensor.contiguous().clone();
             // arr.sub_(mean_t).div_(std_t);
             // arr = torch::sigmoid(arr);
             // infer_slots.acquire();
             // torch::Tensor pred_block_1 = run_prediction_on_block(arr, model);
             // infer_slots.release();
             //
             // all_results_1[id] = std::move(pred_block_1); // [D,H,W]

             const torch::Tensor& grid_tensor = all_grid_tensors[id];
             torch::Tensor arr = grid_tensor.clone();
             for (int c = 0; c < 3; ++c) { arr.select(3, c) = (arr.select(3, c) - mean[c]) / std[c]; }
             arr = sigmoid(arr);
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

    torch::Tensor result_volume_11 = gaussian_filter3d(result_volume_1, 3, -1);
    return std::make_tuple(result_volume_11, global_step);
}

torch::Tensor VortexDetection::gaussian_weights(const torch::Tensor& dists, float sigma) {
    return torch::exp(-0.5 * torch::pow(dists / sigma, 2));
}

#endif
IGAME_NAMESPACE_END