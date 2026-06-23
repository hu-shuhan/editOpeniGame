#pragma once
#include <Eigen/Dense>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "iGameMeshSimplificationUtil.h"

class MeshSimplifierWithAttributes {
public:
    std::vector<FVector>& Vertices;
    std::vector<int>& Indices;
    std::vector<std::vector<float>>& Attributes;
    std::vector<float>& AttributeWeights;
    std::vector<double>& VertexImportance;

    // QEM 状态
    std::vector<FQuadric> VertexQuadrics;
    std::vector<FQuadric> AttributeQuadrics;
    std::vector<FGradient> AttributeGradients;

    std::vector<unsigned char> VertexRemoved;
    std::vector<FCollapseNode> Collapses;
    std::vector<int> Order;      
    std::vector<int> Mapping;
    std::vector<unsigned char> Locked; 
    std::vector<unsigned char> Kind;
    std::vector<unsigned int> L1;
    std::vector<unsigned int> L2;

    // 顶点的邻接面关系
    FVertexAdjacency VertexAdjacency;

    float Lambda = 0.5f; // 属性误差权重

    bool UseVertexImportance = true;
    bool UseDynamicAttributePenalty = true;
    double VertexImportanceScale = 10.0;

    int VertexCount = 0;
    int IndexCount = 0;
    int AttributeCount = 0;
    int TargetCount = 0;
    float FeatureAngle = 60.0f;
    float CosAngle = 0.0f;
    bool IsOptimizedPosition = false;
    int a = 0;

    std::vector<FVector> TargetNormalSums;
    std::vector<float> TargetNormalWeights;

    bool GuidedUpdateEnabled = false;
    int GuidedUpdateIterations = 4;
    float GuidedUpdateLambda = 0.2f;

public:
    MeshSimplifierWithAttributes(std::vector<FVector>& V, std::vector<int>& F, std::vector<std::vector<float>>& A,
                                 std::vector<float>& AttributeWeights, std::vector<double>& VertexImportance,
                                 double Target = 0.5, bool IsOptimizedPosition = false);

    void EnableGuidedUpdate(int iterations = 4, float lambda = 0.2f);

    void SetUseVertexImportance(bool enabled) { UseVertexImportance = enabled; }

    void SetUseDynamicAttributePenalty(bool enabled) { UseDynamicAttributePenalty = enabled; }

    void SetVertexImportanceScale(double scale) { VertexImportanceScale = scale; }

    static void ComputeLPCA(std::vector<FVector>& V, std::vector<int>& F, std::vector<std::vector<float>>& A,
                            std::vector<double>& VertexImportance);

    static void ComputeHeatDiffusionSaliency(std::vector<FVector>& V, std::vector<int>& F,
                                             std::vector<double>& VertexImportance);

    // 入口函数
    bool Execute();

private:
    void ComputeVertexImportanceUsingLPCA();

    void ComputeVertexImportanceUsingHeatDiffusion();

    void BuildAdjacency();

    void ClassifyVertices();

    void InitializeQuadrics();

    FCollapseNode ComputeEdgeCost(int i0, int i1);

    size_t BuildEdgeCollapses();

    void SortEdgeCollapses(size_t Count);

    size_t ExecuteEdgeCollapses(size_t Count);

    static bool HasTriangleFlip(const FVector& a, const FVector& b, const FVector& c, const FVector& d);

    bool IsCollapsable(const FCollapseNode& c);

    void UpdateQuadrics();

    size_t RemapIndices();
};
