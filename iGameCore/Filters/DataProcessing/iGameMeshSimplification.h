#include <Eigen/Dense>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "iGameMeshSimplificationUtil.h"

class ManifoldSimplifier {
public:
    std::vector<FVector>& Vertices;
    std::vector<int>& Indices;                // 三角形索引
    const std::vector<float>& LatentFeatures; // 每个顶点的潜变量 (Flattened: v0_0, v0_1... v1_0...)
    const std::vector<float>& MetricTensors;  // 每个顶点的度量张量 (Flattened: v0_00...v0_08, v1_00...)

    bool bHasLatent = false;
    int LatentDim = 0;
    int VertexCount = 0;
    int IndexCount = 0;
    int TargetCount = 0;

    // QEM 状态
    std::vector<FQuadric> VertexQuadrics;
    std::vector<FQuadric> LatentQuadrics;
    std::vector<unsigned char> VertexRemoved;
    std::vector<FCollapseNode> Collapses; // 坍缩边数组
    std::vector<int> CollapseOrder;       // 坍缩权重的排序数组
    std::vector<int> VertexRemap;
    std::vector<unsigned char> VertexLocked; // 用于标记顶点是否被坍缩

    // 顶点的邻接关系
    FVertexAdjacency VertexAdjacency;

    // 算法参数
    float Lambda = 1.0; // 属性误差权重

    float mmin = 100000;
    float mmax = 0;

public:
    ManifoldSimplifier(std::vector<FVector>& V, std::vector<int>& F, const std::vector<float>& Features = {},
                       const std::vector<float>& Metrics = {});

    // 入口函数
    bool Execute();

private:
    void BuildAdjacency();

    FCollapseNode ComputeCost(int v1, int v2);

    size_t BuildEdgeCollapses();

    void SortEdgeCollapses(size_t Count);

    size_t ExecuteEdgeCollapses(size_t Count);

    static bool HasTriangleFlip(const FVector& a, const FVector& b, const FVector& c, const FVector& d);

    bool IsCollapsable(const FCollapseNode& c);

    void UpdateQuadrics();

    size_t RemapIndices();

    void InitializeQuadrics();

    void ComputeSpatialSensitivity(int v0, int v1, int v2, double M[9], double A_out[3][3]);
};