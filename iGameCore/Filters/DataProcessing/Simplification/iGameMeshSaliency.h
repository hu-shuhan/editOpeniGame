#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "iGameMeshSimplificationUtil.h"

/**
 * 网格显著性计算器
 * 基于曲率和双边滤波计算网格的显著性
 * 
 * 算法步骤:
 * 1. 计算每个顶点的高斯曲率 K_G(v) 和平均曲率 K_H(v)
 * 2. 计算最终曲率 C(v) = 4*K_H(v) - 2*K_G(v)
 * 3. 使用双边滤波计算网格显著性 M(v)
 */
class MeshSaliencyCalculator {
public:
    std::vector<FVector>& Vertices;
    std::vector<int>& Indices;

    // 顶点曲率数据
    std::vector<double> GaussianCurvature;  // K_G(v_i)
    std::vector<FVector> MeanCurvatureNormal; // K(v_i)
    std::vector<double> MeanCurvature;      // K_H(v_i)
    std::vector<double> FinalCurvature;     // C(v_i)
    std::vector<double> NormalCurvature;    // S(v_i)
    std::vector<double> Saliency;           // M(v_x)

    

    // 顶点的邻接关系
    FVertexAdjacency VertexAdjacency;

    // 双边滤波参数
    double SigmaD = 1.0;  // 空间域核参数
    double SigmaR = 1.0;  // 范围域核参数

    int VertexCount = 0;
    int IndexCount = 0;

public:
    MeshSaliencyCalculator(std::vector<FVector>& V, std::vector<int>& F);

    /**
     * 执行显著性计算
     */
    bool Execute();

    /**
     * 设置双边滤波参数
     * @param sigmaD 空间域核参数
     * @param sigmaR 范围域核参数
     */
    void SetBilateralParameters(double sigmaD, double sigmaR);

private:
    /**
     * 构建顶点邻接关系
     */
    void BuildAdjacency();

    /**
     * 计算每个顶点的曲率
     */
    void ComputeCurvature();

    /**
     * 计算高斯曲率 K_G(v_i)
     * 公式 8: K_G(v_i) = (1/(4*AM)) * (2π - Σ_{v_j∈N(i)} θ_j)
     */
    double ComputeGaussianCurvature(int vertexIndex);

    /**
     * 计算平均曲率 K_H(v_i)
     * 公式 9: K_H(v_i) = (1/(4*AM)) * Σ_{v_j∈N(i)} (cot α_j + cot β_j)(v_i - v_j) · N(i)
     */
    double ComputeMeanCurvature(int vertexIndex);

    FVector ComputeMeanCurvatureNormal(int vertexIndex);

    /**
     * 基于法向量的初始离散曲率场 S(v_i)
     * S(v_i) = max_{v_j∈R(i)} arccos(N(v_i) · N(v_j))
     */
    double ComputeNormalBasedCurvature(int vertexIndex);

    /**
     * 对基于法向量的曲率场进行拉普拉斯平滑
     */
    void SmoothNormalCurvature();

    void SmoothMeanCurvature();

    /**
     * 计算混合Voronoi区域面积 AM
     */
    double ComputeMixedArea(int vertexIndex);

    /**
     * 计算顶点法向量 N(i)
     */
    FVector ComputeVertexNormal(int vertexIndex);

    /**
     * 计算网格显著性 (基于双边滤波)
     */
    void ComputeSaliency();

    /**
     * 空间域核 g_d (公式 13)
     * g_d(||v_x - v_y||) = exp(-||v_x - v_y||^2 / (2 * σ_d^2))
     */
    double SpatialKernel(double distance);

    /**
     * 范围域核 f_r (公式 14)
     * f_r(|C(v_x) - C(v_y)|) = exp(-(C(v_x) - C(v_y))^2 / (2 * σ_r^2))
     */
    double RangeKernel(double curvatureDiff);

    /**
     * 获取顶点的一阶邻域 (所有直接相连的顶点)
     */
    std::vector<int> GetOneRingNeighbors(int vertexIndex);

    /**
     * 计算三角形在顶点处的内角
     */
    double ComputeAngleAtVertex(const FVector& v, const FVector& v1, const FVector& v2);
};
