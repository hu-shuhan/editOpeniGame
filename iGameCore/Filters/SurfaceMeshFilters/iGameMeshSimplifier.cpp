#include "iGameMeshSimplifier.h"

IGAME_NAMESPACE_BEGIN
typedef unsigned int int_t;

template<class TValueType>
class TVector3 {
public:
    TValueType x, y, z;
};

class Attribute {
public:
    const void* Primitive;
    int_t Offset;
    int_t Stride;
};

template<class TValueType>
class TQuadric {
public:
    TValueType a00, a11, a22;
    TValueType a10, a20, a21;
    TValueType b0, b1, b2, c;
    TValueType w;

    void operator=(const TQuadric<TValueType>& q) {
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

    void operator+=(const TQuadric<TValueType>& q) {
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

    TQuadric<TValueType> operator+(const TQuadric<TValueType>& q) {
        TQuadric<TValueType> Q;
        memset(&Q, 0, sizeof(TQuadric<TValueType>));
        Q += q;
        return Q;
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

    //float Eval(const Vector3f& v) {
    //    float rx = Q.b0;
    //    float ry = Q.b1;
    //    float rz = Q.b2;

    //    rx += Q.a10 * v.y;
    //    ry += Q.a21 * v.z;
    //    rz += Q.a20 * v.x;

    //    rx *= 2;
    //    ry *= 2;
    //    rz *= 2;

    //    rx += Q.a00 * v.x;
    //    ry += Q.a11 * v.y;
    //    rz += Q.a22 * v.z;

    //    float r = Q.c;
    //    r += rx * v.x;
    //    r += ry * v.y;
    //    r += rz * v.z;

    //    return r;
    //}
};

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

typedef TVector3<float> Vector3;
typedef TVector3<float> Point3;
typedef TQuadric<float> Quadric;
typedef TGradient<float> Gradient;

class MeshSimplifier::TriMeshInternalSimplifier {
public:
    TriMeshInternalSimplifier(std::vector<int_t>& Result, const std::vector<int_t>& Indices,
                              const std::vector<Point3>& VertexPositions,
                              const std::vector<Attribute>& VertexAttributes,
                              const std::vector<float>& AttributeWeights, size_t TargetCount,
                              float TargetError);

    bool operator()() { return DoWork(); }

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
    bool DoWork();

    void BuildVertexAdjacency();

    //-------------- Input's Data--------------//
    std::vector<int_t>& Result;
    const std::vector<int_t>& Indices;
    const std::vector<Point3>& VertexPositions;
    const std::vector<Attribute>& VertexAttributes;
    const std::vector<float>& AttributeWeights;
    size_t TargetCount;
    float TargetError;


    size_t IndexCount;
    size_t FaceCount;
    size_t VertexCount;
    size_t AttributeCount;
    MVertexAdjacency VertexAdjacency;
};

MeshSimplifier::TriMeshInternalSimplifier::TriMeshInternalSimplifier(
    std::vector<int_t>& Result, const std::vector<int_t>& Indices,
    const std::vector<Point3>& VertexPositions,
    const std::vector<Attribute>& VertexAttributes,
    const std::vector<float>& AttributeWeights,
    size_t TargetCount, float TargetError)
    : Result(Result), Indices(Indices), VertexPositions(VertexPositions), VertexAttributes(VertexAttributes),
    AttributeWeights(AttributeWeights), TargetCount(TargetCount),
    TargetError(TargetError)
{
    IndexCount = Indices.size();
    FaceCount = Indices.size() / 3;
    VertexCount = VertexPositions.size();
    AttributeCount = VertexAttributes.size();

}

bool MeshSimplifier::TriMeshInternalSimplifier::DoWork() 
{ 
    BuildVertexAdjacency();
    return false;
}

void MeshSimplifier::TriMeshInternalSimplifier::BuildVertexAdjacency() 
{ 
    VertexAdjacency.Offsets.resize(VertexCount + 1, 0);
    VertexAdjacency.Data.resize(IndexCount);

    for (int_t i = 0; i < IndexCount; ++i) 
    { 
        int_t idx = Indices[i] + 1;
        VertexAdjacency.Offsets[idx]++;
    }

    int_t Offset = 0;
    for (int_t i = 0; i < VertexCount; ++i) 
    {
        int_t idx = i + 1;
        int_t Count = VertexAdjacency.Offsets[idx];
        VertexAdjacency.Offsets[idx] = Offset;
        Offset += Count;
    }

    for (int_t i = 0; i < FaceCount; ++i) {
        int_t v0 = Indices[i * 3 + 0], v1 = Indices[i * 3 + 1], v2 = Indices[i * 3 + 2];

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

bool MeshSimplifier::Execute() { return false; }

MeshSimplifier::MeshSimplifier() { SetNumberOfInputs(1); }

IGAME_NAMESPACE_END


