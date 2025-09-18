#include "iGameMeshSimplifier.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameThreadPool.h"
#include <algorithm>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iomanip>

IGAME_NAMESPACE_BEGIN
namespace meshsmp
{


typedef unsigned int int_t;

template<class T>
class TVector3 {
public:
    T x, y, z;

    T Normalize() {
        double length = std::sqrt(x * x + y * y + z * z);

        if (length > 0) {
            x /= length;
            y /= length;
            z /= length;
        }

        return length;
    }

    T Dot(const TVector3<T>& v) const { return x * v.x + y * v.y + z * v.z; }

    TVector3<T> Cross(const TVector3<T>& v) const {
        return TVector3<T>{y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }

    double Length() const { return std::sqrt(x * x + y * y + z * z); }

    TVector3<T> operator+(const TVector3<T>& v) const { return TVector3<T>{x + v.x, y + v.y, z + v.z}; }
    TVector3<T> operator-(const TVector3<T>& v) const { return TVector3<T>{x - v.x, y - v.y, z - v.z}; }
    TVector3<T> operator/(double k) const {
        if (k == 0.) return TVector3<T>{0, 0, 0};
        return TVector3<T>{static_cast<T>(x / k), static_cast<T>(y / k), static_cast<T>(z / k)};
    }
};

typedef TVector3<float> Vector3;
typedef TVector3<int> Vector3i;
typedef TVector3<float> Point3;

template<class T>
class TBox3 {
public:
    typedef TVector3<T> Point;
    Point Min;
    Point Max;
};
typedef TBox3<float> Box3;

class Attribute {
public:
    /*const*/ float* Primitive;
    int_t Offset;
    int_t Stride;
};

template<class T>
class TQuadric {
public:
    T a00, a11, a22;
    T a10, a20, a21;
    T b0, b1, b2, c;
    T w;

    void operator=(const TQuadric<T>& q) {
        a00 = q.a00;
        a11 = q.a11;
        a22 = q.a22;
        a10 = q.a10;
        a20 = q.a20;
        a21 = q.a21;
        b0 = q.b0;
        b1 = q.b1;
        b2 = q.b2;
        c = q.c;
        w = q.w;
    }

    void operator+=(const TQuadric<T>& q) {
        a00 += q.a00;
        a11 += q.a11;
        a22 += q.a22;
        a10 += q.a10;
        a20 += q.a20;
        a21 += q.a21;
        b0 += q.b0;
        b1 += q.b1;
        b2 += q.b2;
        c += q.c;
        w += q.w;
    }

    TQuadric<T> operator+(const TQuadric<T>& q) const {
        TQuadric<T> Q;
        memset(&Q, 0, sizeof(TQuadric<T>));
        Q += q;
        return Q;
    }

    void ByTriangle(const Point3& p0, const Point3& p1, const Point3& p2) {
        Point3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
        Point3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

        Vector3 normal = p10.Cross(p20);
        float w = normal.Normalize();

        float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

        ByPlane(normal.x, normal.y, normal.z, -distance, sqrtf(w));
    }

    void ByPlane(float a, float b, float c, float d, float w) {
        float aw = a * w;
        float bw = b * w;
        float cw = c * w;
        float dw = d * w;

        this->a00 = a * aw;
        this->a11 = b * bw;
        this->a22 = c * cw;
        this->a10 = a * bw;
        this->a20 = a * cw;
        this->a21 = b * cw;
        this->b0 = a * dw;
        this->b1 = b * dw;
        this->b2 = c * dw;
        this->c = d * dw;
        this->w = w;
    }

    float Eval(const Vector3& v) const {
        float rx = b0;
        float ry = b1;
        float rz = b2;

        rx += a10 * v.y;
        ry += a21 * v.z;
        rz += a20 * v.x;

        rx *= 2;
        ry *= 2;
        rz *= 2;

        rx += a00 * v.x;
        ry += a11 * v.y;
        rz += a22 * v.z;

        float r = c;
        r += rx * v.x;
        r += ry * v.y;
        r += rz * v.z;

        return r;
    }

    float Error(const Vector3& v) {
        float r = Eval(v);
        float s = w == 0.f ? 0.f : 1.f / w;

        return fabsf(r) * s;
    }
};
typedef TQuadric<float> Quadric;

template<class TValueType>
class TGradient {
public:
    TValueType gx, gy, gz, gw;

    void operator=(const TGradient<TValueType>& q) {
        gx = q.gx;
        gy = q.gy;
        gz = q.gz;
        gw = q.gw;
    }

    void operator+=(const TGradient<TValueType>& q) {
        gx += q.gx;
        gy += q.gy;
        gz += q.gz;
        gw += q.gw;
    }

    TGradient<TValueType> operator+(const TGradient<TValueType>& q) {
        TGradient<TValueType> Q;
        memset(&Q, 0, sizeof(TGradient<TValueType>));
        Q += q;
        return Q;
    }
};
typedef TGradient<float> Gradient;

class Octree {
public:
    struct Node {
        Node() {}
        Node(Node* Parent, int Level) {
            this->Parent = Parent;
            this->Level = Level;
        }

        int NodeId{-1};
        int Level{0};
        Node* Parent{nullptr};
        Node* Sons[8]{nullptr};
        Box3 Voxel{};
        bool IsLeaf{true};
    };

    void Initialize(const Box3& bbox) {
        BBox = bbox;
        MaxDepth = 0;
        Num = 1 << MaxDepth;
        Vector3 BoxSize = bbox.Max - bbox.Min;
        R.x = Num / BoxSize.x;
        R.y = Num / BoxSize.y;
        R.z = Num / BoxSize.z;
        N.x = 1. / R.x;
        N.y = 1. / R.y;
        N.z = 1. / R.z;

        Node* root = new Node;
        root->NodeId = 0;
        root->Voxel = bbox;

        Nodes.clear();
        Nodes.push_back(root);
    }

    Node* FindNode(const Vector3i& path) const {
        assert(path.x >= 0 && path.x < Num);
        assert(path.y >= 0 && path.y < Num);
        assert(path.z >= 0 && path.z < Num);

        Node* node = Root();
        int rootLevel = 0;
        int shiftLevel = MaxDepth - 1;

        while (shiftLevel >= rootLevel) {
            int nextSon = 0;
            if ((path.z >> shiftLevel) % 2) nextSon += 1;
            if ((path.y >> shiftLevel) % 2) nextSon += 2;
            if ((path.x >> shiftLevel) % 2) nextSon += 4;
            Node* nextNode = node->Sons[nextSon];
            if (nextNode == nullptr) return node;
            node = nextNode;
            --shiftLevel;
        }
        return node;
    }

    Node* NewSon(Node* parent, int no) {
        int level = parent->Level + 1;

        Node* node = new Node;
        node->NodeId = Nodes.size() - 1;
        node->Parent = parent;
        node->Level = level;
        parent->IsLeaf = false;
        parent->Sons[no] = node;
        Nodes.push_back(node);

        Box3& bbox = parent->Voxel;
        Vector3 center = (parent->Voxel.Max + parent->Voxel.Min) / 2;
        switch (no) {
            case 0: // 左后下
                node->Voxel = Box3{bbox.Min, center};
                break;
            case 1: // 左后上
                node->Voxel = Box3{Vector3{bbox.Min.x, bbox.Min.y, center.z}, Vector3{center.x, center.y, bbox.Max.z}};
                break;
            case 2: // 右后下
                node->Voxel = Box3{Vector3{bbox.Min.x, center.y, bbox.Min.z}, Vector3{center.x, bbox.Max.y, center.z}};
                break;
            case 3: // 右后上
                node->Voxel = Box3{Vector3{bbox.Min.x, center.y, center.z}, Vector3{center.x, bbox.Max.y, bbox.Max.z}};
                break;
            case 4: // 左前下
                node->Voxel = Box3{Vector3{center.x, bbox.Min.y, bbox.Min.z}, Vector3{bbox.Max.x, center.y, center.z}};
                break;
            case 5: // 左前上
                node->Voxel = Box3{Vector3{center.x, bbox.Min.y, center.z}, Vector3{bbox.Max.x, center.y, bbox.Max.z}};
                break;
            case 6: // 右前下
                node->Voxel = Box3{Vector3{center.x, center.y, bbox.Min.z}, Vector3{bbox.Max.x, bbox.Max.y, center.z}};
                break;
            case 7: // 右前上
                node->Voxel = Box3{center, bbox.Max};
                break;
            default:
                break;
        }
        return node;
    }
    void NewAllSons(Node* parent) {
        assert(parent != nullptr);
        assert(parent->IsLeaf);
        int level = parent->Level + 1;
        if (level > MaxDepth) {
            MaxDepth = level;
            Num = 1 << MaxDepth;
            Vector3 BoxSize = BBox.Max - BBox.Min;
            R.x = Num / BoxSize.x;
            R.y = Num / BoxSize.y;
            R.z = Num / BoxSize.z;
            N.x = 1. / R.x;
            N.y = 1. / R.y;
            N.z = 1. / R.z;
        }

        for (int i = 0; i < 8; ++i) { Node* node = NewSon(parent, i); }
    }

    Vector3i Interize(const Vector3& pf) const {
        Vector3i pi{};

        assert(pf.x >= BBox.Min.x && pf.x <= BBox.Max.x);
        assert(pf.y >= BBox.Min.y && pf.y <= BBox.Max.y);
        assert(pf.z >= BBox.Min.z && pf.z <= BBox.Max.z);

        pi.x = int((pf.x - BBox.Min.x) * R.x);
        pi.y = int((pf.y - BBox.Min.y) * R.y);
        pi.z = int((pf.z - BBox.Min.z) * R.z);

        if (pi.x >= Num) pi.x = Num - 1;
        if (pi.y >= Num) pi.y = Num - 1;
        if (pi.z >= Num) pi.z = Num - 1;
        return pi;
    }
    Vector3 DeInterize(const Vector3i& pi) const {
        Vector3 pf{};

        assert(pi.x >= 0 && pi.x < Num);
        assert(pi.y >= 0 && pi.y < Num);
        assert(pi.z >= 0 && pi.z < Num);

        pf.x = pi.x * N.x + BBox.Min.x;
        pf.y = pi.y * N.y + BBox.Min.y;
        pf.z = pi.z * N.z + BBox.Min.z;

        return pf;
    }

    Node* Root() const {
        if (Nodes.size() > 0) return Nodes[0];
        return nullptr;
    }

private:
    int MaxDepth;
    int Num; // 2^MaxDepth

    std::vector<Node*> Nodes;
    Box3 BBox;
    Vector3 N, R;
};

struct Collapse {
    // collapse v0 -> v1
    int_t v0;
    int_t v1;

    float error;
};

static bool debug = true;

static void print() { std::cout << std::endl; }

template<typename First, typename... Rest>
static void print(First&& first, Rest&&... rest) {
    if (debug) {
        std::cout << first << " ";
        print(std::forward<Rest>(rest)...);
    }
}

class Timer {
public:
    void start() { start_ = clock(); }
    void end() { end_ = clock(); }

    clock_t GetTime() const { return double(end_ - start_) /*/ CLOCKS_PER_SEC*/; }

    template<typename... Rest>
    void print(Rest&&... rest) const {
        if (debug) {
            std::cout << "[Time Cost: " << std::setw(3) << std::left << GetTime() << "ms] ";
            print_impl(std::forward<Rest>(rest)...);
        }
    }

private:
    clock_t start_{}, end_{};

    template<typename First, typename... Rest>
    void print_impl(First&& first, Rest&&... rest) const {
        if (debug) {
            std::cout << first;
            print_impl(std::forward<Rest>(rest)...);
        }
    }
    void print_impl() const { std::cout << std::endl; }
};

class TriMeshInternalSimplifier {
public:
    TriMeshInternalSimplifier(std::vector<int_t>& Indices, const std::vector<Point3>& VertexPositions,
                              const std::vector<Attribute>& VertexAttributes,
                              const std::vector<float>& AttributeWeights, size_t TargetCount, float TargetError);

    size_t DoWork();

    size_t DoWorkParallel();

    class MVertexAdjacency {
    public:
        // 三角形中一个顶点的对偶边
        struct DualEdge {
            int_t next;
            int_t prev;
        };

        std::vector<int_t> Offsets;
        std::vector<DualEdge> Data;

        int_t Num(int_t id) const { return Offsets[id + 1] - Offsets[id]; }
        int_t Begin(int_t id) const { return Offsets[id]; }
        int_t End(int_t id) const { return Offsets[id + 1]; }
    };

private:
    void BuildVertexAdjacency();

    void FillVertexQuadrics();

    void FillAttributeQuadrics();

    void ComputeAttributeQuadient(int_t FaceId, Quadric& Q, Gradient* G);

    size_t BuildEdgeCollapses(size_t CollapseCapacity);

    void SortEdgeCollapses(size_t EdgeCollapseCount);

    size_t ExecuteEdgeCollapses(size_t EdgeCollapseCount);

    bool IsCollapsable(int_t v, int_t to_v);

    void UpdateQuadrics();

    size_t RemapIndices();

    void BuildVertexOctree();

    //-------------- Input's Data--------------//
    std::vector<int_t>& Indices;                    // 三角形索引数组
    const std::vector<Point3>& VertexPositions;     // 顶点数组
    const std::vector<Attribute>& VertexAttributes; // 顶点的属性数组
    const std::vector<float>& AttributeWeights;     // 属性权重数组
    size_t TargetCount;                             // 需要减少到的索引数
    float TargetError;                              //

    float ErrorLimit;
    size_t NeedCollapsedIndexCount;
    size_t IndexCount;                // Indices数组长度
    size_t VertexCount;               // 顶点个数
    size_t AttributeCount;            // 属性个数
    MVertexAdjacency VertexAdjacency; // 顶点的邻接面
    std::vector<Quadric> VertexQuadrics;
    std::vector<Quadric> AttributeQuadrics;
    std::vector<Gradient> AttributeGradients;
    std::vector<Collapse> Collapses;         // 坍缩边数组
    std::vector<int_t> CollapseOrder;        // 坍缩权重的排序数组
    std::vector<int_t> VertexRemap;          // 顶点重映射，用于坍缩后的顶点映射
    std::vector<unsigned char> VertexLocked; // 用于标记顶点是否被坍缩
};

TriMeshInternalSimplifier::TriMeshInternalSimplifier(std::vector<int_t>& Indices,
                                                     const std::vector<Point3>& VertexPositions,
                                                     const std::vector<Attribute>& VertexAttributes,
                                                     const std::vector<float>& AttributeWeights, size_t TargetCount,
                                                     float TargetError)
    : Indices(Indices), VertexPositions(VertexPositions), VertexAttributes(VertexAttributes),
      AttributeWeights(AttributeWeights), TargetCount(TargetCount), TargetError(TargetError) {
    IndexCount = Indices.size();
    VertexCount = VertexPositions.size();
    AttributeCount = VertexAttributes.size();
    NeedCollapsedIndexCount = IndexCount - TargetCount;
    ErrorLimit = TargetError * TargetError;
}

size_t TriMeshInternalSimplifier::DoWork() {
    Timer time, time2;
    time2.start();
    time.start();

    // 一些数组的初始化
    VertexQuadrics.resize(VertexCount);
    memset(VertexQuadrics.data(), 0, VertexCount * sizeof(Quadric));
    if (AttributeCount > 0) {
        AttributeQuadrics.resize(VertexCount);
        memset(AttributeQuadrics.data(), 0, VertexCount * sizeof(Quadric));

        AttributeGradients.resize(VertexCount * AttributeCount);
        memset(AttributeGradients.data(), 0, VertexCount * AttributeCount * sizeof(Gradient));
    }

    VertexAdjacency.Offsets.resize(VertexCount + 1);
    VertexAdjacency.Data.resize(IndexCount);
    // 先要建立一次邻接关系
    BuildVertexAdjacency();

    FillVertexQuadrics();
    if (AttributeCount) FillAttributeQuadrics();

    // 坍缩边的最大容量
    size_t CollapseCapacity = IndexCount;
    Collapses.resize(CollapseCapacity);
    CollapseOrder.resize(CollapseCapacity);

    VertexRemap.resize(VertexCount);
    VertexLocked.resize(VertexCount);

    time.end();
    time.print("Initialize");
    int Sequence = 0;

    while (IndexCount > TargetCount) {
        print("Sequence ", Sequence++);

        time.start();
        // 建立新的邻接关系
        BuildVertexAdjacency();
        time.end();
        time.print("1.BuildVertexAdjacency");

        time.start();
        // 初始化坍缩边的误差
        size_t EdgeCollapseCount = BuildEdgeCollapses(CollapseCapacity);
        time.end();
        time.print("2.BuildEdgeCollapses");

        time.start();
        // 给所有的坍缩边排序
        SortEdgeCollapses(EdgeCollapseCount);
        time.end();
        time.print("3.SortEdgeCollapses");

        // 初始化顶点映射
        for (size_t i = 0; i < VertexCount; ++i) VertexRemap[i] = i;

        // 初始化可访问顶点
        memset(VertexLocked.data(), 0, VertexCount * sizeof(unsigned char));

        time.start();
        // 执行边坍缩
        size_t CollapseCount = ExecuteEdgeCollapses(EdgeCollapseCount);
        time.end();
        time.print("4.ExecuteEdgeCollapses");

        time.start();
        UpdateQuadrics();
        time.end();
        time.print("5.UpdateQuadrics");

        time.start();
        IndexCount = RemapIndices();
        time.end();
        time.print("6.RemapIndices");
    }

    time2.end();
    time2.print("Total");
    return IndexCount;
}

size_t TriMeshInternalSimplifier::DoWorkParallel() { return 0; }

void TriMeshInternalSimplifier::BuildVertexAdjacency() {
    memset(VertexAdjacency.Offsets.data(), 0, (VertexCount + 1) * sizeof(int_t));

    for (int_t i = 0; i < IndexCount; ++i) {
        int_t idx = Indices[i] + 1;
        VertexAdjacency.Offsets[idx]++;
    }

    int_t Offset = 0;
    for (int_t i = 0; i < VertexCount; ++i) {
        int_t idx = i + 1;
        int_t Count = VertexAdjacency.Offsets[idx];
        VertexAdjacency.Offsets[idx] = Offset;
        Offset += Count;
    }

    for (int_t i = 0; i < IndexCount; i += 3) {
        int_t v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2];

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

void TriMeshInternalSimplifier::FillVertexQuadrics() {
    for (int_t i = 0; i < IndexCount; i += 3) {
        int_t i0 = Indices[i];
        int_t i1 = Indices[i + 1];
        int_t i2 = Indices[i + 2];

        Quadric Q;
        Q.ByTriangle(VertexPositions[i0], VertexPositions[i1], VertexPositions[i2]);

        VertexQuadrics[i0] += Q;
        VertexQuadrics[i1] += Q;
        VertexQuadrics[i2] += Q;
    }
}

void TriMeshInternalSimplifier::FillAttributeQuadrics() {
    for (size_t i = 0; i < IndexCount; i += 3) {
        int_t i0 = Indices[i + 0];
        int_t i1 = Indices[i + 1];
        int_t i2 = Indices[i + 2];
        int_t faceId = i / 3;

        Quadric Q;
        Gradient G[32];
        ComputeAttributeQuadient(faceId, Q, G);

        AttributeQuadrics[i0] += Q;
        AttributeQuadrics[i1] += Q;
        AttributeQuadrics[i2] += Q;

        for (size_t k = 0; k < AttributeCount; ++k) {
            AttributeGradients[i0 * AttributeCount + k] += G[k];
            AttributeGradients[i1 * AttributeCount + k] += G[k];
            AttributeGradients[i2 * AttributeCount + k] += G[k];
        }
    }
}

void TriMeshInternalSimplifier::ComputeAttributeQuadient(int_t faceId, Quadric& Q, Gradient* G) {
    // 我们使用下面这个线性插值函数计算新位置 pos 处的属性值
    //      eval(pos) = pos.x * gx + pos.y * gy + pos.z * gz + gw
    // 其中，gx/gy/gz 是属性梯度，gw是基准常数值
    // 使用插值处的属性值与真实值的差的平方作为属性误差
    //      Δ(pos) = (eval(pos) - attr)^2


    int_t i0 = Indices[faceId * 3 + 0];
    int_t i1 = Indices[faceId * 3 + 1];
    int_t i2 = Indices[faceId * 3 + 2];

    const Vector3& p0 = VertexPositions[i0];
    const Vector3& p1 = VertexPositions[i1];
    const Vector3& p2 = VertexPositions[i2];

    Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    Vector3 normal = p10.Cross(p20);
    float area = normal.Length() * 0.5f;

    // quadric 使用三角形面积进行加权
    float w = area;

    // 我们使用重心坐标计算梯度，重心坐标的计算方法如下：
    // v = (d11 * d20 - d01 * d21) / denom
    // w = (d00 * d21 - d01 * d20) / denom
    // u = 1 - v - w
    // here v0, v1 are triangle edge vectors, v2 is a vector from point to triangle corner, and dij = dot(vi, vj)
    // note: v2 and d20/d21 can not be evaluated here as v2 is effectively an unknown variable; we need these only as variables for derivation of gradients
    const Vector3& v0 = p10;
    const Vector3& v1 = p20;
    float d00 = v0.x * v0.x + v0.y * v0.y + v0.z * v0.z;
    float d01 = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
    float d11 = v1.x * v1.x + v1.y * v1.y + v1.z * v1.z;
    float denom = d00 * d11 - d01 * d01;
    float denomr = denom == 0 ? 0.f : 1.f / denom;

    // precompute gradient factors
    // these are derived by directly computing derivative of eval(pos) = a0 * u + a1 * v + a2 * w and factoring out expressions that are shared between attributes
    float gx1 = (d11 * v0.x - d01 * v1.x) * denomr;
    float gx2 = (d00 * v1.x - d01 * v0.x) * denomr;
    float gy1 = (d11 * v0.y - d01 * v1.y) * denomr;
    float gy2 = (d00 * v1.y - d01 * v0.y) * denomr;
    float gz1 = (d11 * v0.z - d01 * v1.z) * denomr;
    float gz2 = (d00 * v1.z - d01 * v0.z) * denomr;

    memset(&Q, 0, sizeof(Quadric));

    Q.w = w;

    for (size_t k = 0; k < AttributeCount; ++k) {
        const Attribute& Attr = VertexAttributes[k];
        float a0 = Attr.Primitive[i0 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
        float a1 = Attr.Primitive[i1 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
        float a2 = Attr.Primitive[i2 * Attr.Stride + Attr.Offset] * AttributeWeights[k];


        // compute gradient of eval(pos) for x/y/z/w
        // the formulas below are obtained by directly computing derivative of eval(pos) = a0 * u + a1 * v + a2 * w
        float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
        float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
        float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);
        float gw = a0 - p0.x * gx - p0.y * gy - p0.z * gz;

        // quadric encodes (eval(pos)-attr)^2; this means that the resulting expansion needs to compute, for example, pos.x * pos.y * K
        // since quadrics already encode factors for pos.x * pos.y, we can accumulate almost everything in basic quadric fields
        // note: for simplicity we scale all factors by weight here instead of outside the loop
        Q.a00 += w * (gx * gx);
        Q.a11 += w * (gy * gy);
        Q.a22 += w * (gz * gz);

        Q.a10 += w * (gy * gx);
        Q.a20 += w * (gz * gx);
        Q.a21 += w * (gz * gy);

        Q.b0 += w * (gx * gw);
        Q.b1 += w * (gy * gw);
        Q.b2 += w * (gz * gw);

        Q.c += w * (gw * gw);

        // the only remaining sum components are ones that depend on attr; these will be addded during error evaluation, see quadricError
        G[k].gx = w * gx;
        G[k].gy = w * gy;
        G[k].gz = w * gz;
        G[k].gw = w * gw;
    }
}

size_t TriMeshInternalSimplifier::BuildEdgeCollapses(size_t CollapseCapacity) {
    size_t Count = 0;
    for (size_t i = 0; i < IndexCount; i += 3) {
        static const int next[3] = {1, 2, 0};

        if (Count + 3 > CollapseCapacity) { break; }

        for (int e = 0; e < 3; ++e) {
            int_t i0 = Indices[i + e];
            int_t i1 = Indices[i + next[e]];

            Collapse c = {i0, i1, 0.f};
            const Vector3& v = VertexPositions[i1];
            c.error = VertexQuadrics[i0].Error(v);

            if (AttributeCount) {
                float r = AttributeQuadrics[i0].Eval(v);

                for (size_t k = 0; k < AttributeCount; ++k) {
                    const Attribute& Attr = VertexAttributes[k];
                    const Gradient& G = AttributeGradients[i0 * AttributeCount + k];
                    float a = Attr.Primitive[i1 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
                    float g = v.x * G.gx + v.y * G.gy + v.z * G.gz + G.gw;

                    r += a * (a * AttributeQuadrics[i0].w - 2 * g);
                }

                c.error += fabsf(r);
            }

            Collapses[Count++] = c;
        }
    }
    return Count;
}

void TriMeshInternalSimplifier::SortEdgeCollapses(size_t EdgeCollapseCount) {
    //std::sort(Collapses.begin(), Collapses.begin() + Length,
    //          [](const Collapse& o1, const Collapse& o2) { return o1.error < o2.error; });

    //return;
    const unsigned int sort_bits = 12;
    const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

    // fill histogram for counting sort
    unsigned int histogram[sort_bins];
    memset(histogram, 0, sizeof(histogram));

    for (size_t i = 0; i < EdgeCollapseCount; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error;
        std::memcpy(&error, &Collapses[i].error, sizeof(error));
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
    for (size_t i = 0; i < EdgeCollapseCount; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error;
        std::memcpy(&error, &Collapses[i].error, sizeof(error));
        unsigned int key = (error << 1) >> (32 - sort_bits);
        key = key < sort_bins ? key : sort_bins - 1;

        CollapseOrder[histogram[key]++] = i;
    }
}

size_t TriMeshInternalSimplifier::ExecuteEdgeCollapses(size_t EdgeCollapseCount) {
    // 本次循环需要简化的面数量
    size_t TargetTriangleCount = (IndexCount - TargetCount) / 3;

    // 坍缩的面数量
    size_t CollapseCount = 0;

    for (size_t i = 0; i < EdgeCollapseCount; ++i) {
        const Collapse& c = Collapses[CollapseOrder[i]];

        if (c.error > ErrorLimit) break;

        if (c.error > Collapses[CollapseOrder[EdgeCollapseCount / 2]].error &&
            CollapseCount > TargetTriangleCount / 6) {
            break;
        }

        int_t i0 = c.v0;
        int_t i1 = c.v1;

        if (VertexLocked[i0] | VertexLocked[i1]) continue;

        if (!IsCollapsable(i0, i1)) { continue; }

        VertexRemap[i0] = i1;
        VertexLocked[i0] = 1;
        VertexLocked[i1] = 1;

        CollapseCount += 2;
    }

    return CollapseCount;
}

static bool HasTriangleFlip(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
    Vector3 eb = {b.x - a.x, b.y - a.y, b.z - a.z};
    Vector3 ec = {c.x - a.x, c.y - a.y, c.z - a.z};
    Vector3 ed = {d.x - a.x, d.y - a.y, d.z - a.z};

    Vector3 nbc = {eb.y * ec.z - eb.z * ec.y, eb.z * ec.x - eb.x * ec.z, eb.x * ec.y - eb.y * ec.x};
    Vector3 nbd = {eb.y * ed.z - eb.z * ed.y, eb.z * ed.x - eb.x * ed.z, eb.x * ed.y - eb.y * ed.x};

    float ndp = nbc.x * nbd.x + nbc.y * nbd.y + nbc.z * nbd.z;
    float abc = nbc.x * nbc.x + nbc.y * nbc.y + nbc.z * nbc.z;
    float abd = nbd.x * nbd.x + nbd.y * nbd.y + nbd.z * nbd.z;

    // scale is cos(angle); somewhat arbitrarily set to ~75 degrees
    // note that the "pure" check is ndp <= 0 (90 degree cutoff) but that allows flipping through a series of close-to-90 collapses
    return ndp <= 0.25f * sqrtf(abc * abd);
}

bool TriMeshInternalSimplifier::IsCollapsable(int_t v, int_t to_v) {
    const Vector3& v0 = VertexPositions[v];
    const Vector3& v1 = VertexPositions[to_v];

    size_t Begin = VertexAdjacency.Begin(v);

    for (size_t i = 0; i < VertexAdjacency.Num(v); ++i) {
        auto& e = VertexAdjacency.Data[Begin + i];
        int_t i0 = VertexRemap[e.next];
        int_t i1 = VertexRemap[e.prev];

        if (i0 == i1 || i0 == to_v || i1 == to_v) continue;

        if (HasTriangleFlip(VertexPositions[i0], VertexPositions[i1], v0, v1)) { return false; }
    }
    return true;
}

void TriMeshInternalSimplifier::UpdateQuadrics() {
    for (size_t i = 0; i < VertexCount; ++i) {
        // 要么这个顶点早被删了，要么坍缩时没有影响到该顶点
        if (VertexRemap[i] == i) continue;

        // i号顶点坍缩到 r号顶点上了
        int_t r = VertexRemap[i];

        VertexQuadrics[r] += VertexQuadrics[i];

        if (AttributeCount) {
            AttributeQuadrics[r] += AttributeQuadrics[i];
            for (size_t k = 0; k < AttributeCount; ++k) {
                AttributeGradients[r * AttributeCount + k] += AttributeGradients[i * AttributeCount + k];
            }
        }
    }
}

size_t TriMeshInternalSimplifier::RemapIndices() {
    size_t k = 0;

    for (size_t i = 0; i < IndexCount; i += 3) {
        int_t v0 = VertexRemap[Indices[i + 0]];
        int_t v1 = VertexRemap[Indices[i + 1]];
        int_t v2 = VertexRemap[Indices[i + 2]];

        if (v0 != v1 && v0 != v2 && v1 != v2) {
            Indices[k + 0] = v0;
            Indices[k + 1] = v1;
            Indices[k + 2] = v2;
            k += 3;
        }
    }

    return k;
}

void TriMeshInternalSimplifier::BuildVertexOctree() {}

static float RescalePositions(std::vector<Point3>& VertexPositions, Points::Pointer Points) {
    float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (size_t i = 0; i < Points->GetNumberOfPoints(); ++i) {
        auto& v = Points->GetPoint(i);

        VertexPositions.push_back({v[0], v[1], v[2]});

        for (int j = 0; j < 3; ++j) {
            float vj = v[j];

            minv[j] = minv[j] > vj ? vj : minv[j];
            maxv[j] = maxv[j] < vj ? vj : maxv[j];
        }
    }

    float extent = 0.f;

    extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
    extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
    extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

    float scale = extent == 0 ? 0.f : 1.f / extent;

    for (size_t i = 0; i < Points->GetNumberOfPoints(); ++i) {
        VertexPositions[i].x = (VertexPositions[i].x - minv[0]) * scale;
        VertexPositions[i].y = (VertexPositions[i].y - minv[1]) * scale;
        VertexPositions[i].z = (VertexPositions[i].z - minv[2]) * scale;
    }

    return extent;
}

class TetraMeshInternalSimplifier {
public:
    TetraMeshInternalSimplifier(std::vector<int_t>& Indices, const std::vector<Point3>& VertexPositions,
                                const std::vector<unsigned char>& IsSurfaceVertex,
                                const std::vector<int_t>& SurfaceIndices,
                                const std::vector<Attribute>& VertexAttributes,
                                const std::vector<float>& AttributeWeights, size_t TargetCount, float TargetError);

    size_t DoWork();

    class MVertexAdjacency {
    public:
        // 三角形中一个顶点的对偶边
        struct DualFace {
            int_t handle[3];
        };

        std::vector<int_t> Offsets;
        std::vector<DualFace> Data;

        int_t Num(int_t id) const { return Offsets[id + 1] - Offsets[id]; }
        int_t Begin(int_t id) const { return Offsets[id]; }
        int_t End(int_t id) const { return Offsets[id + 1]; }
    };

private:
    void BuildVertexAdjacency();

    void FillVertexQuadrics();

    void FillAttributeQuadrics();

    void LUSolveLinearSystem(double** A, int* index, double* x, int size) {
        int i, j, ii, idx;
        double sum;
        //
        // Proceed with forward and backsubstitution for L and U
        // matrices.  First, forward substitution.
        //
        for (ii = -1, i = 0; i < size; ++i) {
            idx = index[i];
            sum = x[idx];
            x[idx] = x[i];

            if (ii >= 0) {
                for (j = ii; j <= (i - 1); ++j) { sum -= A[i][j] * x[j]; }
            } else if (sum != 0.0) {
                ii = i;
            }

            x[i] = sum;
        }
        //
        // Now, back substitution
        //
        for (i = size - 1; i >= 0; i--) {
            sum = x[i];
            for (j = i + 1; j < size; ++j) { sum -= A[i][j] * x[j]; }
            x[i] = sum / A[i][i];
        }
    }

    bool LUFactorLinearSystem(double** A, int* index, int size, double* tmpSize) {
        int i, j, k;
        int maxI = 0;
        double largest, temp1, temp2, sum;

        //
        // Loop over rows to get implicit scaling information
        //
        for (i = 0; i < size; ++i) {
            for (largest = 0.0, j = 0; j < size; ++j) {
                if ((temp2 = std::abs(A[i][j])) > largest) { largest = temp2; }
            }

            if (largest == 0.0) { return 0; }
            tmpSize[i] = 1.0 / largest;
        }
        //
        // Loop over all columns using Crout's method
        //
        for (j = 0; j < size; ++j) {
            for (i = 0; i < j; ++i) {
                sum = A[i][j];
                for (k = 0; k < i; ++k) { sum -= A[i][k] * A[k][j]; }
                A[i][j] = sum;
            }
            //
            // Begin search for largest pivot element
            //
            for (largest = 0.0, i = j; i < size; ++i) {
                sum = A[i][j];
                for (k = 0; k < j; ++k) { sum -= A[i][k] * A[k][j]; }
                A[i][j] = sum;

                if ((temp1 = tmpSize[i] * std::abs(sum)) >= largest) {
                    largest = temp1;
                    maxI = i;
                }
            }
            //
            // Check for row interchange
            //
            if (j != maxI) {
                for (k = 0; k < size; ++k) { std::swap(A[maxI][k], A[j][k]); }
                tmpSize[maxI] = tmpSize[j];
            }
            //
            // Divide by pivot element and perform elimination
            //
            index[j] = maxI;

            if (std::abs(A[j][j]) <= 1e-12) { return 0; }

            if (j != (size - 1)) {
                temp1 = 1.0 / A[j][j];
                for (i = j + 1; i < size; ++i) { A[i][j] *= temp1; }
            }
        }

        return 1;
    }

    bool InvertMatrix(double** A, double** AI, int size, int* tmp1Size, double* tmp2Size) {
        int i, j;

        //
        // Factor matrix; then begin solving for inverse one column at a time.
        // Note: tmp1Size returned value is used later, tmp2Size is just working
        // memory whose values are not used in LUSolveLinearSystem
        //
        if (LUFactorLinearSystem(A, tmp1Size, size, tmp2Size) == 0) { return 0; }

        for (j = 0; j < size; ++j) {
            for (i = 0; i < size; ++i) { tmp2Size[i] = 0.0; }
            tmp2Size[j] = 1.0;

            LUSolveLinearSystem(A, tmp1Size, tmp2Size, size);

            for (i = 0; i < size; ++i) { AI[i][j] = tmp2Size[i]; }
        }

        return 1;
    }

    bool InvertMatrix(double** A, double** AI, int size) {
        const int VTK_MAX_SCRATCH_ARRAY_SIZE = 10;
        int iScratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
        int* index = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? iScratch : new int[size]);
        double dScratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
        double* column = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? dScratch : new double[size]);

        bool retVal = InvertMatrix(A, AI, size, index, column);

        if (size > VTK_MAX_SCRATCH_ARRAY_SIZE) {
            delete[] index;
            delete[] column;
        }

        return retVal;
    }

    int JacobianInverse(double** inverse, double derivs[12], int id) {
        int i, j;
        double *m[3], m0[3], m1[3], m2[3];
        double x[3];

        // compute interpolation function derivatives
        // r-derivatives
        derivs[0] = -1.0;
        derivs[1] = 1.0;
        derivs[2] = 0.0;
        derivs[3] = 0.0;

        // s-derivatives
        derivs[4] = -1.0;
        derivs[5] = 0.0;
        derivs[6] = 1.0;
        derivs[7] = 0.0;

        // t-derivatives
        derivs[8] = -1.0;
        derivs[9] = 0.0;
        derivs[10] = 0.0;
        derivs[11] = 1.0;

        // create Jacobian matrix
        m[0] = m0;
        m[1] = m1;
        m[2] = m2;
        for (i = 0; i < 3; i++) // initialize matrix
        {
            m0[i] = m1[i] = m2[i] = 0.0;
        }

        for (j = 0; j < 4; j++) {
            auto& p = VertexPositions[Indices[id * 4 + j]];
            x[0] = p.x;
            x[1] = p.y;
            x[2] = p.z;
            for (i = 0; i < 3; i++) {
                m0[i] += x[i] * derivs[j];
                m1[i] += x[i] * derivs[4 + j];
                m2[i] += x[i] * derivs[8 + j];
            }
        }

        // now find the inverse
        if (InvertMatrix(m, inverse, 3) == 0) { return 0; }

        return 1;
    }

    void ComputeAttributeQuadient(int id, Quadric& Q, Gradient* G) {
        double *jI[3], j0[3], j1[3], j2[3];
        double functionDerivs[12], sum[3], value;
        int i, j, k;

        // compute inverse Jacobian and interpolation function derivatives
        jI[0] = j0;
        jI[1] = j1;
        jI[2] = j2;
        this->JacobianInverse(jI, functionDerivs, id);

        int_t i0 = Indices[id * 4 + 0];
        int_t i1 = Indices[id * 4 + 1];
        int_t i2 = Indices[id * 4 + 2];
        int_t i3 = Indices[id * 4 + 3];

        const Vector3& p0 = VertexPositions[i0];
        const Vector3& p1 = VertexPositions[i1];
        const Vector3& p2 = VertexPositions[i2];
        const Vector3& p3 = VertexPositions[i3];

        memset(&Q, 0, sizeof(Quadric));

        double w = 1.;
        Q.w = 1;

        // now compute derivates of values provided
        for (k = 0; k < AttributeCount; k++) // loop over values per point
        {
            const Attribute& Attr = VertexAttributes[k];
            float a0 = Attr.Primitive[i0 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
            float a1 = Attr.Primitive[i1 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
            float a2 = Attr.Primitive[i2 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
            float a3 = Attr.Primitive[i3 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
            float a[4]{a0, a1, a2, a3};
            sum[0] = sum[1] = sum[2] = 0.0;
            for (i = 0; i < 4; i++) // loop over interp. function derivatives
            {
                value = a[i];
                sum[0] += functionDerivs[i] * value;
                sum[1] += functionDerivs[4 + i] * value;
                sum[2] += functionDerivs[8 + i] * value;
            }

            j = 0;
            float gx = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
            j = 1;
            float gy = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
            j = 2;
            float gz = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
            float gw = a0 - p0.x * G[k].gx - p0.y * G[k].gy - p0.z * G[k].gz;

            Q.a00 += w * (gx * gx);
            Q.a11 += w * (gy * gy);
            Q.a22 += w * (gz * gz);

            Q.a10 += w * (gy * gx);
            Q.a20 += w * (gz * gx);
            Q.a21 += w * (gz * gy);

            Q.b0 += w * (gx * gw);
            Q.b1 += w * (gy * gw);
            Q.b2 += w * (gz * gw);

            Q.c += w * (gw * gw);

            G[k].gx = w * gx;
            G[k].gy = w * gy;
            G[k].gz = w * gz;
            G[k].gw = w * gw;
        }
    }

    size_t BuildEdgeCollapses(size_t CollapseCapacity) {
        size_t Count = 0;
        for (size_t i = 0; i < IndexCount; i += 4) {
            static const int edges[6][2] = {
                    {0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3},
            };

            if (Count + 6 > CollapseCapacity) { break; }

            for (int j = 0; j < 6; ++j) {
                int_t i0 = Indices[i + edges[j][0]];
                int_t i1 = Indices[i + edges[j][1]];

                if (IsSurfaceVertex[i0] ^ IsSurfaceVertex[i1]) { continue; }

                const Vector3& v = VertexPositions[i1];

                Collapse c = {i0, i1, 0.f};

                c.error = VertexQuadrics[i0].Error(v);

                if (AttributeCount) {
                    float r = AttributeQuadrics[i0].Eval(v);

                    for (size_t k = 0; k < AttributeCount; ++k) {
                        const Attribute& Attr = VertexAttributes[k];
                        const Gradient& G = AttributeGradients[i0 * AttributeCount + k];
                        float a = Attr.Primitive[i1 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
                        float g = v.x * G.gx + v.y * G.gy + v.z * G.gz + G.gw;

                        r += a * (a * AttributeQuadrics[i0].w - 2 * g);
                    }

                    c.error += fabsf(r);
                }

                Collapses[Count++] = c;
            }
        }
        return Count;
    }

    void SortEdgeCollapses(size_t EdgeCollapseCount) {
        //std::sort(Collapses.begin(), Collapses.begin() + EdgeCollapseCount,
        //          [](const Collapse& o1, const Collapse& o2) { return o1.error < o2.error; });

        //return;
        const unsigned int sort_bits = 12;
        const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

        // fill histogram for counting sort
        unsigned int histogram[sort_bins];
        memset(histogram, 0, sizeof(histogram));

        for (size_t i = 0; i < EdgeCollapseCount; ++i) {
            // skip sign bit since error is non-negative
            unsigned int error;
            std::memcpy(&error, &Collapses[i].error, sizeof(error));
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
        for (size_t i = 0; i < EdgeCollapseCount; ++i) {
            // skip sign bit since error is non-negative
            unsigned int error;
            std::memcpy(&error, &Collapses[i].error, sizeof(error));
            unsigned int key = (error << 1) >> (32 - sort_bits);
            key = key < sort_bins ? key : sort_bins - 1;

            CollapseOrder[histogram[key]++] = i;
        }
    }

    size_t ExecuteEdgeCollapses(size_t EdgeCollapseCount) {

        //size_t TargetEdgeCount = (IndexCount - TargetCount) / 4 * 6;

        size_t CollapseCount = 0;

        for (size_t i = 0; i < EdgeCollapseCount; ++i) {
            const Collapse& c = Collapses[CollapseOrder[i]];

            //if (c.error > TargetError * TargetError) break;

            if (c.error > Collapses[CollapseOrder[EdgeCollapseCount / 4]].error &&
                CollapseCount > EdgeCollapseCount / 24) {
                break;
            }

            int_t i0 = c.v0;
            int_t i1 = c.v1;

            if (VertexLocked[i0] | VertexLocked[i1]) continue;

            if (!IsCollapsable(i0, i1)) { continue; }

            VertexRemap[i0] = i1;
            VertexLocked[i0] = 1;
            VertexLocked[i1] = 1;

            //SceneManager::Instance()->GetCurrentScene()->GetPainter3D()->SetPen(Color::Red);
            //SceneManager::Instance()->GetCurrentScene()->GetPainter3D()->SetPen(12);
            ////SceneManager::Instance()->GetCurrentScene()->GetPainter3D()->DrawLine(
            ////        Vector3f(VertexPositions[i0].x, VertexPositions[i0].y, VertexPositions[i0].z),
            ////        Vector3f(VertexPositions[i1].x, VertexPositions[i1].y, VertexPositions[i1].z));
            //SceneManager::Instance()->GetCurrentScene()->GetPainter3D()->DrawPoint(
            //        Vector3f(VertexPositions[i0].x, VertexPositions[i0].y, VertexPositions[i0].z));
            //SceneManager::Instance()->GetCurrentScene()->GetPainter3D()->DrawPoint(
            //        Vector3f(VertexPositions[i1].x, VertexPositions[i1].y, VertexPositions[i1].z));

            CollapseCount += 1;
        }

        return CollapseCount;
    }

    void UpdateQuadrics() {
        for (size_t i = 0; i < VertexCount; ++i) {
            // 要么这个顶点早被删了，要么坍缩时没有影响到该顶点
            if (VertexRemap[i] == i) continue;

            // i号顶点坍缩到 r号顶点上了
            int_t r = VertexRemap[i];

            VertexQuadrics[r] += VertexQuadrics[i];

            if (AttributeCount) {
                AttributeQuadrics[r] += AttributeQuadrics[i];
                for (size_t k = 0; k < AttributeCount; ++k) {
                    AttributeGradients[r * AttributeCount + k] += AttributeGradients[i * AttributeCount + k];
                }
            }
        }
    }

    size_t RemapIndices() {
        size_t k = 0;

        for (size_t i = 0; i < IndexCount; i += 4) {
            int_t v0 = VertexRemap[Indices[i + 0]];
            int_t v1 = VertexRemap[Indices[i + 1]];
            int_t v2 = VertexRemap[Indices[i + 2]];
            int_t v3 = VertexRemap[Indices[i + 3]];

            if (v0 != v1 && v0 != v2 && v0 != v3 && v1 != v2 && v1 != v3 && v2 != v3) {
                Indices[k + 0] = v0;
                Indices[k + 1] = v1;
                Indices[k + 2] = v2;
                Indices[k + 3] = v3;
                k += 4;
            }
        }

        return k;
    }

    bool IsCollapsable(int_t v, int_t to_v) {
        const Vector3& v0 = VertexPositions[v];
        const Vector3& v1 = VertexPositions[to_v];

        size_t Begin = VertexAdjacency.Begin(v);

        for (size_t i = 0; i < VertexAdjacency.Num(v); ++i) {
            auto& f = VertexAdjacency.Data[Begin + i];
            int_t i0 = VertexRemap[f.handle[0]];
            int_t i1 = VertexRemap[f.handle[1]];
            int_t i2 = VertexRemap[f.handle[2]];

            if (i0 == i1 || i0 == i2 || i1 == i2 || i0 == to_v || i1 == to_v || i2 == to_v) continue;

            // if (HasTetraFlip(VertexPositions[i0], VertexPositions[i1], VertexPositions[i2], v0, v1)) { return false; }
            if (CheckFlip(v0, v1, VertexPositions[i0], VertexPositions[i1], VertexPositions[i2])) { return false; }
        }
        return true;
    }

    static float SignedVolume(const Point3& a, const Point3& b, const Point3& c, const Point3& d) {
        return (b.x - a.x) * ((c.y - a.y) * (d.z - a.z) - (c.z - a.z) * (d.y - a.y)) -
               (b.y - a.y) * ((c.x - a.x) * (d.z - a.z) - (c.z - a.z) * (d.x - a.x)) +
               (b.z - a.z) * ((c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x));
    }

    static bool HasTetraFlip(const Point3& a, const Point3& b, const Point3& c, const Point3& d, const Point3& e) {
        float v1 = SignedVolume(a, b, c, d);
        float v2 = SignedVolume(a, b, c, e);

        return v1 * v2 < 0; // 体积符号变化表示翻转
    }

    static double SignedDistanceToPlane(const Point3& p, const Point3& planePoint, const Vector3& normal) {
        Vector3 diff = p - planePoint;
        return diff.Dot(normal);
    }

    static bool CheckFlip(const Point3& p1, const Point3& p2, const Point3& pA, const Point3& pB, const Point3& pC) {
        Vector3 v1 = pB - pA;
        Vector3 v2 = pC - pA;
        Vector3 normal = v1.Cross(v2);

        double d1 = SignedDistanceToPlane(p1, pA, normal);
        double d2 = SignedDistanceToPlane(p2, pA, normal);

        return (d1 * d2 < 0);
    }

    //-------------- Input's Data--------------//
    std::vector<int_t>& Indices;                       // 四面体索引数组
    const std::vector<Point3>& VertexPositions;        // 顶点数组
    const std::vector<unsigned char>& IsSurfaceVertex; // 记录哪些顶点在表面
    const std::vector<int_t>& SurfaceIndices;          //
    const std::vector<Attribute>& VertexAttributes;    // 顶点的属性数组
    const std::vector<float>& AttributeWeights;        // 属性权重数组
    size_t TargetCount;                                // 需要减少到的索引数
    float TargetError;                                 //


    size_t IndexCount;                // Indices数组长度
    size_t VertexCount;               // 顶点个数
    size_t AttributeCount;            // 属性个数
    MVertexAdjacency VertexAdjacency; // 顶点的邻接面
    std::vector<Quadric> VertexQuadrics;
    std::vector<Quadric> AttributeQuadrics;
    std::vector<Gradient> AttributeGradients;
    std::vector<Collapse> Collapses;         // 坍缩边数组
    std::vector<int_t> CollapseOrder;        // 坍缩权重的排序数组
    std::vector<int_t> VertexRemap;          // 顶点重映射，用于坍缩后的顶点映射
    std::vector<unsigned char> VertexLocked; // 用于标记顶点是否被坍缩
};

TetraMeshInternalSimplifier::TetraMeshInternalSimplifier(std::vector<int_t>& Indices,
                                                         const std::vector<Point3>& VertexPositions,
                                                         const std::vector<unsigned char>& IsSurfaceVertex,
                                                         const std::vector<int_t>& SurfaceIndices,
                                                         const std::vector<Attribute>& VertexAttributes,
                                                         const std::vector<float>& AttributeWeights, size_t TargetCount,
                                                         float TargetError)
    : Indices(Indices), VertexPositions(VertexPositions), IsSurfaceVertex(IsSurfaceVertex),
      SurfaceIndices(SurfaceIndices), VertexAttributes(VertexAttributes), AttributeWeights(AttributeWeights),
      TargetCount(TargetCount), TargetError(TargetError) {
    IndexCount = Indices.size();
    VertexCount = VertexPositions.size();
    AttributeCount = VertexAttributes.size();
}

size_t TetraMeshInternalSimplifier::DoWork() {
    AttributeCount = 0;
    // 一些数组的初始化
    VertexQuadrics.resize(VertexCount);
    memset(VertexQuadrics.data(), 0, VertexCount * sizeof(Quadric));
    if (AttributeCount > 0) {
        AttributeQuadrics.resize(VertexCount);
        memset(AttributeQuadrics.data(), 0, VertexCount * sizeof(Quadric));

        AttributeGradients.resize(VertexCount * AttributeCount);
        memset(AttributeGradients.data(), 0, VertexCount * AttributeCount * sizeof(Gradient));
    }

    VertexAdjacency.Offsets.resize(VertexCount + 1);
    VertexAdjacency.Data.resize(IndexCount);
    BuildVertexAdjacency();

    FillVertexQuadrics();

    if (AttributeCount > 0) FillAttributeQuadrics();

    // 坍缩边的最大容量
    size_t CollapseCapacity = IndexCount / 4 * 6;
    Collapses.resize(CollapseCapacity);
    CollapseOrder.resize(CollapseCapacity);

    VertexRemap.resize(VertexCount);
    VertexLocked.resize(VertexCount);

    Timer time;
    int Sequence = 0;

    while (IndexCount > TargetCount) {
        print("Sequence ", Sequence++);

        time.start();
        // 建立新的邻接关系
        BuildVertexAdjacency();
        time.end();
        time.print("1.BuildVertexAdjacency");

        time.start();
        // 初始化坍缩边的误差
        size_t EdgeCollapseCount = BuildEdgeCollapses(CollapseCapacity);
        time.end();
        time.print("2.BuildEdgeCollapses");

        time.start();
        // 给所有的坍缩边排序
        SortEdgeCollapses(EdgeCollapseCount);
        time.end();
        time.print("3.SortEdgeCollapses");

        // 初始化顶点映射
        for (size_t i = 0; i < VertexCount; ++i) VertexRemap[i] = i;

        // 初始化可访问顶点
        memset(VertexLocked.data(), 0, VertexCount * sizeof(unsigned char));

        time.start();
        // 执行边坍缩
        size_t CollapseCount = ExecuteEdgeCollapses(EdgeCollapseCount);
        time.end();
        time.print("4.ExecuteEdgeCollapses");

        time.start();
        UpdateQuadrics();
        time.end();
        time.print("5.UpdateQuadrics");

        time.start();
        IndexCount = RemapIndices();
        time.end();
        time.print("6.RemapIndices");
        //break;
    }

    return IndexCount;
}

void TetraMeshInternalSimplifier::BuildVertexAdjacency() {
    memset(VertexAdjacency.Offsets.data(), 0, (VertexCount + 1) * sizeof(int_t));

    for (int_t i = 0; i < IndexCount; ++i) {
        int_t idx = Indices[i] + 1;
        VertexAdjacency.Offsets[idx]++;
    }

    int_t Offset = 0;
    for (int_t i = 0; i < VertexCount; ++i) {
        int_t idx = i + 1;
        int_t Count = VertexAdjacency.Offsets[idx];
        VertexAdjacency.Offsets[idx] = Offset;
        Offset += Count;
    }

    for (int_t i = 0; i < IndexCount; i += 4) {
        int_t v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2], v3 = Indices[i + 3];

        VertexAdjacency.Data[VertexAdjacency.Offsets[v0 + 1]].handle[0] = v1;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v0 + 1]].handle[1] = v2;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v0 + 1]].handle[2] = v3;
        VertexAdjacency.Offsets[v0 + 1]++;

        VertexAdjacency.Data[VertexAdjacency.Offsets[v1 + 1]].handle[0] = v2;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v1 + 1]].handle[1] = v0;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v1 + 1]].handle[2] = v3;
        VertexAdjacency.Offsets[v1 + 1]++;

        VertexAdjacency.Data[VertexAdjacency.Offsets[v2 + 1]].handle[0] = v0;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v2 + 1]].handle[1] = v1;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v2 + 1]].handle[2] = v3;
        VertexAdjacency.Offsets[v2 + 1]++;

        VertexAdjacency.Data[VertexAdjacency.Offsets[v3 + 1]].handle[0] = v0;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v3 + 1]].handle[1] = v2;
        VertexAdjacency.Data[VertexAdjacency.Offsets[v3 + 1]].handle[2] = v1;
        VertexAdjacency.Offsets[v3 + 1]++;
    }
}

void TetraMeshInternalSimplifier::FillVertexQuadrics() {
    for (int_t i = 0; i < SurfaceIndices.size(); i += 3) {
        int_t i0 = Indices[i];
        int_t i1 = Indices[i + 1];
        int_t i2 = Indices[i + 2];

        Quadric Q;
        Q.ByTriangle(VertexPositions[i0], VertexPositions[i1], VertexPositions[i2]);

        VertexQuadrics[i0] += Q;
        VertexQuadrics[i1] += Q;
        VertexQuadrics[i2] += Q;
    }

    for (int_t i = 0; i < IndexCount; i += 4) {
        int_t i0 = Indices[i];
        int_t i1 = Indices[i + 1];
        int_t i2 = Indices[i + 2];
        int_t i3 = Indices[i + 3];

        Quadric Q;
        if (!IsSurfaceVertex[i0]) {
            Q.ByTriangle(VertexPositions[i1], VertexPositions[i2], VertexPositions[i3]);
            VertexQuadrics[i0] += Q;
        }
        if (!IsSurfaceVertex[i1]) {
            Q.ByTriangle(VertexPositions[i2], VertexPositions[i0], VertexPositions[i3]);
            VertexQuadrics[i1] += Q;
        }
        if (!IsSurfaceVertex[i2]) {
            Q.ByTriangle(VertexPositions[i0], VertexPositions[i1], VertexPositions[i3]);
            VertexQuadrics[i2] += Q;
        }
        if (!IsSurfaceVertex[i3]) {
            Q.ByTriangle(VertexPositions[i0], VertexPositions[i2], VertexPositions[i1]);
            VertexQuadrics[i3] += Q;
        }
    }
}

void TetraMeshInternalSimplifier::FillAttributeQuadrics() {
    for (size_t i = 0; i < IndexCount; i += 4) {

        int_t i0 = Indices[i + 0];
        int_t i1 = Indices[i + 1];
        int_t i2 = Indices[i + 2];
        int_t i3 = Indices[i + 3];
        int_t faceId = i / 4;

        Quadric Q;
        Gradient G[32];
        ComputeAttributeQuadient(faceId, Q, G);

        AttributeQuadrics[i0] += Q;
        AttributeQuadrics[i1] += Q;
        AttributeQuadrics[i2] += Q;
        AttributeQuadrics[i3] += Q;

        for (size_t k = 0; k < AttributeCount; ++k) {
            AttributeGradients[i0 * AttributeCount + k] += G[k];
            AttributeGradients[i1 * AttributeCount + k] += G[k];
            AttributeGradients[i2 * AttributeCount + k] += G[k];
            AttributeGradients[i3 * AttributeCount + k] += G[k];
        }
    }
}

} // namespace meshsmp

namespace mesh_tetra_simplifier
{
using namespace meshsmp;

class VertexAdjacency {
public:
    std::vector<int_t> Offsets;
    std::vector<int_t> Data;

    int_t Num(int_t id) const { return Offsets[id + 1] - Offsets[id]; }
    int_t Begin(int_t id) const { return Offsets[id]; }
    int_t End(int_t id) const { return Offsets[id + 1]; }
};

struct Collapse {
    int_t id;
    int_t count;
    float error;
};


class TetraMeshInternalSimplifier {
public:
    TetraMeshInternalSimplifier(std::vector<int_t>& Indices, std::vector<Point3>& VertexPositions,
                                const std::vector<unsigned char>& IsSurfaceVertex,
                                const std::vector<int_t>& SurfaceIndices, std::vector<Attribute>& VertexAttributes,
                                const std::vector<float>& AttributeWeights, size_t TargetCount, float TargetError)
        : Indices(Indices), VertexPositions(VertexPositions), IsSurfaceVertex(IsSurfaceVertex),
          SurfaceIndices(SurfaceIndices), VertexAttributes(VertexAttributes), AttributeWeights(AttributeWeights),
          TargetCount(TargetCount), TargetError(TargetError) {
        IndexCount = Indices.size();
        VertexCount = VertexPositions.size();
        AttributeCount = VertexAttributes.size();
        TetraCount = IndexCount / 4;
    }

    size_t DoWork() {
        VAdjacency.Offsets.resize(VertexCount + 1);
        VAdjacency.Data.resize(IndexCount);
        Adjacency.Offsets.resize(TetraCount + 1);
        //Adjacency.Data.resize(IndexCount);
        //BuildVertexAdjacency();
        BuildSurfaceMesh();

        size_t CollapseCapacity = TetraCount;
        Collapses.resize(CollapseCapacity);
        CollapseOrder.resize(CollapseCapacity);

        VertexRemap.resize(VertexCount);
        VertexLocked.resize(VertexCount);

        while (IndexCount > TargetCount) {
            BuildVertexAdjacency();

            size_t CollapseCount = BuildCollapses(CollapseCapacity);
            if (CollapseCount == 0) break;

            SortCollapses(CollapseCount);

            // 初始化顶点映射
            for (size_t i = 0; i < VertexCount; ++i) VertexRemap[i] = i;

            // 初始化可访问顶点
            memset(VertexLocked.data(), 0, VertexCount * sizeof(unsigned char));

            CollapseCount = ExecuteCollapses(CollapseCount);
            if (CollapseCount == 0) break;

            IndexCount = RemapIndices();
            TetraCount = IndexCount / 4;
        }
        return IndexCount;
    }

    void BuildSurfaceMesh() {}

    void BuildVertexAdjacency() {
        memset(VAdjacency.Offsets.data(), 0, (VertexCount + 1) * sizeof(int_t));

        for (int_t i = 0; i < IndexCount; ++i) {
            int_t idx = Indices[i] + 1;
            VAdjacency.Offsets[idx]++;
        }

        int_t Offset = 0;
        for (int_t i = 0; i < VertexCount; ++i) {
            int_t idx = i + 1;
            int_t Count = VAdjacency.Offsets[idx];
            VAdjacency.Offsets[idx] = Offset;
            Offset += Count;
        }

        for (int_t i = 0; i < IndexCount; i += 4) {
            int_t v0 = Indices[i], v1 = Indices[i + 1], v2 = Indices[i + 2], v3 = Indices[i + 3];
            int_t cellId = i / 4;
            VAdjacency.Data[VAdjacency.Offsets[v0 + 1]] = cellId;
            VAdjacency.Offsets[v0 + 1]++;

            VAdjacency.Data[VAdjacency.Offsets[v1 + 1]] = cellId;
            VAdjacency.Offsets[v1 + 1]++;

            VAdjacency.Data[VAdjacency.Offsets[v2 + 1]] = cellId;
            VAdjacency.Offsets[v2 + 1]++;

            VAdjacency.Data[VAdjacency.Offsets[v3 + 1]] = cellId;
            VAdjacency.Offsets[v3 + 1]++;
        }

        memset(Adjacency.Offsets.data(), 0, (TetraCount + 1) * sizeof(int_t));
        std::vector<unsigned char> Visited(TetraCount);
        std::vector<int_t> Cache(512);

        for (int_t i = 0; i < IndexCount; i += 4) {
            int_t cellId = i / 4;
            int_t Count = 0;
            for (int_t j = 0; j < 4; ++j) {
                int_t v = Indices[i + j];
                size_t Begin = VAdjacency.Begin(v);
                for (size_t k = 0; k < VAdjacency.Num(v); ++k) {
                    int_t id = VAdjacency.Data[Begin + k];
                    if (Visited[id] == 0 && id != cellId) {
                        Adjacency.Offsets[cellId + 1]++;
                        Visited[id] = 1;
                        Cache[Count++] = id;
                    }
                }
            }
            for (int_t j = 0; j < Count; ++j) { Visited[Cache[j]] = 0; }
        }

        Offset = 0;
        for (int_t i = 0; i < TetraCount; ++i) {
            int_t idx = i + 1;
            int_t Count = Adjacency.Offsets[idx];
            Adjacency.Offsets[idx] = Offset;
            Offset += Count;
        }

        if (Adjacency.Data.size() == 0) Adjacency.Data.resize(Offset * 1.2);

        for (int_t i = 0; i < IndexCount; i += 4) {
            int_t cellId = i / 4;
            int_t Count = 0;
            for (int_t j = 0; j < 4; ++j) {
                int_t v = Indices[i + j];
                size_t Begin = VAdjacency.Begin(v);
                for (size_t k = 0; k < VAdjacency.Num(v); ++k) {
                    int_t id = VAdjacency.Data[Begin + k];
                    if (Visited[id] == 0 && id != cellId) {
                        Adjacency.Data[Adjacency.Offsets[cellId + 1]] = id;
                        Adjacency.Offsets[cellId + 1]++;
                        Visited[id] = 1;
                        Cache[Count++] = id;
                    }
                }
            }
            for (int_t j = 0; j < Count; ++j) { Visited[Cache[j]] = 0; }
        }
    }

    size_t BuildCollapses(size_t CollapseCapacity) {
        std::vector<std::pair<float, float>> OriginFeature(IndexCount / 4);
        for (size_t i = 0; i < IndexCount; i += 4) {
            int_t i0 = Indices[i];
            int_t i1 = Indices[i + 1];
            int_t i2 = Indices[i + 2];
            int_t i3 = Indices[i + 3];
            int_t id = i / 4;

            OriginFeature[id] =
                    QAngleEdge(VertexPositions[i0], VertexPositions[i1], VertexPositions[i2], VertexPositions[i3]);
        }

        auto HasFlip = [](const Point3& p1, const Point3& p2, const Point3& pA, const Point3& pB,
                          const Point3& pC) -> bool {
            Vector3 v1 = pB - pA;
            Vector3 v2 = pC - pA;
            Vector3 normal = v1.Cross(v2);

            double d1 = (p1 - pA).Dot(normal);
            double d2 = (p2 - pA).Dot(normal);

            return (d1 * d2 < 0);
        };

        std::vector<unsigned char> Existed(VertexCount);
        std::vector<int_t> ShouldCollapsed(512);

        size_t CollapseCount = 0;
        for (size_t i = 0; i < IndexCount; i += 4) {
            if (CollapseCount + 1 > CollapseCapacity) { break; }
            int_t i0 = Indices[i];
            int_t i1 = Indices[i + 1];
            int_t i2 = Indices[i + 2];
            int_t i3 = Indices[i + 3];

            if (IsSurfaceVertex[i0] | IsSurfaceVertex[i1] | IsSurfaceVertex[i2] | IsSurfaceVertex[i3]) { continue; }
            int_t id = i / 4;
            Collapse c = {id, 0, 0.f};
            Point3 NewPosition =
                    (VertexPositions[i0] + VertexPositions[i1] + VertexPositions[i2] + VertexPositions[i3]) / 4;

            Existed[i0] = 1;
            Existed[i1] = 1;
            Existed[i2] = 1;
            Existed[i3] = 1;
            bool HasFlips = false;
            int_t ShouldCollapsedCount = 0;

            // 遍历体的邻接体
            size_t Begin = Adjacency.Begin(id);
            for (size_t j = 0; j < Adjacency.Num(id); ++j) {
                int_t cid = Adjacency.Data[Begin + j];

                // 遍历邻接体有多少个点和这个体不同
                int_t Count = 0, v;
                int_t Ids[4]{};
                for (size_t k = 0; k < 4; ++k) {
                    int_t t = Indices[cid * 4 + k];
                    if (Existed[t] == 0) {
                        Ids[Count++] = t; // 不存在
                    } else {
                        v = t;
                    }
                }
                if (Count == 3) { int a = 1; }
                if (Count == 3 && HasFlip(VertexPositions[v], NewPosition, VertexPositions[Ids[0]],
                                          VertexPositions[Ids[1]], VertexPositions[Ids[2]])) {
                    HasFlips = true;
                    ShouldCollapsed[ShouldCollapsedCount++] = cid;
                    break;
                }
            }
            Existed[i0] = 0;
            Existed[i1] = 0;
            Existed[i2] = 0;
            Existed[i3] = 0;

            if (HasFlips) continue;

            // 如果是可坍缩的四面体，则计算坍缩代价
            Point3 Temp0 = VertexPositions[i0];
            Point3 Temp1 = VertexPositions[i1];
            Point3 Temp2 = VertexPositions[i2];
            Point3 Temp3 = VertexPositions[i3];
            VertexPositions[i0] = NewPosition;
            VertexPositions[i1] = NewPosition;
            VertexPositions[i2] = NewPosition;
            VertexPositions[i3] = NewPosition;

            float EGeo = 0;
            for (size_t j = 0; j < ShouldCollapsedCount; ++j) {
                int_t s = ShouldCollapsed[j];
                int_t t = ShouldCollapsed[j] * 4;
                auto [angle, edge] = QAngleEdge(VertexPositions[t], VertexPositions[t + 1], VertexPositions[t + 2],
                                                VertexPositions[t + 3]);

                EGeo += sqrt(angle * edge / OriginFeature[s].first / OriginFeature[s].second);
            }

            VertexPositions[i0] = Temp0;
            VertexPositions[i1] = Temp1;
            VertexPositions[i2] = Temp2;
            VertexPositions[i3] = Temp3;

            float EScalar = 0;
            for (size_t k = 0; k < AttributeCount; ++k) {
                auto& Attr = VertexAttributes[k];
                float Val0 = Attr.Primitive[i0 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
                float Val1 = Attr.Primitive[i1 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
                float Val2 = Attr.Primitive[i2 * Attr.Stride + Attr.Offset] * AttributeWeights[k];
                float Val3 = Attr.Primitive[i3 * Attr.Stride + Attr.Offset] * AttributeWeights[k];

                float Val = (Val0 + Val1 + Val2 + Val3) / 4;
                EScalar += abs(Val - Val0);
                EScalar += abs(Val - Val1);
                EScalar += abs(Val - Val2);
                EScalar += abs(Val - Val3);
            }
            c.count = Adjacency.Num(id) - ShouldCollapsedCount;
            c.error = EScalar + EGeo;
            Collapses[CollapseCount++] = c;
        }
        return CollapseCount;
    }

    void SortCollapses(size_t CollapseCount) {
        //std::sort(Collapses.begin(), Collapses.begin() + CollapseCount,
        //          [](const Collapse& o1, const Collapse& o2) { return o1.error < o2.error; });
        //return;

        const unsigned int sort_bits = 12;
        const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

        // fill histogram for counting sort
        unsigned int histogram[sort_bins];
        memset(histogram, 0, sizeof(histogram));

        for (size_t i = 0; i < CollapseCount; ++i) {
            // skip sign bit since error is non-negative
            unsigned int error;
            std::memcpy(&error, &Collapses[i].error, sizeof(error));
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
        for (size_t i = 0; i < CollapseCount; ++i) {
            // skip sign bit since error is non-negative
            unsigned int error;
            std::memcpy(&error, &Collapses[i].error, sizeof(error));
            unsigned int key = (error << 1) >> (32 - sort_bits);
            key = key < sort_bins ? key : sort_bins - 1;

            CollapseOrder[histogram[key]++] = i;
        }
    }

    size_t ExecuteCollapses(size_t CollapseCount) {
        size_t Count = 0;
        for (size_t i = 0; i < CollapseCount; ++i) {
            const Collapse& c = Collapses[CollapseOrder[i]];

            if (c.error > Collapses[CollapseOrder[CollapseCount / 2]].error && Count > CollapseCount / 6) { break; }
            int_t id = c.id;
            int_t t = id * 4;
            int_t i0 = Indices[t + 0];
            int_t i1 = Indices[t + 1];
            int_t i2 = Indices[t + 2];
            int_t i3 = Indices[t + 3];

            if (VertexLocked[i0] | VertexLocked[i1] | VertexLocked[i2] | VertexLocked[i3]) continue;

            VertexPositions[i0] =
                    (VertexPositions[i0] + VertexPositions[i1] + VertexPositions[i2] + VertexPositions[i3]) / 4;

            for (int k = 0; k < AttributeCount; ++k) {
                auto& Attr = VertexAttributes[k];
                Attr.Primitive[i0 * Attr.Stride + Attr.Offset] = (Attr.Primitive[i0 * Attr.Stride + Attr.Offset] +
                                                                  Attr.Primitive[i1 * Attr.Stride + Attr.Offset] +
                                                                  Attr.Primitive[i2 * Attr.Stride + Attr.Offset] +
                                                                  Attr.Primitive[i3 * Attr.Stride + Attr.Offset]) /
                                                                 4;
            }

            VertexRemap[i1] = i0;
            VertexRemap[i2] = i0;
            VertexRemap[i3] = i0;
            VertexLocked[i0] = 1;
            VertexLocked[i1] = 1;
            VertexLocked[i2] = 1;
            VertexLocked[i3] = 1;

            Count += c.count;
        }
        return Count;
    }

    size_t RemapIndices() {
        size_t k = 0;

        for (size_t i = 0; i < IndexCount; i += 4) {
            int_t v0 = VertexRemap[Indices[i + 0]];
            int_t v1 = VertexRemap[Indices[i + 1]];
            int_t v2 = VertexRemap[Indices[i + 2]];
            int_t v3 = VertexRemap[Indices[i + 3]];

            if (v0 != v1 && v0 != v2 && v0 != v3 && v1 != v2 && v1 != v3 && v2 != v3) {
                Indices[k + 0] = v0;
                Indices[k + 1] = v1;
                Indices[k + 2] = v2;
                Indices[k + 3] = v3;
                k += 4;
            }
        }

        return k;
    }

    std::pair<float, float> QAngleEdge(const Point3& v0, const Point3& v1, const Point3& v2, const Point3& v3) {
        auto DihedralAngle = [](const Point3& a, const Point3& b, const Point3& c, const Point3& d) -> float {
            // 计算面 (a, b, c) 和 (a, b, d) 的二面角
            Point3 n1 = (b - a).Cross(c - a);
            Point3 n2 = (b - a).Cross(d - a);

            float len1 = n1.Length();
            float len2 = n2.Length();
            if (len1 == 0 || len2 == 0) return 0.0f;

            float cosTheta = n1.Dot(n2) / (len1 * len2);
            return std::acos(std::clamp(cosTheta, -1.0f, 1.0f)); // 返回弧度制角度
        };

        std::array<float, 6> dihedralAngles = {DihedralAngle(v0, v1, v2, v3), DihedralAngle(v0, v2, v3, v1),
                                               DihedralAngle(v0, v3, v1, v2), DihedralAngle(v1, v2, v3, v0),
                                               DihedralAngle(v1, v3, v0, v2), DihedralAngle(v2, v3, v1, v0)};

        // 找到最小二面角
        float minAngle = *std::min_element(dihedralAngles.begin(), dihedralAngles.end());

        auto Length = [](const Point3& v) -> float { return v.Length(); };

        // 计算所有边长
        std::array<float, 6> edgeLengths = {Length(v1 - v0), Length(v2 - v0), Length(v3 - v0),
                                            Length(v2 - v1), Length(v3 - v1), Length(v3 - v2)};

        // 计算最长边 / 最短边
        float maxEdge = *std::max_element(edgeLengths.begin(), edgeLengths.end());
        float minEdge = *std::min_element(edgeLengths.begin(), edgeLengths.end());
        float edgeRatio = maxEdge / minEdge;

        return {1.23 / minAngle, edgeRatio};
    }

private:
    //-------------- Input's Data--------------//
    std::vector<int_t>& Indices;                       // 四面体索引数组
    std::vector<Point3>& VertexPositions;              // 顶点数组
    const std::vector<unsigned char>& IsSurfaceVertex; // 记录哪些顶点在表面
    const std::vector<int_t>& SurfaceIndices;          //
    std::vector<Attribute>& VertexAttributes;          // 顶点的属性数组
    const std::vector<float>& AttributeWeights;        // 属性权重数组
    size_t TargetCount;                                // 需要减少到的索引数
    float TargetError;                                 //

    size_t IndexCount;     // Indices数组长度
    size_t TetraCount;     // Indices数组长度
    size_t VertexCount;    // 顶点个数
    size_t AttributeCount; // 属性个数

    VertexAdjacency VAdjacency;
    VertexAdjacency Adjacency;
    std::vector<Collapse> Collapses;         // 坍缩边数组
    std::vector<int_t> CollapseOrder;        // 坍缩权重的排序数组
    std::vector<int_t> VertexRemap;          // 顶点重映射，用于坍缩后的顶点映射
    std::vector<unsigned char> VertexLocked; // 用于标记顶点是否被坍缩
};
} // namespace mesh_tetra_simplifier

bool MeshSimplifier::Execute() {

    //Box3 box{{0, 0, 0}, {1, 1, 1}};
    //Octree o;
    //o.Initialize(box);
    //o.NewAllSons(o.Root());
    //o.NewAllSons(o.Root()->Sons[0]);
    //auto i = o.Interize({0.1, 0.1, 0.1});
    //auto* node = o.FindNode(i);
    //print(node->Voxel.Min.x, node->Voxel.Min.y, node->Voxel.Min.z, node->Voxel.Max.x, node->Voxel.Max.y,
    //      node->Voxel.Max.z);

    //i = o.Interize({0.3, 0.3, 0.3});
    //node = o.FindNode(i);
    //print(node->Voxel.Min.x, node->Voxel.Min.y, node->Voxel.Min.z, node->Voxel.Max.x, node->Voxel.Max.y,
    //      node->Voxel.Max.z);

    //i = o.Interize({0.7, 0.7, 0.7});
    //node = o.FindNode(i);
    //print(node->Voxel.Min.x, node->Voxel.Min.y, node->Voxel.Min.z, node->Voxel.Max.x, node->Voxel.Max.y,
    //      node->Voxel.Max.z);
    //return false;

    if (DynamicCast<SurfaceMesh>(GetInput(0))) {
        using namespace meshsmp;
        SurfaceMesh::Pointer Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
        std::vector<int_t> Indices;
        std::vector<Point3> VertexPositions;
        std::vector<Attribute> VertexAttributes;
        std::vector<float> AttributeWeights;

        size_t TargetCount;
        float TargetError;

        igIndex face[IGAME_CELL_MAX_SIZE]{};
        for (int i = 0; i < Mesh->GetNumberOfFaces(); ++i) {
            int size = Mesh->GetFacePointIds(i, face);
            Indices.push_back(face[0]);
            Indices.push_back(face[1]);
            Indices.push_back(face[2]);
        }
        RescalePositions(VertexPositions, Mesh->GetPoints());

        for (int i = 0; Mesh->GetAttributeSet() && i < Mesh->GetAttributeSet()->GetNumberOfAttributes(); ++i) {
            auto& attr = Mesh->GetAttributeSet()->GetAttribute(i);
            for (int j = 0; j < attr.pointer->GetDimension(); ++j) {
                Attribute Attr;
                Attr.Primitive = DynamicCast<FloatArray>(attr.pointer)->RawPointer();
                Attr.Stride = attr.pointer->GetDimension();
                Attr.Offset = j;
                VertexAttributes.push_back(Attr);
                AttributeWeights.push_back(1);
            }
        }
        if (TargetFaceCount != 0) 
        { 
            TargetCount = TargetFaceCount * 3;
        }
        else
        {
            TargetCount = Indices.size() * (1 - this->TargetReduction);
        }
        TargetError = 0.01f;

        TriMeshInternalSimplifier Simplifier(Indices, VertexPositions, VertexAttributes, AttributeWeights, TargetCount,
                                             TargetError);
        size_t IndexCount = Simplifier.DoWork();
        SurfaceMesh::Pointer NewMesh = SurfaceMesh::New();
        NewMesh->SetName(Mesh->GetName());

        CellArray::Pointer Faces = CellArray::New();
        for (int i = 0; i < IndexCount / 3; i++) {
            Faces->AddCellId3(Indices[i * 3 + 0], Indices[i * 3 + 1], Indices[i * 3 + 2]);
        }
        NewMesh->SetFaces(Faces);
        NewMesh->SetPoints(Mesh->GetPoints());
        NewMesh->SetAttributeSet(Mesh->GetAttributeSet());
        SetOutput(NewMesh);
    } else if (DynamicCast<UnstructuredMesh>(GetInput(0))) {
        using namespace mesh_tetra_simplifier;
        UnstructuredMesh::Pointer Mesh;
        SurfaceMesh::Pointer SMesh;

        Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
        SMesh = DynamicCast<SurfaceMesh>(Mesh->GetDisplayObject());

        std::vector<int_t> Indices;
        std::vector<int_t> SurfaceIndices;
        std::vector<Point3> VertexPositions;
        std::vector<unsigned char> IsSurfaceVertex;
        std::vector<Attribute> VertexAttributes;
        std::vector<float> AttributeWeights;
        size_t TargetCount;
        float TargetError;

        igIndex cell[IGAME_CELL_MAX_SIZE]{};
        for (int i = 0; i < Mesh->GetNumberOfCells(); ++i) {
            int size = Mesh->GetCellPointIds(i, cell);
            if (size != 4) return false;
            Indices.push_back(cell[0]);
            Indices.push_back(cell[1]);
            Indices.push_back(cell[2]);
            Indices.push_back(cell[3]);
        }
        float scale = RescalePositions(VertexPositions, Mesh->GetPoints());

        AttributeSet::Pointer NewAttrs = AttributeSet::New();
        for (int k = 0; k < Mesh->GetAttributeSet()->GetNumberOfAttributes(); k++) {
            auto& attr = Mesh->GetAttributeSet()->GetAttribute(k);
            if (attr.attachmentType == IG_CELL) continue;
            FloatArray::Pointer Ptr = FloatArray::New();
            Ptr->SetDimension(attr.pointer->GetDimension());
            Ptr->SetName(attr.pointer->GetName());
            float ele[16]{};
            for (int i = 0; i < Mesh->GetNumberOfPoints(); i++) {
                attr.pointer->GetElement(i, ele);
                Ptr->AddElement(ele);
            }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, Ptr);
        }

        for (int i = 0; i < NewAttrs->GetNumberOfAttributes(); ++i) {
            auto& attr = NewAttrs->GetAttribute(i);
            for (int j = 0; j < attr.pointer->GetDimension(); ++j) {
                Attribute Attr;
                if (!DynamicCast<FloatArray>(attr.pointer)) {
                    FloatArray::Pointer newArray = FloatArray::New();
                    newArray->SetDimension(attr.pointer->GetDimension());
                    for (int k = 0; k < attr.pointer->GetNumberOfValues(); k++) {
                        newArray->AddValue(static_cast<float>(attr.pointer->GetValue(k)));
                    }
                    attr.pointer = newArray;
                }
                Attr.Primitive = DynamicCast<FloatArray>(attr.pointer)->RawPointer();
                Attr.Stride = attr.pointer->GetDimension();
                Attr.Offset = j;
                VertexAttributes.push_back(Attr);
                AttributeWeights.push_back(1);
            }
        }

        TargetCount = Indices.size() * 0.5;
        TargetError = 0.01f;

        {
            auto PointMap = Mesh->GetPointMap();
            IsSurfaceVertex.resize(VertexPositions.size(), 0);

            for (int i = 0; i < PointMap->GetNumberOfValues(); i++) {
                if (PointMap->GetValue(i) != -1) { IsSurfaceVertex[i] = 1; }
            }
        }

        mesh_tetra_simplifier::TetraMeshInternalSimplifier Simplifier(Indices, VertexPositions, IsSurfaceVertex,
                                                                      SurfaceIndices, VertexAttributes,
                                                                      AttributeWeights, TargetCount, TargetError);
        size_t IndexCount = Simplifier.DoWork();
        UnstructuredMesh::Pointer NewMesh = UnstructuredMesh::New();
        NewMesh->SetName(Mesh->GetName());

        CellArray::Pointer Cells = CellArray::New();
        UnsignedIntArray::Pointer Types = UnsignedIntArray::New();
        std::vector<unsigned char> PointExisted(Mesh->GetNumberOfPoints());
        for (int i = 0; i < IndexCount; i += 4) {
            PointExisted[Indices[i + 0]] = 1;
            PointExisted[Indices[i + 1]] = 1;
            PointExisted[Indices[i + 2]] = 1;
            PointExisted[Indices[i + 3]] = 1;
        }
        std::vector<igIndex> PointMap(PointExisted.size());
        int c = 0;
        Points::Pointer NewPoints = Points::New();
        auto bbox = Mesh->GetBoundingBox();
        for (int i = 0; i < PointExisted.size(); i++) {
            if (PointExisted[i]) {
                PointMap[i] = c++;
                NewPoints->AddPoint(VertexPositions[i].x * scale + bbox.min[0],
                                    VertexPositions[i].y * scale + bbox.min[1],
                                    VertexPositions[i].z * scale + bbox.min[2]);
            }
        }
        for (int i = 0; i < IndexCount; i += 4) {
            Cells->AddCellId4(PointMap[Indices[i + 0]], PointMap[Indices[i + 1]], PointMap[Indices[i + 2]],
                              PointMap[Indices[i + 3]]);
            Types->AddValue(IG_TETRA);
        }
        for (int k = 0; k < NewAttrs->GetNumberOfAttributes(); k++) {
            auto& attr = NewAttrs->GetAttribute(k);
            FloatArray::Pointer Ptr = FloatArray::New();
            Ptr->SetDimension(attr.pointer->GetDimension());
            Ptr->SetName(attr.pointer->GetName());
            float ele[16]{};
            int c = 0;
            for (int i = 0; i < Mesh->GetNumberOfPoints(); i++) {
                if (PointExisted[i]) {
                    attr.pointer->GetElement(i, ele);
                    attr.pointer->SetElement(c++, ele);
                }
            }
            attr.pointer->Resize(c);
        }

        NewMesh->SetCells(Cells, Types);
        NewMesh->SetPoints(NewPoints);
        NewMesh->SetAttributeSet(NewAttrs);
        SetOutput(NewMesh);
    }

    return true;
}

MeshSimplifier::MeshSimplifier() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}


IGAME_NAMESPACE_END