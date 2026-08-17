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
    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    m_NumTets = 0;
    for (IGsize ci = 0; ci < nVol; ++ci) {
        int n = m_InputMesh->GetVolumePointIds(ci, ids);
        if (n != 4) continue;
        m_TetVerts.push_back(static_cast<int>(ids[0]));
        m_TetVerts.push_back(static_cast<int>(ids[1]));
        m_TetVerts.push_back(static_cast<int>(ids[2]));
        m_TetVerts.push_back(static_cast<int>(ids[3]));
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
            if (a.isDeleted || !a.pointer) continue;
            if (a.attachmentType != IG_POINT) continue;
            if (!m_UseAllPointAttributes) {
                int curIdx = m_InputMesh->GetCurrentAttributeIndex();
                if (static_cast<int>(ai) != curIdx) continue;
            }
            int dim = a.pointer->GetDimension();
            m_AttrInfo.push_back({a.pointer->GetName(), dim});
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
        if (m_IsBoundary[v[k]]) return std::numeric_limits<double>::infinity();
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
        if (m_IsBoundary[t[k]]) return res;
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
        if (!m_TetAlive[ti] || m_IsBoundaryTet[ti]) continue;
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
                if (m_TetAlive[ni] && !m_IsBoundaryTet[ni]) {
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
    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        int base = ti * 4;
        int m0 = old2new[tv[base]], m1 = old2new[tv[base+1]],
            m2 = old2new[tv[base+2]], m3 = old2new[tv[base+3]];
        if (m0 < 0 || m1 < 0 || m2 < 0 || m3 < 0) continue;
        if (m0==m1 || m0==m2 || m0==m3 || m1==m2 || m1==m3 || m2==m3) continue;
        outCells->AddCellId4(m0, m1, m2, m3);
    }
    outMesh->SetVolumes(outCells);

    // Attributes (denormalized)
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

        auto outAttrs = AttributeSet::New();
        int col = 0;
        for (const auto& info : m_AttrInfo) {
            auto arr = FloatArray::New();
            arr->SetDimension(info.ncomp);
            arr->Resize(newN);
            arr->SetName(info.name);
            for (int i = 0; i < N; ++i) {
                if (!m_VertAlive[i]) continue;
                int ni = old2new[i];
                for (int d = 0; d < info.ncomp; ++d) {
                    arr->SetValue(ni * info.ncomp + d,
                                  static_cast<float>(m_Attrs[i*D + col + d]));
                }
            }
            outAttrs->AddAttribute(info.ncomp == 1 ? IG_SCALAR : IG_VECTOR, IG_POINT, arr);
            col += info.ncomp;
        }
        outMesh->SetAttributeSet(outAttrs);
    }

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
    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    m_NumTets = 0;
    for (IGsize ci = 0; ci < nVol; ++ci) {
        int n = m_InputMesh->GetVolumePointIds(ci, ids);
        if (n != 4) continue;
        for (int j = 0; j < 4; ++j) m_TetVerts.push_back(static_cast<int>(ids[j]));
        m_NumTets++;
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
            m_AttrInfo.push_back({a.pointer->GetName(), dim});
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

    // Boundary
    struct FK3{int a,b,c; bool operator==(const FK3& o)const{return a==o.a&&b==o.b&&c==o.c;}};
    struct FK3H{size_t operator()(const FK3& k)const noexcept{
        uint64_t h=uint64_t(uint32_t(k.a))*0x9E3779B185EBCA87ull;
        h^=uint64_t(uint32_t(k.b))+0x9E3779B185EBCA87ull+(h<<6)+(h>>2);
        h^=uint64_t(uint32_t(k.c))+0x9E3779B185EBCA87ull+(h<<6)+(h>>2);
        return size_t(h);}};
    std::unordered_map<FK3,int,FK3H> fc; fc.reserve(M*4);
    static const int fi[4][3]={{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    for (int ti=0;ti<M;++ti){int b=ti*4;
        for(int f=0;f<4;++f){int a=tv[b+fi[f][0]],bb=tv[b+fi[f][1]],c=tv[b+fi[f][2]];
            if(a>bb)std::swap(a,bb);if(bb>c)std::swap(bb,c);if(a>bb)std::swap(a,bb);
            fc[{a,bb,c}]++;}}
    m_IsBoundary.assign(N,0);
    for(auto& kv:fc) if(kv.second==1){m_IsBoundary[kv.first.a]=1;m_IsBoundary[kv.first.b]=1;m_IsBoundary[kv.first.c]=1;}
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

// ═══════════ EdgeCostFast ═══════════
double TetraEdgeSimplification::EdgeCostFast(int va, int vb) const {
    if (m_IsBoundary[va] || m_IsBoundary[vb])
        return std::numeric_limits<double>::infinity();

    const double* pts = m_Pts.data();
    // midpoint
    double mx = (pts[va*3]+pts[vb*3])*0.5;
    double my = (pts[va*3+1]+pts[vb*3+1])*0.5;
    double mz = (pts[va*3+2]+pts[vb*3+2])*0.5;

    double da[3]={mx-pts[va*3], my-pts[va*3+1], mz-pts[va*3+2]};
    double db[3]={mx-pts[vb*3], my-pts[vb*3+1], mz-pts[vb*3+2]};
    double cost = Sym3_vAv(&m_ADQ[va*9], da[0],da[1],da[2])
                + Sym3_vAv(&m_ADQ[vb*9], db[0],db[1],db[2]);
    return cost + std::max(m_ErrAccum[va], m_ErrAccum[vb]);
}

// ═══════════ ComputeEdgeCost — with flip detection ═══════════
TetraEdgeSimplification::EdgeCostResult
TetraEdgeSimplification::ComputeEdgeCost(int va, int vb) {
    EdgeCostResult res;
    res.valid = false;
    res.cost = std::numeric_limits<double>::infinity();

    if (m_IsBoundary[va] || m_IsBoundary[vb]) return res;

    const double* pts = m_Pts.data();
    const int D = m_AttrDim;

    // Optimal position: midpoint
    res.optPos[0] = (pts[va*3]+pts[vb*3])*0.5;
    res.optPos[1] = (pts[va*3+1]+pts[vb*3+1])*0.5;
    res.optPos[2] = (pts[va*3+2]+pts[vb*3+2])*0.5;

    // Optimal attributes: weighted average by inverse ADQ trace
    if (D > 0) {
        double wa = 1.0/std::max(Sym3_trace(&m_ADQ[va*9]), 1e-30);
        double wb = 1.0/std::max(Sym3_trace(&m_ADQ[vb*9]), 1e-30);
        double invW = 1.0/(wa+wb);
        wa *= invW; wb *= invW;
        res.optAttr.resize(D);
        const double* aa = &m_Attrs[va*D];
        const double* ab = &m_Attrs[vb*D];
        for (int d = 0; d < D; ++d) res.optAttr[d] = wa*aa[d] + wb*ab[d];
    }

    // ADQ cost
    double da[3] = {res.optPos[0]-pts[va*3], res.optPos[1]-pts[va*3+1], res.optPos[2]-pts[va*3+2]};
    double db[3] = {res.optPos[0]-pts[vb*3], res.optPos[1]-pts[vb*3+1], res.optPos[2]-pts[vb*3+2]};
    double totalCost = Sym3_vAv(&m_ADQ[va*9], da[0],da[1],da[2])
                     + Sym3_vAv(&m_ADQ[vb*9], db[0],db[1],db[2])
                     + std::max(m_ErrAccum[va], m_ErrAccum[vb]);

    // Flip detection: check all tets touching va or vb
    double ox=res.optPos[0], oy=res.optPos[1], oz=res.optPos[2];
    int* tv = m_TetVerts.data();

    // Check tets around va
    auto checkFlip = [&](int vertex) -> bool {
        for (int ti : m_VertTets[vertex]) {
            if (!m_TetAlive[ti]) continue;
            int nb = ti*4;
            int nt[4] = {tv[nb],tv[nb+1],tv[nb+2],tv[nb+3]};

            // Does this tet contain both va AND vb? It will be killed, skip.
            bool hasA = false, hasB = false;
            for (int j=0;j<4;++j) {
                if (nt[j]==va) hasA=true;
                if (nt[j]==vb) hasB=true;
            }
            if (hasA && hasB) continue;

            // This tet touches one of {va,vb} — check flip
            double p[4][3];
            for (int j=0;j<4;++j) {
                p[j][0]=pts[nt[j]*3]; p[j][1]=pts[nt[j]*3+1]; p[j][2]=pts[nt[j]*3+2];
            }

            // Before volume
            double e1[3]={p[1][0]-p[0][0],p[1][1]-p[0][1],p[1][2]-p[0][2]};
            double e2[3]={p[2][0]-p[0][0],p[2][1]-p[0][1],p[2][2]-p[0][2]};
            double e3[3]={p[3][0]-p[0][0],p[3][1]-p[0][1],p[3][2]-p[0][2]};
            double volB = e1[0]*(e2[1]*e3[2]-e2[2]*e3[1])
                        - e1[1]*(e2[0]*e3[2]-e2[2]*e3[0])
                        + e1[2]*(e2[0]*e3[1]-e2[1]*e3[0]);

            // Replace the vertex that's va or vb with optPos
            for (int j=0;j<4;++j) {
                if (nt[j]==va || nt[j]==vb) {
                    p[j][0]=ox; p[j][1]=oy; p[j][2]=oz;
                }
            }

            double ea2[3]={p[1][0]-p[0][0],p[1][1]-p[0][1],p[1][2]-p[0][2]};
            double eb2[3]={p[2][0]-p[0][0],p[2][1]-p[0][1],p[2][2]-p[0][2]};
            double ec2[3]={p[3][0]-p[0][0],p[3][1]-p[0][1],p[3][2]-p[0][2]};
            double volA = ea2[0]*(eb2[1]*ec2[2]-eb2[2]*ec2[1])
                        - ea2[1]*(eb2[0]*ec2[2]-eb2[2]*ec2[0])
                        + ea2[2]*(eb2[0]*ec2[1]-eb2[1]*ec2[0]);

            if (volB * volA < 0 || std::abs(volA) < 1e-30) return false;
        }
        return true;
    };

    if (!checkFlip(va) || !checkFlip(vb)) return res;

    res.cost = totalCost;
    res.valid = true;
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
                if (m_IsBoundary[a] || m_IsBoundary[bb]) continue;
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
void TetraEdgeSimplification::DoEdgeCollapse(int va, int vb,
                                              const double optPos[3],
                                              const std::vector<double>& optAttr) {
    const int survivor = va;
    const int removed = vb;
    const int D = m_AttrDim;
    const double* pts = m_Pts.data();
    int* tv = m_TetVerts.data();

    // Merge ADQ: Q_s = Q_a + Q_b
    double* qs = &m_ADQ[survivor*9];
    const double* qr = &m_ADQ[removed*9];
    for (int i = 0; i < 9; ++i) qs[i] += qr[i];

    // Accumulate error
    double da[3] = {optPos[0]-pts[va*3], optPos[1]-pts[va*3+1], optPos[2]-pts[va*3+2]};
    double db[3] = {optPos[0]-pts[vb*3], optPos[1]-pts[vb*3+1], optPos[2]-pts[vb*3+2]};
    double errA = m_ErrAccum[va] + Sym3_vAv(&m_ADQ[va*9], da[0],da[1],da[2]);
    double errB = m_ErrAccum[vb] + Sym3_vAv(&m_ADQ[vb*9], db[0],db[1],db[2]);
    m_ErrAccum[survivor] = std::max(errA, errB);

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

    // Bump versions for lazy heap invalidation
    m_VertVersion[survivor]++;
    m_VertVersion[removed]++;

    // Update boundary flags for survivor's tets
    for (int ti : m_VertTets[survivor]) {
        if (!m_TetAlive[ti]) continue;
        int nb = ti*4;
        if (m_IsBoundary[tv[nb]]|m_IsBoundary[tv[nb+1]]|
            m_IsBoundary[tv[nb+2]]|m_IsBoundary[tv[nb+3]])
            m_IsBoundaryTet[ti] = 1;
    }
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
    int vertCount = N0;

    while (!m_Heap.empty() && vertCount > target) {
        EdgeHeapEntry entry = m_Heap.top();
        m_Heap.pop();

        // Lazy deletion: check vertex versions
        if (!m_VertAlive[entry.va] || !m_VertAlive[entry.vb]) continue;
        if (m_VertVersion[entry.va] != entry.versionA ||
            m_VertVersion[entry.vb] != entry.versionB) continue;

        // Full cost with flip detection
        EdgeCostResult cr = ComputeEdgeCost(entry.va, entry.vb);
        if (!cr.valid) continue;

        DoEdgeCollapse(entry.va, entry.vb, cr.optPos, cr.optAttr);
        collapsed++;
        vertCount -= 1;  // edge collapse: 2 verts → 1

        if (collapsed % 2000 == 0) {
            int nv=0, nt=0;
            for(int i=0;i<m_NumVerts;++i) nv+=m_VertAlive[i];
            for(int i=0;i<m_NumTets;++i) nt+=m_TetAlive[i];
            std::cout << "  [" << collapsed << "] " << nv << " verts, "
                      << nt << " tets, cost=" << cr.cost << "\n";
        }

        // Re-enqueue edges from survivor to its neighbors
        int survivor = entry.va;
        if (!m_VertAlive[survivor]) continue;

        // Collect unique neighbor vertices
        const int* tv = m_TetVerts.data();
        // Use a simple scan — small per-vertex neighborhoods
        std::vector<int> neighbors;
        neighbors.reserve(64);
        for (int ti : m_VertTets[survivor]) {
            if (!m_TetAlive[ti]) continue;
            int nb = ti*4;
            for (int j = 0; j < 4; ++j) {
                int nv = tv[nb+j];
                if (nv != survivor && m_VertAlive[nv] && !m_IsBoundary[nv]) {
                    neighbors.push_back(nv);
                }
            }
        }
        // Deduplicate
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());

        for (int nb : neighbors) {
            int a = survivor < nb ? survivor : nb;
            int b = survivor < nb ? nb : survivor;
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

    auto outCells = CellArray::New();
    for (int ti = 0; ti < m_NumTets; ++ti) {
        if (!m_TetAlive[ti]) continue;
        int b=ti*4;
        int m0=old2new[tv[b]],m1=old2new[tv[b+1]],m2=old2new[tv[b+2]],m3=old2new[tv[b+3]];
        if(m0<0||m1<0||m2<0||m3<0) continue;
        if(m0==m1||m0==m2||m0==m3||m1==m2||m1==m3||m2==m3) continue;
        outCells->AddCellId4(m0,m1,m2,m3);
    }
    outMesh->SetVolumes(outCells);

    if (D > 0) {
        for (const auto& p : m_AttrNormParams) {
            if (p.isScalar) {
                for(int i=0;i<N;++i){if(!m_VertAlive[i])continue; m_Attrs[i*D+p.col]=m_Attrs[i*D+p.col]*p.range+p.minVal;}
            } else {
                for(int i=0;i<N;++i){if(!m_VertAlive[i])continue; for(int d=0;d<p.ncomp;++d)m_Attrs[i*D+p.col+d]*=p.maxMag;}
            }
        }
        auto outAttrs = AttributeSet::New();
        int col = 0;
        for (const auto& info : m_AttrInfo) {
            auto arr = FloatArray::New();
            arr->SetDimension(info.ncomp);
            arr->Resize(newN);
            arr->SetName(info.name);
            for (int i = 0; i < N; ++i) {
                if (!m_VertAlive[i]) continue;
                int ni = old2new[i];
                for (int d = 0; d < info.ncomp; ++d)
                    arr->SetValue(ni*info.ncomp+d, static_cast<float>(m_Attrs[i*D+col+d]));
            }
            outAttrs->AddAttribute(info.ncomp==1 ? IG_SCALAR : IG_VECTOR, IG_POINT, arr);
            col += info.ncomp;
        }
        outMesh->SetAttributeSet(outAttrs);
    }

    SetOutput(outMesh);
    std::cout << "[TetraEdgeSimp] Output: " << newN << " verts, "
              << outCells->GetNumberOfCells() << " tets\n";
    return true;
}

IGAME_NAMESPACE_END
