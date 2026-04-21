#include "iGameStreamlineSimplifier.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace iGame
{

static inline size_t TriIdx(size_t i, size_t j, size_t N) {
    // 要求 i < j
    return i * N - (i * (i + 1)) / 2 + (j - i - 1);
}

// --------------------------------------------------------------------------
bool StreamlineSimplifier::Execute() {
    m_Streamlines.clear();
    m_SelectedIds.clear();
    m_D.clear();
    m_Output = nullptr;

    if (!ExtractStreamlines()) {
        std::cout << "[Simplifier] ExtractStreamlines failed\n";
        return false;
    }
    ComputeHistograms();
    BuildDistanceMatrix();
    ClusterAverage();
    SampleRepresentatives();
    BuildOutputMesh();
    return m_Output != nullptr;
}

// --------------------------------------------------------------------------
bool StreamlineSimplifier::ExtractStreamlines() {
    if (!m_Input) return false;

    auto points = m_Input->GetPoints();
    auto cells = m_Input->GetCells();
    auto types = m_Input->GetCellTypes();
    auto attr = m_Input->GetAttributeSet();
    if (!points || !cells || !attr) return false;

    auto& velAttr = attr->GetAttribute("Velocity");
    auto& idAttr = attr->GetAttribute("StreamlineId");
    if (velAttr.IsNone() || idAttr.IsNone()) {
        std::cout << "[Simplifier] Missing Velocity or StreamlineId attribute\n";
        return false;
    }
    auto velArr = velAttr.pointer; // IG_POINT,每点一个 3D 向量
    auto idArr = idAttr.pointer;   // IG_CELL,每 cell 一个 int

    const IGsize numCells = cells->GetNumberOfCells();
    if (numCells == 0) return false;

    // 1) 找最大 streamline id
    int maxId = -1;
    for (IGsize c = 0; c < numCells; ++c) {
        int id = static_cast<int>(idArr->GetValue(c));
        if (id > maxId) maxId = id;
    }
    if (maxId < 0) return false;
    m_Streamlines.assign(maxId + 1, {});

    // 2) 把每个 LINE 的两个端点 push 到对应流线
    for (IGsize c = 0; c < numCells; ++c) {
        if (types->GetValue(c) != IG_LINE) continue;

        const igIndex* ids = nullptr;
        int n = cells->GetCellIds(c, ids);
        if (n != 2 || !ids) continue;

        int sid = static_cast<int>(idArr->GetValue(c));
        if (sid < 0 || sid > maxId) continue;
        auto& sl = m_Streamlines[sid];

        for (int k = 0; k < 2; ++k) {
            auto p = points->GetPoint(ids[k]); // 假设可 [0..2] 访问
            sl.points.emplace_back(p[0], p[1], p[2]);

            float v[3] = {0, 0, 0};
            velArr->GetElement(ids[k], v); // 假设:拷贝到 float[3]
            sl.velocities.emplace_back(v[0], v[1], v[2]);
            // ↑↑↑
        }

    }

    // 先去掉点数太少的
    m_Streamlines.erase(std::remove_if(m_Streamlines.begin(), m_Streamlines.end(),
                                       [](const StreamlineData& s) { return s.points.size() < 3; }),
                        m_Streamlines.end());

    // 算每条流线弧长
    auto arcLength = [](const StreamlineData& s) -> float {
        float L = 0.f;
        for (size_t i = 1; i < s.points.size(); ++i) L += (s.points[i] - s.points[i - 1]).norm();
        return L;
    };

    std::vector<float> lens(m_Streamlines.size());
    for (size_t i = 0; i < m_Streamlines.size(); ++i) lens[i] = arcLength(m_Streamlines[i]);

    // 用中位数的 30% 作为自动阈值,短于此的直接丢弃
    std::vector<float> sorted = lens;
    std::sort(sorted.begin(), sorted.end());
    float median = sorted[sorted.size() / 2];
    float minLen = median * 0.1f;

    std::cout << "[Simplifier] Length: min=" << sorted.front() << " median=" << median << " max=" << sorted.back()
              << " cutoff=" << minLen << "\n";

    // 过滤
    std::vector<StreamlineData> filtered;
    for (size_t i = 0; i < m_Streamlines.size(); ++i) {
        if (lens[i] >= minLen) filtered.push_back(std::move(m_Streamlines[i]));
    }
    m_Streamlines = std::move(filtered);

    std::cout << "[Simplifier] After length filter: " << m_Streamlines.size() << " streamlines\n";

    return !m_Streamlines.empty();
}

// --------------------------------------------------------------------------
void StreamlineSimplifier::ComputeHistograms() {
    const int B = m_CurvBins; 
    const float PI = 3.14159265358979f;
    const float eps = 1e-12f;

    for (size_t s = 0; s < m_Streamlines.size(); ++s) {
        auto& sl = m_Streamlines[s];
        const auto& pts = sl.points;
        const int n = (int) pts.size();

        sl.histCurv.assign(B, 0.f);
        sl.cdfCurv.assign(B, 0.f);

        if (n < 3) continue;

        // 前向单位向量
        std::vector<Vector3f> a_unit(n - 1);
        for (int i = 0; i < n - 1; ++i) {
            Vector3f e = pts[i + 1] - pts[i];
            float len = e.norm();
            a_unit[i] = (len > eps) ? e / len : Vector3f(0, 0, 0);
        }

        // 离散曲率 = arccos(a_unit[i] · a_unit[i-1])，范围 [0, π)
        std::vector<float> curvatures;
        curvatures.reserve(a_unit.size() - 1);
        for (int i = 1; i < (int) a_unit.size(); ++i) {
            float d =
                    a_unit[i][0] * a_unit[i - 1][0] + a_unit[i][1] * a_unit[i - 1][1] + a_unit[i][2] * a_unit[i - 1][2];
            d = std::max(-1.f, std::min(1.f, d));
            curvatures.push_back(std::acos(d));
        }

        // 分 bin: [0, π) → B 个 bin
        for (float k: curvatures) {
            int b = (int) (k / PI * B);
            if (b >= B) b = B - 1;
            if (b < 0) b = 0;
            sl.histCurv[b] += 1.f;
        }

        // 归一化
        float total = 0.f;
        for (float v: sl.histCurv) total += v;
        if (total > eps)
            for (float& v: sl.histCurv) v /= total;

        // CDF
        float acc = 0.f;
        for (int i = 0; i < B; ++i) {
            acc += sl.histCurv[i];
            sl.cdfCurv[i] = acc;
        }
    }
}

// --------------------------------------------------------------------------
void StreamlineSimplifier::BuildDistanceMatrix() {
    const size_t N = m_Streamlines.size();
    if (N < 2) {
        m_D.clear();
        return;
    }

    m_D.assign(N * (N - 1) / 2, 0.f);
    const int B = m_CurvBins;

    for (size_t i = 0; i < N; ++i) {
        const auto& ci = m_Streamlines[i].cdfCurv;
        for (size_t j = i + 1; j < N; ++j) {
            const auto& cj = m_Streamlines[j].cdfCurv;
            float d = 0.f;
            for (int b = 0; b < B; ++b) d += std::fabs(ci[b] - cj[b]);
            m_D[TriIdx(i, j, N)] = d;
        }
    }
}

// --------------------------------------------------------------------------
//void StreamlineSimplifier::ClusterAverage() {
//    const size_t N = m_Streamlines.size();
//    if (N == 0) return;
//
//    if ((int) N <= m_NumClusters) {
//        for (size_t i = 0; i < N; ++i) m_Streamlines[i].clusterId = (int) i;
//        return;
//    }
//
//    // dense 距离矩阵(方便更新)
//    std::vector<std::vector<float>> M(N, std::vector<float>(N, 0.f));
//    for (size_t i = 0; i < N; ++i)
//        for (size_t j = i + 1; j < N; ++j) M[i][j] = M[j][i] = m_D[TriIdx(i, j, N)];
//
//    std::vector<int> size(N, 1);
//    std::vector<bool> alive(N, true);
//    std::vector<int> parent(N);
//    for (size_t i = 0; i < N; ++i) parent[i] = (int) i;
//
//    int numActive = (int) N;
//    while (numActive > m_NumClusters) {
//        float best = std::numeric_limits<float>::max();
//        int bi = -1, bj = -1;
//        for (size_t i = 0; i < N; ++i) {
//            if (!alive[i]) continue;
//            for (size_t j = i + 1; j < N; ++j) {
//                if (!alive[j]) continue;
//                if (M[i][j] < best) {
//                    best = M[i][j];
//                    bi = (int) i;
//                    bj = (int) j;
//                }
//            }
//        }
//        if (bi < 0) break;
//
//        int sA = size[bi], sB = size[bj];
//        for (size_t k = 0; k < N; ++k) {
//            if (!alive[k] || (int) k == bi || (int) k == bj) continue;
//            float nd = (sA * M[bi][k] + sB * M[bj][k]) / float(sA + sB);
//            M[bi][k] = M[k][bi] = nd;
//        }
//        size[bi] = sA + sB;
//        alive[bj] = false;
//        for (size_t k = 0; k < N; ++k)
//            if (parent[k] == bj) parent[k] = bi;
//        numActive--;
//    }
//
//    // 重新编号为 0..K-1
//    std::unordered_map<int, int> remap;
//    int next = 0;
//    for (size_t i = 0; i < N; ++i) {
//        int r = parent[i];
//        auto it = remap.find(r);
//        if (it == remap.end()) { remap[r] = next++; }
//        m_Streamlines[i].clusterId = remap[r];
//    }
//}
void StreamlineSimplifier::ClusterAverage() {
    const size_t N = m_Streamlines.size();
    if (N == 0) return;
    if ((int) N <= m_NumClusters) {
        for (size_t i = 0; i < N; ++i) m_Streamlines[i].clusterId = (int) i;
        return;
    }

    std::vector<std::vector<float>> M(N, std::vector<float>(N, 0.f));
    for (size_t i = 0; i < N; ++i)
        for (size_t j = i + 1; j < N; ++j) M[i][j] = M[j][i] = m_D[TriIdx(i, j, N)];

    std::vector<int> sz(N, 1);
    std::vector<bool> alive(N, true);
    std::vector<int> parent(N);
    for (size_t i = 0; i < N; ++i) parent[i] = (int) i;

    int numActive = (int) N;
    while (numActive > m_NumClusters) {
        float best = std::numeric_limits<float>::max();
        int bi = -1, bj = -1;
        for (size_t i = 0; i < N; ++i) {
            if (!alive[i]) continue;
            for (size_t j = i + 1; j < N; ++j) {
                if (!alive[j]) continue;
                if (M[i][j] < best) {
                    best = M[i][j];
                    bi = (int) i;
                    bj = (int) j;
                }
            }
        }
        if (bi < 0) break;

        int sA = sz[bi], sB = sz[bj];
        for (size_t k = 0; k < N; ++k) {
            if (!alive[k] || (int) k == bi || (int) k == bj) continue;
            float nd = (sA * M[bi][k] + sB * M[bj][k]) / float(sA + sB);
            M[bi][k] = M[k][bi] = nd;
        }
        sz[bi] = sA + sB;
        alive[bj] = false;
        for (size_t k = 0; k < N; ++k)
            if (parent[k] == bj) parent[k] = bi;
        numActive--;
    }

    std::unordered_map<int, int> remap;
    int next = 0;
    for (size_t i = 0; i < N; ++i) {
        int r = parent[i];
        if (remap.find(r) == remap.end()) remap[r] = next++;
        m_Streamlines[i].clusterId = remap[r];
    }

    std::cout << "[Simplifier] Clusters: " << next << "\n";
}
// --------------------------------------------------------------------------
void StreamlineSimplifier::SampleRepresentatives() {
    m_SelectedIds.clear();

    // 按簇分桶
    int K = 0;
    for (auto& sl: m_Streamlines) K = std::max(K, sl.clusterId + 1);
    if (K == 0) return;

    std::vector<std::vector<int>> buckets(K);
    for (size_t i = 0; i < m_Streamlines.size(); ++i)
        if (m_Streamlines[i].clusterId >= 0) buckets[m_Streamlines[i].clusterId].push_back((int) i);

    int baseQuota = m_TotalTarget / K;
    int remainder = m_TotalTarget % K;

    // 第一轮：小簇全选，记录多余配额
    std::vector<int> quota(K, 0);
    int deficit = 0;
    std::vector<bool> isLarge(K, false);

    for (int k = 0; k < K; ++k) {
        int q = baseQuota + (k < remainder ? 1 : 0);
        if ((int) buckets[k].size() <= q) {
            quota[k] = (int) buckets[k].size(); // 全选
            deficit += q - (int) buckets[k].size();
        } else {
            quota[k] = q;
            isLarge[k] = true;
        }
    }

    // 第二轮：把 deficit 分配给大簇
    std::vector<int> largeClusters;
    for (int k = 0; k < K; ++k)
        if (isLarge[k]) largeClusters.push_back(k);

    if (deficit > 0 && !largeClusters.empty()) {
        int extraPer = deficit / (int) largeClusters.size();
        int extraRem = deficit % (int) largeClusters.size();
        for (int i = 0; i < (int) largeClusters.size(); ++i) {
            int k = largeClusters[i];
            quota[k] += extraPer + (i < extraRem ? 1 : 0);
            quota[k] = std::min(quota[k], (int) buckets[k].size());
        }
    }

    // 每簇等间隔采样
    for (int k = 0; k < K; ++k) {
        const auto& members = buckets[k];
        int q = quota[k];
        if (q <= 0 || members.empty()) continue;

        if (q >= (int) members.size()) {
            // 全选
            for (int id: members) m_SelectedIds.push_back(id);
        } else {
            // linspace 等间隔取
            for (int i = 0; i < q; ++i) {
                int idx = (int) std::round((double) i * (members.size() - 1) / (q - 1));
                m_SelectedIds.push_back(members[idx]);
            }
        }

        std::cout << "  C" << k << ": " << members.size() << " -> sampled " << std::min(q, (int) members.size())
                  << "\n";
    }

    std::cout << "[Simplifier] Total sampled: " << m_SelectedIds.size() << " / " << m_Streamlines.size() << "\n";
}

// --------------------------------------------------------------------------
void StreamlineSimplifier::BuildOutputMesh() {
    auto mesh = UnstructuredMesh::New();
    auto points = Points::New();
    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    auto attrSet = AttributeSet::New();

    auto velArr = FloatArray::New();
    velArr->SetDimension(3);
    velArr->SetName("Velocity");

    auto idArr = IntArray::New();
    idArr->SetDimension(1);
    idArr->SetName("StreamlineId");

    mesh->SetShellRenderingOption(false);

    igIndex g = 0;
    int newSid = 0;
    for (int id: m_SelectedIds) {
        const auto& sl = m_Streamlines[id];
        if (sl.points.size() < 2) continue;

        igIndex start = g;
        for (size_t i = 0; i < sl.points.size(); ++i) {
            points->AddPoint(Point(sl.points[i][0], sl.points[i][1], sl.points[i][2]));
            velArr->AddElement3(sl.velocities[i][0], sl.velocities[i][1], sl.velocities[i][2]);
            g++;
        }
        for (size_t i = 0; i + 1 < sl.points.size(); ++i) {
            int tem[2] = {(int) (start + i), (int) (start + i + 1)};
            cells->AddCellIds(tem, 2);
            types->AddValue(IG_LINE);
            idArr->AddValue(newSid);
        }
        newSid++;
    }

    mesh->SetPoints(points);
    mesh->SetCells(cells, types);
    attrSet->AddAttribute(IG_VECTOR, IG_POINT, velArr);
    attrSet->AddAttribute(IG_SCALAR, IG_CELL, idArr);
    mesh->SetAttributeSet(attrSet);
    mesh->SetName("SimplifiedStreamlines");

    m_Output = mesh;
}

} // namespace iGame