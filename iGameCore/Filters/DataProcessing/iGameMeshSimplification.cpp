#include "iGameMeshSimplification.h"



ManifoldSimplifier::ManifoldSimplifier(std::vector<FVector>& V, std::vector<int>& F,
                                        const std::vector<float>& Features, const std::vector<float>& Metrics)
    : Vertices(V), Indices(F), LatentFeatures(Features), MetricTensors(Metrics) 
{
    VertexRemoved.resize(V.size(), false);
    LatentDim = Features.size() / V.size();

    bHasLatent = LatentDim > 0;

    IndexCount = Indices.size();
    VertexCount = V.size();
    TargetCount = IndexCount / 4; // 简化到一半三角形数
}

// 入口函数
bool ManifoldSimplifier::Execute() 
{
    VertexAdjacency.Offsets.resize(VertexCount + 1);
    VertexAdjacency.Data.resize(IndexCount);
    VertexLocked.resize(VertexCount);
    VertexRemap.resize(VertexCount);
    Collapses.resize(IndexCount);
    CollapseOrder.resize(Collapses.size());

    InitializeQuadrics();

    while (IndexCount > TargetCount) 
    { 
        mmin = 1000000;
        mmax = 0;
        BuildAdjacency();
        size_t Count = BuildEdgeCollapses();
        std::cout << mmin << "  " << mmax << std::endl;
        SortEdgeCollapses(Count);
        for (size_t i = 0; i < VertexCount; ++i) VertexRemap[i] = i; 

        memset(VertexLocked.data(), 0, VertexCount * sizeof(unsigned char));
        size_t CollapsedCount = ExecuteEdgeCollapses(Count);
        UpdateQuadrics();
        IndexCount = RemapIndices();
    }

    Indices.resize(IndexCount);

    return true;
}

void ManifoldSimplifier::BuildAdjacency() {
    memset(VertexAdjacency.Offsets.data(), 0, (VertexCount + 1) * sizeof(int));

    for (int i = 0; i < IndexCount; ++i) {
        int idx = Indices[i] + 1;
        VertexAdjacency.Offsets[idx]++;
    }

    int Offset = 0;
    for (int i = 0; i < VertexCount; ++i) {
        int idx = i + 1;
        int Count = VertexAdjacency.Offsets[idx];
        VertexAdjacency.Offsets[idx] = Offset;
        Offset += Count;
    }

    for (int i = 0; i < IndexCount; i += 3) {
        int v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2];

        VertexAdjacency.Data[VertexAdjacency.Offsets[v0 + 1]].next = v1;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v0 + 1]].prev = v2;
        VertexAdjacency.Offsets[v0 + 1]++;

        VertexAdjacency.Data[VertexAdjacency.Offsets[v1 + 1]].next = v2;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v1 + 1]].prev = v0;
        VertexAdjacency.Offsets[v1 + 1]++;

        VertexAdjacency.Data[VertexAdjacency.Offsets[v2 + 1]].next = v0;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v2 + 1]].prev = v1;
        VertexAdjacency.Offsets[v2 + 1]++;
    }
}

FCollapseNode ManifoldSimplifier::ComputeCost(int v1, int v2) {
    FCollapseNode e;

    e.v1 = v1;
    e.v2 = v2;

    // 总误差 = 几何误差 + lambda * 属性误差
    FQuadric Q_geom = VertexQuadrics[v1] + VertexQuadrics[v2];
    FQuadric Q_sum = Q_geom;
    
    if (bHasLatent) { 
        //Q_sum += LatentQuadrics[v1] + LatentQuadrics[v2] * Lambda;
    }

    // Try to find optimal position
    FVector opt_pos;
    double opt_cost;
    if (Q_geom.Optimize(opt_pos)) { 
        opt_cost = std::abs(Q_sum.Evaluate(opt_pos));
        //if (opt_cost <= 0) { 
        //    opt_pos = (Vertices[v1] + Vertices[v2]) * 0.5f;
        //    opt_cost = Q_sum.Evaluate(opt_pos);
        //}
    } 
    else 
    {
        opt_pos = (Vertices[v1] + Vertices[v2]) * 0.5f;
        opt_cost = std::abs(Q_sum.Evaluate(opt_pos));
    }
    e.target = opt_pos;
    e.cost = opt_cost;

    mmin = std::min(mmin, e.cost);
    mmax = std::max(mmax, e.cost);
    return e;

    //FVector opt_pos;
    //if (Q_geom.findMinimum(opt_pos)) {
    //    auto p1 = Vertices[v1];
    //    auto p2 = Vertices[v2];
    //    e.target = opt_pos;
    //    e.cost = Q_sum.apply(opt_pos);
    //} else {
    //    // Fallback: select best of v1, v2, mid
    //    FVector p1 = Vertices[v1];
    //    FVector p2 = Vertices[v2];
    //    FVector mid = (p1 + p2) * 0.5f;

    //    float c1 = Q_sum.apply(p1);
    //    float c2 = Q_sum.apply(p2);
    //    float cm = Q_sum.apply(mid);
    //    e.target = mid;
    //    e.cost = cm;
    //    if (c1 <= c2 && c1 <= cm) {
    //        e.target = p1;
    //        e.cost = c1;
    //    } else if (c2 <= c1 && c2 <= cm) {
    //        e.target = p2;
    //        e.cost = c2;
    //    } else {
    //        e.target = mid;
    //        e.cost = cm;
    //    }
    //}
    return e;
}

size_t ManifoldSimplifier::BuildEdgeCollapses() {
    size_t count = 0;
    static const int next[3] = {1, 2, 0};
    //for (size_t i = 0; i < IndexCount / 3; i++) {
    //    for (int e = 0; e < 3; ++e) {
    //        const int i0 = Indices[i * 3 + e];
    //        const int i1 = Indices[i * 3 + next[e]];
    //        if (i0 > i1) continue; // 每条边只处理一次，正好不处理边界边，但严格要求拓扑是流形的

    //        Collapses[count++] = ComputeCost(i0, i1);
    //    }
    //}
    FEdgeHash Hash;
    for (int i = 0; i < IndexCount / 3; i++) {
        for (int e = 0; e < 3; ++e) {
            const int i0 = Indices[i * 3 + e];
            const int i1 = Indices[i * 3 + next[e]];
            const int v0 = std::min(i0, i1);
            const int v1 = std::max(i0, i1);
            Edge edge(v0, v1);
            int faceId;
            if (Hash.addOrRemove(edge, i, faceId)) { 
                Collapses[count++] = ComputeCost(v0, v1);
            }
        }
    }

    return count;
}

void ManifoldSimplifier::SortEdgeCollapses(size_t Count) {

    //std::sort(CollapseOrder.begin(), CollapseOrder.begin() + Count,
    //          [this](int a, int b) { return Collapses[a].cost < Collapses[b].cost; });
    //return;
    const unsigned int sort_bits = 12;
    const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

    // fill histogram for counting sort
    unsigned int histogram[sort_bins];
    memset(histogram, 0, sizeof(histogram));

    for (size_t i = 0; i < Count; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error;
        std::memcpy(&error, &Collapses[i].cost, sizeof(error));
        unsigned int key = (error << 1) >> (32 - sort_bits);
        key = key < sort_bins ? key : sort_bins - 1;

        histogram[key]++;
    }

    // compute offsets based on histogram data
    size_t histogram_sum = 0;

    for (size_t i = 0; i < sort_bins; ++i) {
        size_t count = histogram[i];
        histogram[i] = uint32_t(histogram_sum);
        histogram_sum += count;
    }

    // compute sort order based on offsets
    for (size_t i = 0; i < Count; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error;
        std::memcpy(&error, &Collapses[i].cost, sizeof(error));
        unsigned int key = (error << 1) >> (32 - sort_bits);
        key = key < sort_bins ? key : sort_bins - 1;

        CollapseOrder[histogram[key]++] = i;
    }
}

size_t ManifoldSimplifier::ExecuteEdgeCollapses(size_t Count) {
    // 本次循环需要简化的面数量
    size_t TargetTriangleCount = (IndexCount - TargetCount) / 3;

    // 坍缩的面数量
    size_t CollapseCount = 0;

    for (size_t i = 0; i < Count; ++i) {
        const auto& c = Collapses[CollapseOrder[i]];

        if ((c.cost > Collapses[CollapseOrder[Count / 2]].cost && CollapseCount > TargetTriangleCount / 4) ||
            IndexCount <= TargetCount) {
            break;
        }

        int i0 = c.v1;
        int i1 = c.v2;

        if (VertexLocked[i0] | VertexLocked[i1]) continue;

        if (!IsCollapsable(c)) { continue; }

        VertexRemoved[i0] = 1;
        VertexRemap[i0] = i1;
        VertexLocked[i0] = 1;
        VertexLocked[i1] = 1;

        Vertices[i1] = c.target;

        CollapseCount += 2;
    }

    return CollapseCount;
}

bool ManifoldSimplifier::HasTriangleFlip(const FVector& a, const FVector& b, const FVector& c,
                                                const FVector& d) {
    FVector eb = {b.x - a.x, b.y - a.y, b.z - a.z};
    FVector ec = {c.x - a.x, c.y - a.y, c.z - a.z};
    FVector ed = {d.x - a.x, d.y - a.y, d.z - a.z};

    FVector nbc = {eb.y * ec.z - eb.z * ec.y, eb.z * ec.x - eb.x * ec.z, eb.x * ec.y - eb.y * ec.x};
    FVector nbd = {eb.y * ed.z - eb.z * ed.y, eb.z * ed.x - eb.x * ed.z, eb.x * ed.y - eb.y * ed.x};

    float ndp = nbc.x * nbd.x + nbc.y * nbd.y + nbc.z * nbd.z;
    float abc = nbc.x * nbc.x + nbc.y * nbc.y + nbc.z * nbc.z;
    float abd = nbd.x * nbd.x + nbd.y * nbd.y + nbd.z * nbd.z;

    // scale is cos(angle); somewhat arbitrarily set to ~75 degrees
    // note that the "pure" check is ndp <= 0 (90 degree cutoff) but that allows flipping through a series of close-to-90 collapses
    return ndp <= 0.25f * sqrtf(abc * abd);
}

bool ManifoldSimplifier::IsCollapsable(const FCollapseNode& c) {
    const FVector& v0 = Vertices[c.v1];
    const FVector& v1 = Vertices[c.v2];

    size_t Begin = VertexAdjacency.Begin(c.v1);
    for (size_t i = 0; i < VertexAdjacency.Num(c.v1); ++i) {
        auto& e = VertexAdjacency.Data[Begin + i];
        int i0 = VertexRemap[e.next];
        int i1 = VertexRemap[e.prev];

        if (i0 == i1 || i0 == c.v2 || i1 == c.v2) continue;

        if (HasTriangleFlip(Vertices[i0], Vertices[i1], v0, c.target)) { return false; }
    }

    Begin = VertexAdjacency.Begin(c.v2);
    for (size_t i = 0; i < VertexAdjacency.Num(c.v2); ++i) {
        auto& e = VertexAdjacency.Data[Begin + i];
        int i0 = VertexRemap[e.next];
        int i1 = VertexRemap[e.prev];

        if (i0 == i1 || i0 == c.v1 || i1 == c.v1) continue;

        if (HasTriangleFlip(Vertices[i0], Vertices[i1], v1, c.target)) { return false; }
    }
    return true;
}

void ManifoldSimplifier::UpdateQuadrics() {
    for (size_t i = 0; i < VertexCount; ++i) {
        // 要么这个顶点早被删了，要么坍缩时没有影响到该顶点
        if (VertexRemap[i] == i) continue;

        // i号顶点坍缩到 r号顶点上了
        int r = VertexRemap[i];

        VertexQuadrics[r] += VertexQuadrics[i];
        if (bHasLatent) { 
            LatentQuadrics[r] += LatentQuadrics[i];
        }
        
    }
}

size_t ManifoldSimplifier::RemapIndices() {
    size_t k = 0;

    for (size_t i = 0; i < IndexCount; i += 3) {
        int v0 = VertexRemap[Indices[i + 0]];
        int v1 = VertexRemap[Indices[i + 1]];
        int v2 = VertexRemap[Indices[i + 2]];

        if (v0 != v1 && v0 != v2 && v1 != v2) {
            Indices[k + 0] = v0;
            Indices[k + 1] = v1;
            Indices[k + 2] = v2;
            k += 3;
        }
    }

    return k;
}

void ManifoldSimplifier::InitializeQuadrics() {

    VertexQuadrics.resize(Vertices.size());
    memset(VertexQuadrics.data(), 0, VertexCount * sizeof(FQuadric));

    // 1. 几何 Quadric
    for (size_t i = 0; i < IndexCount; i += 3) {
        int v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2];
        FVector p0 = Vertices[v0], p1 = Vertices[v1], p2 = Vertices[v2];

        FVector n = (p1 - p0).Cross(p2 - p0);
        double w = n.Normalize();
        double d = -n.Dot(p0);
        
        FQuadric Q_geom = FQuadric::FromPlane(n.x, n.y, n.z, d, std::sqrt(w));

        VertexQuadrics[v0] += Q_geom;
        VertexQuadrics[v1] += Q_geom;
        VertexQuadrics[v2] += Q_geom;
    }

    if (!bHasLatent) return;

    LatentQuadrics.resize(Vertices.size());
    memset(LatentQuadrics.data(), 0, VertexCount * sizeof(FQuadric));

    // 2. 潜变量 Quadric
    // 对于每个三角形，计算 Latent Feature 的空间梯度
    for (size_t i = 0; i < Indices.size(); i += 3) {
        int v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2];
        FVector p0 = Vertices[v0], p1 = Vertices[v1], p2 = Vertices[v2];

        // 计算三角形局部坐标系的基向量 (u, v)
        FVector e1 = p1 - p0;
        FVector e2 = p2 - p0;

        // 计算梯度 Gradients (Lx3 Matrix)
        // Grad_z * [e1, e2] = [z1-z0, z2-z0]
        // 这里简化计算: 使用最小二乘或直接解线性方程

        // 计算流形度量张量 M (LxL) -> 这里我们强制假定 L=3，即 M 是 3x3
        // 如果有 metric_tensors，取三个顶点的平均
        double M_face[9]{0.0f}; // 3x3 = 9
        if (bHasLatent) {
            for (int k = 0; k < 9; ++k) {
                M_face[k] = (MetricTensors[v0 * 9 + k] + MetricTensors[v1 * 9 + k] + MetricTensors[v2 * 9 + k]) / 3.0f;
            }
        } else {
            // Identity
            M_face[0] = 1.0f;
            M_face[4] = 1.0f;
            M_face[8] = 1.0f;
        }

        // 计算空间敏感度矩阵 A = Grad^T * M * Grad (3x3)
        // 这是一个比较复杂的几何计算，需要求解梯度矩阵
        double A_spatial[3][3];
        ComputeSpatialSensitivity(v0, v1, v2, M_face, A_spatial);

        // 将 A_spatial 转换为 Quadric 并累加
        // 注意: 这个 Quadric 是基于三角形的，需要贡献给三个顶点
        // 按照 Hoppe 的论文，这里通常通过加权面积贡献
        double area = 0.5 * e1.Cross(e2).Length();

        // 为每个顶点构建基于其位置 p 的 Quadric
        // Q = FromMetricTensor(A, p)
        FQuadric Q_v0 = FQuadric::FromMetricTensor(A_spatial, p0);
        FQuadric Q_v1 = FQuadric::FromMetricTensor(A_spatial, p1);
        FQuadric Q_v2 = FQuadric::FromMetricTensor(A_spatial, p2);

        // 加权累加
        double w = area;

        LatentQuadrics[v0] += Q_v0 * w;
        LatentQuadrics[v1] += Q_v1 * w;
        LatentQuadrics[v2] += Q_v2 * w;
    }
}

void ManifoldSimplifier::ComputeSpatialSensitivity(int v0, int v1, int v2, double M[9], double A_out[3][3]) {
    // 使用 Eigen 求解属性梯度
    // 设三角形顶点为 p0, p1, p2
    // 设 Latent Feature 在 p 处的值为 z(p)
    // 我们假设 z(p) 在三角形内是线性的: z(p) = g^T * p + c
    // 其中 g 是梯度向量 (3x1)

    // 建立线性方程组:
    // g^T * p0 + c = z0
    // g^T * p1 + c = z1
    // g^T * p2 + c = z2
    // g^T * n = 0 (假设沿法线方向不变，这对于 Surface Gradient 是成立的)

    // 这是一个 4个未知数 (g_x, g_y, g_z, c) 的方程组。
    // 或者更简单地，我们在局部坐标系求解，或者直接构建矩阵求解。

    // 构建位置矩阵 P (3x3):
    // [ p1 - p0 ]
    // [ p2 - p0 ]
    // [    n    ]
    // 对应的差分 Z_diff:
    // [ z1 - z0 ]
    // [ z2 - z0 ]
    // [    0    ]

    // P * g = Z_diff  => g = P^-1 * Z_diff

    FVector p0 = Vertices[v0], p1 = Vertices[v1], p2 = Vertices[v2];
    FVector e1 = p1 - p0;
    FVector e2 = p2 - p0;
    FVector n = e1.Cross(e2);
    n.Normalize();

    Eigen::Matrix3d P;
    P << e1.x, e1.y, e1.z, e2.x, e2.y, e2.z, n.x, n.y, n.z;

    // 计算 P 的逆
    Eigen::Matrix3d P_inv = P.inverse();

    // 初始化输出矩阵 A_spatial = 0
    memset(A_out, 0, 9 * sizeof(float));

    // 将 M 转换为 Eigen 矩阵 (3x3)
    // 注意 M 存储为扁平化数组，通常是 Row-Major
    Eigen::Matrix3d M_mat;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            // 如果 latent_dim > 3, M 可能只取前 3x3 ?
            // 或者我们假设 M 已经被规约到了 3x3 (在 InitializeQuadrics 中已处理)
            M_mat(i, j) = M[i * 3 + j];
        }
    }

    // 遍历每个 Latent Dimension k，计算其梯度 g_k
    // 实际上我们可以矩阵化操作:
    // G = P_inv * Z_diff_matrix
    // 但为了清晰，逐个维度计算

    std::vector<Eigen::Vector3d> grads(LatentDim);

    for (int k = 0; k < LatentDim; ++k) {
        float dz1 = LatentFeatures[v1 * LatentDim + k] - LatentFeatures[v0 * LatentDim + k];
        float dz2 = LatentFeatures[v2 * LatentDim + k] - LatentFeatures[v0 * LatentDim + k];

        Eigen::Vector3d Z_diff(dz1, dz2, 0.0);

        // g_k = P^-1 * Z_diff
        // 注意: P * g = Z，这里 P 是行向量形式 [e1^T; e2^T; n^T]
        // 所以 [e1^T * g; ...] = ...
        // P * g = Z_diff 是对的。
        // 但 Eigen 默认列向量，所以:
        // [x1 y1 z1] [gx]   [dz1]
        // [x2 y2 z2] [gy] = [dz2]
        // [nx ny nz] [gz]   [ 0 ]
        // 即 P * g = Z_diff

        grads[k] = P_inv * Z_diff;
    }

    // 计算 A = sum_{i,j} M_{ij} * g_i * g_j^T
    // 这里的 i, j 对应 latent dimension 索引
    // M 矩阵大小必须是 (latent_dim x latent_dim)

    for (int i = 0; i < LatentDim; ++i) {
        for (int j = 0; j < LatentDim; ++j) {
            float m_val = M_mat(i, j);
            if (std::abs(m_val) < 1e-9) continue;

            // term = m_val * (g_i * g_j^T)
            // g_i 是 3x1 向量
            Eigen::Matrix3d term = m_val * grads[i] * grads[j].transpose();

            // 累加到 A_out
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c) A_out[r][c] += term(r, c);
        }
    }
}