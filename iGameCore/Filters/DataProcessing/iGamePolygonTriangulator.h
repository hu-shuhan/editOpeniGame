#ifndef iGamePolygonTriangulator_h
#define iGamePolygonTriangulator_h

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

namespace iGame::detail {
using PolygonPoint = std::array<double, 3>;
using PolygonId = int64_t;
using PolygonTriangle = std::array<PolygonId, 3>;
struct PolygonEdge {
    PolygonId a, b;
    PolygonEdge(PolygonId x, PolygonId y) : a(std::min(x, y)), b(std::max(x, y)) {}
    bool operator==(const PolygonEdge& e) const { return a == e.a && b == e.b; }
};
struct PolygonEdgeHash {
    size_t operator()(const PolygonEdge& e) const {
        const size_t a = std::hash<PolygonId>{}(e.a), b = std::hash<PolygonId>{}(e.b);
        return a ^ (b + size_t{0x9e3779b9} + (a << 6) + (a >> 2));
    }
};
using PolygonEdges = std::unordered_set<PolygonEdge, PolygonEdgeHash>;

inline PolygonPoint PolygonSubtract(const PolygonPoint& a, const PolygonPoint& b) {
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}
inline PolygonPoint PolygonCross(const PolygonPoint& a, const PolygonPoint& b) {
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
inline double PolygonDot(const PolygonPoint& a, const PolygonPoint& b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

// Preserve every boundary vertex/edge. A new diagonal must not already belong
// to another polygon (or an earlier triangulation), even for nearly collinear
// chains shared by adjacent faces. Output and occupied edges change only after
// a complete triangulation has been found.
inline bool TriangulatePolygon(const std::vector<PolygonId>& ids,
                               const std::vector<PolygonPoint>& points,
                               PolygonEdges& occupied,
                               std::vector<PolygonTriangle>& triangles) {
    const size_t n = ids.size();
    if (n < 3 || points.size() != n) return false;
    std::unordered_set<PolygonId> unique(ids.begin(), ids.end());
    if (unique.size() != n) return false;
    for (const auto& p : points) for (double x : p) if (!std::isfinite(x)) return false;
    PolygonPoint normal{};
    for (size_t j=1; j+1<n; ++j) {
        const auto c=PolygonCross(PolygonSubtract(points[j],points[0]),
                                  PolygonSubtract(points[j+1],points[0]));
        for (int d=0; d<3; ++d) normal[d]+=c[d];
    }
    const double norm=std::sqrt(PolygonDot(normal,normal));
    if (!(norm>0) || !std::isfinite(norm)) return false;
    for (double& x:normal) x/=norm;
    const auto boundary=[n](size_t a,size_t b){return b==a+1 || (a==0 && b==n-1);};
    const auto allowed=[&](size_t a,size_t b){
        return boundary(a,b) || occupied.find(PolygonEdge(ids[a],ids[b]))==occupied.end();
    };
    // Dynamic programming replaces the mutable ear queue: each subpolygon is
    // completed once, and rejected candidates can be reconsidered naturally.
    // Positive projected triangle areas enforce winding for simple polygons.
    std::vector<double> quality(n*n,-1.0);
    std::vector<int> split(n*n,-1);
    for(size_t a=0;a+1<n;++a) quality[a*n+a+1]=std::numeric_limits<double>::infinity();
    for(size_t length=2;length<n;++length) for(size_t a=0;a+length<n;++a) {
        const size_t b=a+length;
        if(!allowed(a,b)) continue;
        for(size_t k=a+1;k<b;++k) {
            if(quality[a*n+k]<0 || quality[k*n+b]<0 || !allowed(a,k) || !allowed(k,b)) continue;
            const auto u=PolygonSubtract(points[k],points[a]), v=PolygonSubtract(points[b],points[a]);
            const auto cross=PolygonCross(u,v);
            const double signedArea=PolygonDot(cross,normal);
            if(!(signedArea>0)) continue;
            const auto w=PolygonSubtract(points[b],points[k]);
            const double perimeter2=PolygonDot(u,u)+PolygonDot(v,v)+PolygonDot(w,w);
            const double q=signedArea/perimeter2;
            const double candidate=std::min({quality[a*n+k],quality[k*n+b],q});
            if(candidate>quality[a*n+b]) {quality[a*n+b]=candidate;split[a*n+b]=static_cast<int>(k);}
        }
    }
    if(split[n-1]<0) return false;
    std::vector<PolygonTriangle> result;
    std::vector<std::array<size_t,2>> stack{{0,n-1}};
    while(!stack.empty()) {
        const auto interval=stack.back();stack.pop_back();
        const size_t a=interval[0],b=interval[1];
        if(b==a+1) continue;
        const size_t k=static_cast<size_t>(split[a*n+b]);
        result.push_back({ids[a],ids[k],ids[b]});
        stack.push_back({k,b});stack.push_back({a,k});
    }
    if(result.size()!=n-2) return false;
    for(const auto& t:result) for(int j=0;j<3;++j) occupied.emplace(t[j],t[(j+1)%3]);
    triangles.insert(triangles.end(),result.begin(),result.end());
    return true;
}
} // namespace iGame::detail
#endif
