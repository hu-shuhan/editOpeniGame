#ifndef iGamePointKdTree_h
#define iGamePointKdTree_h

#include "iGameMacro.h"
#include "iGamePoints.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <nanoflann.hpp>

IGAME_NAMESPACE_BEGIN

// 对 nanoflann 的薄封装：统一提供 最近点 / k 近邻 / 半径内 查询。
// iGame 自带的 PointFinder 只支持最近点，这里补齐 kNN 与半径查询；
// 距离一律返回"平方距离"。
class PointKdTree {
public:
    static constexpr int Dim = 3;
    using Scalar = float;
    using Index = std::uint32_t;

    struct PointCloud {
        std::vector<std::array<Scalar, Dim>> pts;

        inline std::size_t kdtree_get_point_count() const { return pts.size(); }
        inline Scalar kdtree_get_pt(const std::size_t idx, const std::size_t dim) const {
            return pts[idx][dim];
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX&) const {
            return false;
        }
    };

    using Adaptor = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<Scalar, PointCloud>, PointCloud, Dim, Index>;

    void Build(const Points* points) {
        cloud_.pts.clear();
        index_.reset();
        if (points == nullptr) return;
        const IGsize n = points->GetNumberOfPoints();
        cloud_.pts.resize(static_cast<std::size_t>(n));
        for (IGsize i = 0; i < n; ++i) {
            const Point& p = points->GetPoint(i);
            cloud_.pts[static_cast<std::size_t>(i)] = { p[0], p[1], p[2] };
        }
        if (cloud_.pts.empty()) return;
        index_ = std::make_unique<Adaptor>(
                Dim, cloud_, nanoflann::KDTreeSingleIndexAdaptorParams(16));
        index_->buildIndex();
    }

    bool Empty() const { return index_ == nullptr; }
    std::size_t Size() const { return cloud_.pts.size(); }

    // 最近 k 个点（distSq 为平方距离，按距离升序）。
    void QueryKNearest(const Point& q, int k,
                       std::vector<igIndex>& ids,
                       std::vector<double>& distSq) const {
        ids.clear();
        distSq.clear();
        if (!index_ || k <= 0) return;
        const int kk = std::min<int>(k, static_cast<int>(cloud_.pts.size()));
        std::vector<Index> idx(static_cast<std::size_t>(kk));
        std::vector<Scalar> d2(static_cast<std::size_t>(kk));
        const Scalar qp[Dim] = { q[0], q[1], q[2] };
        nanoflann::KNNResultSet<Scalar, Index> resultSet(static_cast<std::size_t>(kk));
        resultSet.init(idx.data(), d2.data());
        nanoflann::SearchParameters params;
        params.sorted = true;
        index_->findNeighbors(resultSet, qp, params);
        const std::size_t n = resultSet.size();
        ids.resize(n);
        distSq.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            ids[i] = static_cast<igIndex>(idx[i]);
            distSq[i] = static_cast<double>(d2[i]);
        }
    }

    // 半径 r 内的所有点（distSq 为平方距离）。
    void QueryRadius(const Point& q, double r,
                     std::vector<igIndex>& ids,
                     std::vector<double>& distSq) const {
        ids.clear();
        distSq.clear();
        if (!index_ || r <= 0.0) return;
        const Scalar qp[Dim] = { q[0], q[1], q[2] };
        std::vector<nanoflann::ResultItem<Index, Scalar>> matches;
        nanoflann::RadiusResultSet<Scalar, Index> resultSet(static_cast<Scalar>(r * r), matches);
        nanoflann::SearchParameters params;
        params.sorted = true;
        index_->findNeighbors(resultSet, qp, params);
        ids.reserve(matches.size());
        distSq.reserve(matches.size());
        for (const auto& m : matches) {
            ids.push_back(static_cast<igIndex>(m.first));
            distSq.push_back(static_cast<double>(m.second));
        }
    }

private:
    PointCloud cloud_;
    std::unique_ptr<Adaptor> index_;
};

IGAME_NAMESPACE_END

#endif
