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

static size_t boundEdgeCollapses(const EdgeAdjacency& adjacency, size_t vertex_count, size_t index_count,
                                 unsigned char* vertex_kind) {
    size_t dual_count = 0;

    for (size_t i = 0; i < vertex_count; ++i) {
        unsigned char k = vertex_kind[i];
        unsigned int e = adjacency.offsets[i + 1] - adjacency.offsets[i];

        dual_count += e;
    }

    assert(dual_count <= index_count);

    // pad capacity by 3 so that we can check for overflow once per triangle instead of once per edge
    return (index_count - dual_count / 2) + 3;
}

struct Collapse {
    unsigned int v0;
    unsigned int v1;

    union {
        unsigned int bidi;
        float error;
        unsigned int errorui;
    };
};

static size_t pickEdgeCollapses(Collapse* collapses, size_t collapse_capacity, const unsigned int* indices,
                                size_t index_count, const unsigned int* remap, const unsigned char* vertex_kind,
                                const unsigned int* loop, const unsigned int* loopback) {
    size_t collapse_count = 0;

    for (size_t i = 0; i < index_count; i += 3) {
        static const int next[3] = {1, 2, 0};

        // this should never happen as boundEdgeCollapses should give an upper bound for the collapse count, but in an unlikely event it does we can just drop extra collapses
        if (collapse_count + 3 > collapse_capacity) break;

        for (int e = 0; e < 3; ++e) {
            unsigned int i0 = indices[i + e];
            unsigned int i1 = indices[i + next[e]];

            // this can happen either when input has a zero-length edge, or when we perform collapses for complex
            // topology w/seams and collapse a manifold vertex that connects to both wedges onto one of them
            // we leave edges like this alone since they may be important for preserving mesh integrity
            if (remap[i0] == remap[i1]) continue;

            unsigned char k0 = vertex_kind[i0];
            unsigned char k1 = vertex_kind[i1];

            // the edge has to be collapsible in at least one direction
            //if (!(kCanCollapse[k0][k1] | kCanCollapse[k1][k0])) continue;

            // manifold and seam edges should occur twice (i0->i1 and i1->i0) - skip redundant edges
            //if (kHasOpposite[k0][k1] && remap[i1] > remap[i0]) continue;

            // two vertices are on a border or a seam, but there's no direct edge between them
            // this indicates that they belong to two different edge loops and we should not collapse this edge
            // loop[] tracks half edges so we only need to check i0->i1
            //if (k0 == k1 && (k0 == Kind_Border || k0 == Kind_Seam) && loop[i0] != i1) continue;

            //if (k0 == Kind_Locked || k1 == Kind_Locked) {
                // the same check as above, but for border/seam -> locked collapses
                // loop[] and loopback[] track half edges so we only need to check one of them
                //if ((k0 == Kind_Border || k0 == Kind_Seam) && loop[i0] != i1) continue;
                //if ((k1 == Kind_Border || k1 == Kind_Seam) && loopback[i1] != i0) continue;
            //}

            // edge can be collapsed in either direction - we will pick the one with minimum error
            // note: we evaluate error later during collapse ranking, here we just tag the edge as bidirectional
            //if (kCanCollapse[k0][k1] & kCanCollapse[k1][k0]) {
                Collapse c = {i0, i1, {/* bidi= */ 1}};
                collapses[collapse_count++] = c;
            //} else {
                // edge can only be collapsed in one direction
            //    unsigned int e0 = kCanCollapse[k0][k1] ? i0 : i1;
            //    unsigned int e1 = kCanCollapse[k0][k1] ? i1 : i0;

            //    Collapse c = {e0, e1, {/* bidi= */ 0}};
            //    collapses[collapse_count++] = c;
            //}
        }
    }

    return collapse_count;
}

static float quadricEval(const Quadric& Q, const Vector3& v) {
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

static float quadricError(const Quadric& Q, const Vector3& v) {
    float r = quadricEval(Q, v);
    float s = Q.w == 0.f ? 0.f : 1.f / Q.w;

    return fabsf(r) * s;
}

static float quadricError(const Quadric& Q, const QuadricGrad* G, size_t attribute_count, const Vector3& v,
                          const float* va) {
    float r = quadricEval(Q, v);

    // see quadricFromAttributes for general derivation; here we need to add the parts of (eval(pos) - attr)^2 that depend on attr
    for (size_t k = 0; k < attribute_count; ++k) {
        float a = va[k];
        float g = v.x * G[k].gx + v.y * G[k].gy + v.z * G[k].gz + G[k].gw;

        r += a * (a * Q.w - 2 * g);
    }

    // note: unlike position error, we do not normalize by Q.w to retain edge scaling as described in quadricFromAttributes
    return fabsf(r);
}

static void rankEdgeCollapses(Collapse* collapses, size_t collapse_count, const Vector3* vertex_positions,
                              const float* vertex_attributes, const Quadric* vertex_quadrics,
                              const Quadric* attribute_quadrics, const QuadricGrad* attribute_gradients,
                              size_t attribute_count, const unsigned int* remap) {
    for (size_t i = 0; i < collapse_count; ++i) {
        Collapse& c = collapses[i];

        unsigned int i0 = c.v0;
        unsigned int i1 = c.v1;

        // most edges are bidirectional which means we need to evaluate errors for two collapses
        // to keep this code branchless we just use the same edge for unidirectional edges
        unsigned int j0 = c.bidi ? i1 : i0;
        unsigned int j1 = c.bidi ? i0 : i1;

        float ei = quadricError(vertex_quadrics[remap[i0]], vertex_positions[i1]);
        float ej = quadricError(vertex_quadrics[remap[j0]], vertex_positions[j1]);


        if (attribute_count) {
                // note: ideally we would evaluate max/avg of attribute errors for seam edges, but it's not clear if it's worth the extra cost
                ei += quadricError(attribute_quadrics[i0], &attribute_gradients[i0 * attribute_count], attribute_count,
                                   vertex_positions[i1], &vertex_attributes[i1 * attribute_count]);
                ej += quadricError(attribute_quadrics[j0], &attribute_gradients[j0 * attribute_count], attribute_count,
                                   vertex_positions[j1], &vertex_attributes[j1 * attribute_count]);
        }

        // pick edge direction with minimal error
        c.v0 = ei <= ej ? i0 : j0;
        c.v1 = ei <= ej ? i1 : j1;
        c.error = ei <= ej ? ei : ej;
    }
}

static void sortEdgeCollapses(unsigned int* sort_order, const Collapse* collapses, size_t collapse_count) {
    // we use counting sort to order collapses by error; since the exact sort order is not as critical,
    // only top 12 bits of exponent+mantissa (8 bits of exponent and 4 bits of mantissa) are used.
    // to avoid excessive stack usage, we clamp the exponent range as collapses with errors much higher than 1 are not useful.
    const unsigned int sort_bits = 12;
    const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

    // fill histogram for counting sort
    unsigned int histogram[sort_bins];
    memset(histogram, 0, sizeof(histogram));

    for (size_t i = 0; i < collapse_count; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error = collapses[i].errorui;
        unsigned int key = (error << 1) >> (32 - sort_bits);
        key = key < sort_bins ? key : sort_bins - 1;

        histogram[key]++;
    }

    // compute offsets based on histogram data
    size_t histogram_sum = 0;

    for (size_t i = 0; i < sort_bins; ++i) {
        size_t count = histogram[i];
        histogram[i] = unsigned(histogram_sum);
        histogram_sum += count;
    }

    assert(histogram_sum == collapse_count);

    // compute sort order based on offsets
    for (size_t i = 0; i < collapse_count; ++i) {
        // skip sign bit since error is non-negative
        unsigned int error = collapses[i].errorui;
        unsigned int key = (error << 1) >> (32 - sort_bits);
        key = key < sort_bins ? key : sort_bins - 1;

        sort_order[histogram[key]++] = unsigned(i);
    }
}

// does triangle ABC flip when C is replaced with D?
static bool hasTriangleFlip(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
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

static bool hasTriangleFlips(const EdgeAdjacency& adjacency, const Vector3* vertex_positions,
                             const unsigned int* collapse_remap, unsigned int i0, unsigned int i1) {
    assert(collapse_remap[i0] == i0);
    assert(collapse_remap[i1] == i1);

    const Vector3& v0 = vertex_positions[i0];
    const Vector3& v1 = vertex_positions[i1];


    const EdgeAdjacency::Edge* edges = &adjacency.data[adjacency.offsets[i0]];
    size_t count = adjacency.offsets[i0 + 1] - adjacency.offsets[i0];

    for (size_t i = 0; i < count; ++i) {
        unsigned int a = collapse_remap[edges[i].next];
        unsigned int b = collapse_remap[edges[i].prev];

        // skip triangles that will get collapsed by i0->i1 collapse or already got collapsed previously
        if (a == i1 || b == i1 || a == b) continue;

        // early-out when at least one triangle flips due to a collapse
        if (hasTriangleFlip(vertex_positions[a], vertex_positions[b], v0, v1)) {
#if TRACE >= 2
                printf("edge block %d -> %d: flip welded %d %d %d\n", i0, i1, a, i0, b);
#endif

                return true;
        }
    }

    return false;
}

static size_t performEdgeCollapses(unsigned int* collapse_remap, unsigned char* collapse_locked,
                                   const Collapse* collapses, size_t collapse_count, const unsigned int* collapse_order,
                                   const unsigned int* remap, const unsigned int* wedge,
                                   const unsigned char* vertex_kind, const unsigned int* loop,
                                   const unsigned int* loopback, const Vector3* vertex_positions,
                                   const EdgeAdjacency& adjacency, size_t triangle_collapse_goal, float error_limit,
                                   float& result_error) {
    size_t edge_collapses = 0;
    size_t triangle_collapses = 0;

    //大多数折叠移除2个三角形；用它来建立一个错误限制的边界
    // edge_collapse_goal是一个估计值；Triangle_collapse_goal将用于实际限制崩溃
    size_t edge_collapse_goal = triangle_collapse_goal / 2;

#if TRACE
    size_t stats[7] = {};
#endif

    for (size_t i = 0; i < collapse_count; ++i) {
        const Collapse& c = collapses[collapse_order[i]];


        if (c.error > error_limit) {
            break;
        }

        if (triangle_collapses >= triangle_collapse_goal) {
            break;
        }

        //我们根据最优最后一次崩溃的误差来限制每次传递的误差；因为许多坍塌将被锁定
        //由于它们将与其他成功的折叠共享顶点，我们需要将可接受误差增加一些因子
        float error_goal = edge_collapse_goal < collapse_count
                                   ? 1.5f * collapses[collapse_order[edge_collapse_goal]].error
                                   : FLT_MAX;

        // on average, each collapse is expected to lock 6 other collapses; to avoid degenerate passes on meshes with odd
        // topology, we only abort if we got over 1/6 collapses accordingly.
        if (c.error > error_goal && c.error > result_error && triangle_collapses > triangle_collapse_goal / 6) {
                break;
        }

        unsigned int i0 = c.v0;
        unsigned int i1 = c.v1;

        unsigned int r0 = remap[i0];
        unsigned int r1 = remap[i1];

        unsigned char kind = vertex_kind[i0];

        // we don't collapse vertices that had source or target vertex involved in a collapse
        // it's important to not move the vertices twice since it complicates the tracking/remapping logic
        // it's important to not move other vertices towards a moved vertex to preserve error since we don't re-rank collapses mid-pass
        if (collapse_locked[r0] | collapse_locked[r1]) {
                continue;
        }

        if (hasTriangleFlips(adjacency, vertex_positions, collapse_remap, r0, r1)) {
                // adjust collapse goal since this collapse is invalid and shouldn't factor into error goal
                edge_collapse_goal++;
                continue;
        }

#if TRACE >= 2
        printf("edge commit %d -> %d: kind %d->%d, error %f\n", i0, i1, vertex_kind[i0], vertex_kind[i1],
               sqrtf(c.error));
#endif

        assert(collapse_remap[r0] == r0);
        assert(collapse_remap[r1] == r1);

        //if (kind == Kind_Complex) {
        //        // remap all vertices in the complex to the target vertex
        //        unsigned int v = i0;

        //        do {
        //            collapse_remap[v] = i1;
        //            v = wedge[v];
        //        } while (v != i0);
        //} else if (kind == Kind_Seam) {
        //        // for seam collapses we need to move the seam pair together; this is a bit tricky to compute since we need to rely on edge loops as target vertex may be locked (and thus have more than two wedges)
        //        unsigned int s0 = wedge[i0];
        //        unsigned int s1 = loop[i0] == i1 ? loopback[s0] : loop[s0];
        //        assert(s0 != i0 && wedge[s0] == i0);
        //        assert(s1 != ~0u && remap[s1] == r1);

        //        // additional asserts to verify that the seam pair is consistent
        //        assert(kind != vertex_kind[i1] || s1 == wedge[i1]);
        //        assert(loop[i0] == i1 || loopback[i0] == i1);
        //        assert(loop[s0] == s1 || loopback[s0] == s1);

        //        // note: this should never happen due to the assertion above, but when disabled if we ever hit this case we'll get a memory safety issue; for now play it safe
        //        s1 = (s1 != ~0u) ? s1 : wedge[i1];

        //        collapse_remap[i0] = i1;
        //        collapse_remap[s0] = s1;
        //} else 
        {
                //assert(wedge[i0] == i0);

                collapse_remap[i0] = i1;
        }

        // note: we technically don't need to lock r1 if it's a locked vertex, as it can't move and its quadric won't be used
        // however, this results in slightly worse error on some meshes because the locked collapses get an unfair advantage wrt scheduling
        collapse_locked[r0] = 1;
        collapse_locked[r1] = 1;

        // border edges collapse 1 triangle, other edges collapse 2 or more
        triangle_collapses += 2;
        edge_collapses++;

        result_error = result_error < c.error ? c.error : result_error;
    }

    return edge_collapses;
}

static void updateQuadrics(const unsigned int* collapse_remap, size_t vertex_count, Quadric* vertex_quadrics,
                           Quadric* attribute_quadrics, QuadricGrad* attribute_gradients, size_t attribute_count,
                           const Vector3* vertex_positions, const unsigned int* remap, float& vertex_error) {
    for (size_t i = 0; i < vertex_count; ++i) {
        if (collapse_remap[i] == i) continue;

        unsigned int i0 = unsigned(i);
        unsigned int i1 = collapse_remap[i];

        unsigned int r0 = remap[i0];
        unsigned int r1 = remap[i1];

        // ensure we only update vertex_quadrics once: primary vertex must be moved if any wedge is moved
        if (i0 == r0) quadricAdd(vertex_quadrics[r1], vertex_quadrics[r0]);

        if (attribute_count) {
                quadricAdd(attribute_quadrics[i1], attribute_quadrics[i0]);
                quadricAdd(&attribute_gradients[i1 * attribute_count], &attribute_gradients[i0 * attribute_count],
                           attribute_count);

                if (i0 == r0) {
                    // when attributes are used, distance error needs to be recomputed as collapses don't track it; it is safe to do this after the quadric adjustment
                    float derr = quadricError(vertex_quadrics[r0], vertex_positions[r1]);
                    vertex_error = vertex_error < derr ? derr : vertex_error;
                }
        }
    }
}

static size_t remapIndexBuffer(unsigned int* indices, size_t index_count, const unsigned int* collapse_remap) {
    size_t write = 0;

    for (size_t i = 0; i < index_count; i += 3) {
        unsigned int v0 = collapse_remap[indices[i + 0]];
        unsigned int v1 = collapse_remap[indices[i + 1]];
        unsigned int v2 = collapse_remap[indices[i + 2]];

        // we never move the vertex twice during a single pass
        assert(collapse_remap[v0] == v0);
        assert(collapse_remap[v1] == v1);
        assert(collapse_remap[v2] == v2);

        if (v0 != v1 && v0 != v2 && v1 != v2) {
                indices[write + 0] = v0;
                indices[write + 1] = v1;
                indices[write + 2] = v2;
                write += 3;
        }
    }

    return write;
}

}