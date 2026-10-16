#ifndef iGameStarTetrahedralizer_h
#define iGameStarTetrahedralizer_h

#include "iGamePolygonTriangulator.h"
#include <map>
#include <queue>
#include <string>

namespace iGame::detail {
inline double TetraDeterminant(const PolygonPoint& a, const PolygonPoint& b,
                              const PolygonPoint& c, const PolygonPoint& d) {
    return PolygonDot(PolygonSubtract(b,a),
                      PolygonCross(PolygonSubtract(c,a),PolygonSubtract(d,a)));
}

struct StarCell {
    PolygonPoint center{};
    std::vector<PolygonTriangle> boundary; // consistently outward oriented
    double volume = 0;
};

// Validate a closed oriented triangular sphere and find a point in its kernel.
// Local coordinates and local scale keep tolerances independent of translation
// and model units. Success guarantees a complete radial subdivision; failure
// never returns an incomplete list of tetrahedra.
template<class GetPoint>
bool PrepareStarCell(std::vector<PolygonTriangle> triangles, GetPoint point,
                     StarCell& result, std::string& reason,
                     bool storeCenterAsFloat = true) {
    const auto fail = [&](const char* text) { reason=text; return false; };
    if (triangles.size()<4) return fail("边界三角形不足四个");
    std::map<PolygonId,PolygonPoint> vertices;
    std::map<PolygonEdge,std::vector<std::pair<size_t,int>>,
             bool(*)(const PolygonEdge&,const PolygonEdge&)> edges(
        [](const PolygonEdge& a,const PolygonEdge& b){return a.a<b.a || (a.a==b.a && a.b<b.b);});
    std::map<PolygonTriangle,int> unique;
    for(size_t t=0;t<triangles.size();++t) {
        auto key=triangles[t];std::sort(key.begin(),key.end());
        if(key[0]==key[1] || key[1]==key[2] || !unique.emplace(key,1).second)
            return fail("边界存在退化或重复三角形");
        for(int j=0;j<3;++j) {
            const auto a=triangles[t][j],b=triangles[t][(j+1)%3];
            vertices.emplace(a,point(a));
            edges[PolygonEdge(a,b)].push_back({t,a<b?1:-1});
        }
    }
    if(vertices.size()<4) return fail("单元不同顶点不足四个");
    if(static_cast<int64_t>(vertices.size())-static_cast<int64_t>(edges.size())+
       static_cast<int64_t>(triangles.size())!=2) return fail("边界不是单一球面拓扑");
    std::vector<std::vector<std::pair<size_t,int>>> adjacency(triangles.size());
    for(const auto& [edge,uses]:edges) {
        if(uses.size()!=2) return fail("单元边界存在开口或非流形边");
        const int relation=-uses[0].second*uses[1].second;
        adjacency[uses[0].first].push_back({uses[1].first,relation});
        adjacency[uses[1].first].push_back({uses[0].first,relation});
    }
    std::vector<int> orientation(triangles.size(),0);orientation[0]=1;
    std::queue<size_t> queue;queue.push(0);size_t visited=0;
    while(!queue.empty()) {
        const size_t t=queue.front();queue.pop();++visited;
        for(auto [next,relation]:adjacency[t]) {
            const int wanted=orientation[t]*relation;
            if(orientation[next]==0){orientation[next]=wanted;queue.push(next);}
            else if(orientation[next]!=wanted) return fail("单元边界不可一致定向");
        }
    }
    if(visited!=triangles.size()) return fail("单元边界不连通");
    PolygonPoint reference{},lo=vertices.begin()->second,hi=lo;
    for(const auto& [id,p]:vertices) for(int d=0;d<3;++d) {
        if(!std::isfinite(p[d])) return fail("单元包含非有限坐标");
        reference[d]+=p[d];lo[d]=std::min(lo[d],p[d]);hi[d]=std::max(hi[d],p[d]);
    }
    for(double& v:reference) v/=vertices.size();
    const double scale=std::sqrt(PolygonDot(PolygonSubtract(hi,lo),PolygonSubtract(hi,lo)));
    if(!(scale>0)) return fail("单元几何尺度为零");
    const double volumeTolerance=128*std::numeric_limits<double>::epsilon()*scale*scale*scale;
    double boundaryDet=0;
    for(size_t t=0;t<triangles.size();++t) {
        if(orientation[t]<0)std::swap(triangles[t][1],triangles[t][2]);
        const auto& ids=triangles[t];
        boundaryDet+=TetraDeterminant(reference,vertices[ids[0]],vertices[ids[1]],vertices[ids[2]]);
    }
    if(!std::isfinite(boundaryDet) || std::abs(boundaryDet)<=volumeTolerance)
        return fail("三角形边界的有向体积为零或退化");
    if(boundaryDet<0) {
        for(auto& t:triangles)std::swap(t[1],t[2]);
        boundaryDet=-boundaryDet;
    }
    struct Plane {PolygonPoint p,normal;};
    std::vector<Plane> planes;
    for(const auto& t:triangles) {
        const auto a=vertices[t[0]],b=vertices[t[1]],c=vertices[t[2]];
        auto normal=PolygonCross(PolygonSubtract(b,a),PolygonSubtract(c,a));
        const double length=std::sqrt(PolygonDot(normal,normal));
        if(!(length>0) || !std::isfinite(length))return fail("边界包含零面积三角形");
        for(double& v:normal)v/=length;
        planes.push_back({PolygonSubtract(a,reference),normal});
    }
    // Keep the vertex-average center if possible; otherwise project onto the
    // inward half-spaces to find a kernel point with a finite interior margin.
    PolygonPoint local{};
    auto inside=[&](const PolygonPoint& candidate) {
        PolygonPoint absolute{};
        for(int d=0;d<3;++d) {
            absolute[d]=reference[d]+candidate[d];
            if(storeCenterAsFloat)absolute[d]=static_cast<float>(absolute[d]);
        }
        for(const auto& t:triangles)
            if(!(TetraDeterminant(absolute,vertices[t[0]],vertices[t[1]],vertices[t[2]])>volumeTolerance))return false;
        return true;
    };
    if(!inside(local)) {
        bool found=false;
        for(int attempt=0;attempt<6 && !found;++attempt) {
            local={0,0,0};const double margin=scale*1e-3*std::pow(0.1,attempt);
            for(int iteration=0;iteration<512;++iteration) {
                double worst=0;
                for(const auto& plane:planes) {
                    const double violation=PolygonDot(plane.normal,PolygonSubtract(local,plane.p))+margin;
                    worst=std::max(worst,violation);
                    if(violation>0)for(int d=0;d<3;++d)local[d]-=violation*plane.normal[d];
                }
                if(worst<margin*1e-3 && inside(local)){found=true;break;}
            }
        }
        if(!found) return fail("未找到能连接全部边界三角形的内部中心（单元可能非星形）");
    }
    // A subdivided cell stores a float center. An existing tetrahedron is
    // retained without adding one, so only its double geometric center is used
    // for validation (a valid tiny tet may have no representable float center).
    PolygonPoint center{};
    for(int d=0;d<3;++d) {
        center[d]=reference[d]+local[d];
        if(storeCenterAsFloat)center[d]=static_cast<float>(center[d]);
    }
    double sumDet=0;
    std::map<PolygonId,double> angles;
    for(const auto& t:triangles) {
        const double det=TetraDeterminant(center,vertices[t[0]],vertices[t[1]],vertices[t[2]]);
        if(!std::isfinite(det) || det<=volumeTolerance)
            return fail("存储精度下中心位于边界外或生成退化四面体");
        sumDet+=det;
        // A radial map must wrap once at every boundary vertex. Together with
        // the closed oriented sphere and positive cone volumes, this prevents
        // overlapping radial sectors and multiple coverage around a vertex.
        for(int j=0;j<3;++j) {
            auto u=PolygonSubtract(vertices[t[j]],center);
            auto v=PolygonSubtract(vertices[t[(j+1)%3]],center);
            auto w=PolygonSubtract(vertices[t[(j+2)%3]],center);
            const double length=std::sqrt(PolygonDot(u,u));for(double& x:u)x/=length;
            const double uv=PolygonDot(u,v),uw=PolygonDot(u,w);
            for(int d=0;d<3;++d){v[d]-=uv*u[d];w[d]-=uw*u[d];}
            const auto cross=PolygonCross(v,w);
            angles[t[j]]+=std::atan2(std::sqrt(PolygonDot(cross,cross)),PolygonDot(v,w));
        }
    }
    for(const auto& [id,angle]:angles)
        if(!std::isfinite(angle) || std::abs(angle-2*std::acos(-1.0))>1e-5)
            return fail("中心投影的边界存在重叠、分支或覆盖不完整");
    if(std::abs(sumDet-boundaryDet)>1e-9*boundaryDet+volumeTolerance)
        return fail("四面体总体积与三角形边界体积不一致");
    result.center=center;result.boundary=std::move(triangles);result.volume=sumDet/6;
    return true;
}
} // namespace iGame::detail
#endif
