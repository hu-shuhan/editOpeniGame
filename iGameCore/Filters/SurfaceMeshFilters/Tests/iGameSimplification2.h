#ifndef Simplification2_h
#define Simplification2_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGamePriorityQueue.h"
#include "iGameQuadric.h"
#include "iGameFlexArray.h"
#include "iGameUnstructuredMesh.h"
#include "iGameSurMeshFilterBase.h"

IGAME_NAMESPACE_BEGIN
class Simplification2 : public Filter {
public:
    I_OBJECT(Simplification2);
    static Pointer New() { return new Simplification2; }

    static constexpr int QEM_FASTEST = 0;
    static constexpr int QEM_NICEST = 1;

    static constexpr int QEM_INTERIOR_EDGE = 0;      // 内部边
    static constexpr int QEM_HALF_BOUNDARY_EDGE = 1; // 边界半边
    static constexpr int QEM_BOUNDARY_EDGE = 2;      // 边界边

    void SetTargetReduction(double target) {  this->TargetReduction = target; }

    void SetPreserveBoundary(bool flag) { this->PreserveBoundary = flag; }

    void SetActivedAttribIndices(const std::vector<int>& list) { activedAttribIndices = list; }

    void SetAllScalarCheck(bool flag) { this->IsAllScalarCheck = flag; }

    bool Execute() override;

private:
    int TargetFaceNum = 0;             // 目标面数
    double TargetReduction = 0.5;      // 减少的百分比
    bool NormalCheck = false;           // 是否进行法线检查。
    double NormalThr = M_PI / 6.0;     // 法线检查的阈值，以弧度表示。
    double CosineThr = cos(NormalThr); // 法线检查的余弦阈值。
    bool OptimalPosition = true;       // 是否使用最优位置。
    bool PreserveBoundary = false;     // 是否保持边界。
    double QuadricEpsilon = 1e-15;     // 二次型的阈值。
    bool QualityCheck = false;         // 是否进行质量检查。
    double QualityThr = 0.3;           // 用于质量检查的质量阈值。
    bool ScalarCheck = true;           // 是否进行标量检查。      
    bool IsAllScalarCheck = true;

    struct AttribInfo {
        int mapId;
        IGenum type;
        IGenum attach;
        ArrayObject::Pointer ptr;
        int size;
        int dimension, offset;
        double weights;
        double magMin, magMax;

        double tempOriValue1[16]{};
        double tempOriValue2[16]{};
        ArrayObject::Pointer optimalAttrib;
        bool hasNext;
    };

    std::vector<int> activedAttribIndices;

protected:
    Simplification2();
    ~Simplification2() override = default;

    void Initialize();

    void InitMemory();

    void InitAttributes();

    void InitQuadric();

    void InitLaplace();

    int EvaluateEdge(igIndex edgeId);

    void InsertEdgeToHeap(igIndex edgeId);

    double ComputePriority(igIndex edgeId, double& geo_priority);

    Vector3f ComputePosition(igIndex edgeId);

    Vector3f Normal(igIndex faceId);

    Quadric<double> QuadricFace(igIndex faceId);

    double QualityFace(igIndex faceId);

    igIndex GetOppEdge(igIndex ptId, igIndex faceId);

    void GetEdgeToOneRingPoints(igIndex edgeId, SurfaceMesh::ReturnContainer& ptIds);

    bool IsInTriangle(const Point& p, const Point& a, const Point& b, const Point& c);

    bool Projection(const Point& p, igIndex faceId, double& d, Vector3d& proj);

    Vector3d GetCentroidParam(const Point& p, igIndex faceId);

    bool GetNormalAndArea(igIndex faceId, double& area, Vector3d& n);

    double GetCosTheta(Vector3d n1, Vector3d n2);

	SurfaceMesh::Pointer mesh{};
    PriorityQueue::Pointer heap{};
    std::vector<Quadric<double>> quadrics;
    FloatArray::Pointer laplaces;

    std::vector<Vector3f> optimalPos;
    std::vector<Vector3d> optimalScalar;
    IGenum mode{QEM_FASTEST};
    AttributeSet::Pointer newAttrs{}, oldAttrs{}, attrbs{};

    std::vector<AttribInfo> attributes{};
    int attributes_count{};
    DoubleArray::Pointer origValue{};

    IGsize npts{}, nedges{}, nfaces{};

};
IGAME_NAMESPACE_END
#endif