#include "iGameExtractEnclosedPointsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

struct Tri {
    Vector3d v0, v1, v2;
    size_t face;
    double bounds[6];
};

using EdgeKey = std::pair<igIndex, igIndex>;

inline EdgeKey MakeEdgeKey(igIndex a, igIndex b) { return a < b ? EdgeKey(a, b) : EdgeKey(b, a); }

// 不调用 SurfaceMesh::BuildEdges()，避免污染输入。
bool IsClosedSurface(const SurfaceMesh::Pointer& surface) {
    if (surface == nullptr) return false;
    const IGsize nFaces = surface->GetNumberOfFaces();
    if (nFaces == 0) return false;

    CellArray* faces = surface->GetFaces();
    if (faces == nullptr) return false;

    std::map<EdgeKey, int> edgeCount;
    for (IGsize f = 0; f < nFaces; ++f) {
        const igIndex* ids = nullptr;
        const int n = faces->GetCellIds(f, ids);
        if (n < 3) continue;
        for (int i = 0; i < n; ++i) { ++edgeCount[MakeEdgeKey(ids[i], ids[(i + 1) % n])]; }
    }
    for (const auto& kv: edgeCount) {
        if (kv.second != 2) return false;
    }
    return true;
}

void BuildTriangles(const SurfaceMesh::Pointer& surface, std::vector<Tri>& outTris) {
    const IGsize nFaces = surface->GetNumberOfFaces();
    CellArray* faces = surface->GetFaces();
    outTris.clear();
    outTris.reserve(nFaces);

    for (IGsize f = 0; f < nFaces; ++f) {
        const igIndex* ids = nullptr;
        const int n = faces->GetCellIds(f, ids);
        if (n < 3) continue;

        // vtkQuad chooses the shortest diagonal (point IDs break ties).
        int start = 0;
        if (n == 4) {
            const auto& p0 = surface->GetPoint(ids[0]);
            const auto& p1 = surface->GetPoint(ids[1]);
            const auto& p2 = surface->GetPoint(ids[2]);
            const auto& p3 = surface->GetPoint(ids[3]);
            auto distance2 = [](const Point& a, const Point& b) {
                double result = 0;
                for (int axis = 0; axis < 3; ++axis) {
                    const double d = double(a[axis]) - double(b[axis]); result += d * d;
                }
                return result;
            };
            const double d0 = distance2(p0, p2), d1 = distance2(p1, p3);
            const int maxIndex = int(std::max_element(ids, ids + n) - ids);
            if (d1 < d0 || (d0 == d1 && maxIndex % 2 == 1)) start = 1;
        }
        const Point& p0 = surface->GetPoint(ids[start]);
        double bounds[6];
        for (int a = 0; a < 3; ++a) {
            bounds[2*a] = bounds[2*a+1] = double(p0[a]);
            for (int j = 1; j < n; ++j) {
                const double value = surface->GetPoint(ids[(start+j)%n])[a];
                bounds[2*a] = std::min(bounds[2*a], value);
                bounds[2*a+1] = std::max(bounds[2*a+1], value);
            }
        }
        for (int i = 1; i + 1 < n; ++i) {
            const Point& p1 = surface->GetPoint(ids[(start + i) % n]);
            const Point& p2 = surface->GetPoint(ids[(start + i + 1) % n]);
            Tri tri;
            tri.face = static_cast<size_t>(f);
            std::copy(bounds, bounds + 6, tri.bounds);
            tri.v0 = Vector3d(p0[0], p0[1], p0[2]);
            tri.v1 = Vector3d(p1[0], p1[1], p1[2]);
            tri.v2 = Vector3d(p2[0], p2[1], p2[2]);
            outTris.push_back(tri);
        }
    }
}

double SegmentDistance2(const Vector3d& p, const Vector3d& a, const Vector3d& b) {
    const auto edge = b - a;
    const double length2 = edge.dot(edge);
    const double u = length2 == 0 ? 0 : std::clamp((p - a).dot(edge) / length2, 0.0, 1.0);
    const auto delta = p - (a + edge * u);
    return delta.dot(delta);
}

// vtkBox::IntersectBox only pads degenerate axes; its remaining tolerance is
// tested at the entry point. Expanding every face box changes boundary parity.
bool IntersectsCellBounds(const Tri& tri, const Vector3d& p, const Vector3d& d, double tolerance) {
    const double tol = tolerance > 0 ? tolerance : std::numeric_limits<float>::epsilon();
    double bounds[6], times[3], plane[3];
    bool middle[3], inside = true;
    for (int a = 0; a < 3; ++a) {
        const double padding = tri.bounds[2*a+1] > tri.bounds[2*a] ? 0 : tol;
        bounds[2*a] = tri.bounds[2*a] - padding;
        bounds[2*a+1] = tri.bounds[2*a+1] + padding;
        middle[a] = p[a] >= bounds[2*a] && p[a] <= bounds[2*a+1];
        if (!middle[a]) inside = false;
        plane[a] = p[a] < bounds[2*a] ? bounds[2*a] : bounds[2*a+1];
        times[a] = !middle[a] && d[a] != 0 ? (plane[a] - p[a]) / d[a] : -1;
    }
    if (inside) return true;
    int axis = 0;
    for (int a = 1; a < 3; ++a) if (times[a] > times[axis]) axis = a;
    const double t = times[axis];
    if (t < 0 || t > 1) return false;
    for (int a = 0; a < 3; ++a) if (a != axis) {
        const double value = p[a] + t * d[a];
        if (value < bounds[2*a] - tol || value > bounds[2*a+1] + tol) return false;
    }
    return true;
}

// Finite segment / plane intersection followed by the triangle's geometric
// tolerance test, matching vtkTriangle rather than rejecting t near zero.
bool RayTriangle(const Vector3d& orig, const Vector3d& dir, const Tri& tri, double tolerance, double& tOut) {
    // Preserve vtkTriangle::ComputeNormalDirection's subtraction order: for
    // boundary origins, algebraically equivalent cross products round differently.
    const auto n = (tri.v0 - tri.v2).cross(tri.v1 - tri.v2);
    auto normal = n;
    const double normalLength = normal.norm();
    if (normalLength == 0) return false;
    normal = normal / normalLength;
    const double numerator = normal.dot(tri.v1) - normal.dot(orig);
    const double denominator = normal.dot(dir);
    double t = 0;
    if (std::abs(denominator) <= std::abs(numerator) * 1e-6) return false;
    if (denominator != 0) t = numerator / denominator;
    else if (numerator != 0) return false;
    if (t < 0 || t > 1) return false;
    const auto x = orig + dir * t;
    const auto projected = x - n * ((x - tri.v1).dot(n) / n.dot(n));
    int axis = 0;
    for (int a = 1; a < 3; ++a) if (std::abs(n[a]) > std::abs(n[axis])) axis = a;
    int indices[2], nextIndex = 0;
    for (int i = 0; i < 3; ++i) if (i != axis) indices[nextIndex++] = i;
    const int a = indices[0], b = indices[1];
    const auto e1 = tri.v1 - tri.v0, e2 = tri.v2 - tri.v0, rhs = projected - tri.v0;
    const double det = e1[a] * e2[b] - e1[b] * e2[a];
    if (det == 0) return false;
    const double u = (rhs[a] * e2[b] - rhs[b] * e2[a]) / det;
    const double v = (e1[a] * rhs[b] - e1[b] * rhs[a]) / det;
    const bool inside = u >= 0 && v >= 0 && u + v <= 1;
    if (!inside && std::min({SegmentDistance2(x, tri.v0, tri.v1),
            SegmentDistance2(x, tri.v1, tri.v2), SegmentDistance2(x, tri.v2, tri.v0)}) > tolerance * tolerance)
        return false;
    tOut = t;
    return true;
}

// vtkRandomPool's default minimal-standard sequence and 10,000-value chunks.
// Each point starts at its own index; it does not share mutable RNG state.
std::vector<double> RandomPool(size_t pointCount) {
    const size_t size = std::max(pointCount, size_t{1500});
    std::vector<double> pool(size);
    for (size_t begin = 0; begin < size; begin += 10000) {
        long long seed = size < 10000 ? 31415 : static_cast<long long>(begin / 10000);
        if (seed < 1) seed += 2147483646;
        auto next = [&]() { seed = (seed * 16807) % 2147483647; };
        next(); next(); next();
        for (size_t i = begin; i < std::min(begin + 10000, size); ++i) {
            pool[i] = double(seed) / 2147483647; next();
        }
    }
    return pool;
}

// Built once per execution. Median splits keep the tree balanced even when
// triangle centroids coincide; leaves refer to the original triangle storage.
class TriangleBVH {
    struct Bounds {
        double lo[3], hi[3];
        Bounds() {
            for (int a = 0; a < 3; ++a) {
                lo[a] = std::numeric_limits<double>::infinity();
                hi[a] = -lo[a];
            }
        }
        void Add(const Vector3d& p) {
            for (int a = 0; a < 3; ++a) {
                lo[a] = std::min(lo[a], p[a]);
                hi[a] = std::max(hi[a], p[a]);
            }
        }
        bool Contains(const Vector3d& p) const {
            for (int a = 0; a < 3; ++a)
                if (p[a] < lo[a] || p[a] > hi[a]) return false;
            return true;
        }
        bool Intersects(const Vector3d& p, const Vector3d& d, double tolerance) const {
            double nearT = 0.0, farT = 1.0;
            for (int a = 0; a < 3; ++a) {
                // Round outward so grazing rays are not rejected by the BVH.
                const double low = std::nextafter(lo[a] - tolerance, -std::numeric_limits<double>::infinity());
                const double high = std::nextafter(hi[a] + tolerance, std::numeric_limits<double>::infinity());
                if (d[a] == 0.0) {
                    if (p[a] < low || p[a] > high) return false;
                    continue;
                }
                double t0 = (low - p[a]) / d[a];
                double t1 = (high - p[a]) / d[a];
                if (t0 > t1) std::swap(t0, t1);
                nearT = std::max(nearT, t0);
                farT = std::min(farT, t1);
                if (nearT > farT) return false;
            }
            return true;
        }
    };
    struct Node {
        Bounds bounds;
        size_t begin = 0, end = 0;
        int left = -1, right = -1;
    };
    const std::vector<Tri>& triangles;
    std::vector<size_t> order;
    std::vector<Node> nodes;

    int Build(size_t begin, size_t end) {
        Node node;
        node.begin = begin;
        node.end = end;
        Bounds centers;
        for (size_t i = begin; i < end; ++i) {
            const auto& t = triangles[order[i]];
            node.bounds.Add(t.v0); node.bounds.Add(t.v1); node.bounds.Add(t.v2);
            centers.Add((t.v0 + t.v1 + t.v2) / 3.0);
        }
        const int id = static_cast<int>(nodes.size());
        nodes.push_back(node);
        if (end - begin > 8) {
            int axis = 0;
            for (int a = 1; a < 3; ++a)
                if (centers.hi[a] - centers.lo[a] > centers.hi[axis] - centers.lo[axis]) axis = a;
            const size_t mid = begin + (end - begin) / 2;
            std::nth_element(order.begin() + begin, order.begin() + mid, order.begin() + end,
                [&](size_t a, size_t b) {
                    const auto& x = triangles[a]; const auto& y = triangles[b];
                    return (x.v0[axis] + x.v1[axis] + x.v2[axis]) <
                           (y.v0[axis] + y.v1[axis] + y.v2[axis]);
                });
            const int left = Build(begin, mid);
            const int right = Build(mid, end);
            nodes[id].left = left;
            nodes[id].right = right;
        }
        return id;
    }
    void CollectHits(int id, const Vector3d& p, const Vector3d& d, double tolerance,
                     std::vector<std::pair<size_t, double>>& hits) const {
        const auto& node = nodes[id];
        if (!node.bounds.Intersects(p, d, tolerance)) return;
        if (node.left >= 0) {
            CollectHits(node.left, p, d, tolerance, hits);
            CollectHits(node.right, p, d, tolerance, hits);
            return;
        }
        for (size_t i = node.begin; i < node.end; ++i) {
            double t;
            const auto& triangle = triangles[order[i]];
            if (IntersectsCellBounds(triangle, p, d, tolerance) && RayTriangle(p, d, triangle, tolerance, t))
                hits.emplace_back(triangle.face, t);
        }
    }
public:
    explicit TriangleBVH(const std::vector<Tri>& tris) : triangles(tris), order(tris.size()) {
        std::iota(order.begin(), order.end(), size_t{0});
        if (!tris.empty()) Build(0, tris.size());
    }
    bool Contains(const Vector3d& p) const { return !nodes.empty() && nodes[0].bounds.Contains(p); }
    Vector3d Center() const {
        const auto& b = nodes[0].bounds;
        return Vector3d((b.lo[0]+b.hi[0])/2, (b.lo[1]+b.hi[1])/2, (b.lo[2]+b.hi[2])/2);
    }
    double Length() const {
        const auto& b = nodes[0].bounds;
        return Vector3d(b.hi[0]-b.lo[0], b.hi[1]-b.lo[1], b.hi[2]-b.lo[2]).norm();
    }
    int CountHits(const Vector3d& p, const Vector3d& d, double tolerance, double mergeTolerance) const {
        std::vector<std::pair<size_t, double>> hits;
        CollectHits(0, p, d, tolerance, hits);
        // A surface cell reports only its nearest hit, even if both triangles
        // of a quad intersect. Then vtkIntersectionCounter merges by ray t.
        std::sort(hits.begin(), hits.end());
        std::vector<double> times;
        for (size_t i = 0; i < hits.size(); ++i)
            if (i == 0 || hits[i].first != hits[i-1].first) times.push_back(hits[i].second);
        std::sort(times.begin(), times.end());
        int count = 0;
        double previous = 0;
        for (double t : times) {
            if (count == 0 || t - previous > mergeTolerance) { ++count; previous = t; }
        }
        return count;
    }
};

bool IsPointInside(const Vector3d& p, const TriangleBVH& bvh, double tolerance,
                   const std::vector<double>& pool, size_t index) {
    if (!bvh.Contains(p)) return false;
    const double length = bvh.Length();
    const double rayLength = 2.0 * (length + (p - bvh.Center()).norm());
    int votes = 0;
    for (int iteration = 1; iteration < 10 && std::abs(votes) < 2; ++iteration) {
        Vector3d ray;
        do {
            for (int a = 0; a < 3; ++a) ray[a] = 1.0 - 2.0 * pool[(index++) % pool.size()];
        } while (ray.norm() == 0);
        // Round endpoint and subtract again, as vtkPlane does for the segment.
        const auto end = p + (ray / ray.norm()) * rayLength;
        const int hits = bvh.CountHits(p, end - p, tolerance * length, tolerance);
        votes += (hits % 2 ? 1 : -1);
    }
    return votes >= 0;
}

ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& inArray) {
    if (inArray == nullptr) return nullptr;
    switch (inArray->GetArrayType()) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        default:
            return nullptr;
    }
}

void CopyPointAttributes(AttributeSet* inSet, AttributeSet* outSet, const std::vector<igIndex>& newToOld) {
    if (inSet == nullptr || outSet == nullptr) return;

    auto inAttrs = inSet->GetAllPointAttributes();
    if (inAttrs == nullptr) return;

    const IGsize outNum = static_cast<IGsize>(newToOld.size());
    const size_t nAttrs = inAttrs->GetNumberOfElements();

    for (size_t i = 0; i < nAttrs; ++i) {
        auto& inAttr = inAttrs->GetElement(i);
        if (inAttr.IsNone() || inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inArray = inAttr.GetPointer();
        if (inArray == nullptr) continue;

        auto outArray = CreateArrayLike(inArray);
        if (outArray == nullptr) continue;

        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        outArray->Resize(outNum);

        const int dim = inArray->GetDimension();
        std::vector<double> tuple(static_cast<size_t>(dim), 0.0);
        const IGsize inNumElems = inArray->GetNumberOfElements();

        for (IGsize j = 0; j < outNum; ++j) {
            const igIndex src = newToOld[static_cast<size_t>(j)];
            if (src >= 0 && static_cast<IGsize>(src) < inNumElems) {
                inArray->GetElement(src, tuple.data());
            } else {
                std::fill(tuple.begin(), tuple.end(), 0.0);
            }
            outArray->SetElement(j, tuple.data());
        }

        outSet->AddAttribute(inAttr.GetType(), IG_POINT, outArray, inAttr.GetDataRange());
    }
}

} // namespace

bool ExtractEnclosedPointsFilter::Execute() {
    UpdateProgress(0.0);
    SetOutput(0, nullptr);
    SetOutput(1, nullptr);
    if (!std::isfinite(m_Tolerance) || m_Tolerance < 0) return false;

    auto pointInput = DynamicCast<PointSet>(GetInput(0));
    auto surfaceInput = DynamicCast<SurfaceMesh>(GetInput(1));

    if (pointInput.IsNull()) {
        std::cerr << "[ExtractEnclosedPoints] Input 0 (point set) is invalid" << std::endl;
        return false;
    }
    if (surfaceInput.IsNull()) {
        std::cerr << "[ExtractEnclosedPoints] Input 1 (surface mesh) is invalid" << std::endl;
        return false;
    }
    if (surfaceInput->GetNumberOfFaces() == 0) {
        std::cerr << "[ExtractEnclosedPoints] Input surface has no face" << std::endl;
        return false;
    }

    if (m_CheckSurface && !IsClosedSurface(surfaceInput)) {
        std::cerr << "[ExtractEnclosedPoints] Surface is not closed; execution failed"
                  << std::endl;
        return false;
    }
    UpdateProgress(0.1);

    std::vector<Tri> tris;
    BuildTriangles(surfaceInput, tris);
    if (tris.empty()) {
        return false;
    }
    UpdateProgress(0.2);
    const TriangleBVH bvh(tris);

    const IGsize nPts = pointInput->GetNumberOfPoints();
    auto outPoints = Points::New();
    outPoints->Reserve(nPts);

    std::vector<igIndex> newToOld;
    newToOld.reserve(static_cast<size_t>(nPts));

    const auto pool = RandomPool(static_cast<size_t>(nPts));
    auto outlierPoints = Points::New();
    std::vector<igIndex> outlierToOld;

    for (IGsize i = 0; i < nPts; ++i) {
        const Point& p = pointInput->GetPoint(i);
        const Vector3d pD(p[0], p[1], p[2]);

        const bool inside = IsPointInside(pD, bvh, m_Tolerance, pool, static_cast<size_t>(i));
        const bool keep = m_InsideOut ? !inside : inside;

        if (keep) {
            outPoints->AddPoint(p);
            newToOld.push_back(static_cast<igIndex>(i));
        } else if (m_GenerateOutliers) {
            outlierPoints->AddPoint(p);
            outlierToOld.push_back(static_cast<igIndex>(i));
        }

        if ((i & 0xFFF) == 0 && nPts > 0) {
            UpdateProgress(0.2 + 0.7 * static_cast<double>(i) / static_cast<double>(nPts));
        }
    }
    UpdateProgress(0.9);

    auto output = PointSet::New();
    output->SetPoints(outPoints);
    output->SetName(pointInput->GetName() + "_enclosed");

    CopyPointAttributes(pointInput->GetAttributeSet(), output->GetAttributeSet(), newToOld);
    if (m_GenerateOutliers) {
        auto outliers = PointSet::New();
        outliers->SetPoints(outlierPoints);
        outliers->SetName(pointInput->GetName() + "_outliers");
        CopyPointAttributes(pointInput->GetAttributeSet(), outliers->GetAttributeSet(), outlierToOld);
        SetOutput(1, outliers);
    }

    UpdateProgress(1.0);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
