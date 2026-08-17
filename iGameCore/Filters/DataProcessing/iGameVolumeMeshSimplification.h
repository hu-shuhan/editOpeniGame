#pragma once
#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include "iGameFilter.h"
#include "Simplification/iGameMeshSimplificationUtil.h"
#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGamePoints.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class TetraSimplification : public Filter {
public:
    I_OBJECT(TetraSimplification);
    static Pointer New() { return new TetraSimplification; }

    bool Execute() override;

    // ─── Setters (called from UI) ───
    void SetTargetReduction(float r) { m_TargetReduction = r; }
    void SetTargetTetraCount(int n) { m_TargetTetraCount = n; }
    void SetBoundaryPenalty(double p) { m_BoundaryPenalty = p; }
    void SetLambda(double l) { m_Lambda = l; }
    void SetPreserveBoundary(bool b) { m_PreserveBoundary = b; }
    void SetUseAllPointAttributes(bool b) { m_UseAllPointAttributes = b; }
    void SetAttributeWeights(const std::vector<float>& w) { m_AttributeWeights = w; }
    void SetStretchFactor(double s) { m_StretchFactor = s; }
    void SetMaxAspectRatio(double a) { m_MaxAspectRatio = a; }

private:
    TetraSimplification() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    };
    ~TetraSimplification() override = default;

    // ─── Pipeline stages ───
    bool LoadMesh();
    void Normalize();
    void BuildTopology();
    void BuildADQ();
    void InitHeap();
    void Simplify();
    bool SaveMesh();

    // ─── Cost computation ───
    double ADQCostOnly(int ti) const;

    struct CostResult {
        double cost;
        double optPos[3];
        std::vector<double> optAttr; // D-dimensional
        bool valid;
    };
    CostResult ComputeCost(int ti);

    // ─── Collapse execution ───
    void DoCollapse(int ti, const double optPos[3],
                    const std::vector<double>& optAttr);

    // ─── Inline 3×3 helpers (symmetric matrix stored as double[9]) ───
    // A stored row-major: A[r*3+c]
    static inline double Sym3_vAv(const double A[9], double dx, double dy, double dz) {
        return A[0]*dx*dx + A[4]*dy*dy + A[8]*dz*dz
             + 2.0*(A[1]*dx*dy + A[2]*dx*dz + A[5]*dy*dz);
    }
    static inline double Sym3_trace(const double A[9]) {
        return A[0] + A[4] + A[8];
    }

    // ─── Heap entry ───
    struct HeapEntry {
        double cost;
        int tetIdx;
        int version;
        bool operator>(const HeapEntry& o) const { return cost > o.cost; }
    };

    // ─── User parameters ───
    float  m_TargetReduction = 0.5f;
    int    m_TargetTetraCount = 0;
    double m_BoundaryPenalty = 100.0;
    double m_Lambda = 0.1;
    bool   m_PreserveBoundary = false;
    bool   m_UseAllPointAttributes = true;
    std::vector<float> m_AttributeWeights;

    // ─── Geometric quality parameters ───
    double m_StretchFactor = 10.0;
    double m_MaxAspectRatio = 30.0;

    // ─── Mesh data — flat arrays for cache performance ───
    int m_NumVerts = 0;
    int m_NumTets  = 0;
    int m_AttrDim  = 0;

    std::vector<double> m_Pts;     // [N*3] flat: x,y,z per vertex
    std::vector<double> m_Attrs;   // [N*D] flat: per vertex
    std::vector<int>    m_TetVerts; // [M*4] flat: 4 vert indices per tet

    // Attribute metadata
    struct AttrInfo { std::string name; int ncomp; };
    std::vector<AttrInfo> m_AttrInfo;

    // ─── Normalization ───
    double m_PtsMin[3] = {0,0,0};
    double m_PtsScale = 1.0;

    struct AttrNormParam {
        bool isScalar;
        double minVal, range, maxMag;
        int col, ncomp;
    };
    std::vector<AttrNormParam> m_AttrNormParams;

    // ─── Topology — dynamic adjacency using vectors ───
    std::vector<std::vector<int>> m_VertTets;  // vertex → list of tet indices

    std::vector<uint8_t> m_TetAlive;
    std::vector<uint8_t> m_VertAlive;
    std::vector<uint8_t> m_IsBoundary;       // per vertex
    std::vector<uint8_t> m_IsBoundaryTet;    // per tet

    // ─── ADQ matrices — flat [N*9] row-major ───
    std::vector<double> m_ADQ;       // [N*9] symmetric 3×3 per vertex
    std::vector<double> m_ErrAccum;  // [N]

    // ─── Priority queue ───
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>> m_Heap;
    std::vector<int> m_TetVersion;

    // ─── Scratch buffer for visited tets (reused, avoids alloc per call) ───
    mutable std::vector<uint8_t> m_Visited; // [M] stamped with generation
    mutable int m_VisitGen = 0;

    // ─── Input/Output references ───
    VolumeMesh::Pointer m_InputMesh;
};

// ════════════════════════════════════════════════════════════════════
//  TetraEdgeSimplification — ADQ-based EDGE collapse for tet meshes
// ════════════════════════════════════════════════════════════════════
class TetraEdgeSimplification : public Filter {
public:
    I_OBJECT(TetraEdgeSimplification);
    static Pointer New() { return new TetraEdgeSimplification; }

    bool Execute() override;

    // ─── Setters ───
    void SetTargetReduction(float r) { m_TargetReduction = r; }
    void SetTargetTetraCount(int n) { m_TargetTetraCount = n; }
    void SetBoundaryPenalty(double p) { m_BoundaryPenalty = p; }
    void SetLambda(double l) { m_Lambda = l; }
    void SetPreserveBoundary(bool b) { m_PreserveBoundary = b; }
    void SetUseAllPointAttributes(bool b) { m_UseAllPointAttributes = b; }
    void SetAttributeWeights(const std::vector<float>& w) { m_AttributeWeights = w; }
    void SetStretchFactor(double s) { m_StretchFactor = s; }
    void SetMaxAspectRatio(double a) { m_MaxAspectRatio = a; }

private:
    TetraEdgeSimplification() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~TetraEdgeSimplification() override = default;

    // ─── Pipeline ───
    bool LoadMesh();
    void Normalize();
    void BuildTopology();
    void BuildADQ();
    void BuildEdgesAndHeap();
    void Simplify();
    bool SaveMesh();

    // ─── Edge cost ───
    struct EdgeCostResult {
        double cost;
        double optPos[3];
        std::vector<double> optAttr;
        bool valid;
    };
    double EdgeCostFast(int va, int vb) const;
    EdgeCostResult ComputeEdgeCost(int va, int vb);

    // ─── Edge collapse ───
    void DoEdgeCollapse(int va, int vb, const double optPos[3],
                        const std::vector<double>& optAttr);

    // ─── Inline 3×3 helpers ───
    static inline double Sym3_vAv(const double A[9], double dx, double dy, double dz) {
        return A[0]*dx*dx + A[4]*dy*dy + A[8]*dz*dz
             + 2.0*(A[1]*dx*dy + A[2]*dx*dz + A[5]*dy*dz);
    }
    static inline double Sym3_trace(const double A[9]) {
        return A[0] + A[4] + A[8];
    }

    // ─── Heap entry (keyed by edge) ───
    struct EdgeHeapEntry {
        double cost;
        int va, vb;          // edge endpoints (va < vb)
        int versionA, versionB;
        bool operator>(const EdgeHeapEntry& o) const { return cost > o.cost; }
    };

    // ─── Parameters ───
    float  m_TargetReduction = 0.5f;
    int    m_TargetTetraCount = 0;
    double m_BoundaryPenalty = 100.0;
    double m_Lambda = 0.1;
    bool   m_PreserveBoundary = false;
    bool   m_UseAllPointAttributes = true;
    std::vector<float> m_AttributeWeights;
    double m_StretchFactor = 10.0;
    double m_MaxAspectRatio = 30.0;

    // ─── Mesh data ───
    int m_NumVerts = 0;
    int m_NumTets  = 0;
    int m_AttrDim  = 0;

    std::vector<double> m_Pts;      // [N*3]
    std::vector<double> m_Attrs;    // [N*D]
    std::vector<int>    m_TetVerts; // [M*4]

    struct AttrInfo { std::string name; int ncomp; };
    std::vector<AttrInfo> m_AttrInfo;

    // ─── Normalization ───
    double m_PtsMin[3] = {0,0,0};
    double m_PtsScale = 1.0;
    struct AttrNormParam {
        bool isScalar;
        double minVal, range, maxMag;
        int col, ncomp;
    };
    std::vector<AttrNormParam> m_AttrNormParams;

    // ─── Topology ───
    std::vector<std::vector<int>> m_VertTets;
    std::vector<uint8_t> m_TetAlive;
    std::vector<uint8_t> m_VertAlive;
    std::vector<uint8_t> m_IsBoundary;
    std::vector<uint8_t> m_IsBoundaryTet;

    // ─── ADQ ───
    std::vector<double> m_ADQ;       // [N*9]
    std::vector<double> m_ErrAccum;  // [N]

    // ─── Edge heap ───
    std::priority_queue<EdgeHeapEntry, std::vector<EdgeHeapEntry>,
                        std::greater<EdgeHeapEntry>> m_Heap;
    std::vector<int> m_VertVersion;  // per-vertex version for lazy deletion

    // ─── Input/Output ───
    VolumeMesh::Pointer m_InputMesh;
};

IGAME_NAMESPACE_END
