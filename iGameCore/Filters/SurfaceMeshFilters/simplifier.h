#pragma once
#include <memory>
#include <cassert>

namespace tri{

size_t simplifyWithAttributes(unsigned int* destination, const unsigned int* indices, size_t index_count,
                              const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride,
                              const float* vertex_attributes, size_t vertex_attributes_stride,
                              const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock,
                              size_t target_index_count, float target_error, unsigned int options, float* result_error);

class Allocator {
public:
    template<typename T>
    struct StorageT {
        static void*(* allocate)(size_t);
        static void(* deallocate)(void*);
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
void*(* Allocator::StorageT<T>::allocate)(size_t) = operator new;
template<typename T>
void(* Allocator::StorageT<T>::deallocate)(void*) = operator delete;


struct EdgeAdjacency {
    struct Edge {
        unsigned int next;
        unsigned int prev;
    };

    unsigned int* offsets;
    Edge* data;
};

static void prepareEdgeAdjacency(EdgeAdjacency& adjacency, size_t index_count, size_t vertex_count,
                                 Allocator& allocator) {
    adjacency.offsets = allocator.allocate<unsigned int>(vertex_count + 1);
    adjacency.data = allocator.allocate<EdgeAdjacency::Edge>(index_count);
}

static void updateEdgeAdjacency(EdgeAdjacency& adjacency, const unsigned int* indices, size_t index_count,
                                size_t vertex_count, const unsigned int* remap) {
    size_t face_count = index_count / 3;
    unsigned int* offsets = adjacency.offsets + 1;
    EdgeAdjacency::Edge* data = adjacency.data;

    // fill edge counts
    memset(offsets, 0, vertex_count * sizeof(unsigned int));

    for (size_t i = 0; i < index_count; ++i) {
        unsigned int v = remap ? remap[indices[i]] : indices[i];
        assert(v < vertex_count);

        offsets[v]++;
    }

    // fill offset table
    unsigned int offset = 0;

    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int count = offsets[i];
        offsets[i] = offset;
        offset += count;
    }

    assert(offset == index_count);

    // fill edge data
    for (size_t i = 0; i < face_count; ++i) {
        unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

        if (remap) {
            a = remap[a];
            b = remap[b];
            c = remap[c];
        }

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

    // finalize offsets
    adjacency.offsets[0] = 0;
    assert(adjacency.offsets[vertex_count] == index_count);
}

struct PositionHasher {
    const float* vertex_positions;
    size_t vertex_stride_float;
    const unsigned int* sparse_remap;

    size_t hash(unsigned int index) const {
        unsigned int ri = sparse_remap ? sparse_remap[index] : index;
        const unsigned int* key = reinterpret_cast<const unsigned int*>(vertex_positions + ri * vertex_stride_float);

        // scramble bits to make sure that integer coordinates have entropy in lower bits
        unsigned int x = key[0] ^ (key[0] >> 17);
        unsigned int y = key[1] ^ (key[1] >> 17);
        unsigned int z = key[2] ^ (key[2] >> 17);

        // Optimized Spatial Hashing for Collision Detection of Deformable Objects
        return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
    }

    bool equal(unsigned int lhs, unsigned int rhs) const {
        unsigned int li = sparse_remap ? sparse_remap[lhs] : lhs;
        unsigned int ri = sparse_remap ? sparse_remap[rhs] : rhs;

        return memcmp(vertex_positions + li * vertex_stride_float, vertex_positions + ri * vertex_stride_float,
                      sizeof(float) * 3) == 0;
    }
};

static size_t hashBuckets2(size_t count) {
    size_t buckets = 1;
    while (buckets < count + count / 4) buckets *= 2;

    return buckets;
}

template<typename T, typename Hash>
static T* hashLookup2(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty) {
    assert(buckets > 0);
    assert((buckets & (buckets - 1)) == 0);

    size_t hashmod = buckets - 1;
    size_t bucket = hash.hash(key) & hashmod;

    for (size_t probe = 0; probe <= hashmod; ++probe) {
        T& item = table[bucket];

        if (item == empty) return &item;

        if (hash.equal(item, key)) return &item;

        // hash collision, quadratic probing
        bucket = (bucket + probe + 1) & hashmod;
    }

    assert(false && "Hash table is full"); // unreachable
    return NULL;
}

static void buildPositionRemap(unsigned int* remap, unsigned int* wedge, const float* vertex_positions_data,
                               size_t vertex_count, size_t vertex_positions_stride, const unsigned int* sparse_remap,
                               Allocator& allocator) {
    PositionHasher hasher = {vertex_positions_data, vertex_positions_stride / sizeof(float), sparse_remap};

    size_t table_size = hashBuckets2(vertex_count);
    unsigned int* table = allocator.allocate<unsigned int>(table_size);
    memset(table, -1, table_size * sizeof(unsigned int));

    // build forward remap: for each vertex, which other (canonical) vertex does it map to?
    // we use position equivalence for this, and remap vertices to other existing vertices
    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int index = unsigned(i);
        unsigned int* entry = hashLookup2(table, table_size, hasher, index, ~0u);

        if (*entry == ~0u) *entry = index;

        remap[index] = *entry;
    }

    // build wedge table: for each vertex, which other vertex is the next wedge that also maps to the same vertex?
    // entries in table form a (cyclic) wedge loop per vertex; for manifold vertices, wedge[i] == remap[i] == i
    for (size_t i = 0; i < vertex_count; ++i) wedge[i] = unsigned(i);

    for (size_t i = 0; i < vertex_count; ++i)
        if (remap[i] != i) {
            unsigned int r = remap[i];

            wedge[i] = wedge[r];
            wedge[r] = unsigned(i);
        }

    allocator.deallocate(table);
}


struct Vector3 {
    float x, y, z;
};

static float rescalePositions(Vector3* result, const float* vertex_positions_data, size_t vertex_count,
                              size_t vertex_positions_stride, const unsigned int* sparse_remap = NULL) {
    size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

    float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);
        const float* v = vertex_positions_data + ri * vertex_stride_float;

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
                              size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count,
                              const unsigned int* attribute_remap, const unsigned int* sparse_remap) {
    size_t vertex_attributes_stride_float = vertex_attributes_stride / sizeof(float);

    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);

        for (size_t k = 0; k < attribute_count; ++k) {
            unsigned int rk = attribute_remap[k];
            float a = vertex_attributes_data[ri * vertex_attributes_stride_float + rk];

            result[i * attribute_count + k] = a * attribute_weights[rk];
        }
    }
}

struct Quadric {
    // a00*x^2 + a11*y^2 + a22*z^2 + 2*(a10*xy + a20*xz + a21*yz) + b0*x + b1*y + b2*z + c
    float a00, a11, a22;
    float a10, a20, a21;
    float b0, b1, b2, c;
    float w;
};

struct QuadricGrad {
    // gx*x + gy*y + gz*z + gw
    float gx, gy, gz, gw;
};

static float normalize(Vector3& v) {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (length > 0) {
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    return length;
}

static void quadricFromPlane(Quadric& Q, float a, float b, float c, float d, float w) {
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

static void quadricAdd(QuadricGrad* G, const QuadricGrad* R, size_t attribute_count) {
    for (size_t k = 0; k < attribute_count; ++k) {
        G[k].gx += R[k].gx;
        G[k].gy += R[k].gy;
        G[k].gz += R[k].gz;
        G[k].gw += R[k].gw;
    }
}

static void quadricFromTriangle(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2, float weight) {
    Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    // normal = cross(p1 - p0, p2 - p0)
    Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
    float area = normalize(normal);

    float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

    // we use sqrtf(area) so that the error is scaled linearly; this tends to improve silhouettes
    quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance, sqrtf(area) * weight);
}

static void fillFaceQuadrics(Quadric* vertex_quadrics, const unsigned int* indices, size_t index_count,
                             const Vector3* vertex_positions, const unsigned int* remap) {
    for (size_t i = 0; i < index_count; i += 3) {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        Quadric Q;
        quadricFromTriangle(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], 1.f);

        quadricAdd(vertex_quadrics[remap[i0]], Q);
        quadricAdd(vertex_quadrics[remap[i1]], Q);
        quadricAdd(vertex_quadrics[remap[i2]], Q);
    }
}

static void quadricFromAttributes(Quadric& Q, QuadricGrad* G, const Vector3& p0, const Vector3& p1, const Vector3& p2,
                                  const float* va0, const float* va1, const float* va2, size_t attribute_count) {
    // for each attribute we want to encode the following function into the quadric:
    // (eval(pos) - attr)^2
    // where eval(pos) interpolates attribute across the triangle like so:
    // eval(pos) = pos.x * gx + pos.y * gy + pos.z * gz + gw
    // where gx/gy/gz/gw are gradients
    Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    // normal = cross(p1 - p0, p2 - p0)
    Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
    float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) * 0.5f;

    // quadric is weighted with the square of edge length (= area)
    // this equalizes the units with the positional error (which, after normalization, is a square of distance)
    // as a result, a change in weighted attribute of 1 along distance d is approximately equivalent to a change in position of d
    float w = area;

    // we compute gradients using barycentric coordinates; barycentric coordinates can be computed as follows:
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

static void fillAttributeQuadrics(Quadric* attribute_quadrics, QuadricGrad* attribute_gradients,
                                  const unsigned int* indices, size_t index_count, const Vector3* vertex_positions,
                                  const float* vertex_attributes, size_t attribute_count) {
    for (size_t i = 0; i < index_count; i += 3) {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        Quadric QA;
        QuadricGrad G[32];
        quadricFromAttributes(QA, G, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2],
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

}