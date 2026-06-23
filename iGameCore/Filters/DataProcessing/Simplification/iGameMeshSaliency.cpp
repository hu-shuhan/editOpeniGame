#include "iGameMeshSaliency.h"
#include <cstring>
#include <iostream>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MeshSaliencyCalculator::MeshSaliencyCalculator(std::vector<FVector>& V, std::vector<int>& F)
    : Vertices(V), Indices(F)
{
    VertexCount = V.size();
    IndexCount = F.size();
}

bool MeshSaliencyCalculator::Execute()
{
    // 初始化数据结构
    GaussianCurvature.resize(VertexCount, 0.0);
    MeanCurvatureNormal.resize(VertexCount);
    MeanCurvature.resize(VertexCount, 0.0);
    FinalCurvature.resize(VertexCount, 0.0);
    NormalCurvature.resize(VertexCount, 0.0);
    Saliency.resize(VertexCount, 0.0);

    VertexAdjacency.Offsets.resize(VertexCount + 1);
    VertexAdjacency.Data.resize(IndexCount);

    // 步骤1: 构建邻接关系
    BuildAdjacency();

    // 步骤2: 计算曲率
    ComputeCurvature();

    // 步骤2.1: 对基于法向量的曲率场进行拉普拉斯平滑
    SmoothNormalCurvature();

    SmoothMeanCurvature();

    for (int i = 0; i < VertexCount; ++i) {
        FinalCurvature[i] = 4.0 * MeanCurvature[i] - 2.0 * GaussianCurvature[i];
    }

    // 步骤3: 计算显著性
    ComputeSaliency();

    return true;
}

void MeshSaliencyCalculator::SetBilateralParameters(double sigmaD, double sigmaR)
{
    SigmaD = sigmaD;
    SigmaR = sigmaR;
}

void MeshSaliencyCalculator::BuildAdjacency()
{
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

void MeshSaliencyCalculator::ComputeCurvature()
{
    for (int i = 0; i < VertexCount; ++i) {
        GaussianCurvature[i] = ComputeGaussianCurvature(i);
        MeanCurvature[i] = ComputeMeanCurvature(i);
        FinalCurvature[i] = 4.0 * MeanCurvature[i] - 2.0 * GaussianCurvature[i];
        NormalCurvature[i] = ComputeNormalBasedCurvature(i);
    }
}

void MeshSaliencyCalculator::SmoothNormalCurvature()
{
    if (VertexCount == 0) return;

    const int iterations = 0;
    const double mu = 0.3;     // 平滑因子
    const double eta = 1e-4;   // 余切权重修正参数

    std::vector<double> s = NormalCurvature;
    std::vector<double> delta(VertexCount, 0.0);

    for (int it = 0; it < iterations; ++it) {
        std::fill(delta.begin(), delta.end(), 0.0);

        for (int i = 0; i < VertexCount; ++i) {
            std::vector<int> neighbors = GetOneRingNeighbors(i);
            if (neighbors.empty()) continue;

            const FVector& vi = Vertices[i];

            double Wi = 0.0;
            double accum = 0.0;

            int begin = VertexAdjacency.Begin(i);
            int count = VertexAdjacency.Num(i);

            for (int vj : neighbors) {
                double cotAlpha = 0.0;
                double cotBeta = 0.0;
                int foundCount = 0;

                for (int k = 0; k < count; ++k) {
                    auto& edge = VertexAdjacency.Data[begin + k];

                    if (edge.next == vj || edge.prev == vj) {
                        int opposite = (edge.next == vj) ? edge.prev : edge.next;
                        const FVector& vo = Vertices[opposite];
                        const FVector& vjPos = Vertices[vj];

                        FVector e1 = vi - vo;
                        FVector e2 = vjPos - vo;
                        double dot = e1.Dot(e2);
                        double cross_len = (e1.Cross(e2)).Length();
                        if (cross_len > 1e-10) {
                            double cotVal = dot / cross_len;
                            if (foundCount == 0) cotAlpha = cotVal;
                            else cotBeta = cotVal;
                            foundCount++;
                            if (foundCount == 2) break;
                        }
                    }
                }

                if (foundCount == 0) continue;

                double cotSum = cotAlpha + cotBeta;
                double wij = (cotSum > 0.0) ? (0.5 * cotSum) : eta;

                Wi += wij;
                accum += wij * (s[vj] - s[i]);
            }

            if (Wi > 0.0) {
                delta[i] = accum / Wi;
            } else {
                delta[i] = 0.0;
            }
        }

        for (int i = 0; i < VertexCount; ++i) {
            s[i] = s[i] + (1.0 - mu) * delta[i];
        }
    }

    for (int i = 0; i < VertexCount; ++i) { 
        s[i] = std::max(s[i], NormalCurvature[i]);
    }

    NormalCurvature.swap(s);
}

double MeshSaliencyCalculator::ComputeGaussianCurvature(int vertexIndex)
{
    // 公式 8: K_G(v_i) = (1/(4*AM)) * (2π - Σ_{v_j∈N(i)} θ_j)
    // θ_j 是环绕 v_i 的三角形在 v_j 处的内角

    double angleSum = 0.0;

    // 遍历该顶点的所有邻接三角形
    int begin = VertexAdjacency.Begin(vertexIndex);
    int count = VertexAdjacency.Num(vertexIndex);

    for (int j = 0; j < count; ++j) {
        auto& edge = VertexAdjacency.Data[begin + j];
        int v1 = edge.next;
        int v2 = edge.prev;

        // 计算在顶点 vertexIndex 处的内角
        double angle = ComputeAngleAtVertex(Vertices[vertexIndex], Vertices[v1], Vertices[v2]);
        angleSum += angle;
    }

    // 计算混合Voronoi区域面积
    //double AM = ComputeMixedArea(vertexIndex);

    // 面积过小会导致曲率数值非常大，这里设置一个下限做稳定化
    //const double areaEps = 1e-6;
    //if (AM < areaEps) {
    //    AM = areaEps;
    //}

    // K_G = (2π - Σθ_j) / (4*AM)
    // 注意: 原公式是 K_G = (2π - Σθ_j) / A，这里用 1/(4*AM) 作为归一化
    double KG = (2.0 * M_PI - angleSum); // / (4.0 * AM);

    // 进一步裁剪极端异常的曲率值，避免可视化时被少量异常点主导
    //const double maxCurv = 1e3;
    //if (KG > maxCurv) KG = maxCurv;
    //else if (KG < -maxCurv) KG = -maxCurv;

    return KG;
}

double MeshSaliencyCalculator::ComputeMeanCurvature(int vertexIndex)
{
    FVector K = ComputeMeanCurvatureNormal(vertexIndex);
    MeanCurvatureNormal[vertexIndex] = K;

    double H = 0.5 * K.Length();

    FVector normal = ComputeVertexNormal(vertexIndex);
    double sign = (K.Dot(normal) >= 0.0) ? 1.0 : -1.0;
    H *= sign;

    const double maxMeanCurv = 1e3;
    if (H > maxMeanCurv) H = maxMeanCurv;
    else if (H < -maxMeanCurv) H = -maxMeanCurv;

    return H;
}

FVector MeshSaliencyCalculator::ComputeMeanCurvatureNormal(int vertexIndex)
{
    double AM = ComputeMixedArea(vertexIndex);

    //const double areaEps = 1e-6;
    //if (AM < areaEps) AM = areaEps;

    const FVector& vi = Vertices[vertexIndex];
    std::vector<int> neighbors = GetOneRingNeighbors(vertexIndex);

    FVector sum = {0, 0, 0};

    int begin = VertexAdjacency.Begin(vertexIndex);
    int count = VertexAdjacency.Num(vertexIndex);

    for (int vj : neighbors) {
        double cotAlpha = 0.0;
        double cotBeta = 0.0;
        int foundCount = 0;

        for (int k = 0; k < count; ++k) {
            auto& edge = VertexAdjacency.Data[begin + k];

            if (edge.next == vj) {
                int opposite = edge.prev;
                const FVector& vo = Vertices[opposite];
                const FVector& vjPos = Vertices[vj];

                FVector e1 = vi - vo;
                FVector e2 = vjPos - vo;
                double dot = e1.Dot(e2);
                double cross_len = (e1.Cross(e2)).Length();
                if (cross_len > 1e-15) {
                    double cotVal = dot / cross_len;
                    if (foundCount == 0) cotAlpha = cotVal;
                    else cotBeta = cotVal;
                    foundCount++;
                }
            } else if (edge.prev == vj) {
                int opposite = edge.next;
                const FVector& vo = Vertices[opposite];
                const FVector& vjPos = Vertices[vj];

                FVector e1 = vi - vo;
                FVector e2 = vjPos - vo;
                double dot = e1.Dot(e2);
                double cross_len = (e1.Cross(e2)).Length();
                if (cross_len > 1e-15) {
                    double cotVal = dot / cross_len;
                    if (foundCount == 0) cotAlpha = cotVal;
                    else cotBeta = cotVal;
                    foundCount++;
                }
            }

            if (foundCount == 2) break;
        }

        FVector diff = vi - Vertices[vj];
        double weight = cotAlpha + cotBeta;

        //const double maxCot = 1e3;
        //if (weight > maxCot) weight = maxCot;
        //else if (weight < -maxCot) weight = -maxCot;

        sum = sum + diff * static_cast<float>(weight);
    }

    double scale = 1.0 / (2.0 * AM);
    sum = sum * static_cast<float>(scale);

    return sum;
}

void MeshSaliencyCalculator::SmoothMeanCurvature()
{
    if (VertexCount == 0) return;

    const int iterations = 2;
    const double mu = 0.3;
    const double eta = 1e-4;

    std::vector<double> h = MeanCurvature;
    std::vector<double> delta(VertexCount, 0.0);

    for (int it = 0; it < iterations; ++it) {
        std::fill(delta.begin(), delta.end(), 0.0);

        for (int i = 0; i < VertexCount; ++i) {
            std::vector<int> neighbors = GetOneRingNeighbors(i);
            if (neighbors.empty()) continue;

            const FVector& vi = Vertices[i];

            double Wi = 0.0;
            double accum = 0.0;

            int begin_i = VertexAdjacency.Begin(i);
            int count_i = VertexAdjacency.Num(i);

            for (int vj : neighbors) {
                double cotAlpha = 0.0;
                double cotBeta = 0.0;
                int foundCount = 0;

                for (int k = 0; k < count_i; ++k) {
                    auto& edge = VertexAdjacency.Data[begin_i + k];

                    if (edge.next == vj || edge.prev == vj) {
                        int opposite = (edge.next == vj) ? edge.prev : edge.next;
                        const FVector& vo = Vertices[opposite];
                        const FVector& vjPos = Vertices[vj];

                        FVector e1 = vi - vo;
                        FVector e2 = vjPos - vo;
                        double dot = e1.Dot(e2);
                        double cross_len = (e1.Cross(e2)).Length();
                        if (cross_len > 1e-10) {
                            double cotVal = dot / cross_len;
                            if (foundCount == 0) cotAlpha = cotVal;
                            else cotBeta = cotVal;
                            foundCount++;
                            if (foundCount == 2) break;
                        }
                    }
                }

                if (foundCount == 0) continue;

                double cotSum = cotAlpha + cotBeta;
                double wij = (cotSum > 0.0) ? (0.5 * cotSum) : eta;

                Wi += wij;
                accum += wij * (h[vj] - h[i]);
            }

            if (Wi > 0.0) {
                delta[i] = accum / Wi;
            } else {
                delta[i] = 0.0;
            }
        }

        for (int i = 0; i < VertexCount; ++i) {
            h[i] = h[i] + (1.0 - mu) * delta[i];
        }
    }

    MeanCurvature.swap(h);
}

double MeshSaliencyCalculator::ComputeMixedArea(int vertexIndex)
{
    double area = 0.0;

    int begin = VertexAdjacency.Begin(vertexIndex);
    int count = VertexAdjacency.Num(vertexIndex);

    const FVector& pi = Vertices[vertexIndex];

    for (int j = 0; j < count; ++j) {
        auto& edge = VertexAdjacency.Data[begin + j];
        int v1 = edge.next;
        int v2 = edge.prev;

        const FVector& pj = Vertices[v1];
        const FVector& pk = Vertices[v2];

        FVector eij = pj - pi;
        FVector eik = pk - pi;

        FVector cross_i = eij.Cross(eik);
        double crossLen = cross_i.Length();
        if (crossLen < 1e-20) continue;

        double triangleArea = 0.5 * crossLen;

        double dot_i = eij.Dot(eik);

        FVector eji = pi - pj;
        FVector ejk = pk - pj;
        double dot_j = eji.Dot(ejk);

        FVector eki = pi - pk;
        FVector ekj = pj - pk;
        double dot_k = eki.Dot(ekj);

        bool obtuse_i = dot_i < 0.0;
        bool obtuse_j = dot_j < 0.0;
        bool obtuse_k = dot_k < 0.0;

        if (obtuse_i) {
            area += 0.5 * triangleArea;
            continue;
        }

        if (obtuse_j || obtuse_k) {
            area += 0.25 * triangleArea;
            continue;
        }

        FVector cross_j = eji.Cross(ejk);
        double crossLen_j = cross_j.Length();
        double cot_j = (crossLen_j > 1e-20) ? (dot_j / crossLen_j) : 0.0;

        FVector cross_k = eki.Cross(ekj);
        double crossLen_k = cross_k.Length();
        double cot_k = (crossLen_k > 1e-20) ? (dot_k / crossLen_k) : 0.0;

        double len_ij2 = eij.LengthSquared();
        double len_ik2 = eik.LengthSquared();

        area += 0.125 * (cot_j * len_ik2 + cot_k * len_ij2);
    }

    return area;
}

double MeshSaliencyCalculator::ComputeNormalBasedCurvature(int vertexIndex)
{
    FVector Ni = ComputeVertexNormal(vertexIndex);

    std::vector<int> neighbors = GetOneRingNeighbors(vertexIndex);

    double maxAngle = 0.0;

    for (int vj : neighbors) {
        FVector Nj = ComputeVertexNormal(vj);

        double dot = Ni.Dot(Nj);
        if (dot > 1.0) dot = 1.0;
        else if (dot < -1.0) dot = -1.0;

        double angle = std::acos(dot);
        if (angle > maxAngle) {
            maxAngle = angle;
        }
    }

    return maxAngle;
}

FVector MeshSaliencyCalculator::ComputeVertexNormal(int vertexIndex)
{
    // 计算顶点法向量 (加权平均邻接三角形的法向量)
    FVector normal = {0, 0, 0};

    int begin = VertexAdjacency.Begin(vertexIndex);
    int count = VertexAdjacency.Num(vertexIndex);

    for (int j = 0; j < count; ++j) {
        auto& edge = VertexAdjacency.Data[begin + j];
        int v1 = edge.next;
        int v2 = edge.prev;

        const FVector& p0 = Vertices[vertexIndex];
        const FVector& p1 = Vertices[v1];
        const FVector& p2 = Vertices[v2];

        // 计算三角形法向量
        FVector e1 = p1 - p0;
        FVector e2 = p2 - p0;
        FVector faceNormal = e1.Cross(e2);

        normal = normal + faceNormal;
    }

    normal.Normalize();
    return normal;
}

void MeshSaliencyCalculator::ComputeSaliency()
{
    // 公式 15: 基于双边滤波的网格显著性计算
    // M(v_x) = Σ_{v_y∈P}(C(v_x) - C(v_y)) · g_d(||v_x - v_y||) · f_r(|C(v_x) - C(v_y)|) / W
    // W = Σ_{v_y∈P} g_d(||v_x - v_y||) · f_r(|C(v_x) - C(v_y)|)  (公式 12)

    // 为了避免极端曲率差和极小权重导致的数值问题，这里对曲率差和最终显著性做适度约束
    const double maxCurvDiff = 1e3;   // 曲率差上限
    const double minWeight   = 1e-12; // 最小有效权重
    const double maxSaliency = 1e3;   // 显著性上限

    for (int x = 0; x < VertexCount; ++x) {
        // P 是顶点 v_x 的一阶邻域
        std::vector<int> neighbors = GetOneRingNeighbors(x);

        double W = 0.0;         // 归一化因子 (公式 12)
        double numerator = 0.0; // 分子

        const FVector& vx = Vertices[x];
        double Cx = FinalCurvature[x];

        for (int y : neighbors) {
            const FVector& vy = Vertices[y];
            double Cy = FinalCurvature[y];

            // 计算空间距离
            FVector diff = vx - vy;
            double distance = diff.Length();

            // 计算曲率差，并限制其范围
            double rawDiff = Cx - Cy;
            double curvatureDiff = std::abs(rawDiff);
            if (curvatureDiff > maxCurvDiff) {
                rawDiff = (rawDiff > 0.0 ? maxCurvDiff : -maxCurvDiff);
                curvatureDiff = maxCurvDiff;
            }

            // 空间域核 g_d (公式 13)
            double gd = SpatialKernel(distance);

            // 范围域核 f_r (公式 14)
            double fr = RangeKernel(curvatureDiff);

            double w = gd * fr;
            if (w < minWeight) continue;

            // 累加到归一化因子
            W += w;

            // 累加到分子
            numerator += rawDiff * w;
        }

        // 计算显著性值 M(v_x) (公式 15)
        if (W > minWeight) {
            double M = numerator / W;
            // 显著性为“差异强度”，通常取非负
            M = std::abs(M);
            if (M > maxSaliency) M = maxSaliency;
            Saliency[x] = M;
        } else {
            Saliency[x] = 0.0;
        }
    }
}

double MeshSaliencyCalculator::SpatialKernel(double distance)
{
    // 公式 13: g_d(||v_x - v_y||) = exp(-||v_x - v_y||^2 / (2 * σ_d^2))
    double sigma2 = 2.0 * SigmaD * SigmaD;
    return std::exp(-(distance * distance) / sigma2);
}

double MeshSaliencyCalculator::RangeKernel(double curvatureDiff)
{
    // 公式 14: f_r(|C(v_x) - C(v_y)|) = exp(-(C(v_x) - C(v_y))^2 / (2 * σ_r^2))
    double sigma2 = 2.0 * SigmaR * SigmaR;
    return std::exp(-(curvatureDiff * curvatureDiff) / sigma2);
}

std::vector<int> MeshSaliencyCalculator::GetOneRingNeighbors(int vertexIndex)
{
    // 获取顶点的一阶邻域 (所有直接相连的顶点)
    std::set<int> neighborSet;

    int begin = VertexAdjacency.Begin(vertexIndex);
    int count = VertexAdjacency.Num(vertexIndex);

    for (int j = 0; j < count; ++j) {
        auto& edge = VertexAdjacency.Data[begin + j];
        neighborSet.insert(edge.next);
        neighborSet.insert(edge.prev);
    }

    return std::vector<int>(neighborSet.begin(), neighborSet.end());
}

double MeshSaliencyCalculator::ComputeAngleAtVertex(const FVector& v, const FVector& v1, const FVector& v2)
{
    // 计算在顶点 v 处，由 v1 和 v2 形成的夹角
    FVector e1 = v1 - v;
    FVector e2 = v2 - v;

    double len1 = e1.Length();
    double len2 = e2.Length();

    //if (len1 < 1e-10 || len2 < 1e-10) {
    //    return 0.0;
    //}

    double cosAngle = e1.Dot(e2) / (len1 * len2);
    
    // Clamp to [-1, 1] to avoid numerical issues with acos
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle));

    return std::acos(cosAngle);
}
