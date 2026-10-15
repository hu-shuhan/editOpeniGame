#include "iGameVolumeMeshSimplification.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include "iGameFlatArray.h"
#include "iGameAttributeSet.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

// ─── tiny helpers ───────────────────────────────────────────────
static inline void Sort3i(int& a, int& b, int& c) {
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
}

namespace
{
template<typename ArrayType, typename AttrInfoType>
typename ArrayType::Pointer BuildPointAttribute(AttrInfoType info, AttributeSet::Pointer& outAttrs, int col,
                         int N, int newN, std::vector<uint8_t>& m_VertAlive, std::vector<int> old2new, std::vector<double> m_Attrs,int D) {
    typename ArrayType::Pointer arr = ArrayType::New();
    arr->SetDimension(info.ncomp);
    arr->Resize(newN);
    arr->SetName(info.name);
    for (int i = 0; i < N; ++i) {
        if (!m_VertAlive[i]) continue;

        int ni = old2new[i];

        for (int d = 0; d < info.ncomp; ++d) { arr->SetValue(ni * info.ncomp + d, m_Attrs[i * D + col + d]); }
    }
    return arr;
}

template<typename ArrayType>
typename ArrayType::Pointer BuildCellAttribute(const AttributeSet::Attribute& inAttr,
                                               const std::vector<igIndex>& outCellSourceIds) {
    typename ArrayType::Pointer arr = ArrayType::New();

    const int dim = inAttr.pointer->GetDimension();

    arr->SetName(inAttr.pointer->GetName());
    arr->SetDimension(dim);
    arr->Resize(outCellSourceIds.size());

    std::vector<double> values(dim);

    for (igIndex newCellId = 0; newCellId < outCellSourceIds.size(); ++newCellId) {
        const igIndex oldCellId = outCellSourceIds[newCellId];

        inAttr.pointer->GetElement(oldCellId, values.data());

        for (int d = 0; d < dim; ++d) { arr->SetValue(newCellId * dim + d, values[d]); }
    }

    return arr;
}

}

// ════════════════════════════════════════════════════════════════
//  Execute
// ════════════════════════════════════════════════════════════════
bool TetraSimplification::Execute() {
    if (!LoadMesh()) return false;
    Normalize();
    BuildTopology();
    BuildADQ();

    clock_t t0 = clock();
    Simplify();
    double dt = double(clock() - t0) / CLOCKS_PER_SEC;
    std::cout << "[TetraSimplification] Simplification took " << dt << " s\n";

    return SaveMesh();
}

// ════════════════════════════════════════════════════════════════
//  LoadMesh
// ════════════════════════════════════════════════════════════════
bool TetraSimplification::LoadMesh() {
    auto obj = GetInput(0);
    if (!obj) return false;

    if (obj->GetDataObjectType() == IG_VOLUME_MESH) {
        m_InputMesh = DynamicCast<VolumeMesh>(obj);
    } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        auto um = DynamicCast<UnstructuredMesh>(obj);
        if (um) m_InputMesh = um->TransferToVolumeMesh();
    }
    if (!m_InputMesh) return false;

    auto points = m_InputMesh->GetPoints();
    if (!points) return false;

    m_NumVerts = static_cast<int>(points->GetNumberOfPoints());
    m_Pts.resize(m_NumVerts * 3);
    for (int i = 0; i < m_NumVerts; ++i) {
        const Point& p = points->GetPoint(i);
        m_Pts[i * 3 + 0] = p[0];
        m_Pts[i * 3 + 1] = p[1];
        m_Pts[i * 3 + 2] = p[2];
    }

    // Read tets
    const IGsize nVol = m_InputMesh->GetNumberOfVolumes();
    m_TetVerts.clear();
    m_TetVerts.reserve(nVol * 4);
    m_TetSourceIds.clear();
    m_TetSourceIds.reserve(nVol);
    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    m_NumTets = 0;
    for (IGsize ci = 0; ci < nVol; ++ci) {
        int n = m_InputMesh->GetVolumePointIds(ci, ids);
        if (n != 4) continue;
        m_TetVerts.push_back(static_cast<int>(ids[0]));
        m_TetVerts.push_back(static_cast<int>(ids[1]));
        m_TetVerts.push_back(static_cast<int>(ids[2]));
        m_TetVerts.push_back(static_cast<int>(ids[3]));
        m_TetSourceIds.push_back(ci);
        m_NumTets++;
    }

    // Read point attributes
    auto attrs = m_InputMesh->GetAttributeSet();
    m_AttrInfo.clear();
    int totalDim = 0;

    if (attrs) {
        auto all = attrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto a = all->GetElement(ai);
            if (a.isDeleted || !a.pointer || a.attachmentType != IG_POINT) continue;
            if (!m_UseAllPointAttributes) {
                int curIdx = m_InputMesh->GetCurrentAttributeIndex();
                if (static_cast<int>(ai) != curIdx) continue;
            }
            int dim = a.pointer->GetDimension();
            auto arrayType = a.pointer->GetArrayType();
            auto attributeType = a.type;
            auto attachmentType = a.attachmentType;
            m_AttrInfo.push_back({a.pointer->GetName(), dim, arrayType,attributeType,attachmentType});
            totalDim += dim;
        }
    }

    m_AttrDim = totalDim;
    m_Attrs.assign(m_NumVerts * totalDim, 0.0);

    if (totalDim > 0 && attrs) {
        int col = 0;
        auto all = attrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto a = all->GetElement(ai);
            if (a.isDeleted || !a.pointer) continue;
            if (a.attachmentType != IG_POINT) continue;
            if (!m_UseAllPointAttributes) {
                int curIdx = m_InputMesh->GetCurrentAttributeIndex();
                if (static_cast<int>(ai) != curIdx) continue;
            }
            int dim = a.pointer->GetDimension();
            double vals[IGAME_CELL_MAX_SIZE]{};
            for (int i = 0; i < m_NumVerts; ++i) {
                a.pointer->GetElement(i, vals);
                for (int d = 0; d < dim; ++d) {
                    m_Attrs[i * totalDim + col + d] = vals[d];
                }
            }
            col += dim;
        }
    }

    std::cout << "[TetraSimplification] Loaded " << m_NumVerts << " verts, "
              << m_NumTets << " tets, " << m_AttrDim << " attr dims\n";
    return true;
}

// ════════════════════════════════════════════════════════════════
//  Normalize
// ════════════════════════════════════════════════════════════════
void TetraSimplification::Normalize() {
    const int N = m_NumVerts;
    const int D = m_AttrDim;

    // Position bounding box
    double pmin[3] = {1e30, 1e30, 1e30};
    double pmax[3] = {-1e30, -1e30, -1e30};
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < 3; ++j) {
            double v = m_Pts[i * 3 + j];
            if (v < pmin[j]) pmin[j] = v;
            if (v > pmax[j]) pmax[j] = v;
        }
    }
    m_PtsMin[0] = pmin[0]; m_PtsMin[1] = pmin[1]; m_PtsMin[2] = pmin[2];
    m_PtsScale = std::max({pmax[0]-pmin[0], pmax[1]-pmin[1], pmax[2]-pmin[2], 1e-12});
    double invScale = 1.0 / m_PtsScale;
    for (int i = 0; i < N * 3; ++i) {
        m_Pts[i] = (m_Pts[i] - pmin[i % 3]) * invScale;
    }

    // Normalize attributes
    m_AttrNormParams.clear();
    int col = 0;
    for (const auto& info : m_AttrInfo) {
        if (info.ncomp == 1) {
            double mn = 1e30, mx = -1e30;
            for (int i = 0; i < N; ++i) {
                double v = m_Attrs[i * D + col];
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            double rng = mx - mn;
            if (rng > 1e-12) {
                double invRng = 1.0 / rng;
                for (int i = 0; i < N; ++i) {
                    m_Attrs[i * D + col] = (m_Attrs[i * D + col] - mn) * invRng;
                }
            }
            m_AttrNormParams.push_back({true, mn, rng > 1e-12 ? rng : 1.0, 1.0, col, 1});
        } else {
            double maxMag = 0.0;
            for (int i = 0; i < N; ++i) {
                double mag2 = 0.0;
                for (int d = 0; d < info.ncomp; ++d) {
                    double v = m_Attrs[i * D + col + d];
                    mag2 += v * v;
                }
                double mag = std::sqrt(mag2);
                if (mag > maxMag) maxMag = mag;
            }
            if (maxMag > 1e-12) {
                double inv = 1.0 / maxMag;
                for (int i = 0; i < N; ++i) {
                    for (int d = 0; d < info.ncomp; ++d) {
                        m_Attrs[i * D + col + d] *= inv;
                    }
                }
            }
            m_AttrNormParams.push_back({false, 0, 1, maxMag > 1e-12 ? maxMag : 1.0, col, info.ncomp});
        }
        col += info.ncomp;
    }
}

// ════════════════════════════════════════════════════════════════
//  BuildTopology
// ════════════════════════════════════════════════════════════════

struct FKTet {
    int a, b, c;
    bool operator==(const FKTet& o) const { return a == o.a && b == o.b && c == o.c; }
};
struct FKTetHash {
    size_t operator()(const FKTet& k) const noexcept {
        uint64_t h = uint64_t(uint32_t(k.a)) * 0x9E3779B185EBCA87ull;
        h ^= uint64_t(uint32_t(k.b)) + 0x9E3779B185EBCA87ull + (h << 6) + (h >> 2);
        h ^= uint64_t(uint32_t(k.c)) + 0x9E3779B185EBCA87ull + (h << 6) + (h >> 2);
        return size_t(h);
    }
};

void TetraSimplification::BuildTopology() {
    const int N = m_NumVerts;
    const int M = m_NumTets;
    const int* tv = m_TetVerts.data();

    // Build vertex→tet adjacency using vectors (much lighter than unordered_set)
    m_VertTets.assign(N, std::vector<int>());
    // Pre-count for reserve
    std::vector<int> degree(N, 0);
    for (int i = 0; i < M * 4; ++i) degree[tv[i]]++;
    for (int v = 0; v < N; ++v) m_VertTets[v].reserve(degree[v]);

    for (int ti = 0; ti < M; ++ti) {
        int base = ti * 4;
        m_VertTets[tv[base + 0]].push_back(ti);
        m_VertTets[tv[base + 1]].push_back(ti);
        m_VertTets[tv[base + 2]].push_back(ti);
        m_VertTets[tv[base + 3]].push_back(ti);
    }

    m_TetAlive.assign(M, 1);
    m_VertAlive.assign(N, 1);
    m_TetVersion.assign(M, 0);

    // Boundary detection
    std::unordered_map<FKTet, int, FKTetHash> faceCounts;
    faceCounts.reserve(M * 4);
    static const int fIdx[4][3] = {{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    for (int ti = 0; ti < M; ++ti) {
        int base = ti * 4;
        for (int fi = 0; fi < 4; ++fi) {
            int a = tv[base + fIdx[fi][0]];
            int b = tv[base + fIdx[fi][1]];
            int c = tv[base + fIdx[fi][2]];
            Sort3i(a, b, c);
            faceCounts[{a, b, c}]++;
        }
    }

    m_IsBoundary.assign(N, 0);
    for (const auto& kv : faceCounts) {
        if (kv.second == 1) {
            m_IsBoundary[kv.first.a] = 1;
            m_IsBoundary[kv.first.b] = 1;
            m_IsBoundary[kv.first.c] = 1;
        }
    }

    m_IsBoundaryTet.assign(M, 0);
    for (int ti = 0; ti < M; ++ti) {
        int base = ti * 4;
        if (m_IsBoundary[tv[base]] | m_IsBoundary[tv[base+1]] |
            m_IsBoundary[tv[base+2]] | m_IsBoundary[tv[base+3]]) {
            m_IsBoundaryTet[ti] = 1;
        }
    }

    // Init visited buffer
    m_Visited.assign(M, 0);
    m_VisitGen = 0;

    int nBdy = 0;
    for (int i = 0; i < N; ++i) nBdy += m_IsBoundary[i];
    int nBdyTet = 0;
    for (int i = 0; i < M; ++i) nBdyTet += m_IsBoundaryTet[i];
    std::cout << "[TetraSimplification] " << nBdy << " bdy verts, " << nBdyTet << " bdy tets\n";
}

// ════════════════════════════════════════════════════════════════
//  BuildADQ — all inline, no Eigen in hot loop
// ════════════════════════════════════════════════════════════════
void TetraSimplification::BuildADQ() {
    const int N = m_NumVerts;
    const int D = m_AttrDim;
    const int M = m_NumTets;

    m_ADQ.assign(N * 9, 0.0);
    m_ErrAccum.assign(N, 0.0);

    if (D == 0) return;

    const double* pts = m_Pts.data();
    const double* attr = m_Attrs.data();
    const int* tv = m_TetVerts.data();

    for (int ti = 0; ti < M; ++ti) {
        int base = ti * 4;
        int v0 = tv[base], v1 = tv[base+1], v2 = tv[base+2], v3 = tv[base+3];

        // dp[j] = p[v_{j+1}] - p[v0],  j=0,1,2  → 3×3 row-major
        double dp[9];
        dp[0] = pts[v1*3]   - pts[v0*3];   dp[1] = pts[v1*3+1] - pts[v0*3+1]; dp[2] = pts[v1*3+2] - pts[v0*3+2];
        dp[3] = pts[v2*3]   - pts[v0*3];   dp[4] = pts[v2*3+1] - pts[v0*3+1]; dp[5] = pts[v2*3+2] - pts[v0*3+2];
        dp[6] = pts[v3*3]   - pts[v0*3];   dp[7] = pts[v3*3+1] - pts[v0*3+1]; dp[8] = pts[v3*3+2] - pts[v0*3+2];

        // det(dp)
        double det = dp[0]*(dp[4]*dp[8]-dp[5]*dp[7])
                   - dp[1]*(dp[3]*dp[8]-dp[5]*dp[6])
                   + dp[2]*(dp[3]*dp[7]-dp[4]*dp[6]);
        if (std::abs(det) < 1e-30) continue;

        // inv(dp) — 3×3 inverse via cofactors
        double invDet = 1.0 / det;
        double inv[9];
        inv[0] = (dp[4]*dp[8]-dp[5]*dp[7])*invDet;
        inv[1] = (dp[2]*dp[7]-dp[1]*dp[8])*invDet;
        inv[2] = (dp[1]*dp[5]-dp[2]*dp[4])*invDet;
        inv[3] = (dp[5]*dp[6]-dp[3]*dp[8])*invDet;
        inv[4] = (dp[0]*dp[8]-dp[2]*dp[6])*invDet;
        inv[5] = (dp[2]*dp[3]-dp[0]*dp[5])*invDet;
        inv[6] = (dp[3]*dp[7]-dp[4]*dp[6])*invDet;
        inv[7] = (dp[1]*dp[6]-dp[0]*dp[7])*invDet;
        inv[8] = (dp[0]*dp[4]-dp[1]*dp[3])*invDet;

        double vol = std::abs(det) / 6.0;

        // J = inv(dp) * da   →  (3, D)
        // JJt = J * J^T      →  (3, 3)
        // We compute JJt directly without materialising J for each d
        // JJt[r][c] = sum_d J[r][d]*J[c][d]
        //           = sum_d (sum_k inv[r][k]*da[k][d]) * (sum_l inv[c][l]*da[l][d])

        // Compute J row by row, accumulate JJt
        double JJt[9] = {0,0,0,0,0,0,0,0,0};

        // For each attribute dimension, compute J column and accumulate outer product
        const double* a0 = attr + v0 * D;
        const double* a1 = attr + v1 * D;
        const double* a2 = attr + v2 * D;
        const double* a3 = attr + v3 * D;

        for (int d = 0; d < D; ++d) {
            double da0 = a1[d] - a0[d];
            double da1 = a2[d] - a0[d];
            double da2 = a3[d] - a0[d];

            // J[r][d] = inv[r][0]*da0 + inv[r][1]*da1 + inv[r][2]*da2
            double j0 = inv[0]*da0 + inv[1]*da1 + inv[2]*da2;
            double j1 = inv[3]*da0 + inv[4]*da1 + inv[5]*da2;
            double j2 = inv[6]*da0 + inv[7]*da1 + inv[8]*da2;

            // accumulate outer product
            JJt[0] += j0*j0; JJt[1] += j0*j1; JJt[2] += j0*j2;
            JJt[3] += j1*j0; JJt[4] += j1*j1; JJt[5] += j1*j2;
            JJt[6] += j2*j0; JJt[7] += j2*j1; JJt[8] += j2*j2;
        }

        // contrib = vol * JJt, add to each vertex
        for (int i = 0; i < 9; ++i) JJt[i] *= vol;

        double* q0 = &m_ADQ[v0*9]; double* q1 = &m_ADQ[v1*9];
        double* q2 = &m_ADQ[v2*9]; double* q3 = &m_ADQ[v3*9];
        for (int i = 0; i < 9; ++i) {
            q0[i] += JJt[i]; q1[i] += JJt[i];
            q2[i] += JJt[i]; q3[i] += JJt[i];
        }
    }

    std::cout << "[TetraSimplification] ADQ built for " << N << " vertices\n";
}

// ════════════════════════════════════════════════════════════════
//  ADQCostOnly — ultra-fast, no allocations
// ════════════════════════════════════════════════════════════════
double TetraSimplification::ADQCostOnly(int ti) const {
    int base = ti * 4;
    const int* tv = m_TetVerts.data();
    int v[4] = {tv[base], tv[base+1], tv[base+2], tv[base+3]};

    // Skip if any boundary vertex
    for (int k = 0; k < 4; ++k) {
        if (m_PreserveBoundary&&m_IsBoundary[v[k]]) return std::numeric_limits<double>::infinity();
    }

    // Centroid position
    const double* pts = m_Pts.data();
    double pos[3] = {0, 0, 0};
    for (int k = 0; k < 4; ++k) {
        pos[0] += pts[v[k]*3];
        pos[1] += pts[v[k]*3+1];
        pos[2] += pts[v[k]*3+2];
    }
    pos[0] *= 0.25; pos[1] *= 0.25; pos[2] *= 0.25;

    double total = 0.0;
    double maxAcc = 0.0;
    for (int k = 0; k < 4; ++k) {
        double dx = pos[0] - pts[v[k]*3];
        double dy = pos[1] - pts[v[k]*3+1];
        double dz = pos[2] - pts[v[k]*3+2];
        total += Sym3_vAv(&m_ADQ[v[k]*9], dx, dy, dz);
        if (m_ErrAccum[v[k]] > maxAcc) maxAcc = m_ErrAccum[v[k]];
    }
    return total + maxAcc;
}

// ════════════════════════════════════════════════════════════════
//  ComputeCost — full cost with geometric checks, no heap allocs
// ════════════════════════════════════════════════════════════════
TetraSimplification::CostResult TetraSimplification::ComputeCost(int ti) {
    CostResult res;
    res.valid = false;
    res.cost = std::numeric_limits<double>::infinity();

    int tbase = ti * 4;
    int* tv = m_TetVerts.data();
    int t[4] = {tv[tbase], tv[tbase+1], tv[tbase+2], tv[tbase+3]};
    const int D = m_AttrDim;
    const double* pts = m_Pts.data();

    // Skip if any boundary vertex
    for (int k = 0; k < 4; ++k) {
        if (m_PreserveBoundary&&m_IsBoundary[t[k]]) return res;
    }

    // Centroid position (user disabled QEM solve)
    res.optPos[0] = (pts[t[0]*3]   + pts[t[1]*3]   + pts[t[2]*3]   + pts[t[3]*3])   * 0.25;
    res.optPos[1] = (pts[t[0]*3+1] + pts[t[1]*3+1] + pts[t[2]*3+1] + pts[t[3]*3+1]) * 0.25;
    res.optPos[2] = (pts[t[0]*3+2] + pts[t[1]*3+2] + pts[t[2]*3+2] + pts[t[3]*3+2]) * 0.25;

    // Weighted attribute interpolation
    if (D > 0) {
        double w[4];
        double wsum = 0.0;
        for (int k = 0; k < 4; ++k) {
            w[k] = 1.0 / std::max(Sym3_trace(&m_ADQ[t[k]*9]), 1e-30);
            wsum += w[k];
        }
        double invW = 1.0 / wsum;
        res.optAttr.assign(D, 0.0);
        for (int k = 0; k < 4; ++k) {
            double wk = w[k] * invW;
            const double* ak = &m_Attrs[t[k] * D];
            for (int d = 0; d < D; ++d) {
                res.optAttr[d] += wk * ak[d];
            }
        }
    }

    // ADQ cost
    double totalCost = 0.0;
    double maxAccum = 0.0;
    for (int k = 0; k < 4; ++k) {
        double dx = res.optPos[0] - pts[t[k]*3];
        double dy = res.optPos[1] - pts[t[k]*3+1];
        double dz = res.optPos[2] - pts[t[k]*3+2];
        totalCost += Sym3_vAv(&m_ADQ[t[k]*9], dx, dy, dz);
        if (m_ErrAccum[t[k]] > maxAccum) maxAccum = m_ErrAccum[t[k]];
    }
    totalCost += maxAccum;

    // ─── Geometric checks on neighboring tets ───
    double stretchSq = m_StretchFactor * m_StretchFactor;
    double aspectSq = m_MaxAspectRatio * m_MaxAspectRatio;
    double ox = res.optPos[0], oy = res.optPos[1], oz = res.optPos[2];

    // Use generation-stamped visited buffer (no allocation)
    ++m_VisitGen;
    if (m_VisitGen > 250) {
        std::memset(m_Visited.data(), 0, m_Visited.size());
        m_VisitGen = 1;
    }
    uint8_t gen = static_cast<uint8_t>(m_VisitGen);

    for (int k = 0; k < 4; ++k) {
        const auto& adj = m_VertTets[t[k]];
        for (int ni : adj) {
            if (m_Visited[ni] == gen || !m_TetAlive[ni] || ni == ti) continue;
            m_Visited[ni] = gen;

            int nb = ni * 4;
            int nt[4] = {tv[nb], tv[nb+1], tv[nb+2], tv[nb+3]};

            // Count shared vertices (inline, no set)
            int sharedCount = 0;
            int sharedIdx = -1;
            for (int j = 0; j < 4; ++j) {
                int nv = nt[j];
                if (nv == t[0] || nv == t[1] || nv == t[2] || nv == t[3]) {
                    sharedCount++;
                    sharedIdx = j;
                }
            }
            if (sharedCount >= 2 || sharedCount == 0) continue;

            // Get 4 vertex positions
            double p[4][3];
            for (int j = 0; j < 4; ++j) {
                p[j][0] = pts[nt[j]*3]; p[j][1] = pts[nt[j]*3+1]; p[j][2] = pts[nt[j]*3+2];
            }

            // Before volume
            double e1[3] = {p[1][0]-p[0][0], p[1][1]-p[0][1], p[1][2]-p[0][2]};
            double e2[3] = {p[2][0]-p[0][0], p[2][1]-p[0][1], p[2][2]-p[0][2]};
            double e3[3] = {p[3][0]-p[0][0], p[3][1]-p[0][1], p[3][2]-p[0][2]};
            double volB = e1[0]*(e2[1]*e3[2]-e2[2]*e3[1])
                        - e1[1]*(e2[0]*e3[2]-e2[2]*e3[0])
                        + e1[2]*(e2[0]*e3[1]-e2[1]*e3[0]);

            // Replace shared vertex
            p[sharedIdx][0] = ox; p[sharedIdx][1] = oy; p[sharedIdx][2] = oz;

            double ea[3] = {p[1][0]-p[0][0], p[1][1]-p[0][1], p[1][2]-p[0][2]};
            double eb[3] = {p[2][0]-p[0][0], p[2][1]-p[0][1], p[2][2]-p[0][2]};
            double ec[3] = {p[3][0]-p[0][0], p[3][1]-p[0][1], p[3][2]-p[0][2]};
            double volA = ea[0]*(eb[1]*ec[2]-eb[2]*ec[1])
                        - ea[1]*(eb[0]*ec[2]-eb[2]*ec[0])
                        + ea[2]*(eb[0]*ec[1]-eb[1]*ec[0]);

            // Flip check
            if (volB * volA < 0 || std::abs(volA) < 1e-30) return res;

            // Stretch check
            //int bi[3], bk = 0;
            //for (int j = 0; j < 4; ++j) { if (j != sharedIdx) bi[bk++] = j; }
            //double b0[3] = {p[bi[0]][0], p[bi[0]][1], p[bi[0]][2]};
            //double nx = (p[bi[1]][1]-b0[1])*(p[bi[2]][2]-b0[2]) - (p[bi[1]][2]-b0[2])*(p[bi[2]][1]-b0[1]);
            //double ny = (p[bi[1]][2]-b0[2])*(p[bi[2]][0]-b0[0]) - (p[bi[1]][0]-b0[0])*(p[bi[2]][2]-b0[2]);
            //double nz = (p[bi[1]][0]-b0[0])*(p[bi[2]][1]-b0[1]) - (p[bi[1]][1]-b0[1])*(p[bi[2]][0]-b0[0]);
            //double nrmSq = nx*nx + ny*ny + nz*nz;
            //if (nrmSq > 1e-30) {
            //    double origX = pts[nt[sharedIdx]*3], origY = pts[nt[sharedIdx]*3+1], origZ = pts[nt[sharedIdx]*3+2];
            //    double hb = (origX-b0[0])*nx + (origY-b0[1])*ny + (origZ-b0[2])*nz;
            //    double ha = (ox-b0[0])*nx + (oy-b0[1])*ny + (oz-b0[2])*nz;
            //    if (hb*hb > 1e-30 && ha*ha > stretchSq * hb*hb) return res;
            //}

            //// Aspect ratio
            //double eSq[6];
            //int ei = 0;
            //for (int a = 0; a < 3; ++a) {
            //    for (int b = a + 1; b < 4; ++b) {
            //        double dd[3] = {p[a][0]-p[b][0], p[a][1]-p[b][1], p[a][2]-p[b][2]};
            //        eSq[ei++] = dd[0]*dd[0] + dd[1]*dd[1] + dd[2]*dd[2];
            //    }
            //}
            //double mn = eSq[0], mx = eSq[0];
            //for (int i = 1; i < 6; ++i) {
            //    if (eSq[i] < mn) mn = eSq[i];
            //    if (eSq[i] > mx) mx = eSq[i];
            //}
            //if (mn < 1e-30 || mx > aspectSq * mn) return res;
        }
    }

    res.cost = totalCost;
    res.valid = true;
    return res;
}

// ════════════════════════════════════════════════════════════════
//  InitHeap
// ════════════════════════════════════════════════════════════════
void TetraSimplification::InitHeap() {
    while (!m_Heap.empty()) m_Heap.pop();

    int count = 0;
    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        if (m_PreserveBoundary && m_IsBoundaryTet[ti]) continue;
        double c = ADQCostOnly(ti);
        if (c < std::numeric_limits<double>::infinity()) {
            m_Heap.push({c, ti, 0});
            count++;
        }
    }
    std::cout << "[TetraSimplification] " << count << " candidates in heap\n";
}

// ════════════════════════════════════════════════════════════════
//  DoCollapse
// ════════════════════════════════════════════════════════════════
void TetraSimplification::DoCollapse(int ti, const double optPos[3],
                                      const std::vector<double>& optAttr) {
    int tbase = ti * 4;
    int* tv = m_TetVerts.data();
    int t[4] = {tv[tbase], tv[tbase+1], tv[tbase+2], tv[tbase+3]};
    int survivor = t[0];
    const int D = m_AttrDim;
    const double* pts = m_Pts.data();

    // Merge ADQ
    double merged[9] = {0,0,0,0,0,0,0,0,0};
    for (int k = 0; k < 4; ++k) {
        const double* qk = &m_ADQ[t[k]*9];
        for (int i = 0; i < 9; ++i) merged[i] += qk[i];
    }

    double maxErr = 0.0;
    for (int k = 0; k < 4; ++k) {
        double dx = optPos[0] - pts[t[k]*3];
        double dy = optPos[1] - pts[t[k]*3+1];
        double dz = optPos[2] - pts[t[k]*3+2];
        double e = m_ErrAccum[t[k]] + Sym3_vAv(&m_ADQ[t[k]*9], dx, dy, dz);
        if (e > maxErr) maxErr = e;
    }

    double* qs = &m_ADQ[survivor*9];
    for (int i = 0; i < 9; ++i) qs[i] = merged[i];
    m_ErrAccum[survivor] = maxErr;

    // Update position
    m_Pts[survivor*3]   = optPos[0];
    m_Pts[survivor*3+1] = optPos[1];
    m_Pts[survivor*3+2] = optPos[2];
    if (D > 0 && static_cast<int>(optAttr.size()) == D) {
        double* as = &m_Attrs[survivor * D];
        for (int d = 0; d < D; ++d) as[d] = optAttr[d];
    }

    // Kill collapsed tet
    m_TetAlive[ti] = 0;

    // Kill tets sharing ≥2 vertices (inline check)
    for (int k = 0; k < 4; ++k) {
        for (int ni : m_VertTets[t[k]]) {
            if (!m_TetAlive[ni] || ni == ti) continue;
            int nb = ni * 4;
            int shared = 0;
            for (int j = 0; j < 4; ++j) {
                int nv = tv[nb+j];
                if (nv == t[0] || nv == t[1] || nv == t[2] || nv == t[3]) shared++;
            }
            if (shared >= 2) {
                m_TetAlive[ni] = 0;
                m_TetVersion[ni]++;
            }
        }
    }

    // Redirect removed → survivor
    for (int rk = 1; rk < 4; ++rk) {
        int rv = t[rk];
        for (int ni : m_VertTets[rv]) {
            if (!m_TetAlive[ni]) continue;
            int nb = ni * 4;
            for (int j = 0; j < 4; ++j) {
                if (tv[nb+j] == rv) tv[nb+j] = survivor;
            }
            m_VertTets[survivor].push_back(ni);
            m_TetVersion[ni]++;

            // Degenerate check
            int u0 = tv[nb], u1 = tv[nb+1], u2 = tv[nb+2], u3 = tv[nb+3];
            if (u0==u1 || u0==u2 || u0==u3 || u1==u2 || u1==u3 || u2==u3) {
                m_TetAlive[ni] = 0;
            }
        }
        m_VertAlive[rv] = 0;
        m_VertTets[rv].clear();
    }

    // Update boundary flags for survivor's tets
    for (int ni : m_VertTets[survivor]) {
        if (m_TetAlive[ni]) {
            int nb = ni * 4;
            if (m_IsBoundary[tv[nb]] | m_IsBoundary[tv[nb+1]] |
                m_IsBoundary[tv[nb+2]] | m_IsBoundary[tv[nb+3]]) {
                m_IsBoundaryTet[ni] = 1;
            }
        }
    }
}

// ════════════════════════════════════════════════════════════════
//  Simplify
// ════════════════════════════════════════════════════════════════
void TetraSimplification::Simplify() {
    int N0 = 0;
    for (int i = 0; i < m_NumVerts; ++i) N0 += m_VertAlive[i];

    int target;
    if (m_TargetTetraCount > 0) {
        target = m_TargetTetraCount;
    } else {
        target = std::max(4, static_cast<int>(N0 * m_TargetReduction));
    }

    std::cout << "[TetraSimplification] Simplifying: " << N0
              << " verts -> target " << target << "\n";

    InitHeap();

    int collapsed = 0;
    int vertCount = N0;

    while (!m_Heap.empty() && vertCount > target) {
        HeapEntry entry = m_Heap.top();
        m_Heap.pop();

        if (!m_TetAlive[entry.tetIdx] || m_TetVersion[entry.tetIdx] != entry.version)
            continue;

        CostResult cr = ComputeCost(entry.tetIdx);
        if (!cr.valid) continue;

        DoCollapse(entry.tetIdx, cr.optPos, cr.optAttr);
        collapsed++;
        vertCount -= 3;

        if (collapsed % 2000 == 0) {
            int nv = 0, nt = 0;
            for (int i = 0; i < m_NumVerts; ++i) nv += m_VertAlive[i];
            for (int i = 0; i < m_NumTets; ++i) nt += m_TetAlive[i];
            std::cout << "  [" << collapsed << "] " << nv << " verts, "
                      << nt << " tets, cost=" << cr.cost << "\n";
        }

        // Re-enqueue affected neighbors
        int survivor = m_TetVerts[entry.tetIdx * 4];
        if (m_VertAlive[survivor]) {
            for (int ni : m_VertTets[survivor]) {
                if (m_TetAlive[ni] && !(m_PreserveBoundary&&m_IsBoundaryTet[ni])) {
                    double c = ADQCostOnly(ni);
                    if (c < std::numeric_limits<double>::infinity()) {
                        m_Heap.push({c, ni, m_TetVersion[ni]});
                    }
                }
            }
        }
    }

    int nv = 0, nt = 0;
    for (int i = 0; i < m_NumVerts; ++i) nv += m_VertAlive[i];
    for (int i = 0; i < m_NumTets; ++i) nt += m_TetAlive[i];
    std::cout << "[TetraSimplification] Done: " << collapsed << " collapses, "
              << nv << " verts, " << nt << " tets\n";
}

// ════════════════════════════════════════════════════════════════
//  SaveMesh
// ════════════════════════════════════════════════════════════════
bool TetraSimplification::SaveMesh() {
    const int N = m_NumVerts;
    const int D = m_AttrDim;
    const int* tv = m_TetVerts.data();

    // old→new mapping
    std::vector<int> old2new(N, -1);
    int newIdx = 0;
    for (int i = 0; i < N; ++i) {
        if (m_VertAlive[i]) old2new[i] = newIdx++;
    }
    const int newN = newIdx;

    auto outMesh = VolumeMesh::New();
    outMesh->SetName(m_InputMesh->GetName() + "_simplified");

    // Points (denormalized)
    auto outPoints = Points::New();
    for (int i = 0; i < N; ++i) {
        if (!m_VertAlive[i]) continue;
        outPoints->AddPoint(Point(
            m_Pts[i*3]   * m_PtsScale + m_PtsMin[0],
            m_Pts[i*3+1] * m_PtsScale + m_PtsMin[1],
            m_Pts[i*3+2] * m_PtsScale + m_PtsMin[2]));
    }
    outMesh->SetPoints(outPoints);

    // Tets
    auto outCells = CellArray::New();
    std::vector<igIndex> outCellSourceIds;
    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        int base = ti * 4;
        int m0 = old2new[tv[base]], m1 = old2new[tv[base+1]],
            m2 = old2new[tv[base+2]], m3 = old2new[tv[base+3]];
        if (m0 < 0 || m1 < 0 || m2 < 0 || m3 < 0) continue;
        if (m0==m1 || m0==m2 || m0==m3 || m1==m2 || m1==m3 || m2==m3) continue;
        outCells->AddCellId4(m0, m1, m2, m3);
        outCellSourceIds.push_back(m_TetSourceIds[ti]);
    }
    outMesh->SetVolumes(outCells);

    // Attributes (denormalized)
    auto outAttrs = AttributeSet::New();
    if (D > 0) {
        for (const auto& p : m_AttrNormParams) {
            if (p.isScalar) {
                for (int i = 0; i < N; ++i) {
                    if (!m_VertAlive[i]) continue;
                    m_Attrs[i*D + p.col] = m_Attrs[i*D + p.col] * p.range + p.minVal;
                }
            } else {
                for (int i = 0; i < N; ++i) {
                    if (!m_VertAlive[i]) continue;
                    for (int d = 0; d < p.ncomp; ++d)
                        m_Attrs[i*D + p.col + d] *= p.maxMag;
                }
            }
        }

        int col = 0;
        for (const auto& info : m_AttrInfo) {
            ArrayObject::Pointer arr = nullptr;
            const int attrCol = col;
            col += info.ncomp;
            switch (info.arrayType) {
                case IG_FloatArray:
                    arr = BuildPointAttribute<FloatArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_DoubleArray:
                    arr = BuildPointAttribute<DoubleArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_IntArray:
                    arr = BuildPointAttribute<IntArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedIntArray:
                    arr = BuildPointAttribute<UnsignedIntArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_CharArray:
                    arr = BuildPointAttribute<CharArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedCharArray:
                    arr = BuildPointAttribute<UnsignedCharArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_ShortArray:
                    arr = BuildPointAttribute<ShortArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedShortArray:
                    arr = BuildPointAttribute<UnsignedShortArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_LongLongArray:
                    arr = BuildPointAttribute<LongLongArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs,D);
                    break;

                case IG_UnsignedLongLongArray:
                    arr = BuildPointAttribute<UnsignedLongLongArray, TetraSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                default:
                    break;
            }
            if (arr) { outAttrs->AddAttribute(info.attributeType, IG_POINT, arr); }

        }
    }

    auto inputAttrs = m_InputMesh->GetAttributeSet();
    if (inputAttrs) {
        auto all = inputAttrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto attr = all->GetElement(ai);
            if (attr.isDeleted || !attr.pointer) continue;
            if (attr.attachmentType != IG_CELL) continue;
            ArrayObject::Pointer arr = nullptr;
            switch (attr.pointer->GetArrayType()) {
                case IG_FloatArray:
                    arr = BuildCellAttribute<FloatArray>(attr, outCellSourceIds);
                    break;

                case IG_DoubleArray:
                    arr = BuildCellAttribute<DoubleArray>(attr, outCellSourceIds);
                    break;

                case IG_IntArray:
                    arr = BuildCellAttribute<IntArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedIntArray:
                    arr = BuildCellAttribute<UnsignedIntArray>(attr, outCellSourceIds);
                    break;

                case IG_CharArray:
                    arr = BuildCellAttribute<CharArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedCharArray:
                    arr = BuildCellAttribute<UnsignedCharArray>(attr, outCellSourceIds);
                    break;

                case IG_ShortArray:
                    arr = BuildCellAttribute<ShortArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedShortArray:
                    arr = BuildCellAttribute<UnsignedShortArray>(attr, outCellSourceIds);
                    break;

                case IG_LongLongArray:
                    arr = BuildCellAttribute<LongLongArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedLongLongArray:
                    arr = BuildCellAttribute<UnsignedLongLongArray>(attr, outCellSourceIds);
                    break;

                default:
                    break;
            }
            if (arr) { outAttrs->AddAttribute(attr.type, IG_CELL, arr); }
        }
    }
    outMesh->SetAttributeSet(outAttrs);

    SetOutput(outMesh);
    std::cout << "[TetraSimplification] Output: " << newN << " verts, "
              << outCells->GetNumberOfCells() << " tets\n";
    return true;
}

IGAME_NAMESPACE_END

// ╔════════════════════════════════════════════════════════════════╗
// ║        TetraEdgeSimplification  — edge‐collapse variant       ║
// ╚════════════════════════════════════════════════════════════════╝

IGAME_NAMESPACE_BEGIN

// ═══════════ Execute ═══════════
bool TetraEdgeSimplification::Execute() {
    if (!LoadMesh()) return false;
    Normalize();
    BuildTopology();
    BuildADQ();

    clock_t t0 = clock();
    Simplify();
    double dt = double(clock() - t0) / CLOCKS_PER_SEC;
    std::cout << "[TetraEdgeSimp] Simplification took " << dt << " s\n";

    return SaveMesh();
}

// ═══════════ LoadMesh ═══════════
bool TetraEdgeSimplification::LoadMesh() {
    auto obj = GetInput(0);
    if (!obj) return false;

    if (obj->GetDataObjectType() == IG_VOLUME_MESH) {
        m_InputMesh = DynamicCast<VolumeMesh>(obj);
    } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        auto um = DynamicCast<UnstructuredMesh>(obj);
        if (um) m_InputMesh = um->TransferToVolumeMesh();
    }
    if (!m_InputMesh) return false;

    auto points = m_InputMesh->GetPoints();
    if (!points) return false;

    m_NumVerts = static_cast<int>(points->GetNumberOfPoints());
    m_Pts.resize(m_NumVerts * 3);
    for (int i = 0; i < m_NumVerts; ++i) {
        const Point& p = points->GetPoint(i);
        m_Pts[i*3] = p[0]; m_Pts[i*3+1] = p[1]; m_Pts[i*3+2] = p[2];
    }

    const IGsize nVol = m_InputMesh->GetNumberOfVolumes();
    m_TetVerts.clear();
    m_TetVerts.reserve(nVol * 4);
    m_TetSourceIds.clear();
    m_TetSourceIds.reserve(nVol);
    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    m_NumTets = 0;
    for (IGsize ci = 0; ci < nVol; ++ci) {
        int n = m_InputMesh->GetVolumePointIds(ci, ids);
        if (n != 4) continue;
        for (int j = 0; j < 4; ++j) m_TetVerts.push_back(static_cast<int>(ids[j]));
        m_NumTets++;
        m_TetSourceIds.push_back(ci);
    }

    auto attrs = m_InputMesh->GetAttributeSet();
    m_AttrInfo.clear();
    int totalDim = 0;
    if (attrs) {
        auto all = attrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto a = all->GetElement(ai);
            if (a.isDeleted || !a.pointer || a.attachmentType != IG_POINT) continue;
            if (!m_UseAllPointAttributes) {
                if (static_cast<int>(ai) != m_InputMesh->GetCurrentAttributeIndex()) continue;
            }
            int dim = a.pointer->GetDimension();
            auto arrayType = a.pointer->GetArrayType();
            auto attributeType = a.type;
            auto attachmentType = a.attachmentType;
            m_AttrInfo.push_back({a.pointer->GetName(), dim, arrayType, attributeType, attachmentType});
            totalDim += dim;
        }
    }
    m_AttrDim = totalDim;
    m_Attrs.assign(m_NumVerts * totalDim, 0.0);
    if (totalDim > 0 && attrs) {
        int col = 0;
        auto all = attrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto a = all->GetElement(ai);
            if (a.isDeleted || !a.pointer || a.attachmentType != IG_POINT) continue;
            if (!m_UseAllPointAttributes) {
                if (static_cast<int>(ai) != m_InputMesh->GetCurrentAttributeIndex()) continue;
            }
            int dim = a.pointer->GetDimension();
            double vals[IGAME_CELL_MAX_SIZE]{};
            for (int i = 0; i < m_NumVerts; ++i) {
                a.pointer->GetElement(i, vals);
                for (int d = 0; d < dim; ++d)
                    m_Attrs[i * totalDim + col + d] = vals[d];
            }
            col += dim;
        }
    }
    std::cout << "[TetraEdgeSimp] Loaded " << m_NumVerts << " verts, "
              << m_NumTets << " tets, " << m_AttrDim << " attr dims\n";
    return true;
}

// ═══════════ Normalize ═══════════
void TetraEdgeSimplification::Normalize() {
    const int N = m_NumVerts, D = m_AttrDim;
    double pmin[3]={1e30,1e30,1e30}, pmax[3]={-1e30,-1e30,-1e30};
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < 3; ++j) {
            double v = m_Pts[i*3+j];
            if (v < pmin[j]) pmin[j] = v;
            if (v > pmax[j]) pmax[j] = v;
        }
    m_PtsMin[0]=pmin[0]; m_PtsMin[1]=pmin[1]; m_PtsMin[2]=pmin[2];
    m_PtsScale = std::max({pmax[0]-pmin[0], pmax[1]-pmin[1], pmax[2]-pmin[2], 1e-12});
    double inv = 1.0 / m_PtsScale;
    for (int i = 0; i < N*3; ++i) m_Pts[i] = (m_Pts[i] - pmin[i%3]) * inv;

    m_AttrNormParams.clear();
    int col = 0;
    for (const auto& info : m_AttrInfo) {
        if (info.ncomp == 1) {
            double mn=1e30, mx=-1e30;
            for (int i=0;i<N;++i){double v=m_Attrs[i*D+col]; if(v<mn)mn=v; if(v>mx)mx=v;}
            double rng=mx-mn;
            if (rng>1e-12){double ir=1.0/rng; for(int i=0;i<N;++i) m_Attrs[i*D+col]=(m_Attrs[i*D+col]-mn)*ir;}
            m_AttrNormParams.push_back({true,mn,rng>1e-12?rng:1.0,1.0,col,1});
        } else {
            double maxMag=0;
            for(int i=0;i<N;++i){double m2=0;for(int d=0;d<info.ncomp;++d){double v=m_Attrs[i*D+col+d];m2+=v*v;} double m=std::sqrt(m2);if(m>maxMag)maxMag=m;}
            if(maxMag>1e-12){double iv=1.0/maxMag;for(int i=0;i<N;++i)for(int d=0;d<info.ncomp;++d)m_Attrs[i*D+col+d]*=iv;}
            m_AttrNormParams.push_back({false,0,1,maxMag>1e-12?maxMag:1.0,col,info.ncomp});
        }
        col += info.ncomp;
    }
}

TetraEdgeSimplification::FaceKey
TetraEdgeSimplification::MakeFaceKey(int a, int b, int c) {
    // 同一个面可能写成 (a,b,c)、(b,c,a) 等不同顺序。排序后所有排列都会得到
    // 同一个规范化键，从而共用同一个计数器。
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
    return {a, b, c};
}

uint64_t TetraEdgeSimplification::MakeEdgeKey(int a, int b) {
    // 顶点编号是 32 位整数。将排好序的一对编号压入一个 uint64_t，
    // 可以避免在高频查询路径中额外分配边对象。
    if (a > b) std::swap(a, b);
    return (uint64_t(uint32_t(a)) << 32) | uint64_t(uint32_t(b));
}

void TetraEdgeSimplification::AddBoundaryFace(const FaceKey& face) {
    if (!m_BoundaryFaces.insert(face).second) return;

    // 只要一个顶点仍被至少一个当前边界面使用，它就是边界点。这里必须保存计数，
    // 因为删除一个边界面时，不能把仍被其他边界面使用的顶点误标为内部点。
    const int vertices[3] = {face.a, face.b, face.c};
    for (int v : vertices) {
        ++m_BoundaryFaceCountPerVertex[v];
        m_IsBoundary[v] = 1;
        m_VertBoundaryFaces[v].insert(face);
    }

    // 每个边界三角形的三条边都是当前边界边。相邻边界三角形通常会共享一条边，
    // 因而同一个边键会被加入两次，所以这里保存的是邻接次数而不是简单布尔值。
    ++m_BoundaryEdgeFaceCounts[MakeEdgeKey(face.a, face.b)];
    ++m_BoundaryEdgeFaceCounts[MakeEdgeKey(face.a, face.c)];
    ++m_BoundaryEdgeFaceCounts[MakeEdgeKey(face.b, face.c)];
}

void TetraEdgeSimplification::RemoveBoundaryFace(const FaceKey& face) {
    if (m_BoundaryFaces.erase(face) == 0) return;

    const int vertices[3] = {face.a, face.b, face.c};
    for (int v : vertices) {
        if (m_BoundaryFaceCountPerVertex[v] > 0)
            --m_BoundaryFaceCountPerVertex[v];
        m_IsBoundary[v] = m_BoundaryFaceCountPerVertex[v] > 0;
        m_VertBoundaryFaces[v].erase(face);
    }

    const uint64_t edges[3] = {
        MakeEdgeKey(face.a, face.b),
        MakeEdgeKey(face.a, face.c),
        MakeEdgeKey(face.b, face.c)
    };
    for (uint64_t edge : edges) {
        auto it = m_BoundaryEdgeFaceCounts.find(edge);
        if (it == m_BoundaryEdgeFaceCounts.end()) continue;
        if (--it->second == 0) m_BoundaryEdgeFaceCounts.erase(it);
    }
}

void TetraEdgeSimplification::ChangeFaceCount(const FaceKey& face, int delta) {
    auto it = m_FaceCounts.find(face);
    const int oldCount = it == m_FaceCounts.end() ? 0 : it->second;

    // 面的邻接次数跨过 1 时，边界状态会发生变化：
    // 0 -> 1：出现一个边界面
    // 1 -> 2：该面变成内部面
    // 2 -> 1：该面暴露并变成边界面
    // 1 -> 0：该面消失
    if (oldCount == 1) RemoveBoundaryFace(face);

    const int newCount = oldCount + delta;
    assert(newCount >= 0);
    if (newCount == 0) {
        if (it != m_FaceCounts.end()) m_FaceCounts.erase(it);
    } else {
        m_FaceCounts[face] = newCount;
    }

    if (newCount == 1) AddBoundaryFace(face);
}

bool TetraEdgeSimplification::IsBoundaryEdge(int a, int b) const {
    return m_BoundaryEdgeFaceCounts.find(MakeEdgeKey(a, b)) !=
           m_BoundaryEdgeFaceCounts.end();
}

// ═══════════ BuildTopology ═══════════
void TetraEdgeSimplification::BuildTopology() {
    const int N = m_NumVerts, M = m_NumTets;
    const int* tv = m_TetVerts.data();

    m_VertTets.assign(N, std::vector<int>());
    std::vector<int> deg(N, 0);
    for (int i = 0; i < M*4; ++i) deg[tv[i]]++;
    for (int v = 0; v < N; ++v) m_VertTets[v].reserve(deg[v]);
    for (int ti = 0; ti < M; ++ti) {
        int b = ti*4;
        m_VertTets[tv[b]].push_back(ti);
        m_VertTets[tv[b+1]].push_back(ti);
        m_VertTets[tv[b+2]].push_back(ti);
        m_VertTets[tv[b+3]].push_back(ti);
    }

    m_TetAlive.assign(M, 1);
    m_VertAlive.assign(N, 1);
    m_VertVersion.assign(N, 0);

    // 统计当前每个面的邻接次数。只被一个四面体使用的面位于外表面；
    // 正常的内部面应当被两个四面体共同使用。
    m_FaceCounts.clear();
    m_FaceCounts.reserve(M * 4);
    m_BoundaryFaces.clear();
    m_BoundaryFaces.reserve(M * 2);
    m_BoundaryEdgeFaceCounts.clear();
    m_BoundaryEdgeFaceCounts.reserve(M * 2);
    m_BoundaryFaceCountPerVertex.assign(N, 0);
    m_IsBoundary.assign(N, 0);
    m_VertBoundaryFaces.assign(N, {});

    static const int fi[4][3]={{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    for (int ti = 0; ti < M; ++ti) {
        const int b = ti * 4;
        for (int f = 0; f < 4; ++f) {
            ChangeFaceCount(MakeFaceKey(tv[b + fi[f][0]],
                                       tv[b + fi[f][1]],
                                       tv[b + fi[f][2]]), 1);
        }
    }

    // 为每个边界顶点建立齐次平面二次误差矩阵。这些矩阵描述的是原始外表面；
    // 顶点坍缩时只合并矩阵而不重新生成，因此连续多次坍缩也不能随意偏离输入边界。
    m_BoundaryQ.assign(N * 16, 0.0);
    m_BoundaryPlaneCount.assign(N, 0.0);
    const double* pts = m_Pts.data();
    for (const FaceKey& face : m_BoundaryFaces) {
        const double* p0 = &pts[face.a * 3];
        const double* p1 = &pts[face.b * 3];
        const double* p2 = &pts[face.c * 3];
        const double ux = p1[0] - p0[0], uy = p1[1] - p0[1], uz = p1[2] - p0[2];
        const double vx = p2[0] - p0[0], vy = p2[1] - p0[1], vz = p2[2] - p0[2];
        double plane[4] = {
            uy * vz - uz * vy,
            uz * vx - ux * vz,
            ux * vy - uy * vx,
            0.0
        };
        const double length = std::sqrt(plane[0] * plane[0] +
                                        plane[1] * plane[1] +
                                        plane[2] * plane[2]);
        if (length < 1e-30) continue;
        plane[0] /= length;
        plane[1] /= length;
        plane[2] /= length;
        plane[3] = -(plane[0] * p0[0] + plane[1] * p0[1] + plane[2] * p0[2]);

        const int vertices[3] = {face.a, face.b, face.c};
        for (int v : vertices) {
            ++m_BoundaryPlaneCount[v];
            double* q = &m_BoundaryQ[v * 16];
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    q[r * 4 + c] += plane[r] * plane[c];
        }
    }
    m_IsBoundaryTet.assign(M,0);
    for(int ti=0;ti<M;++ti){int b=ti*4;
        if(m_IsBoundary[tv[b]]|m_IsBoundary[tv[b+1]]|m_IsBoundary[tv[b+2]]|m_IsBoundary[tv[b+3]])
            m_IsBoundaryTet[ti]=1;}

    int nBdy=0; for(int i=0;i<N;++i)nBdy+=m_IsBoundary[i];
    std::cout << "[TetraEdgeSimp] " << nBdy << " bdy verts\n";
}

// ═══════════ BuildADQ ═══════════
void TetraEdgeSimplification::BuildADQ() {
    const int N=m_NumVerts, D=m_AttrDim, M=m_NumTets;
    m_ADQ.assign(N*9, 0.0);
    m_ErrAccum.assign(N, 0.0);
    if (D==0) return;

    const double*pts=m_Pts.data(), *attr=m_Attrs.data();
    const int*tv=m_TetVerts.data();

    for (int ti=0;ti<M;++ti){
        int b=ti*4; int v0=tv[b],v1=tv[b+1],v2=tv[b+2],v3=tv[b+3];
        double dp[9];
        dp[0]=pts[v1*3]-pts[v0*3];   dp[1]=pts[v1*3+1]-pts[v0*3+1]; dp[2]=pts[v1*3+2]-pts[v0*3+2];
        dp[3]=pts[v2*3]-pts[v0*3];   dp[4]=pts[v2*3+1]-pts[v0*3+1]; dp[5]=pts[v2*3+2]-pts[v0*3+2];
        dp[6]=pts[v3*3]-pts[v0*3];   dp[7]=pts[v3*3+1]-pts[v0*3+1]; dp[8]=pts[v3*3+2]-pts[v0*3+2];
        double det=dp[0]*(dp[4]*dp[8]-dp[5]*dp[7])-dp[1]*(dp[3]*dp[8]-dp[5]*dp[6])+dp[2]*(dp[3]*dp[7]-dp[4]*dp[6]);
        if(std::abs(det)<1e-30) continue;
        double id=1.0/det;
        double inv[9]={
            (dp[4]*dp[8]-dp[5]*dp[7])*id,(dp[2]*dp[7]-dp[1]*dp[8])*id,(dp[1]*dp[5]-dp[2]*dp[4])*id,
            (dp[5]*dp[6]-dp[3]*dp[8])*id,(dp[0]*dp[8]-dp[2]*dp[6])*id,(dp[2]*dp[3]-dp[0]*dp[5])*id,
            (dp[3]*dp[7]-dp[4]*dp[6])*id,(dp[1]*dp[6]-dp[0]*dp[7])*id,(dp[0]*dp[4]-dp[1]*dp[3])*id};
        double vol=std::abs(det)/6.0;
        double JJt[9]={0,0,0,0,0,0,0,0,0};
        const double*a0=attr+v0*D,*a1=attr+v1*D,*a2=attr+v2*D,*a3=attr+v3*D;
        for(int d=0;d<D;++d){
            double da0=a1[d]-a0[d],da1=a2[d]-a0[d],da2=a3[d]-a0[d];
            double j0=inv[0]*da0+inv[1]*da1+inv[2]*da2;
            double j1=inv[3]*da0+inv[4]*da1+inv[5]*da2;
            double j2=inv[6]*da0+inv[7]*da1+inv[8]*da2;
            JJt[0]+=j0*j0;JJt[1]+=j0*j1;JJt[2]+=j0*j2;
            JJt[3]+=j1*j0;JJt[4]+=j1*j1;JJt[5]+=j1*j2;
            JJt[6]+=j2*j0;JJt[7]+=j2*j1;JJt[8]+=j2*j2;
        }
        for(int i=0;i<9;++i)JJt[i]*=vol;
        double*q0=&m_ADQ[v0*9],*q1=&m_ADQ[v1*9],*q2=&m_ADQ[v2*9],*q3=&m_ADQ[v3*9];
        for(int i=0;i<9;++i){q0[i]+=JJt[i];q1[i]+=JJt[i];q2[i]+=JJt[i];q3[i]+=JJt[i];}
    }
    std::cout << "[TetraEdgeSimp] ADQ built for " << N << " vertices\n";
}

bool TetraEdgeSimplification::IsTopologicallyCollapsible(int va, int vb) const {
    // 对合法的单纯复形坍缩，va 与 vb 的 link 交集必须恰好等于边 (va,vb) 的 link。
    // 直白地说：允许把围绕这条边的一圈单元缩掉，但不能把其他仅仅同时接触
    // 两个端点的区域强行粘在一起。
    using VertexSet = std::unordered_set<int>;
    using EdgeSet = std::unordered_set<uint64_t>;
    using FaceSet = std::unordered_set<FaceKey, FaceKeyHash>;
    struct LinkData {
        VertexSet vertices;
        EdgeSet edges;
        FaceSet faces;
    };

    const int* tv = m_TetVerts.data();
    auto buildVertexLink = [&](int vertex) {
        LinkData link;
        for (int ti : m_VertTets[vertex]) {
            if (!m_TetAlive[ti]) continue;
            const int base = ti * 4;
            int other[3], count = 0;
            for (int j = 0; j < 4; ++j) {
                const int v = tv[base + j];
                if (v != vertex && count < 3) other[count++] = v;
            }
            if (count != 3) continue;

            for (int v : other) link.vertices.insert(v);
            link.edges.insert(MakeEdgeKey(other[0], other[1]));
            link.edges.insert(MakeEdgeKey(other[0], other[2]));
            link.edges.insert(MakeEdgeKey(other[1], other[2]));
            link.faces.insert(MakeFaceKey(other[0], other[1], other[2]));
        }
        return link;
    };

    const LinkData linkA = buildVertexLink(va);
    const LinkData linkB = buildVertexLink(vb);
    LinkData edgeLink;

    // 每个邻接四面体 [va,vb,x,y] 都贡献一条对边 [x,y]。这些对边在内部边周围
    // 必须组成一个环，在边界边周围必须组成一条路径；出现分叉或不连通分量
    // 表示局部非流形或存在裂缝，因此拒绝坍缩。
    std::unordered_map<int, std::vector<int>> oppositeGraph;
    int incidentTetCount = 0;
    for (int ti : m_VertTets[va]) {
        if (!m_TetAlive[ti]) continue;
        const int base = ti * 4;
        bool hasB = false;
        for (int j = 0; j < 4; ++j) hasB |= tv[base + j] == vb;
        if (!hasB) continue;

        int opposite[2], count = 0;
        for (int j = 0; j < 4; ++j) {
            const int v = tv[base + j];
            if (v != va && v != vb && count < 2) opposite[count++] = v;
        }
        if (count != 2) return false;
        ++incidentTetCount;
        edgeLink.vertices.insert(opposite[0]);
        edgeLink.vertices.insert(opposite[1]);
        if (!edgeLink.edges.insert(MakeEdgeKey(opposite[0], opposite[1])).second)
            return false; // 对边重复意味着局部存在重复四面体
        oppositeGraph[opposite[0]].push_back(opposite[1]);
        oppositeGraph[opposite[1]].push_back(opposite[0]);
    }
    if (incidentTetCount == 0 || oppositeGraph.empty()) return false;

    for (int v : linkA.vertices)
        if (linkB.vertices.count(v) && !edgeLink.vertices.count(v)) return false;
    for (int v : edgeLink.vertices)
        if (!linkA.vertices.count(v) || !linkB.vertices.count(v)) return false;

    for (uint64_t edge : linkA.edges)
        if (linkB.edges.count(edge) && !edgeLink.edges.count(edge)) return false;
    for (uint64_t edge : edgeLink.edges)
        if (!linkA.edges.count(edge) || !linkB.edges.count(edge)) return false;

    // 边的 link 中不应包含三角形。如果两个端点的 link 还共享某个三角形，
    // 说明待坍缩边之外存在额外的公共邻域。
    for (const FaceKey& face : linkA.faces)
        if (linkB.faces.count(face)) return false;

    int degreeOneCount = 0;
    for (const auto& item : oppositeGraph) {
        const size_t degree = item.second.size();
        if (degree == 1) ++degreeOneCount;
        else if (degree != 2) return false;
    }
    if (IsBoundaryEdge(va, vb)) {
        if (degreeOneCount != 2) return false; // 边界边的 link 必须是一条路径
    } else if (degreeOneCount != 0) {
        return false; // 内部边的 link 必须首尾闭合成环
    }

    // 只检查度数无法排除两条互不相连的路径或两个独立环，因此还要显式遍历连通性。
    VertexSet visited;
    std::vector<int> stack{oppositeGraph.begin()->first};
    while (!stack.empty()) {
        const int v = stack.back();
        stack.pop_back();
        if (!visited.insert(v).second) continue;
        for (int neighbor : oppositeGraph.at(v)) stack.push_back(neighbor);
    }
    if (visited.size() != oppositeGraph.size()) return false;

    // 在完整局部星形邻域中模拟连接关系，并拒绝重复四面体。将 vb 替换成 va 后
    // 新产生的重复四面体一定包含 va，因此检查两个端点星形邻域的并集即可。
    struct TetKey {
        std::array<int, 4> v;
        bool operator==(const TetKey& o) const { return v == o.v; }
    };
    struct TetKeyHash {
        size_t operator()(const TetKey& tet) const noexcept {
            size_t h = 0;
            for (int v : tet.v)
                h ^= std::hash<int>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_set<int> localTetIds;
    for (int ti : m_VertTets[va]) if (m_TetAlive[ti]) localTetIds.insert(ti);
    for (int ti : m_VertTets[vb]) if (m_TetAlive[ti]) localTetIds.insert(ti);
    std::unordered_set<TetKey, TetKeyHash> simulatedTets;
    for (int ti : localTetIds) {
        const int base = ti * 4;
        bool hasA = false, hasB = false;
        TetKey key{{tv[base], tv[base + 1], tv[base + 2], tv[base + 3]}};
        for (int& v : key.v) {
            hasA |= v == va;
            hasB |= v == vb;
            if (v == vb) v = va;
        }
        if (hasA && hasB) continue; // 该四面体会按预期退化并被删除
        std::sort(key.v.begin(), key.v.end());
        if (std::adjacent_find(key.v.begin(), key.v.end()) != key.v.end()) return false;
        if (!simulatedTets.insert(key).second) return false;
    }
    return true;
}

bool TetraEdgeSimplification::IsTetGeometryValid(
        int va, int vb, const double pos[3]) const {
    const int* tv = m_TetVerts.data();
    const double* pts = m_Pts.data();
    std::unordered_set<int> incident;
    for (int ti : m_VertTets[va]) if (m_TetAlive[ti]) incident.insert(ti);
    for (int ti : m_VertTets[vb]) if (m_TetAlive[ti]) incident.insert(ti);

    auto signedVolume6 = [](const double p[4][3]) {
        const double ax=p[1][0]-p[0][0], ay=p[1][1]-p[0][1], az=p[1][2]-p[0][2];
        const double bx=p[2][0]-p[0][0], by=p[2][1]-p[0][1], bz=p[2][2]-p[0][2];
        const double cx=p[3][0]-p[0][0], cy=p[3][1]-p[0][1], cz=p[3][2]-p[0][2];
        return ax*(by*cz-bz*cy) - ay*(bx*cz-bz*cx) + az*(bx*cy-by*cx);
    };

    for (int ti : incident) {
        const int base = ti * 4;
        bool hasA = false, hasB = false;
        double before[4][3], after[4][3];
        for (int j = 0; j < 4; ++j) {
            const int v = tv[base + j];
            hasA |= v == va;
            hasB |= v == vb;
            for (int d = 0; d < 3; ++d) before[j][d] = after[j][d] = pts[v * 3 + d];
            if (v == va || v == vb)
                for (int d = 0; d < 3; ++d) after[j][d] = pos[d];
        }
        if (hasA && hasB) continue; // these tets collapse away by design

        const double oldVolume = signedVolume6(before);
        const double newVolume = signedVolume6(after);
        // 体积同号可防止翻转；绝对值下限可防止四面体被压成数值上的零体积单元。
        if (!std::isfinite(oldVolume) || !std::isfinite(newVolume) ||
            oldVolume * newVolume <= 0.0 || std::abs(newVolume) < 1e-15)
            return false;
    }
    return true;
}

bool TetraEdgeSimplification::IsBoundaryGeometryValid(
        int va, int vb, const double pos[3]) const {
    // 纯内部边坍缩没有需要额外保护的表面三角形。
    if (!m_IsBoundary[va] && !m_IsBoundary[vb]) return true;

    std::unordered_set<FaceKey, FaceKeyHash> incidentFaces;
    for (const FaceKey& face : m_VertBoundaryFaces[va]) incidentFaces.insert(face);
    for (const FaceKey& face : m_VertBoundaryFaces[vb]) incidentFaces.insert(face);

    std::unordered_set<FaceKey, FaceKeyHash> simulatedFaces;
    const double* pts = m_Pts.data();
    for (const FaceKey& face : incidentFaces) {
        const bool hasA = face.a == va || face.b == va || face.c == va;
        const bool hasB = face.a == vb || face.b == vb || face.c == vb;
        if (hasA && hasB) continue; // 与流形边界边相邻的两个三角形应当随坍缩消失

        int ids[3] = {face.a, face.b, face.c};
        double before[3][3], after[3][3];
        for (int j = 0; j < 3; ++j) {
            const int v = ids[j];
            for (int d = 0; d < 3; ++d) before[j][d] = after[j][d] = pts[v * 3 + d];
            if (v == va || v == vb) {
                ids[j] = va;
                for (int d = 0; d < 3; ++d) after[j][d] = pos[d];
            }
        }

        const FaceKey newFace = MakeFaceKey(ids[0], ids[1], ids[2]);
        if (newFace.a == newFace.b || newFace.b == newFace.c) return false;
        if (!simulatedFaces.insert(newFace).second) return false;

        auto normal = [](const double p[3][3], double n[3]) {
            const double ux=p[1][0]-p[0][0], uy=p[1][1]-p[0][1], uz=p[1][2]-p[0][2];
            const double vx=p[2][0]-p[0][0], vy=p[2][1]-p[0][1], vz=p[2][2]-p[0][2];
            n[0]=uy*vz-uz*vy; n[1]=uz*vx-ux*vz; n[2]=ux*vy-uy*vx;
        };
        double oldNormal[3], newNormal[3];
        normal(before, oldNormal);
        normal(after, newNormal);
        const double oldLength = std::sqrt(oldNormal[0]*oldNormal[0] + oldNormal[1]*oldNormal[1] + oldNormal[2]*oldNormal[2]);
        const double newLength = std::sqrt(newNormal[0]*newNormal[0] + newNormal[1]*newNormal[1] + newNormal[2]*newNormal[2]);
        if (oldLength < 1e-30 || newLength < 1e-15) return false;
        const double normalDot = (oldNormal[0]*newNormal[0] + oldNormal[1]*newNormal[1] + oldNormal[2]*newNormal[2]) /
                                 (oldLength * newLength);
        if (!std::isfinite(normalDot) || normalDot < m_MinBoundaryNormalDot) return false;
    }
    return true;
}

double TetraEdgeSimplification::BoundaryQuadricCost(
        int va, int vb, const double pos[3]) const {
    const double x[4] = {pos[0], pos[1], pos[2], 1.0};
    double cost = 0.0;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            const double q = m_BoundaryQ[va * 16 + r * 4 + c] +
                             m_BoundaryQ[vb * 16 + r * 4 + c];
            cost += x[r] * q * x[c];
        }
    }
    return std::max(0.0, cost); // 消除浮点舍入造成的微小负数
}

// ═══════════ EdgeCostFast ═══════════
double TetraEdgeSimplification::EdgeCostFast(int va, int vb) const {
    if (m_PreserveBoundary) {
        const bool boundaryA = m_IsBoundary[va] != 0;
        const bool boundaryB = m_IsBoundary[vb] != 0;

        // 边界保护模式明确区分三种情况：
        // 1）内部点—内部点：正常坍缩；
        // 2）真实边界边上的边界点—边界点：受约束坍缩；
        // 3）边界点—内部点，或两个边界点之间的内部弦边：拒绝，
        //    因为它们可能把外壳拉向内部，或把两个外壳区域错误粘合。
        if (boundaryA != boundaryB || (boundaryA && !IsBoundaryEdge(va, vb)))
            return std::numeric_limits<double>::infinity();
    }

    // Lower bound: minimize over the same positions used by full validation.
    // Geometry/topology can reject positions later, but cannot lower this cost.
    const double* pts = m_Pts.data();
    double best = std::numeric_limits<double>::infinity();
    const bool boundary=m_PreserveBoundary && m_IsBoundary[va] && m_IsBoundary[vb];
    for(int candidate=boundary?0:2;candidate<3;++candidate) {
        const double t=candidate==0?0.0:candidate==1?1.0:0.5;
        double pos[3],da[3],db[3];
        for(int d=0;d<3;++d) {
            pos[d]=(1-t)*pts[va*3+d]+t*pts[vb*3+d];
            da[d]=pos[d]-pts[va*3+d];db[d]=pos[d]-pts[vb*3+d];
        }
        double cost=Sym3_vAv(&m_ADQ[va*9],da[0],da[1],da[2])
                   +Sym3_vAv(&m_ADQ[vb*9],db[0],db[1],db[2])
                   +std::max(m_ErrAccum[va],m_ErrAccum[vb]);
        if(boundary)cost+=m_BoundaryPenalty*BoundaryQuadricCost(va,vb,pos);
        if(std::isfinite(cost))best=std::min(best,cost);
    }
    return best;
}

// ═══════════ ComputeEdgeCost — with flip detection ═══════════
TetraEdgeSimplification::EdgeCostResult
TetraEdgeSimplification::ComputeEdgeCost(int va, int vb) {
    EdgeCostResult res;
    res.valid = false;
    res.cost = std::numeric_limits<double>::infinity();

    const bool boundaryA = m_IsBoundary[va] != 0;
    const bool boundaryB = m_IsBoundary[vb] != 0;
    if (m_PreserveBoundary) {
        if (boundaryA != boundaryB) return res;
        if (boundaryA && !IsBoundaryEdge(va, vb)) return res;
    }
    if (!IsTopologicallyCollapsible(va, vb)) return res;

    const double* pts = m_Pts.data();
    const int D = m_AttrDim;

    // 受保护边界边不能使用任意三维最优位置。这里测试两个端点和边中点，
    // 三者都位于当前分片线性边界边上；再由边界二次误差选择最符合原始边界
    // 平面的位置。内部边继续保持原有的中点策略。
    const double candidates[3][3] = {
        {pts[va*3], pts[va*3+1], pts[va*3+2]},
        {pts[vb*3], pts[vb*3+1], pts[vb*3+2]},
        {(pts[va*3]+pts[vb*3])*0.5,
         (pts[va*3+1]+pts[vb*3+1])*0.5,
         (pts[va*3+2]+pts[vb*3+2])*0.5}
    };
    const bool constrainedBoundary = m_PreserveBoundary && boundaryA && boundaryB;
    const int firstCandidate = constrainedBoundary ? 0 : 2;
    double selectedT=0.5;
    for (int candidate = firstCandidate; candidate < 3; ++candidate) {
        const double* pos = candidates[candidate];
        if (!IsTetGeometryValid(va, vb, pos)) continue;
        if (constrainedBoundary && !IsBoundaryGeometryValid(va, vb, pos)) continue;
        if (constrainedBoundary) {
            const double count=m_BoundaryPlaneCount[va]+m_BoundaryPlaneCount[vb];
            const double error=BoundaryQuadricCost(va,vb,pos);
            // Hard budget, independent of heap ranking and target point count.
            if(!std::isfinite(error) || count<=0 ||
               error > count*m_MaxBoundaryRmsError*m_MaxBoundaryRmsError + 1e-14)continue;
        }

        const double da[3] = {pos[0]-pts[va*3], pos[1]-pts[va*3+1], pos[2]-pts[va*3+2]};
        const double db[3] = {pos[0]-pts[vb*3], pos[1]-pts[vb*3+1], pos[2]-pts[vb*3+2]};
        double cost = Sym3_vAv(&m_ADQ[va*9], da[0], da[1], da[2])
                    + Sym3_vAv(&m_ADQ[vb*9], db[0], db[1], db[2])
                    + std::max(m_ErrAccum[va], m_ErrAccum[vb]);
        if (constrainedBoundary)
            cost += m_BoundaryPenalty * BoundaryQuadricCost(va, vb, pos);

        if (cost < res.cost) {
            selectedT=candidate==0?0.0:candidate==1?1.0:0.5;
            res.cost = cost;
            res.optPos[0] = pos[0];
            res.optPos[1] = pos[1];
            res.optPos[2] = pos[2];
            res.valid = true;
        }
    }
    if (!res.valid) return res;

    // Interpolate at the selected geometric position, preserving affine fields.
    if (D > 0) {
        res.optAttr.resize(D);
        const double* aa=&m_Attrs[va*D];const double* ab=&m_Attrs[vb*D];
        for(int d=0;d<D;++d)res.optAttr[d]=(1-selectedT)*aa[d]+selectedT*ab[d];
    }
    return res;
}

// ═══════════ BuildEdgesAndHeap ═══════════
void TetraEdgeSimplification::BuildEdgesAndHeap() {
    while (!m_Heap.empty()) m_Heap.pop();

    // Extract unique edges from all alive tets
    struct EK { int a, b; bool operator==(const EK& o) const { return a==o.a && b==o.b; } };
    struct EKH { size_t operator()(const EK& e) const {
        return std::hash<int64_t>()((int64_t(e.a)<<32)|int64_t(e.b)); }};
    std::unordered_set<EK, EKH> seen;
    seen.reserve(m_NumTets * 6);

    const int* tv = m_TetVerts.data();
    int count = 0;

    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        int b = ti * 4;
        int v[4] = {tv[b],tv[b+1],tv[b+2],tv[b+3]};
        for (int i = 0; i < 3; ++i) {
            for (int j = i+1; j < 4; ++j) {
                int a = v[i], bb = v[j];
                if (a > bb) std::swap(a, bb);
                if (!seen.insert({a,bb}).second) continue;
                // EdgeCostFast 负责边界分类。保护模式下，真实边界边仍保留为候选，
                // 边界点—内部点的混合边和两个边界点之间的内部弦边会被拒绝。
                double c = EdgeCostFast(a, bb);
                if (c < std::numeric_limits<double>::infinity()) {
                    m_Heap.push({c, a, bb, m_VertVersion[a], m_VertVersion[bb]});
                    count++;
                }
            }
        }
    }
    std::cout << "[TetraEdgeSimp] " << count << " edge candidates in heap\n";
}

// ═══════════ DoEdgeCollapse ═══════════
std::vector<int> TetraEdgeSimplification::DoEdgeCollapse(
        int va, int vb, const double optPos[3],
        const std::vector<double>& optAttr) {
    const int survivor = va;
    const int removed = vb;
    const int D = m_AttrDim;
    const double* pts = m_Pts.data();
    int* tv = m_TetVerts.data();

    // 只有与任一端点相邻的四面体会发生变化。修改连接关系之前，先从邻接表中
    // 删除这些四面体的旧面；修改完成后，再用新面键插回仍然存活的四面体。
    // 这样无需重新扫描整个网格，也能让边界面、边界边和边界点保持最新。
    std::unordered_set<int> affectedTets;
    std::unordered_set<int> affectedVertices;
    for (int ti : m_VertTets[survivor]) if (m_TetAlive[ti]) affectedTets.insert(ti);
    for (int ti : m_VertTets[removed]) if (m_TetAlive[ti]) affectedTets.insert(ti);
    for (int ti : affectedTets) {
        const int base = ti * 4;
        for (int j = 0; j < 4; ++j) affectedVertices.insert(tv[base + j]);
    }

    static const int faceIndices[4][3] = {{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    for (int ti : affectedTets) {
        const int base = ti * 4;
        for (const auto& face : faceIndices) {
            ChangeFaceCount(MakeFaceKey(tv[base + face[0]],
                                       tv[base + face[1]],
                                       tv[base + face[2]]), -1);
        }
    }

    // 必须在合并 va、vb 的 ADQ 之前计算累计误差；如果合并后再算，
    // errA 会把 vb 的矩阵重复计算一次。
    double da[3] = {optPos[0]-pts[va*3], optPos[1]-pts[va*3+1], optPos[2]-pts[va*3+2]};
    double db[3] = {optPos[0]-pts[vb*3], optPos[1]-pts[vb*3+1], optPos[2]-pts[vb*3+2]};
    double errA = m_ErrAccum[va] + Sym3_vAv(&m_ADQ[va*9], da[0],da[1],da[2]);
    double errB = m_ErrAccum[vb] + Sym3_vAv(&m_ADQ[vb*9], db[0],db[1],db[2]);

    // 合并 ADQ：Q_s = Q_a + Q_b
    double* qs = &m_ADQ[survivor*9];
    const double* qr = &m_ADQ[removed*9];
    for (int i = 0; i < 9; ++i) qs[i] += qr[i];

    m_ErrAccum[survivor] = std::max(errA, errB);

    // 边界二次误差矩阵也需要累加，原因与普通 QEM 相同：保留点必须继续代表
    // 原先由两个端点共同代表的全部原始边界平面。
    double* boundaryQs = &m_BoundaryQ[survivor * 16];
    const double* boundaryQr = &m_BoundaryQ[removed * 16];
    for (int i = 0; i < 16; ++i) boundaryQs[i] += boundaryQr[i];
    m_BoundaryPlaneCount[survivor] += m_BoundaryPlaneCount[removed];

    // Update position & attributes
    m_Pts[survivor*3]   = optPos[0];
    m_Pts[survivor*3+1] = optPos[1];
    m_Pts[survivor*3+2] = optPos[2];
    if (D > 0 && static_cast<int>(optAttr.size()) == D) {
        double* as = &m_Attrs[survivor*D];
        for (int d = 0; d < D; ++d) as[d] = optAttr[d];
    }

    // Kill tets that contain BOTH va and vb
    for (int ti : m_VertTets[removed]) {
        if (!m_TetAlive[ti]) continue;
        int nb = ti*4;
        bool hasSurvivor = false;
        for (int j = 0; j < 4; ++j) {
            if (tv[nb+j] == survivor) hasSurvivor = true;
        }
        if (hasSurvivor) {
            // This tet has both endpoints → kill it
            m_TetAlive[ti] = 0;
        } else {
            // Redirect removed → survivor
            for (int j = 0; j < 4; ++j) {
                if (tv[nb+j] == removed) tv[nb+j] = survivor;
            }
            m_VertTets[survivor].push_back(ti);

            // Degenerate check after redirect
            int u0=tv[nb],u1=tv[nb+1],u2=tv[nb+2],u3=tv[nb+3];
            if (u0==u1||u0==u2||u0==u3||u1==u2||u1==u3||u2==u3)
                m_TetAlive[ti] = 0;
        }
    }

    // Mark removed vertex as dead
    m_VertAlive[removed] = 0;
    m_VertTets[removed].clear();

    // 使用新的连接关系重新插入存活的局部四面体。面计数的变化会自动暴露
    // 新边界面，或把重新被两个四面体共享的面隐藏为内部面。
    for (int ti : affectedTets) {
        if (!m_TetAlive[ti]) continue;
        const int base = ti * 4;
        for (const auto& face : faceIndices) {
            ChangeFaceCount(MakeFaceKey(tv[base + face[0]],
                                       tv[base + face[1]],
                                       tv[base + face[2]]), 1);
        }
        for (int j = 0; j < 4; ++j) affectedVertices.insert(tv[base + j]);
    }

    // 与该局部区域相接的堆条目，其拓扑、边界分类或代价都可能已经过期。
    // 增加版本号可惰性作废旧条目，随后 Simplify() 会立即插入重新计算的局部边。
    affectedVertices.insert(survivor);
    affectedVertices.insert(removed);
    for (int v : affectedVertices) ++m_VertVersion[v];

    // m_IsBoundaryTet 虽然只是缓存，仍需保持正确，以便诊断和后续代码使用。
    for (int ti : affectedTets) {
        if (!m_TetAlive[ti]) continue;
        int nb = ti*4;
        m_IsBoundaryTet[ti] = m_IsBoundary[tv[nb]]|m_IsBoundary[tv[nb+1]]|
                              m_IsBoundary[tv[nb+2]]|m_IsBoundary[tv[nb+3]];
    }

    std::vector<int> aliveAffected;
    aliveAffected.reserve(affectedVertices.size());
    for (int v : affectedVertices)
        if (m_VertAlive[v]) aliveAffected.push_back(v);
    return aliveAffected;
}

// ═══════════ Simplify ═══════════
void TetraEdgeSimplification::Simplify() {
    int N0 = 0;
    for (int i = 0; i < m_NumVerts; ++i) N0 += m_VertAlive[i];

    int target;
    if (m_TargetTetraCount > 0) {
        target = m_TargetTetraCount;
    } else {
        target = std::max(4, static_cast<int>(N0 * m_TargetReduction));
    }

    std::cout << "[TetraEdgeSimp] Simplifying: " << N0 << " verts -> target " << target << "\n";

    BuildEdgesAndHeap();

    int collapsed = 0;
    int boundaryCollapsed = 0;
    int vertCount = N0;

    while (!m_Heap.empty() && vertCount > target) {
        EdgeHeapEntry entry = m_Heap.top();
        m_Heap.pop();

        // Lazy deletion: check vertex versions
        if (!m_VertAlive[entry.va] || !m_VertAlive[entry.vb]) continue;
        if (m_VertVersion[entry.va] != entry.versionA ||
            m_VertVersion[entry.vb] != entry.versionB) continue;

        // 完整代价检查包括 link condition、四面体朝向、边界三角形合法性，
        // 以及受保护边界的位置约束和误差检查。
        EdgeCostResult cr = ComputeEdgeCost(entry.va, entry.vb);
        if (!cr.valid) continue;
        // Refine a lower-bound entry and let cheaper entries be validated first.
        if(!m_Heap.empty() && cr.cost > m_Heap.top().cost) {
            m_Heap.push({cr.cost,entry.va,entry.vb,entry.versionA,entry.versionB});
            continue;
        }

        const bool isProtectedBoundaryCollapse =
            m_PreserveBoundary && m_IsBoundary[entry.va] && m_IsBoundary[entry.vb];
        const std::vector<int> affectedVertices =
            DoEdgeCollapse(entry.va, entry.vb, cr.optPos, cr.optAttr);
        collapsed++;
        if (isProtectedBoundaryCollapse) ++boundaryCollapsed;
        vertCount -= 1;  // edge collapse: 2 verts → 1

        if (collapsed % 2000 == 0) {
            int nv=0, nt=0;
            for(int i=0;i<m_NumVerts;++i) nv+=m_VertAlive[i];
            for(int i=0;i<m_NumTets;++i) nt+=m_TetAlive[i];
            std::cout << "  [" << collapsed << "] " << nv << " verts, "
                      << nt << " tets, cost=" << cr.cost << "\n";
        }

        // 边界状态的变化不只发生在保留点的邻边上，例如删除四面体可能暴露其对面。
        // 因此要重新插入变化局部区域中的所有存活边，而不能只处理保留点的邻边。
        const int* tv = m_TetVerts.data();
        std::unordered_set<uint64_t> localEdges;
        std::unordered_set<int> changed(affectedVertices.begin(),affectedVertices.end());
        for (int vertex : affectedVertices) {
            for (int ti : m_VertTets[vertex]) {
                if (!m_TetAlive[ti]) continue;
                const int base = ti * 4;
                for (int i = 0; i < 3; ++i) {
                    for (int j = i + 1; j < 4; ++j) {
                        const int a = tv[base + i];
                        const int b = tv[base + j];
                        if (m_VertAlive[a] && m_VertAlive[b] && (changed.count(a) || changed.count(b)))
                            localEdges.insert(MakeEdgeKey(a, b));
                    }
                }
            }
        }

        for (uint64_t edge : localEdges) {
            const int a = int(uint32_t(edge >> 32));
            const int b = int(uint32_t(edge));
            double c = EdgeCostFast(a, b);
            if (c < std::numeric_limits<double>::infinity()) {
                m_Heap.push({c, a, b, m_VertVersion[a], m_VertVersion[b]});
            }
        }
    }

    int nv=0, nt=0;
    for(int i=0;i<m_NumVerts;++i) nv+=m_VertAlive[i];
    for(int i=0;i<m_NumTets;++i) nt+=m_TetAlive[i];
    std::cout << "[TetraEdgeSimp] Done: " << collapsed << " collapses, "
              << boundaryCollapsed << " protected boundary collapses, "
              << nv << " verts, " << nt << " tets\n";
}

// ═══════════ SaveMesh ═══════════
bool TetraEdgeSimplification::SaveMesh() {
    const int N=m_NumVerts, D=m_AttrDim;
    const int* tv = m_TetVerts.data();

    std::vector<int> old2new(N, -1);
    int newIdx = 0;
    for (int i = 0; i < N; ++i) if (m_VertAlive[i]) old2new[i] = newIdx++;
    const int newN = newIdx;

    auto outMesh = VolumeMesh::New();
    outMesh->SetName(m_InputMesh->GetName() + "_edge_simplified");

    auto outPoints = Points::New();
    for (int i = 0; i < N; ++i) {
        if (!m_VertAlive[i]) continue;
        outPoints->AddPoint(Point(
            m_Pts[i*3]*m_PtsScale+m_PtsMin[0],
            m_Pts[i*3+1]*m_PtsScale+m_PtsMin[1],
            m_Pts[i*3+2]*m_PtsScale+m_PtsMin[2]));
    }
    outMesh->SetPoints(outPoints);
    std::vector<igIndex> outCellSourceIds;
    auto outCells = CellArray::New();
    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        int b=ti*4;
        int m0=old2new[tv[b]],m1=old2new[tv[b+1]],m2=old2new[tv[b+2]],m3=old2new[tv[b+3]];
        if(m0<0||m1<0||m2<0||m3<0) continue;
        if(m0==m1||m0==m2||m0==m3||m1==m2||m1==m3||m2==m3) continue;
        outCells->AddCellId4(m0,m1,m2,m3);
        outCellSourceIds.push_back(m_TetSourceIds[ti]);
    }
    outMesh->SetVolumes(outCells);

    auto outAttrs = AttributeSet::New();
    if (D > 0) {
        for (const auto& p : m_AttrNormParams) {
            if (p.isScalar) {
                for(int i=0;i<N;++i){if(!m_VertAlive[i])continue; m_Attrs[i*D+p.col]=m_Attrs[i*D+p.col]*p.range+p.minVal;}
            } else {
                for(int i=0;i<N;++i){if(!m_VertAlive[i])continue; for(int d=0;d<p.ncomp;++d)m_Attrs[i*D+p.col+d]*=p.maxMag;}
            }
        }
        int col = 0;
        for (const auto& info: m_AttrInfo) {
            ArrayObject::Pointer arr = nullptr;
            const int attrCol = col;
            col += info.ncomp;
            switch (info.arrayType) {
                case IG_FloatArray:
                    arr = BuildPointAttribute<FloatArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_DoubleArray:
                    arr = BuildPointAttribute<DoubleArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_IntArray:
                    arr = BuildPointAttribute<IntArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedIntArray:
                    arr = BuildPointAttribute<UnsignedIntArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_CharArray:
                    arr = BuildPointAttribute<CharArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedCharArray:
                    arr = BuildPointAttribute<UnsignedCharArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_ShortArray:
                    arr = BuildPointAttribute<ShortArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedShortArray:
                    arr = BuildPointAttribute<UnsignedShortArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_LongLongArray:
                    arr = BuildPointAttribute<LongLongArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                case IG_UnsignedLongLongArray:
                    arr = BuildPointAttribute<UnsignedLongLongArray, TetraEdgeSimplification::AttrInfo>(
                            info, outAttrs, attrCol, N, newN, m_VertAlive, old2new, m_Attrs, D);
                    break;

                default:
                    break;
            }
            if (arr) { outAttrs->AddAttribute(info.attributeType, IG_POINT, arr); }
        }
    }

    auto inputAttrs = m_InputMesh->GetAttributeSet();
    if (inputAttrs) {
        auto all = inputAttrs->GetAllAttributes();
        for (IGsize ai = 0; ai < all->GetNumberOfElements(); ++ai) {
            auto attr = all->GetElement(ai);
            if (attr.isDeleted || !attr.pointer) continue;
            if (attr.attachmentType != IG_CELL) continue;
            ArrayObject::Pointer arr = nullptr;
            switch (attr.pointer->GetArrayType()) {
                case IG_FloatArray:
                    arr = BuildCellAttribute<FloatArray>(attr, outCellSourceIds);
                    break;

                case IG_DoubleArray:
                    arr = BuildCellAttribute<DoubleArray>(attr, outCellSourceIds);
                    break;

                case IG_IntArray:
                    arr = BuildCellAttribute<IntArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedIntArray:
                    arr = BuildCellAttribute<UnsignedIntArray>(attr, outCellSourceIds);
                    break;

                case IG_CharArray:
                    arr = BuildCellAttribute<CharArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedCharArray:
                    arr = BuildCellAttribute<UnsignedCharArray>(attr, outCellSourceIds);
                    break;

                case IG_ShortArray:
                    arr = BuildCellAttribute<ShortArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedShortArray:
                    arr = BuildCellAttribute<UnsignedShortArray>(attr, outCellSourceIds);
                    break;

                case IG_LongLongArray:
                    arr = BuildCellAttribute<LongLongArray>(attr, outCellSourceIds);
                    break;

                case IG_UnsignedLongLongArray:
                    arr = BuildCellAttribute<UnsignedLongLongArray>(attr, outCellSourceIds);
                    break;

                default:
                    break;
            }
            if (arr) { outAttrs->AddAttribute(attr.type, IG_CELL, arr); }
        }
    }
    outMesh->SetAttributeSet(outAttrs);

    SetOutput(outMesh);
    std::cout << "[TetraEdgeSimp] Output: " << newN << " verts, "
              << outCells->GetNumberOfCells() << " tets\n";
    return true;
}

IGAME_NAMESPACE_END
