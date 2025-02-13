#include "meshsimplifier.h"
#include "../UHEMesh/HEMesh.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>


class Allocator {
public:
    template<typename T>
    struct StorageT {
        static void* (*allocate)(size_t);
        static void (*deallocate)(void*);
    };

    typedef StorageT<void> Storage;

    Allocator() : blocks(), count(0) {}

    ~Allocator() {
        for (size_t i = count; i > 0; --i) Storage::deallocate(blocks[i - 1]);
    }

    template<typename T>
    T* allocate(size_t size) {
        assert(count < sizeof(blocks) / sizeof(blocks[0]));
        T* result = static_cast<T*>(Storage::allocate(size > size_t(-1) / sizeof(T) ? size_t(-1) : size * sizeof(T)));
        blocks[count++] = result;
        return result;
    }

    void deallocate(void* ptr) {
        assert(count > 0 && blocks[count - 1] == ptr);
        Storage::deallocate(ptr);
        count--;
    }

private:
    void* blocks[24];
    size_t count;
};

template<typename T>
void* (*Allocator::StorageT<T>::allocate)(size_t) = operator new;
template<typename T>
void (*Allocator::StorageT<T>::deallocate)(void*) = operator delete;

using namespace Ubpa;

class Vertex;
class Edge;
class Face;
using TraitsVEP = HEMeshTraits_EmptyH<Vertex, Edge, Face>;

class Vertex : public TVertex<TraitsVEP> {
public:
    Vertex() {}
    Vertex(unsigned int index) : index(index) {}

    unsigned int Index() const { return index; }
public:
    unsigned int index;
};
class Edge : public TEdge<TraitsVEP> {
public:
    Edge() {}
};
class Face : public TPolygon<TraitsVEP> {
public:
    Face() {}
};

using Mesh = HEMesh<TraitsVEP>;

struct Vector3f {
    float x, y, z;
};

struct Collapse {
    Edge* e;
    unsigned int v0;
    unsigned int v1;
    
    Vector3f v;
    float error;

    bool operator<(const Collapse& o) const { return error > o.error; }
};

using Heap = std::priority_queue<Collapse>;



static float normalize(Vector3f& v) {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (length > 0) {
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    return length;
}

static Vector3f cross(const Vector3f& v1, const Vector3f& v2) {
    return Vector3f{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

struct Quadric {
    // a00*x^2 + a11*y^2 + a22*z^2 + 2*(a10*xy + a20*xz + a21*yz) + b0*x + b1*y + b2*z + c
    float a00, a11, a22;
    float a10, a20, a21;
    float b0, b1, b2, c;
    float w;
};

// 梯度 value = gx*x + gy*y + gz*z + gw
struct QuadricGrad {
    float gx, gy, gz, gw;
};

static bool quadricFindMinimum(Quadric& Q, Vector3f& v) {
    Eigen::Matrix3d A;
    Eigen::Vector3d be;
    A << Q.a00, Q.a10, Q.a20, Q.a10, Q.a11, Q.a21, Q.a20, Q.a21, Q.a22;
    be << -Q.b0, -Q.b1, -Q.b2;

    Eigen::FullPivLU<Eigen::MatrixXd> lu(A);
    if (lu.isInvertible()) {
        Eigen::Vector3d xe = lu.solve(be);
        v.x = xe[0];
        v.y = xe[1];
        v.z = xe[2];
        return true;
    }
    return false;
}

static float quadricEval(const Quadric& Q, const Vector3f& v) {
    float rx = Q.b0;
    float ry = Q.b1;
    float rz = Q.b2;

    rx += Q.a10 * v.y;
    ry += Q.a21 * v.z;
    rz += Q.a20 * v.x;

    rx *= 2;
    ry *= 2;
    rz *= 2;

    rx += Q.a00 * v.x;
    ry += Q.a11 * v.y;
    rz += Q.a22 * v.z;

    float r = Q.c;
    r += rx * v.x;
    r += ry * v.y;
    r += rz * v.z;

    return r;
}

static float quadricError(const Quadric& Q, const Vector3f& v) {
    float r = quadricEval(Q, v);
    float s = Q.w == 0.f ? 0.f : 1.f / Q.w;

    return fabsf(r) * s;
}

static void quadricByPlane(Quadric& Q, float a, float b, float c, float d, float w) {
    float aw = a * w;
    float bw = b * w;
    float cw = c * w;
    float dw = d * w;

    Q.a00 = a * aw;
    Q.a11 = b * bw;
    Q.a22 = c * cw;
    Q.a10 = a * bw;
    Q.a20 = a * cw;
    Q.a21 = b * cw;
    Q.b0 = a * dw;
    Q.b1 = b * dw;
    Q.b2 = c * dw;
    Q.c = d * dw;
    Q.w = w;
}

static void quadricByTriangle(Quadric& Q, const Vector3f& p0, const Vector3f& p1, const Vector3f& p2, float weight) {
    Vector3f p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    Vector3f p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    Vector3f normal = cross(p10, p20);
    float area = normalize(normal);

    float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

    // 使用 sqrtf（面积）使误差线性缩放，这往往会改善轮廓
    quadricByPlane(Q, normal.x, normal.y, normal.z, -distance, sqrtf(area) * weight);
}

static void quadricAdd(Quadric& Q, const Quadric& R) {
    Q.a00 += R.a00;
    Q.a11 += R.a11;
    Q.a22 += R.a22;
    Q.a10 += R.a10;
    Q.a20 += R.a20;
    Q.a21 += R.a21;
    Q.b0 += R.b0;
    Q.b1 += R.b1;
    Q.b2 += R.b2;
    Q.c += R.c;
    Q.w += R.w;
}

static Quadric quadricAdded(Quadric& L, const Quadric& R) {
    return Quadric{L.a00 + R.a00, L.a11 + R.a11, L.a22 + R.a22, L.a10 + R.a10, L.a20 + R.a20, L.a21 + R.a21,
                   L.b0 + R.b0,   L.b1 + R.b1,   L.b2 + R.b2,   L.c + R.c,     L.w + R.w};
}

static void quadricAdd(QuadricGrad* G, const QuadricGrad* R, size_t attribute_count) {
    for (size_t k = 0; k < attribute_count; ++k) {
        G[k].gx += R[k].gx;
        G[k].gy += R[k].gy;
        G[k].gz += R[k].gz;
        G[k].gw += R[k].gw;
    }
}

// 保存所有三角形内每一个顶点的对边，data的长度为（三角形 * 3）
struct EdgeAdjacency {
    struct Edge {
        unsigned int next; // 三角形内下一个顶点的索引
        unsigned int prev; // 三角形内上一个顶点的索引
    };

    unsigned int* offsets; // 保存邻接边的起始位置
    Edge* data;

    size_t getEdgeAdjN(unsigned int i) { 
        return static_cast<size_t>(offsets[i + 1] - offsets[i]);
    }
};

struct Adjacency {
    struct Edge {
        unsigned int next; // 三角形内下一个顶点的索引
        unsigned int prev; // 三角形内上一个顶点的索引
    };

    struct Link {
        Edge link_edge;
        unsigned int next_link;
    };

    static constexpr unsigned int Invalid = std::numeric_limits<unsigned int>::max();

    unsigned int* counts; // 保存邻接结构的数量
    unsigned int* starts; // 保存邻接结构的起始位置
    Link* data;
};

static void initMesh(Mesh& mesh, const unsigned int* indices, size_t index_count, size_t vertex_count) {
    mesh.Init(indices, index_count, vertex_count);
}

static void initAdjacency(Adjacency& adjacency, size_t index_count, size_t vertex_count, Allocator& allocator) 
{
    adjacency.counts = allocator.allocate<unsigned int>(vertex_count);
    adjacency.starts = allocator.allocate<unsigned int>(vertex_count);
    adjacency.data = allocator.allocate<Adjacency::Link>(index_count);
}

static void updateAdjacency(Adjacency& adjacency, const unsigned int* indices, size_t index_count, size_t vertex_count,
                            Allocator& allocator) {
    size_t face_count = index_count / 3;
    unsigned int* starts = adjacency.starts;
    unsigned int* counts = adjacency.counts;
    Adjacency::Link* data = adjacency.data;
    unsigned int* offsets = allocator.allocate<unsigned int>(vertex_count);

    memset(counts, 0, vertex_count * sizeof(unsigned int));

    // 计算顶点的度
    for (size_t i = 0; i < index_count; ++i) {
        unsigned int v = indices[i];
        assert(v < vertex_count);
        counts[v]++;
    }

    // 计算前缀和
    unsigned int offset = 0;
    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int count = counts[i];
        offsets[i] = starts[i] = offset;
        offset += count;
    }

    // 填充邻接边的信息
    for (size_t i = 0; i < face_count; ++i) {
        unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

        data[offsets[a]].link_edge = {b, c};
        data[offsets[a]].next_link = offsets[a] + 1;
        offsets[a]++;

        data[offsets[b]].link_edge = {c, a};
        data[offsets[b]].next_link = offsets[b] + 1;
        offsets[b]++;

        data[offsets[c]].link_edge = {a, b};
        data[offsets[c]].next_link = offsets[c] + 1;
        offsets[c]++;
    }

    for (size_t i = 0; i < vertex_count; ++i) { 
        data[offsets[i] - 1].next_link = Adjacency::Invalid;
    }

    allocator.deallocate(offsets);
}

static void initEdgeAdjacency(EdgeAdjacency& adjacency, size_t index_count, size_t vertex_count,
                                 Allocator& allocator) {
    adjacency.offsets = allocator.allocate<unsigned int>(vertex_count + 1);
    adjacency.data = allocator.allocate<EdgeAdjacency::Edge>(index_count);
}

static void updateEdgeAdjacency(EdgeAdjacency& adjacency, const unsigned int* indices, size_t index_count,
                                size_t vertex_count) {
    size_t face_count = index_count / 3;
    adjacency.offsets[0] = 0;
    unsigned int* offsets = adjacency.offsets + 1;
    EdgeAdjacency::Edge* data = adjacency.data;

    memset(offsets, 0, vertex_count * sizeof(unsigned int));

    // 计算顶点的度
    for (size_t i = 0; i < index_count; ++i) {
        unsigned int v = indices[i];
        assert(v < vertex_count);
        offsets[v]++;
    }

    // 计算前缀和
    unsigned int offset = 0;
    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int count = offsets[i];
        offsets[i] = offset;
        offset += count;
    }

    // 填充邻接边的信息
    for (size_t i = 0; i < face_count; ++i) {
        unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

        data[offsets[a]].next = b;
        data[offsets[a]].prev = c;
        offsets[a]++;

        data[offsets[b]].next = c;
        data[offsets[b]].prev = a;
        offsets[b]++;

        data[offsets[c]].next = a;
        data[offsets[c]].prev = b;
        offsets[c]++;
    }

    assert(adjacency.offsets[vertex_count] == index_count);
}

static float rescalePositions(Vector3f* result, const float* vertex_positions_data, size_t vertex_count) {
    float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (size_t i = 0; i < vertex_count; ++i) {
        const float* v = vertex_positions_data + i * 3;

        if (result) {
            result[i].x = v[0];
            result[i].y = v[1];
            result[i].z = v[2];
        }

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

    if (result) {
        float scale = extent == 0 ? 0.f : 1.f / extent;

        for (size_t i = 0; i < vertex_count; ++i) {
            result[i].x = (result[i].x - minv[0]) * scale;
            result[i].y = (result[i].y - minv[1]) * scale;
            result[i].z = (result[i].z - minv[2]) * scale;
        }
    }

    return extent;
}

static void rescaleAttributes(float* result, const float* vertex_attributes_data, size_t vertex_count,
                              const float* attribute_weights, size_t attribute_count, const unsigned int* attribute_remap) 
{
    for (size_t i = 0; i < vertex_count; ++i) {
        for (size_t k = 0; k < attribute_count; ++k) {
            unsigned int rk = attribute_remap[k];
            float a = vertex_attributes_data[i * attribute_count + rk];

            result[i * attribute_count + k] = a * attribute_weights[rk];
        }
    }
}


static void initFaceQuadrics(Quadric* vertex_quadrics, const unsigned int* indices, size_t index_count,
                             const Vector3f* vertex_positions) {
    for (size_t i = 0; i < index_count; i += 3) {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        Quadric Q;
        quadricByTriangle(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], 1.f);

        quadricAdd(vertex_quadrics[i0], Q);
        quadricAdd(vertex_quadrics[i1], Q);
        quadricAdd(vertex_quadrics[i2], Q);
    }
}


static void quadricByAttributes(Quadric& Q, QuadricGrad* G, const Vector3f& p0, const Vector3f& p1, const Vector3f& p2,
                                  const float* va0, const float* va1, const float* va2, size_t attribute_count) {

    // 我们使用下面这个线性插值函数计算新位置 pos 处的属性值
    //      eval(pos) = pos.x * gx + pos.y * gy + pos.z * gz + gw
    // 其中，gx/gy/gz 是属性梯度，gw是基准常数值
    // 使用插值处的属性值与真实值的差的平方作为属性误差
    //      Δ(pos) = (eval(pos) - attr)^2

    Vector3f p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    Vector3f p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    Vector3f normal = cross(p10, p20);
    float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) * 0.5f;

    // quadric 使用三角形面积进行加权
    float w = area;

    // 我们使用重心坐标计算梯度，重心坐标的计算方法如下：
    // v = (d11 * d20 - d01 * d21) / denom
    // w = (d00 * d21 - d01 * d20) / denom
    // u = 1 - v - w
    // here v0, v1 are triangle edge vectors, v2 is a vector from point to triangle corner, and dij = dot(vi, vj)
    // note: v2 and d20/d21 can not be evaluated here as v2 is effectively an unknown variable; we need these only as variables for derivation of gradients
    const Vector3f& v0 = p10;
    const Vector3f& v1 = p20;
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

    for (size_t k = 0; k < attribute_count; ++k) {
        float a0 = va0[k], a1 = va1[k], a2 = va2[k];

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


static void initAttributeQuadrics(Quadric* attribute_quadrics, QuadricGrad* attribute_gradients, const unsigned int* indices,
                      size_t index_count, const Vector3f* vertex_positions, const float* vertex_attributes,
                      size_t attribute_count) {
    for (size_t i = 0; i < index_count; i += 3) {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        Quadric QA;
        QuadricGrad G[32];
        quadricByAttributes(QA, G, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2],
                              &vertex_attributes[i0 * attribute_count], &vertex_attributes[i1 * attribute_count],
                              &vertex_attributes[i2 * attribute_count], attribute_count);

        quadricAdd(attribute_quadrics[i0], QA);
        quadricAdd(attribute_quadrics[i1], QA);
        quadricAdd(attribute_quadrics[i2], QA);

        quadricAdd(&attribute_gradients[i0 * attribute_count], G, attribute_count);
        quadricAdd(&attribute_gradients[i1 * attribute_count], G, attribute_count);
        quadricAdd(&attribute_gradients[i2 * attribute_count], G, attribute_count);
    }
}

static void initEdgeCollapses(Heap& heap, Mesh& mesh,
    Quadric* vertex_quadrics, Quadric* attribute_quadrics, QuadricGrad* attribute_gradients) 
{
    for (auto* e : mesh.Edges()) {

        unsigned int i0 = e->HalfEdge()->Origin()->Index();
        unsigned int i1 = e->HalfEdge()->End()->Index();

        Quadric vq = quadricAdded(vertex_quadrics[i0], vertex_quadrics[i1]);

        Collapse c = {e, i0, i1};
        bool find = quadricFindMinimum(vq, c.v);
        c.error = quadricError(vq, c.v);

        if (attribute_quadrics) {
            Quadric aq = quadricAdded(attribute_quadrics[i0], attribute_quadrics[i1]);
            quadricError(aq, c.v);
        }

        heap.push(c);
    }
}

using HashMap = std::unordered_map<size_t, int>;

static size_t EdgeHash(unsigned int v0, unsigned int v1) {
    if (v0 > v1) std::swap(v0, v1);
    return std::hash<unsigned int>()(v0) ^ std::hash<unsigned int>()(v1);
}

static size_t performEdgeCollapses(Heap& heap, HashMap& hashmap, Mesh& mesh, Vector3f* vertex_positions,
    Quadric* vertex_quadrics, Quadric* attribute_quadrics, QuadricGrad* attribute_gradients,
    size_t attribute_count, size_t triangle_collapse_goal, float error_limit, float& result_error) 
{

    size_t triangle_collapses = 0;

    while (triangle_collapses < triangle_collapse_goal) { 

        if (heap.empty()) 
            break;

        Collapse c = heap.top();
        heap.pop();

        Edge* e = c.e;
        Vertex* v0 = e->HalfEdge()->Origin();
        Vertex* v1 = e->HalfEdge()->End();

        unsigned int i0 = v0->Index();
        unsigned int i1 = v1->Index();

        size_t hash_value = EdgeHash(i0, i1);

        while (hashmap.count(hash_value)) {
            hashmap.erase(hash_value);

            if (heap.empty())
                return triangle_collapses;

            c = heap.top();
            heap.pop();

            e = c.e;
            v0 = e->HalfEdge()->Origin();
            v1 = e->HalfEdge()->End();
            i0 = v0->Index();
            i1 = v1->Index();

            hash_value = EdgeHash(i0, i1);
        }

        if (c.error > error_limit) { break; }

        if (!mesh.IsCollapsable(e)) { continue; }

        triangle_collapses += e->IsOnBoundary() ? 1 : 2;

        //for (auto* adj : v0->AdjVertices()) {
        //     size_t hash = EdgeHash(i0, adj->Index());
        //     hashmap[hash]++;
        //}
        //for (auto* adj: v1->AdjVertices()) {
        //     size_t hash = EdgeHash(i1, adj->Index());
        //     hashmap[hash]++;
        //}

        //Vertex* nv = mesh.CollapseEdge(e, v1->Index());
        //vertex_positions[nv->Index()] = c.v;

        //for (auto* adj: nv->AdjEdges()) {

        //    unsigned int v0 = adj->HalfEdge()->Origin()->Index();
        //    unsigned int v1 = adj->HalfEdge()->End()->Index();
        //    Quadric vq = quadricAdded(vertex_quadrics[v0], vertex_quadrics[v1]);

        //    Collapse c = {adj, v0, v1};
        //    bool find = quadricFindMinimum(vq, c.v);
        //    c.error = quadricError(vq, c.v);

        //    if (attribute_count) {
        //        Quadric aq = quadricAdded(attribute_quadrics[v0], attribute_quadrics[v1]);
        //        c.error += quadricError(aq, c.v);
        //    }

        //    heap.push(c);
        //}

        result_error = result_error < c.error ? c.error : result_error;

        //unsigned int* p = &adjacency.starts[r1];
        //while ((*p) != Adjacency::Invalid) { 
        //    Adjacency::Link& node = adjacency.data[(*p)];
        //    size_t hash = EdgeHash(node.link_edge.next, r1);
        //    hashmap[hash]++;

        //    std::cout << node.link_edge.prev << " " << node.link_edge.next << std::endl;

        //    //hash = EdgeHash(node.link_edge.prev, r1);
        //    //hashmap[hash]++;
        //    p = &node.next_link;
        //}


        //collapse_remap[i0] = r1;
        //vertex_positions[r1] = c.v;
        //quadricAdd(vertex_quadrics[r1], vertex_quadrics[r0]);

        //if (attribute_count) { 
        //    quadricAdd(attribute_quadrics[r1], attribute_quadrics[r0]);
        //    quadricAdd(&attribute_gradients[r1 * attribute_count], &attribute_gradients[r0 * attribute_count],
        //               attribute_count);
        //}

        //p = &adjacency.starts[r0];
        //while ((*p) != Adjacency::Invalid) { 
        //    Adjacency::Link& node = adjacency.data[(*p)];
        //    unsigned int* next = &node.next_link;
        //    if (node.link_edge.prev == r1 || node.link_edge.next == r1) 
        //        (*p) = (*next);
        //    p = next;
        //}

        //p = &adjacency.starts[r1];
        //while ((*p) != Adjacency::Invalid) {
        //    Adjacency::Link& node = adjacency.data[(*p)];
        //    unsigned int* next = &node.next_link;
        //    if (node.link_edge.prev == r0 || node.link_edge.next == r0) 
        //        (*p) = (*next); 
        //    p = next;
        //}

        //(*p) = adjacency.starts[r0];

        //p = &adjacency.starts[r1];
        //std::cout << *p << std::endl;

        //while ((*p) != Adjacency::Invalid) {
        //    Adjacency::Link& node = adjacency.data[(*p)];
        //    size_t hash = EdgeHash(node.link_edge.next, r1);
        //    
        //    unsigned int v0 = node.link_edge.next;
        //    unsigned int v1 = r1;

        //    Quadric vq = quadricAdded(vertex_quadrics[v0], vertex_quadrics[v1]);

        //    Collapse c = {v0, v1};
        //    bool find = quadricFindMinimum(vq, c.v);
        //    c.error = quadricError(vq, c.v);

        //    if (attribute_count) { 
        //        Quadric aq = quadricAdded(attribute_quadrics[v0], attribute_quadrics[v1]);
        //        c.error += quadricError(aq, c.v);
        //    }

        //    heap.push(c);

        //    p = &node.next_link;
        //}

    }

    return triangle_collapses;
}

size_t meshsmp_simplifyTriMeshWithAttributes(unsigned int* indices, size_t index_count, float* vertex_positions_data,
    size_t vertex_count, const float* vertex_attributes_data,
    size_t attribute_count, const float* attribute_weights,
    size_t target_index_count, float target_error, float* out_result_error)
{
    Allocator allocator;

    clock_t start = clock();
    Mesh mesh;
    initMesh(mesh, indices, index_count, vertex_count);
    std::cout << clock() - start << std::endl;

    // 建立边表
    //EdgeAdjacency adjacency = {};
    //initEdgeAdjacency(adjacency, index_count, vertex_count, allocator);
    //updateEdgeAdjacency(adjacency, indices, index_count, vertex_count);

    // 建立邻接表
    Adjacency adjacency = {};
    initAdjacency(adjacency, index_count, vertex_count, allocator);
    updateAdjacency(adjacency, indices, index_count, vertex_count, allocator);

    // 模型归一化，统一所有模型的误差度量单位
    Vector3f* vertex_positions = allocator.allocate<Vector3f>(vertex_count);
    float vertex_scale = rescalePositions(vertex_positions, vertex_positions_data, vertex_count);

    // 属性初始化，根据属性权重计算映射值
    float* vertex_attributes = nullptr;
    if (attribute_count) {
        unsigned int attribute_remap[32]{};

        size_t attributes_used = 0;
        for (size_t i = 0; i < attribute_count; ++i)
            if (attribute_weights[i] > 0) attribute_remap[attributes_used++] = unsigned(i);

        attribute_count = attributes_used;
        vertex_attributes = allocator.allocate<float>(vertex_count * attribute_count);
        rescaleAttributes(vertex_attributes, vertex_attributes_data, vertex_count, attribute_weights, attribute_count,
                          attribute_remap);
    }

    // 初始化坐标Quadric，属性Quadric
    Quadric* vertex_quadrics = allocator.allocate<Quadric>(vertex_count);
    memset(vertex_quadrics, 0, vertex_count * sizeof(Quadric));

    Quadric* attribute_quadrics = nullptr;
    QuadricGrad* attribute_gradients = nullptr;
    if (attribute_count) {
        attribute_quadrics = allocator.allocate<Quadric>(vertex_count);
        memset(attribute_quadrics, 0, vertex_count * sizeof(Quadric));

        attribute_gradients = allocator.allocate<QuadricGrad>(vertex_count * attribute_count);
        memset(attribute_gradients, 0, vertex_count * attribute_count * sizeof(QuadricGrad));
    }

    initFaceQuadrics(vertex_quadrics, indices, index_count, vertex_positions);

    if (attribute_count)
        initAttributeQuadrics(attribute_quadrics, attribute_gradients, indices, index_count, vertex_positions,
                              vertex_attributes, attribute_count);

    Heap heap;
    HashMap hashmap;
    float result_error = 0;
    size_t triangle_collapse_goal = (index_count - target_index_count) / 3;
    
    float error_scale = 1.f;
    float error_limit = (target_error * target_error) / (error_scale * error_scale);

    initEdgeCollapses(heap, mesh, vertex_quadrics, attribute_quadrics, attribute_gradients);

    //unsigned int* collapse_remap = allocator.allocate<unsigned int>(vertex_count);
    //unsigned int* collapse_delayed = allocator.allocate<unsigned int>(vertex_count);
    //for (size_t i = 0; i < vertex_count; ++i) 
    //    collapse_remap[i] = unsigned(i);
    //memset(collapse_delayed, 0, vertex_count);

    size_t collapses = performEdgeCollapses(heap, hashmap, mesh, vertex_positions, vertex_quadrics, attribute_quadrics,
                                            attribute_gradients, attribute_count, triangle_collapse_goal, error_limit,
                                            result_error);

    if (out_result_error) *out_result_error = sqrtf(result_error) * error_scale;

    
    return 1;
}