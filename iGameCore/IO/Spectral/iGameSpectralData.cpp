#include "iGameSpectralData.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace iGame::Spectral {
namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct Binary {
    const unsigned char* p;
    size_t remaining;
    bool swap = false;
    template<class T> T Get() {
        Require(remaining >= sizeof(T), "Truncated Jacobi DAT file");
        unsigned char b[sizeof(T)];
        std::memcpy(b, p, sizeof(T));
        p += sizeof(T); remaining -= sizeof(T);
        if (swap) std::reverse(b, b + sizeof(T));
        T value; std::memcpy(&value, b, sizeof(T)); return value;
    }
    double Number() {
        double value = Get<double>();
        Require(std::isfinite(value), "Non-finite Jacobi coordinate or coefficient");
        return value;
    }
};
double Phi(Basis basis, int i, double x) {
    if (basis == Basis::ModifiedA) {
        if (i == 0) return (1 - x) / 2;
        if (i == 1) return (1 + x) / 2;
        return (1 - x*x) / 4 * Jacobi(i - 2, 1, 1, x);
    }
    double result = Jacobi(i, 0, 0, x);
    return basis == Basis::OrthoA ? result * std::sqrt((2*i + 1)/2.0) : result;
}
constexpr int signs[8][3] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
Vec3 LinearPosition(const Element& e, Vec3 p) {
    Vec3 result{};
    if (e.shape == Shape::Prism) {
        double r = (1+p[0])*(1-p[2])/2 - 1, s=p[1], t=p[2];
        const double w[6] = {-(r+t)*(1-s)/4, (1+r)*(1-s)/4,
            (1+r)*(1+s)/4, -(r+t)*(1+s)/4, (1-s)*(1+t)/4, (1+s)*(1+t)/4};
        for (int i=0;i<6;++i) for (int d=0;d<3;++d) result[d]+=w[i]*e.vertices[i][d];
    } else {
        int n=e.shape==Shape::Quadrilateral?4:8;
        for (int i=0;i<n;++i) {
            double w=(1+signs[i][0]*p[0])*(1+signs[i][1]*p[1])/4;
            if (n==8) w *= (1+signs[i][2]*p[2])/2;
            for (int d=0;d<3;++d) result[d]+=w*e.vertices[i][d];
        }
    }
    return result;
}
std::vector<double> Lagrange(const std::vector<double>& nodes, double x) {
    std::vector<double> w(nodes.size(),1);
    for (size_t i=0;i<nodes.size();++i)
        for (size_t j=0;j<nodes.size();++j) if (i!=j) w[i]*=(x-nodes[j])/(nodes[i]-nodes[j]);
    return w;
}
Vec3 EdgePosition(const Element& e, Vec3 p) {
    Vec3 result=LinearPosition(e,p);
    int dims=e.shape==Shape::Quadrilateral?2:3;
    for (const auto& curve:e.curves) if (!curve.face) {
        int a=curve.corners[0],b=curve.corners[1],axis=-1;
        double weight=1;
        for (int d=0;d<dims;++d) {
            if (signs[a][d]!=signs[b][d]) axis=d;
            else weight*=(1+signs[a][d]*p[d])/2;
        }
        Require(axis>=0,"Invalid curved edge topology");
        double x=p[axis]*signs[b][axis];
        auto w=Lagrange(curve.nodes,x);
        for (int d=0;d<3;++d) {
            double q=0;
            for (size_t i=0;i<w.size();++i) q+=w[i]*curve.points[i][d];
            double linear=(1-x)/2*e.vertices[a][d]+(1+x)/2*e.vertices[b][d];
            result[d]+=weight*(q-linear);
        }
    }
    return result;
}
}

// Recurrence and modal ordering adapted from Jacobi.hpp,
// HexahedronCommon.cu, PrismCommon.cu and NektarPlusPlusExtension/Expansions.cu.
// Original code license (MIT):
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
double Jacobi(int n, int a, int b, double x) {
    if (n==0) return 1;
    double prev=1, current=(a-b+(a+b+2)*x)/2.0;
    for (int k=2;k<=n;++k) {
        double ab=a+b;
        double a1=2.0*k*(k+ab)*(2*k+ab-2);
        double a2=(2*k+ab-1)*(a*a-b*b);
        double a3=(2*k+ab-2)*(2*k+ab-1)*(2*k+ab);
        double a4=2.0*(k+a-1)*(k+b-1)*(2*k+ab);
        double next=((a2+a3*x)*current-a4*prev)/a1;
        prev=current; current=next;
    }
    return current;
}
double Element::Evaluate(const std::string& field, const Vec3& p) const {
    const auto& c=fields.at(field);
    std::array<std::vector<double>,3> phi;
    for(int d=0;d<3;++d) {
        phi[d].resize(modes[d]);
        for(int i=0;i<modes[d];++i) phi[d][i]=Phi(basis[d],i,p[d]);
    }
    size_t index=0; double result=0;
    if (shape==Shape::Prism) {
        double power=1;
        for(int i=0;i<modes[0];++i) {
            for(int j=0;j<modes[1];++j)
                for(int k=0;k<modes[2]-i;++k) {
                    double value;
                    if(nektar) {
                        double collapsed=i==0?Phi(Basis::ModifiedA,k,p[2]):
                            std::pow((1-p[2])/2,i)*(k==0?1:(1+p[2])/2*Jacobi(k-1,2*i-1,1,p[2]));
                        // Nektar++'s top singular vertex is shared by both
                        // bottom-vertex branches: phi_(0,j,1) uses A0+A1=1.
                        value=(i==0&&k==1?1:phi[0][i])*phi[1][j]*collapsed;
                    } else value=phi[0][i]*phi[1][j]*power*Jacobi(k,2*i+1,0,p[2]);
                    result+=c.at(index++)*value;
                }
            power*=1-p[2];
        }
    } else if (nektar) {
        // Nektar++ has i fastest; Jacobi DAT has k fastest.
        for(int k=0;k<modes[2];++k) for(int j=0;j<modes[1];++j) for(int i=0;i<modes[0];++i)
            result+=c.at(index++)*phi[0][i]*phi[1][j]*(shape==Shape::Quadrilateral?1:phi[2][k]);
    } else {
        for(int i=0;i<modes[0];++i) for(int j=0;j<modes[1];++j) for(int k=0;k<modes[2];++k)
            result+=c.at(index++)*phi[0][i]*phi[1][j]*phi[2][k];
    }
    Require(index==c.size(),"Spectral coefficient count mismatch");
    Require(std::isfinite(result),"Non-finite evaluated spectral field");
    return result;
}
Vec3 Element::Position(const Vec3& p) const {
    if (curves.empty()) return LinearPosition(*this,p);
    Vec3 result=EdgePosition(*this,p);
    for (const auto& curve:curves) if(curve.face) {
        int a=curve.corners[0],b=curve.corners[1],d=curve.corners[3];
        int u=-1,v=-1,fixed=-1;
        for(int axis=0;axis<3;++axis) {
            if(signs[a][axis]!=signs[b][axis]) u=axis;
            else if(signs[a][axis]!=signs[d][axis]) v=axis;
            else fixed=axis;
        }
        Require(u>=0&&v>=0,"Invalid curved face topology");
        auto wu=Lagrange(curve.nodes,p[u]*signs[b][u]);
        auto wv=Lagrange(curve.nodes,p[v]*signs[d][v]);
        Vec3 q{};
        for(size_t j=0;j<wv.size();++j) for(size_t i=0;i<wu.size();++i)
            for(int axis=0;axis<3;++axis) q[axis]+=wu[i]*wv[j]*curve.points[j*wu.size()+i][axis];
        Vec3 onFace=p;
        double weight=1;
        if(shape!=Shape::Quadrilateral) {
            Require(fixed>=0,"Invalid curved hex face");
            onFace[fixed]=signs[a][fixed]; weight=(1+signs[a][fixed]*p[fixed])/2;
        }
        auto baseline=EdgePosition(*this,onFace);
        for(int axis=0;axis<3;++axis) result[axis]+=weight*(q[axis]-baseline[axis]);
    }
    for(double x:result) Require(std::isfinite(x),"Non-finite spectral geometry");
    return result;
}
Data ReadJacobi(const void* bytes, size_t size) {
    constexpr char header[]="Finite Element Volume  ";
    Require(bytes&&size>=sizeof(header)+8,"Jacobi DAT header missing");
    Require(std::memcmp(bytes,header,sizeof(header))==0,"Not a Jacobi Finite Element Volume DAT");
    Binary in{static_cast<const unsigned char*>(bytes)+sizeof(header),size-sizeof(header)};
    auto endian=in.Get<std::int32_t>();
    if(endian!=1) { Require(endian==0x01000000,"Invalid DAT endian marker"); in.swap=true; }
    int count=in.Get<std::int32_t>();
    Require(count>0&&static_cast<size_t>(count)<=in.remaining/228,"Invalid DAT element count");
    Data data; data.fields={"scalar"}; data.elements.reserve(count);
    for(int id=0;id<count;++id) {
        Element e; e.id=id;
        int type=in.Get<std::int32_t>();
        Require(type==1||type==3,"DAT supports only hex and prism elements");
        e.shape=type==1?Shape::Hexahedron:Shape::Prism;
        int hasRange=in.Get<std::int32_t>();
        Require(hasRange==0||hasRange==1,"Invalid DAT range flag");
        // Some legacy files use +/- DBL_MAX sentinel values.
        in.Get<double>();in.Get<double>();
        for(auto& v:e.vertices) for(double& x:v) x=in.Number(); // prisms contain two padding vertices
        for(int& m:e.modes) {
            int degree=in.Get<std::int32_t>();
            Require(degree>=0&&degree<=64,"DAT polynomial degree outside supported range 0..64");
            m=degree+1;
        }
        size_t n=0;
        if(type==1) n=size_t(e.modes[0])*e.modes[1]*e.modes[2];
        else {
            // The DAT file writer uses this count and assumes equal triangle orders.
            Require(e.modes[0]==e.modes[2],"DAT prism requires equal first and third degrees");
            n=size_t(e.modes[0])*e.modes[1]*(e.modes[2]+1)/2;
        }
        Require(n<=in.remaining/8,"Truncated DAT coefficients");
        auto& c=e.fields["scalar"]; c.reserve(n);
        for(size_t i=0;i<n;++i) c.push_back(in.Number());
        data.elements.push_back(std::move(e));
    }
    Require(in.remaining==0,"Unexpected trailing DAT data");
    return data;
}
void Sample(const Data& data,int subdivisions,
            const std::function<void(const Vec3&,const std::vector<double>&)>& point,
            const std::function<void(Shape,const std::vector<size_t>&,int)>& cell) {
    Require(subdivisions>=0&&subdivisions<=128,"Subdivisions must be 0 (automatic) or 1..128");
    size_t offset=0;
    for(const auto& e:data.elements) {
        int n=subdivisions?subdivisions:std::max(2,*std::max_element(e.modes.begin(),e.modes.end()));
        Require(n<=128,"Automatic sampling exceeds supported resolution");
        const int width=n+1;
        auto emitPoint=[&](Vec3 p) {
            std::vector<double> values; values.reserve(data.fields.size());
            for(const auto& f:data.fields) values.push_back(e.Evaluate(f,p));
            point(e.Position(p),values);
        };
        if(e.shape==Shape::Prism) {
            size_t triangle=size_t(n+1)*(n+2)/2;
            auto index=[&](int i,int j,int k) { return offset+size_t(j)*triangle+size_t(k)*width-size_t(k)*(k-1)/2+i; };
            for(int j=0;j<=n;++j) for(int k=0;k<=n;++k) for(int i=0;i<=n-k;++i) {
                double c=2.0*k/n-1, a=k==n?-1:2.0*i/(n-k)-1;
                emitPoint({a,2.0*j/n-1,c});
            }
            for(int j=0;j<n;++j) for(int k=0;k<n;++k) for(int i=0;i<n-k;++i) {
                auto prism=[&](int a,int ak,int b,int bk,int c,int ck) {
                    // Triangle (r,t), then extrusion along s. Reverse the triangle
                    // order so the wedge has positive orientation in (r,s,t).
                    cell(Shape::Prism,{index(a,j,ak),index(c,j,ck),index(b,j,bk),
                                      index(a,j+1,ak),index(c,j+1,ck),index(b,j+1,bk)},e.id);
                };
                prism(i,k,i+1,k,i,k+1);
                if(i+k<n-1) prism(i+1,k,i+1,k+1,i,k+1);
            }
            offset+=triangle*width;
        } else {
            bool quad=e.shape==Shape::Quadrilateral;
            auto index=[&](int i,int j,int k) {return offset+(size_t(k)*width+j)*width+i;};
            for(int k=0;k<=(quad?0:n);++k) for(int j=0;j<=n;++j) for(int i=0;i<=n;++i)
                emitPoint({2.0*i/n-1,2.0*j/n-1,quad?0:2.0*k/n-1});
            for(int k=0;k<(quad?1:n);++k) for(int j=0;j<n;++j) for(int i=0;i<n;++i) {
                std::vector<size_t> ids={index(i,j,k),index(i+1,j,k),index(i+1,j+1,k),index(i,j+1,k)};
                if(!quad) { ids.push_back(index(i,j,k+1));ids.push_back(index(i+1,j,k+1));
                    ids.push_back(index(i+1,j+1,k+1));ids.push_back(index(i,j+1,k+1)); }
                cell(e.shape,ids,e.id);
            }
            offset+=size_t(width)*width*(quad?1:width);
        }
    }
}
}
