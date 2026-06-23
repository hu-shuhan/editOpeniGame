#include "iGameMeshSimplificationWithAttributes.h"
#include <limits>

MeshSimplifierWithAttributes::MeshSimplifierWithAttributes(std::vector<FVector>& V, std::vector<int>& F,
                                                           std::vector<std::vector<float>>& A,
                                                           std::vector<float>& AttributeWeights,
                                                           std::vector<double>& VertexImportance, double T,
                                                           bool IsOptimizedPosition)
    : Vertices(V), Indices(F), Attributes(A), AttributeWeights(AttributeWeights),
      VertexImportance(VertexImportance),
      IsOptimizedPosition(IsOptimizedPosition)
{
    IndexCount = Indices.size();
    VertexCount = V.size();
    AttributeCount = Attributes.size();
    TargetCount = IndexCount * T;
}

void MeshSimplifierWithAttributes::EnableGuidedUpdate(int iterations, float lambda)
{
    GuidedUpdateEnabled = true;
    GuidedUpdateIterations = iterations;
    GuidedUpdateLambda = lambda;
}

void MeshSimplifierWithAttributes::ComputeLPCA(std::vector<FVector>& V, std::vector<int>& F,
                                               std::vector<std::vector<float>>& A,
                                               std::vector<double>& VertexImportance) 
{
    std::vector<float> AttributeWeights(A.size(), 1.0f);
    // Create an instance. Target and IsOptimizedPosition don't matter for this.
    MeshSimplifierWithAttributes simplifier(V, F, A, AttributeWeights, VertexImportance);

    simplifier.VertexAdjacency.Offsets.resize(V.size() + 1);
    simplifier.VertexAdjacency.Data.resize(F.size());
    simplifier.BuildAdjacency();
    simplifier.ComputeVertexImportanceUsingLPCA();
}

void MeshSimplifierWithAttributes::ComputeHeatDiffusionSaliency(std::vector<FVector>& V, std::vector<int>& F,
                                                                std::vector<double>& VertexImportance) {
    std::vector<std::vector<float>> A; // Empty attributes
    std::vector<float> AttributeWeights;
    MeshSimplifierWithAttributes simplifier(V, F, A, AttributeWeights, VertexImportance);

    simplifier.VertexAdjacency.Offsets.resize(V.size() + 1);
    simplifier.VertexAdjacency.Data.resize(F.size());
    simplifier.BuildAdjacency();
    simplifier.ComputeVertexImportanceUsingHeatDiffusion();
}

bool MeshSimplifierWithAttributes::Execute() 
{
    VertexAdjacency.Offsets.resize(VertexCount + 1);
    VertexAdjacency.Data.resize(IndexCount);
    VertexRemoved.resize(VertexCount, 0);
    Locked.resize(VertexCount, 0);
    Kind.resize(VertexCount);
    L1.resize(VertexCount, -1);
    L2.resize(VertexCount, -1);
    Mapping.resize(VertexCount);
    Collapses.resize(IndexCount);
    Order.resize(Collapses.size());

    //CosAngle = std::cos(RadiansFromDegrees(FeatureAngle));
    //std::cout << CosAngle << std::endl;
    BuildAdjacency();

    //ComputeVertexImportanceUsingLPCA();

    ClassifyVertices();

    InitializeQuadrics();

    while (IndexCount > TargetCount) 
    { 
        BuildAdjacency();
        size_t Count = BuildEdgeCollapses();
        SortEdgeCollapses(Count);
        for (size_t i = 0; i < VertexCount; ++i) Mapping[i] = i; 
        
        memset(Locked.data(), 0, VertexCount * sizeof(unsigned char));
        size_t CollapsedCount = ExecuteEdgeCollapses(Count);
        UpdateQuadrics();
        IndexCount = RemapIndices();
        //break;
    }
    Indices.resize(IndexCount);

    return true;
}

void MeshSimplifierWithAttributes::BuildAdjacency() {

    std::fill(VertexAdjacency.Offsets.begin(), VertexAdjacency.Offsets.end(), 0);

    for (int i = 0; i < IndexCount; ++i) { ++VertexAdjacency.Offsets[Indices[i] + 1]; }

    int Acc = 0;
    for (int i = 0; i < VertexCount; ++i) {
        int Idx = i + 1;
        int Tmp = VertexAdjacency.Offsets[Idx];
        VertexAdjacency.Offsets[Idx] = Acc;
        Acc += Tmp;
    }

    for (int i = 0; i < IndexCount; i += 3) {
        int A = Indices[i];
        int B = Indices[i + 1];
        int C = Indices[i + 2];

        VertexAdjacency.Data[VertexAdjacency.Offsets[A + 1]++] = {B, C};
        VertexAdjacency.Data[VertexAdjacency.Offsets[B + 1]++] = {C, A};
        VertexAdjacency.Data[VertexAdjacency.Offsets[C + 1]++] = {A, B};
    }
}

void MeshSimplifierWithAttributes::ComputeVertexImportanceUsingLPCA() {
    if (Attributes.empty()) return;

    // 1. Normalize Attributes
    int numAttributes = Attributes.size();
    //std::vector<std::vector<float>> normalizedAttributes(numAttributes);

    //for (int d = 0; d < numAttributes; ++d) {
    //    float minVal = std::numeric_limits<float>::max();
    //    float maxVal = std::numeric_limits<float>::lowest();
    //    if (Attributes[d].empty()) continue; 

    //    for (float val : Attributes[d]) {
    //        if (val < minVal) minVal = val;
    //        if (val > maxVal) maxVal = val;
    //    }

    //    float range = maxVal - minVal;
    //    normalizedAttributes[d].resize(VertexCount);
    //    if (range < 1e-9f) {
    //        for (int i = 0; i < VertexCount; ++i) normalizedAttributes[d][i] = 0.0f;
    //    } else {
    //        float invRange = 1.0f / range;
    //        for (int i = 0; i < VertexCount; ++i) {
    //            normalizedAttributes[d][i] = (Attributes[d][i] - minVal) * invRange;
    //        }
    //    }
    //}

    // 2. Compute Local Covariance Matrix and PCA
    // Ensure VertexImportance is sized correctly
    if (VertexImportance.size() != VertexCount) {
        VertexImportance.resize(VertexCount);
    }

    for (int i = 0; i < VertexCount; ++i) {
        // Get neighbors
        int count = VertexAdjacency.Num(i);
        auto* edges = VertexAdjacency.Data.data() + VertexAdjacency.Begin(i);
        
        std::vector<int> neighbors;
        neighbors.reserve(count);
        for (int j = 0; j < count; ++j) {
            neighbors.push_back(edges[j].next);
        }

        // Deduplicate neighbors
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        
        int k = neighbors.size();
        if (k < 2) {
            VertexImportance[i] = 0.0;
            continue;
        }

        // Construct Data Matrix X (k x D)
        Eigen::MatrixXd X(k, numAttributes);
        Eigen::VectorXd meanVec = Eigen::VectorXd::Zero(numAttributes);

        for (int row = 0; row < k; ++row) {
            int neighborIdx = neighbors[row];
            for (int col = 0; col < numAttributes; ++col) {
                //float val = normalizedAttributes[col][neighborIdx];
                float val = Attributes[col][neighborIdx];
                X(row, col) = val;
                meanVec(col) += val;
            }
        }
        meanVec /= double(k);

        // Center the data: M = X - mean
        Eigen::MatrixXd M = X.rowwise() - meanVec.transpose();

        // Covariance Matrix C = (1/(k-1)) * M^T * M
        Eigen::MatrixXd C = (M.transpose() * M) / double(k - 1);

        // Eigen decomposition
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(C);
        if (solver.info() != Eigen::Success) {
            VertexImportance[i] = 0.0;
            continue;
        }

        // Eigenvalues are sorted in ascending order
        double maxLambda = solver.eigenvalues()[numAttributes - 1];
        if (maxLambda < 0) maxLambda = 0;

        VertexImportance[i] = std::sqrt(maxLambda);
    }
}

void MeshSimplifierWithAttributes::ComputeVertexImportanceUsingHeatDiffusion() {


    // ---------------------------------------------------------
    // 步骤 1: 计算面法线 (Face Normals) 并建立顶点到面的映射
    // ---------------------------------------------------------
    int FaceCount = IndexCount / 3;
    std::vector<FVector> faceNormals(FaceCount);
    std::vector<std::vector<int>> vertexToFaces(VertexCount);

    for (int i = 0; i < IndexCount; i += 3) {
        int i0 = Indices[i];
        int i1 = Indices[i + 1];
        int i2 = Indices[i + 2];
        int faceIdx = i / 3;

        FVector p0 = Vertices[i0];
        FVector p1 = Vertices[i1];
        FVector p2 = Vertices[i2];

        FVector fn = (p1 - p0).Cross(p2 - p0);
        fn.Normalize(); // 这里必须归一化
        faceNormals[faceIdx] = fn;

        vertexToFaces[i0].push_back(faceIdx);
        vertexToFaces[i1].push_back(faceIdx);
        vertexToFaces[i2].push_back(faceIdx);
    }

    // ---------------------------------------------------------
    // 步骤 2: 极其鲁棒的初始热源 H0 (Face-Normal Tropical Angle)
    // ---------------------------------------------------------
    std::vector<double> H0(VertexCount, 0.0);
    for (int i = 0; i < VertexCount; ++i) {
        const auto& faces = vertexToFaces[i];
        double minDot = 1.0;

        // 寻找该顶点周围所有相邻面中，夹角最大（点乘最小）的两个面
        for (size_t a = 0; a < faces.size(); ++a) {
            for (size_t b = a + 1; b < faces.size(); ++b) {
                double d = faceNormals[faces[a]].Dot(faceNormals[faces[b]]);
                if (d < minDot) { minDot = d; }
            }
        }
        // 完全平坦区域为 0，90度折角为 1.0
        H0[i] = 1.0 - minDot;
    }
    
    std::vector<double> H_prev = H0;
    std::vector<double> H(VertexCount);
    std::vector<double> S_RMS_Accumulator(VertexCount, 0.0);

    // ---------------------------------------------------------
    // 步骤 3: 带有距离反比权重 (IDW) 的热流扩散
    // ---------------------------------------------------------
    int k = 5;
    double lambda = 0.5;

    for (int t = 1; t <= k; ++t) {
        for (int i = 0; i < VertexCount; ++i) {
            int count = VertexAdjacency.Num(i);
            if (count == 0) {
                H[i] = H_prev[i];
                continue;
            }
            auto* edges = VertexAdjacency.Data.data() + VertexAdjacency.Begin(i);

            std::vector<int> neighbors;
            neighbors.reserve(count);
            for (int j = 0; j < count; ++j) { neighbors.push_back(edges[j].next); }
            std::sort(neighbors.begin(), neighbors.end());
            neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());

            double sumH = 0.0;
            double sumW = 0.0;

            for (int neighbor: neighbors) {
                // 使用反距离权重，消除各向异性扩散
                double dist = (Vertices[i] - Vertices[neighbor]).Length();
                double w = 1.0 / std::max(1e-6, dist);
                sumH += H_prev[neighbor] * w;
                sumW += w;
            }

            double avgH = (sumW > 0) ? (sumH / sumW) : 0.0;
            H[i] = (1.0 - lambda) * H_prev[i] + lambda * avgH;
        }

        // ---------------------------------------------------------
        // 步骤 4: 时序积分显著度 (单向截断防止鬼影)
        // ---------------------------------------------------------
        for (int i = 0; i < VertexCount; ++i) {
            // 重点修改：仅当顶点散失热量 (H0 > H) 时才累加，忽略被动受热的平坦点
            double diff = std::max(0.0, H0[i] - H[i]);
            S_RMS_Accumulator[i] += diff * diff;
        }
        H_prev = H;
    }

    // ---------------------------------------------------------
    // 步骤 5: 均方根与归一化
    // ---------------------------------------------------------
    if (VertexImportance.size() != VertexCount) { VertexImportance.resize(VertexCount); }

    double minS = std::numeric_limits<double>::max();
    double maxS = std::numeric_limits<double>::lowest();

    for (int i = 0; i < VertexCount; ++i) {
        VertexImportance[i] = std::sqrt(S_RMS_Accumulator[i] / k);
        if (VertexImportance[i] < minS) minS = VertexImportance[i];
        if (VertexImportance[i] > maxS) maxS = VertexImportance[i];
    }

    double range = maxS - minS;
    if (range > 1e-9) {
        for (int i = 0; i < VertexCount; ++i) { VertexImportance[i] = (VertexImportance[i] - minS) / range; }
    } else {
        for (int i = 0; i < VertexCount; ++i) { VertexImportance[i] = 0.0; }
    }
}

void MeshSimplifierWithAttributes::ClassifyVertices() {

    std::memset(L1.data(), 0xFF, VertexCount * sizeof(unsigned int));
    std::memset(L2.data(), 0xFF, VertexCount * sizeof(unsigned int));

    unsigned int* in = L2.data();
    unsigned int* out = L1.data();

    auto ExistsReverseEdge = [&](unsigned int a, unsigned int b) {
        unsigned int n = VertexAdjacency.Num(a);
        const auto* ptr = VertexAdjacency.Data.data() + VertexAdjacency.Begin(a);
        return std::any_of(ptr, ptr + n, [b](const auto& e) { return e.next == b; });
    };

    for (unsigned int v = 0; v < VertexCount; ++v) {
        unsigned int cnt = VertexAdjacency.Num(v);
        const auto* list = VertexAdjacency.Data.data() + VertexAdjacency.Begin(v);

        for (unsigned int k = 0; k < cnt; ++k) {
            unsigned int t = list[k].next;

            if (t == v) {
                in[v] = out[v] = v;
            } else if (!ExistsReverseEdge(t, v)) {
                in[t] = (in[t] == ~0u) ? v : t;
                out[v] = (out[v] == ~0u) ? t : v;
            }
        }
    }

    for (unsigned int v = 0; v < VertexCount; ++v) {
        unsigned int inc = in[v];
        unsigned int otc = out[v];

        if (inc == ~0u && otc == ~0u) Kind[v] = Kind_Manifold;
        else if (inc != v && otc != v)
            Kind[v] = Kind_Border;
        else
            Kind[v] = Kind_Locked;
    }

    std::vector<FVector> FaceNormals;
    FaceNormals.reserve(IndexCount / 3);

    for (int f = 0; f < IndexCount; f += 3) {
        FaceNormals.emplace_back(
                ComputeNormal(Vertices[Indices[f]], Vertices[Indices[f + 1]], Vertices[Indices[f + 2]]));
    }

    static constexpr int Cycle[3] = {1, 2, 0};

    FEdgeHash EdgeHash;
    for (int f = 0; f < static_cast<int>(FaceNormals.size()); ++f) {
        int base = f * 3;
        for (int e = 0; e < 3; ++e) {
            int a = Indices[base + e];
            int b = Indices[base + Cycle[e]];

            int lo = a < b ? a : b;
            int hi = a < b ? b : a;

            int other;
            if (EdgeHash.AddOrRemove({lo, hi}, f, other)) {
                if (Dot(FaceNormals[f], FaceNormals[other]) <= CosAngle) {
                    Kind[a] = Kind_Feature;
                    Kind[b] = Kind_Feature;
                }
            }
        }
    }
}


void MeshSimplifierWithAttributes::InitializeQuadrics() 
{
    VertexQuadrics.resize(VertexCount);
    std::memset(VertexQuadrics.data(), 0, VertexCount * sizeof(FQuadric));

    for (int t = 0; t < IndexCount; t += 3) {
        const int v0 = Indices[t];
        const int v1 = Indices[t + 1];
        const int v2 = Indices[t + 2];

        FVector p0 = Vertices[v0];
        FVector p1 = Vertices[v1];
        FVector p2 = Vertices[v2];

        FVector n = (p1 - p0).Cross(p2 - p0);
        double area = n.Normalize();
        double d = -n.Dot(p0);

        FQuadric Qface = FQuadric::FromPlane(n.x, n.y, n.z, d, 1.0);

        double imp = 0.0;
        if (UseVertexImportance && VertexImportance.size() == VertexCount) {
            imp = (VertexImportance[v0] + VertexImportance[v1] + VertexImportance[v2]) / 3.0;
        }

        const double scale = 1.0 + VertexImportanceScale * imp;
        if (scale != 1.0) {
            Qface.a00 *= scale;
            Qface.a11 *= scale;
            Qface.a22 *= scale;
            Qface.a10 *= scale;
            Qface.a20 *= scale;
            Qface.a21 *= scale;
            Qface.b0 *= scale;
            Qface.b1 *= scale;
            Qface.b2 *= scale;
            Qface.c *= scale;
        }

        VertexQuadrics[v0] += Qface;
        VertexQuadrics[v1] += Qface;
        VertexQuadrics[v2] += Qface;
    }

    static constexpr int next[4] = {1, 2, 0, 1};

    for (int t = 0; t < IndexCount; t += 3) {
        for (int e = 0; e < 3; ++e) {
            const unsigned int a = Indices[t + e];
            const unsigned int b = Indices[t + next[e]];

            if (Kind[a] != Kind_Border && Kind[b] != Kind_Border) continue;

            const unsigned int c = Indices[t + next[e + 1]];

            FQuadric Qedge = FQuadric::FromTriangleEdge(Vertices[a], Vertices[b], Vertices[c], 10.f);

            VertexQuadrics[a] += Qedge;
            VertexQuadrics[b] += Qedge;
        }
    }

    AttributeQuadrics.resize(VertexCount);
    std::memset(AttributeQuadrics.data(), 0, VertexCount * sizeof(FQuadric));

    AttributeGradients.resize(VertexCount * AttributeCount);
    std::memset(AttributeGradients.data(), 0, VertexCount * AttributeCount * sizeof(FGradient));

for (int f = 0; f < IndexCount; f += 3) {
        const int i0 = Indices[f];
        const int i1 = Indices[f + 1];
        const int i2 = Indices[f + 2];

        const FVector& v0 = Vertices[i0];
        const FVector& v1 = Vertices[i1];
        const FVector& v2 = Vertices[i2];

        const FVector e10 = v1 - v0;
        const FVector e20 = v2 - v0;

        const FVector crs = e10.Cross(e20);
        const float triArea = 0.5f * crs.Length();

        const float m00 = e10.x * e10.x + e10.y * e10.y + e10.z * e10.z;
        const float m01 = e10.x * e20.x + e10.y * e20.y + e10.z * e20.z;
        const float m11 = e20.x * e20.x + e20.y * e20.y + e20.z * e20.z;

        const float det = m00 * m11 - m01 * m01;
        const float invDet = det ? 1.f / det : 0.f;

        const float gx_u = (m11 * e10.x - m01 * e20.x) * invDet;
        const float gy_u = (m11 * e10.y - m01 * e20.y) * invDet;
        const float gz_u = (m11 * e10.z - m01 * e20.z) * invDet;

        const float gx_v = (m00 * e20.x - m01 * e10.x) * invDet;
        const float gy_v = (m00 * e20.y - m01 * e10.y) * invDet;
        const float gz_v = (m00 * e20.z - m01 * e10.z) * invDet;

        FQuadric Q;
        std::memset(&Q, 0, sizeof(FQuadric));
        Q.w = triArea;

        std::vector<FGradient> Grad(AttributeCount);

        for (size_t k = 0; k < AttributeCount; ++k) {
            const float w = AttributeWeights[k];

            const float a = Attributes[k][i0] * w;
            const float b = Attributes[k][i1] * w;
            const float c = Attributes[k][i2] * w;

            const float dba = b - a;
            const float dca = c - a;

            const float dx = gx_u * dba + gx_v * dca;
            const float dy = gy_u * dba + gy_v * dca;
            const float dz = gz_u * dba + gz_v * dca;

            const float gw = a - v0.x * dx - v0.y * dy - v0.z * dz;

            const float wx = triArea * dx;
            const float wy = triArea * dy;
            const float wz = triArea * dz;
            const float ww = triArea * gw;

            Q.a00 += wx * dx;
            Q.a11 += wy * dy;
            Q.a22 += wz * dz;
            Q.a10 += wy * dx;
            Q.a20 += wz * dx;
            Q.a21 += wz * dy;
            Q.b0 += wx * gw;
            Q.b1 += wy * gw;
            Q.b2 += wz * gw;
            Q.c += ww * gw;

            Grad[k].gx = wx;
            Grad[k].gy = wy;
            Grad[k].gz = wz;
            Grad[k].gw = ww;
        }

        AttributeQuadrics[i0] += Q;
        AttributeQuadrics[i1] += Q;
        AttributeQuadrics[i2] += Q;

        for (size_t k = 0; k < AttributeCount; ++k) {
            const int o0 = i0 * AttributeCount + k;
            const int o1 = i1 * AttributeCount + k;
            const int o2 = i2 * AttributeCount + k;

            AttributeGradients[o0] += Grad[k];
            AttributeGradients[o1] += Grad[k];
            AttributeGradients[o2] += Grad[k];
        }
    }
}

FCollapseNode MeshSimplifierWithAttributes::ComputeEdgeCost(int i0, int i1) {
    FCollapseNode e;

    e.i0 = i0;
    e.i1 = i1;

    if (IsOptimizedPosition)
    {
        FQuadric Q_geom = VertexQuadrics[i0] + VertexQuadrics[i1];

        FVector opt_pos;
        if (!Q_geom.Optimize(opt_pos)) { opt_pos = (Vertices[i0] + Vertices[i1]) * 0.5f; }

        double opt_cost = Q_geom.Error(opt_pos);

        if (AttributeCount && UseDynamicAttributePenalty) {
            const FQuadric Q_attr = AttributeQuadrics[i0] + AttributeQuadrics[i1];
            double r = Q_attr.Evaluate(opt_pos);

            for (size_t k = 0; k < AttributeCount; ++k) {
                const FGradient& G1 = AttributeGradients[i0 * AttributeCount + k];
                float a1 = Attributes[k][i0] * AttributeWeights[k];
                float g1 = opt_pos.x * G1.gx + opt_pos.y * G1.gy + opt_pos.z * G1.gz + G1.gw;
                r += a1 * (a1 * AttributeQuadrics[i0].w - 2 * g1);

                const FGradient& G2 = AttributeGradients[i1 * AttributeCount + k];
                float a2 = Attributes[k][i1] * AttributeWeights[k];
                float g2 = opt_pos.x * G2.gx + opt_pos.y * G2.gy + opt_pos.z * G2.gz + G2.gw;
                r += a2 * (a2 * AttributeQuadrics[i1].w - 2 * g2);
            }

            //double l2 = (Vertices[i0] - Vertices[i1]).LengthSquared();
            //double len = l2 > 0.0 ? std::sqrt(l2) : 0.0;

            //double h1 = 0.0;
            //if (len > 0.0) {
            //    double invw0 = 1.0 / (AttributeQuadrics[i0].w + 1e-12);
            //    double invw1 = 1.0 / (AttributeQuadrics[i1].w + 1e-12);

            //    for (int k = 0; k < AttributeCount; ++k) {
            //        const FGradient& G0 = AttributeGradients[i0 * AttributeCount + k];

            //        double dx = G0.gx - AttributeGradients[i1 * AttributeCount + k].gx;
            //        double dy = G0.gy - AttributeGradients[i1 * AttributeCount + k].gy;
            //        double dz = G0.gz - AttributeGradients[i1 * AttributeCount + k].gz;

            //        h1 += (dx * dx + dy * dy + dz * dz) * len;
            //    }
            //}

            //opt_cost += Lambda * (std::abs(r) + 0.01 * h1);
            opt_cost += Lambda * std::abs(r);
        }
        e.target = opt_pos;
        e.cost = float(opt_cost);
    }
    else
    {
        const FVector& v = Vertices[i1];
        double cost = VertexQuadrics[i0].Error(v);

        if (AttributeCount && UseDynamicAttributePenalty) {
            double r = AttributeQuadrics[i0].Evaluate(v);

            for (size_t k = 0; k < AttributeCount; ++k) {
                const FGradient& G1 = AttributeGradients[i0 * AttributeCount + k];
                float a1 = Attributes[k][i0] * AttributeWeights[k];
                float g1 = v.x * G1.gx + v.y * G1.gy + v.z * G1.gz + G1.gw;
                r += a1 * (a1 * AttributeQuadrics[i0].w - 2 * g1);
            }

            double l2 = (Vertices[i0] - Vertices[i1]).LengthSquared();
            double len = l2 > 0.0 ? std::sqrt(l2) : 0.0;

            double h1 = 0.0;
            if (len > 0.0) {
                double invw0 = 1.0 / (AttributeQuadrics[i0].w + 1e-12);
                double invw1 = 1.0 / (AttributeQuadrics[i1].w + 1e-12);

                for (int k = 0; k < AttributeCount; ++k) {
                    const FGradient& G0 = AttributeGradients[i0 * AttributeCount + k];

                    double dx = G0.gx - AttributeGradients[i1 * AttributeCount + k].gx;
                    double dy = G0.gy - AttributeGradients[i1 * AttributeCount + k].gy;
                    double dz = G0.gz - AttributeGradients[i1 * AttributeCount + k].gz;

                    h1 += (dx * dx + dy * dy + dz * dz) * len;
                }
            }

            cost += Lambda * (std::abs(r) + 0.01 * h1);
        }
        e.target = v;
        e.cost = float(cost);
    }
    return e;
}

size_t MeshSimplifierWithAttributes::BuildEdgeCollapses() {
    size_t Count = 0;

    static const int next[4] = {1, 2, 0, 1};
    for (size_t i = 0; i < IndexCount / 3; i++) {
        for (int e = 0; e < 3; ++e) {
            const int i0 = Indices[i * 3 + e];
            const int i1 = Indices[i * 3 + next[e]];
            // const int i2 = Indices[i * 3 + next[e + 1]];
            Collapses[Count] = ComputeEdgeCost(i0, i1);

            //unsigned int count = VertexAdjacency.Num(i0);
            //const auto* edges = VertexAdjacency.Data.data() + VertexAdjacency.Begin(i0);

            //for (size_t j = 0; j < count; ++j) {
            //    if (edges[j].next == i1 && edges[j].prev != i2) { 
            //        Collapses[Count] = ComputeEdgeCost(i2, edges[i].prev);
            //        if (Collapses[Count].cost < c.cost) { 
            //            Collapses[Count] = c;
            //        }
            //        break;
            //    }
            //}
            Count++;
        }
    }
    //FEdgeHash Hash;
    //for (int i = 0; i < IndexCount / 3; i++) {
    //    for (int e = 0; e < 3; ++e) {
    //        const int i0 = Indices[i * 3 + e];
    //        const int i1 = Indices[i * 3 + next[e]];
    //        const int i2 = Indices[i * 3 + next[e + 1]];
    //        const int v0 = std::min(i0, i1);
    //        const int v1 = std::max(i0, i1);
    //        Edge edge{v0, v1};
    //        int faceId;
    //        if (Hash.AddOrRemove(edge, i, faceId)) { 
    //            int j;
    //            for (j = 0; j < 3; j++) { 
    //                if (Indices[faceId * 3 + j] != i0 && Indices[faceId * 3 + j] != i1) { 
    //                    break;
    //                }
    //            }
    //            const int i3 = Indices[faceId * 3 + j];
    //            auto c1 = ComputeEdgeCost(i0, i1);
    //            auto c2 = ComputeEdgeCost(i1, i0);

    //            //if ((Vertices[i0] - Vertices[i1]).LengthSquared() < (Vertices[i2] - Vertices[i3]).LengthSquared() + 1e-15) {
    //            //    if (c1.cost < c2.cost) {
    //            //        Collapses[Count] = c1;
    //            //    } else{
    //            //        Collapses[Count] = c2;
    //            //    }
    //            //    Count++;
    //            //    continue;
    //            //}

    //            auto c3 = ComputeEdgeCost(i2, i3);
    //            auto c4 = ComputeEdgeCost(i3, i2);
    //            c3.duiou = c4.duiou = true;
    //            c3.f0 = c4.f1 = i;
    //            c3.f1 = c4.f0 = faceId;
    //            if (c1.cost < c2.cost && c1.cost < c3.cost && c1.cost < c4.cost) {
    //                Collapses[Count] = c1;
    //            } else if (c2.cost < c1.cost && c2.cost < c3.cost && c2.cost < c4.cost) {
    //                Collapses[Count] = c2;
    //            } else if (c3.cost < c1.cost && c3.cost < c2.cost && c3.cost < c4.cost) {
    //                Collapses[Count] = c3;
    //            } else {
    //                Collapses[Count] = c4;
    //            }
    //            Count++;
    //        }
    //    }
    //}

    //// 边界边
    //for (int i = 0; i < Hash.capacity; i++) {
    //    if (Hash.data[i]) {
    //        auto* p = Hash.data[i];
    //        while (p) {
    //            Collapses[Count++] = ComputeEdgeCost(p->key.v1, p->key.v2);
    //            // Collapses[Count++] = ComputeEdgeCost(p->key.v2, p->key.v1);
    //            p = p->next;
    //        }
    //    }
    //}

    return Count;
}

void MeshSimplifierWithAttributes::SortEdgeCollapses(size_t Count)
{
    constexpr unsigned int KeyBits = 12u;
    constexpr unsigned int BinCount = (1u << KeyBits) + 512u;

    std::vector<uint32_t> BinOffsets(BinCount + 1, 0);
    std::vector<uint32_t> TempOrder(Count);


    for (size_t i = 0; i < Count; ++i) {
        union {
            float f;
            uint32_t u;
        } cvt;
        cvt.f = Collapses[i].cost;

        uint32_t key = (cvt.u << 1u) >> (32u - KeyBits);
        key = key < BinCount ? key : BinCount - 1u;

        ++BinOffsets[key + 1];
    }

    for (size_t b = 1; b <= BinCount; ++b) { BinOffsets[b] += BinOffsets[b - 1]; }

    for (size_t i = Count; i-- > 0;) {
        union {
            float f;
            uint32_t u;
        } cvt;
        cvt.f = Collapses[i].cost;

        uint32_t key = (cvt.u << 1u) >> (32u - KeyBits);
        key = key < BinCount ? key : BinCount - 1u;

        TempOrder[--BinOffsets[key + 1]] = static_cast<uint32_t>(i);
    }

    for (size_t i = 0; i < Count; ++i) { Order[i] = TempOrder[i]; }
}

size_t MeshSimplifierWithAttributes::ExecuteEdgeCollapses(size_t Count) 
{
    // 本次循环需要简化的面数量
    size_t TargetTriangleCount = (IndexCount - TargetCount) / 3;
    static const int next[4] = {1, 2, 0, 1};
    // 坍缩的面数量
    size_t CollapseCount = 0;

    for (size_t i = 0; i < Count; ++i) {
        const auto& c = Collapses[Order[i]];

        if ((c.cost > Collapses[Order[Count / 3]].cost && CollapseCount > TargetTriangleCount / 3) ||
            IndexCount <= TargetCount) {
            break;
        }

        int i0 = c.i0;
        int i1 = c.i1;

        if (Locked[i0] | Locked[i1]) continue;
        //if (VertexKind[i0] == Kind_Feature || VertexKind[i1] == Kind_Feature) continue;

        if (!IsCollapsable(c)) { continue; }

        if (c.duiou) {
            Indices[c.f0 * 3] = Indices[c.f0 * 3 + 1] = 0;
            Indices[c.f1 * 3] = Indices[c.f1 * 3 + 1] = 0;
            a++;
        }

        VertexRemoved[i0] = 1;
        Mapping[i0] = i1;
        Locked[i0] = 1;
        Locked[i1] = 1;

        //if (!TargetNormalSums.empty()) {
        //    TargetNormalSums[i1] = TargetNormalSums[i1] + TargetNormalSums[i0];
        //    TargetNormalWeights[i1] += TargetNormalWeights[i0];
        //    TargetNormalSums[i0] = FVector{0.f, 0.f, 0.f};
        //    TargetNormalWeights[i0] = 0.f;
        //}

        if (IsOptimizedPosition)
        {
            Vertices[i1] = c.target;

            if (AttributeCount) {
                const FVector& pos = Vertices[i1];
                double wsum = AttributeQuadrics[i0].w + AttributeQuadrics[i1].w;

                if (wsum > 1e-12) {
                    double inv_wsum = 1.0 / wsum;

                    for (int k = 0; k < AttributeCount; ++k) {
                        float wk = AttributeWeights[k];
                        if (wk == 0.f) continue;

                        const FGradient& G0 = AttributeGradients[i0 * AttributeCount + k];
                        const FGradient& G1 = AttributeGradients[i1 * AttributeCount + k];

                        double gx = G0.gx + G1.gx;
                        double gy = G0.gy + G1.gy;
                        double gz = G0.gz + G1.gz;
                        double gw = G0.gw + G1.gw;

                        double gsum = pos.x * gx + pos.y * gy + pos.z * gz + gw;

                        double aw = gsum * inv_wsum;
                        Attributes[k][i1] = float(aw / double(wk));
                    }
                }
            }
        }
        //if (c.duiou) {
        //    break;
        //}
        CollapseCount += 2;
    }

    return CollapseCount;
}

bool MeshSimplifierWithAttributes::HasTriangleFlip(const FVector& vA, const FVector& vB, const FVector& vC,
                                                   const FVector& vD) {
    FVector eAB = vB - vA;
    FVector eAC = vC - vA;
    FVector eAD = vD - vA;

    FVector nBC = {eAB.y * eAC.z - eAB.z * eAC.y, eAB.z * eAC.x - eAB.x * eAC.z, eAB.x * eAC.y - eAB.y * eAC.x};

    FVector nBD = {eAB.y * eAD.z - eAB.z * eAD.y, eAB.z * eAD.x - eAB.x * eAD.z, eAB.x * eAD.y - eAB.y * eAD.x};

    double dotNB = nBC.x * nBD.x + nBC.y * nBD.y + nBC.z * nBD.z;
    double lenBC = nBC.x * nBC.x + nBC.y * nBC.y + nBC.z * nBC.z;
    double lenBD = nBD.x * nBD.x + nBD.y * nBD.y + nBD.z * nBD.z;

    return dotNB <= 0.25 * sqrt(lenBC * lenBD);
}

bool MeshSimplifierWithAttributes::IsCollapsable(const FCollapseNode& c) {
    const FVector& v0 = Vertices[c.i0];
    const FVector& v1 = Vertices[c.i1];
    static const int next[4] = {1, 2, 0, 1};
    //if (c.duiou) {
    //    //int j;
    //    //for (j = 0; j < 3; j++) {
    //    //    if (Indices[c.f0 * 3 + j] == c.i0) break;
    //    //}
    //    
    //    size_t Begin = VertexAdjacency.Begin(c.i0);
    //    for (size_t i = 0; i < VertexAdjacency.Num(c.i0); ++i) {
    //        auto& e = VertexAdjacency.Data[Begin + i];
    //        int i0 = VertexRemap[e.next];
    //        int i1 = VertexRemap[e.prev];

    //        if (i0 == i1 || (c.i0 + i0 + i1 == Indices[c.f0 * 3] + Indices[c.f0 * 3 + 1] + Indices[c.f0 * 3 + 2]) ||
    //            (c.i0 + i0 + i1 == Indices[c.f1 * 3] + Indices[c.f1 * 3 + 1] + Indices[c.f1 * 3 + 2]))
    //            continue;

    //        if (HasTriangleFlip(Vertices[i0], Vertices[i1], v0, c.target)) { return false; }
    //    }
    //    //std::cout << 1111 << std::endl;
    //    return true;
    //}
    
    size_t Begin = VertexAdjacency.Begin(c.i0);
    for (size_t i = 0; i < VertexAdjacency.Num(c.i0); ++i) {
        auto& e = VertexAdjacency.Data[Begin + i];
        int i0 = Mapping[e.next];
        int i1 = Mapping[e.prev];

        if (i0 == i1 || i0 == c.i1 || i1 == c.i1) continue;

        if (HasTriangleFlip(Vertices[i0], Vertices[i1], v0, c.target)) { return false; }
    }

    if (IsOptimizedPosition)
    {
        Begin = VertexAdjacency.Begin(c.i1);
        for (size_t i = 0; i < VertexAdjacency.Num(c.i1); ++i) {
            auto& e = VertexAdjacency.Data[Begin + i];
            int i0 = Mapping[e.next];
            int i1 = Mapping[e.prev];

            if (i0 == i1 || i0 == c.i0 || i1 == c.i0) continue;

            if (HasTriangleFlip(Vertices[i0], Vertices[i1], v1, c.target)) { return false; }
        }
    }

    return true;
}

void MeshSimplifierWithAttributes::UpdateQuadrics() {
    for (size_t v = 0; v < VertexCount; ++v) {
        const int target = Mapping[v];
        if (target == static_cast<int>(v)) continue;

        VertexQuadrics[target] += VertexQuadrics[v];

        if (!AttributeCount) continue;

        AttributeQuadrics[target] += AttributeQuadrics[v];

        const size_t src = v * AttributeCount;
        const size_t dst = target * AttributeCount;

        for (size_t k = 0; k < AttributeCount; ++k) { AttributeGradients[dst + k] += AttributeGradients[src + k]; }
    }
}

size_t MeshSimplifierWithAttributes::RemapIndices() {
    size_t write = 0;

    for (size_t read = 0; read < IndexCount; read += 3) {
        const int a = Mapping[Indices[read + 0]];
        const int b = Mapping[Indices[read + 1]];
        const int c = Mapping[Indices[read + 2]];

        if (a == b || a == c || b == c) continue;

        Indices[write + 0] = a;
        Indices[write + 1] = b;
        Indices[write + 2] = c;
        write += 3;
    }

    return write;
}
