#include "iGameAIRemeshing.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace iGameAI {

struct EdgeKey { int a,b; }; static inline EdgeKey mk(int i,int j){ if(i>j) std::swap(i,j); return {i,j}; }
struct EdgeKeyHash { size_t operator()(const EdgeKey& k) const { return (size_t)k.a*1315423911u + (size_t)k.b; } };
static inline bool operator==(const EdgeKey& x, const EdgeKey& y){ return x.a==y.a && x.b==y.b; }

struct Mesh {
	std::vector<float>& P; // xyz
	std::vector<int>& F;   // tri
	std::vector<std::vector<int>> v2f, v2v; // adjacency
	std::unordered_map<EdgeKey,int,EdgeKeyHash> edges; // edge->any face id
	std::vector<char> boundaryV, boundaryE; // marks

	int nv() const { return (int)P.size()/3; }
	int nf() const { return (int)F.size()/3; }
};

static inline float len3(const float* a,const float* b){float dx=a[0]-b[0],dy=a[1]-b[1],dz=a[2]-b[2];return std::sqrt(dx*dx+dy*dy+dz*dz);} 
static inline void add3(float* a,const float* b){a[0]+=b[0];a[1]+=b[1];a[2]+=b[2];}
static inline void sub3(const float* a,const float* b,float* o){o[0]=a[0]-b[0];o[1]=a[1]-b[1];o[2]=a[2]-b[2];}
static inline void cross3(const float* a,const float* b,float* o){o[0]=a[1]*b[2]-a[2]*b[1];o[1]=a[2]*b[0]-a[0]*b[2];o[2]=a[0]*b[1]-a[1]*b[0];}
static inline float dot3(const float* a,const float* b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static inline float norm3(const float* a){return std::sqrt(dot3(a,a));}

static void buildAdj(Mesh& M){
	int n=M.nv(), m=M.nf();
	M.v2f.assign(n,{}); M.v2v.assign(n,{});
	M.edges.clear(); M.boundaryV.assign(n,0);
	std::unordered_map<EdgeKey,int,EdgeKeyHash> cnt;
	for(int f=0; f<m; ++f){int i=M.F[3*f], j=M.F[3*f+1], k=M.F[3*f+2];
		M.v2f[i].push_back(f); M.v2f[j].push_back(f); M.v2f[k].push_back(f);
		M.v2v[i].push_back(j); M.v2v[i].push_back(k);
		M.v2v[j].push_back(i); M.v2v[j].push_back(k);
		M.v2v[k].push_back(i); M.v2v[k].push_back(j);
		cnt[mk(i,j)]++; cnt[mk(j,k)]++; cnt[mk(k,i)]++;
		M.edges[mk(i,j)]=f; M.edges[mk(j,k)]=f; M.edges[mk(k,i)]=f;
	}
	for(auto& kv:cnt){ if(kv.second==1){ M.boundaryV[kv.first.a]=1; M.boundaryV[kv.first.b]=1; } }
}

static float avgEdgeLen(const Mesh& M){
	float sum=0; int c=0; float p[3],q[3];
	for(auto& e:M.edges){int a=e.first.a,b=e.first.b; const float* pa=&M.P[3*a]; const float* pb=&M.P[3*b]; sum+=len3(pa,pb); c++;}
	return c?sum/c:0.f;
}

static void vertexNormals(const Mesh& M,std::vector<float>& N){
	N.assign(M.nv()*3,0.f);
	for(int f=0; f<M.nf(); ++f){int i=M.F[3*f],j=M.F[3*f+1],k=M.F[3*f+2];
		float e1[3],e2[3],n[3]; sub3(&M.P[3*j],&M.P[3*i],e1); sub3(&M.P[3*k],&M.P[3*i],e2); cross3(e1,e2,n);
		add3(&N[3*i],n); add3(&N[3*j],n); add3(&N[3*k],n);
	}
	for(int v=0; v<M.nv(); ++v){ float* n=&N[3*v]; float l=norm3(n); if(l>1e-12f){n[0]/=l;n[1]/=l;n[2]/=l;} }
}

static void estimateCurvature(const Mesh& M,std::vector<float>& k){
	// 近似 |H| 用一环法线变化：k ~ 平均二面角/平均边长
	std::vector<float> N; vertexNormals(M,N);
	k.assign(M.nv(),0.f);
	for(int v=0; v<M.nv(); ++v){ const float* nv=&N[3*v]; float acc=0; int c=0; for(int u: M.v2v[v]){ const float* nu=&N[3*u]; float a=std::acos(std::max(-1.f,std::min(1.f,dot3(nv,nu)))); acc+=a; c++; } k[v]= c? acc/std::max(1e-6f,(float)M.v2v[v].size()):0.f; }
}

static void smoothScalarField(const Mesh& M,std::vector<float>& s,int iters){
	std::vector<float> tmp(s.size());
	for(int t=0;t<iters;++t){
		for(int v=0; v<M.nv(); ++v){ if(M.boundaryV[v]){ tmp[v]=s[v]; continue; } float sum=0; int c=0; for(int u: M.v2v[v]){ sum+=s[u]; c++; } tmp[v]= c?0.5f*s[v]+0.5f*(sum/c):s[v]; }
		s.swap(tmp);
	}
}

static void targetEdgeLength(const Mesh& M,const std::vector<float>& ks,float h0,float alpha,float hmin,float hmax,std::vector<float>& hv){
	hv.resize(M.nv());
	for(int v=0; v<M.nv(); ++v){ float h = h0/(1.f+alpha*ks[v]); h = std::max(hmin,std::min(hmax,h)); hv[v]=h; }
}

static bool collapseOK(const Mesh& M,int a,int b){ if(M.boundaryV[a]&&M.boundaryV[b]) return false; return true; }

static void splitLongEdges(Mesh& M,const std::vector<float>& hv){
	std::vector<std::pair<EdgeKey,int>> toSplit; toSplit.reserve(M.edges.size());
	for(auto& e:M.edges){int a=e.first.a,b=e.first.b; float L=len3(&M.P[3*a],&M.P[3*b]); float h=std::min(hv[a],hv[b]); if(L>(5.f/3.f)*h) toSplit.push_back({e.first,e.second});}
	if(toSplit.empty()) return;
	for(auto& it: toSplit){ int a=it.first.a,b=it.first.b; float mid[3]={(M.P[3*a]+M.P[3*b])*0.5f,(M.P[3*a+1]+M.P[3*b+1])*0.5f,(M.P[3*a+2]+M.P[3*b+2])*0.5f}; int nv=M.nv(); M.P.push_back(mid[0]); M.P.push_back(mid[1]); M.P.push_back(mid[2]);
		// 简易替换：遍历三角形，凡含边(a,b)则将其拆成两面(a,nv,other)与(nv,b,other)
		for(int f=0; f<M.nf(); ++f){int i=M.F[3*f],j=M.F[3*f+1],k=M.F[3*f+2]; if((i==a&&j==b)||(i==b&&j==a)||(j==a&&k==b)||(j==b&&k==a)||(k==a&&i==b)||(k==b&&i==a)){
			int other = (i!=a&&i!=b)?i:((j!=a&&j!=b)?j:k);
			M.F[3*f]=a; M.F[3*f+1]=nv; M.F[3*f+2]=other; M.F.push_back(nv); M.F.push_back(b); M.F.push_back(other);
		}
		}
	}
	buildAdj(M);
}

static void collapseShortEdges(Mesh& M,const std::vector<float>& hv){
	std::vector<EdgeKey> toCol; for(auto& e:M.edges){int a=e.first.a,b=e.first.b; float L=len3(&M.P[3*a],&M.P[3*b]); float h=std::min(hv[a],hv[b]); if(L<(4.f/5.f)*h && collapseOK(M,a,b)) toCol.push_back(e.first);} if(toCol.empty()) return;
	for(auto& ek: toCol){int a=ek.a,b=ek.b; float mid[3]={(M.P[3*a]+M.P[3*b])*0.5f,(M.P[3*a+1]+M.P[3*b+1])*0.5f,(M.P[3*a+2]+M.P[3*b+2])*0.5f}; M.P[3*a]=mid[0]; M.P[3*a+1]=mid[1]; M.P[3*a+2]=mid[2];
		for(int f=0; f<M.nf(); ++f){ for(int t=0;t<3;++t){ if(M.F[3*f+t]==b) M.F[3*f+t]=a; } }
	}
	// 删除退化三角形
	std::vector<int> newF; newF.reserve(M.F.size()); for(int f=0; f<M.nf(); ++f){int i=M.F[3*f],j=M.F[3*f+1],k=M.F[3*f+2]; if(i!=j && j!=k && k!=i) { newF.push_back(i); newF.push_back(j); newF.push_back(k);} }
	M.F.swap(newF); buildAdj(M);
}

static void flipImprove(Mesh& M){ /* 可根据品质目标实现；此处留空以保持最小可用骨架 */ }

static void tangentialRelax(Mesh& M,float step){
	std::vector<float> N; vertexNormals(M,N);
	for(int v=0; v<M.nv(); ++v){ if(M.boundaryV[v]) continue; float c[3]={0,0,0}; int cnum=0; for(int u: M.v2v[v]){ c[0]+=M.P[3*u]; c[1]+=M.P[3*u+1]; c[2]+=M.P[3*u+2]; cnum++; } if(cnum==0) continue; c[0]/=cnum; c[1]/=cnum; c[2]/=cnum; float mv[3]={c[0]-M.P[3*v], c[1]-M.P[3*v+1], c[2]-M.P[3*v+2]}; float* n=&N[3*v]; float dn = mv[0]*n[0]+mv[1]*n[1]+mv[2]*n[2]; mv[0]-=dn*n[0]; mv[1]-=dn*n[1]; mv[2]-=dn*n[2]; M.P[3*v]+=step*mv[0]; M.P[3*v+1]+=step*mv[1]; M.P[3*v+2]+=step*mv[2]; }
}

// --- Enhanced variants (placeholders calling base implementations) ---
static void computeCotanMeanCurvature(Mesh& M, std::vector<float>& k) {
	// TODO: replace with cotan-based mean curvature magnitude; currently reuse estimateCurvature
	estimateCurvature(M, k);
}

static void cotanRelax(Mesh& M, float step) {
	// TODO: replace with cotan Laplacian tangential smoothing
	tangentialRelax(M, step);
}

static void flipImproveQuality(Mesh& M) {
	// TODO: implement edge flipping based on angle/valence quality
	flipImprove(M);
}

void AdaptivelyIsotropicRemesh(std::vector<float>& points, std::vector<int>& faces, const AIRemeshingOptions& opt){
	Mesh M{points,faces}; buildAdj(M);
	float h0 = avgEdgeLen(M); if(opt.h0>0) h0 = opt.h0; float hmin=h0*opt.hminRatio, hmax=h0*opt.hmaxRatio;
	std::vector<float> k; 
	computeCotanMeanCurvature(M,k); 
	smoothScalarField(M,k,opt.curvatureSmoothIters);
	for(int it=0; it<opt.iterations; ++it){ 
		std::vector<float> hv; 
		targetEdgeLength(M,k,h0,opt.alpha,hmin,hmax,hv); 
		splitLongEdges(M,hv); 
		collapseShortEdges(M,hv); 
		flipImproveQuality(M);
		cotanRelax(M,opt.relaxStep);
		buildAdj(M); 
	}
	// 输出已就地更新
}

}


