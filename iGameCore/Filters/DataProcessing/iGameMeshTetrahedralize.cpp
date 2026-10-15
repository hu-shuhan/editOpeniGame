#include "iGameMeshTetrahedralize.h"
#include "iGameFlatArray.h"
#include "iGameStarTetrahedralizer.h"
#include <limits>
#include <set>

IGAME_NAMESPACE_BEGIN
namespace {
struct NewPointSource {
    igIndex outPointId{-1};
    std::vector<igIndex> srcPointIds;
    std::vector<double> weights;
};

struct FaceKeyHash {
    size_t operator()(const std::vector<igIndex>& ids) const {
        size_t hash=ids.size();
        for(auto id:ids)hash^=std::hash<igIndex>{}(id)+size_t{0x9e3779b9}+(hash<<6)+(hash>>2);
        return hash;
    }
};

// If the kernel search moved the center, interpolate affine point fields using
// a containing tetrahedron of original vertices rather than averaging values
// at a location that is no longer the vertex centroid.
bool CenterAttributeWeights(const std::vector<igIndex>& ids, Points* points,
                            const detail::PolygonPoint& center, NewPointSource& source) {
    const auto get=[&](igIndex id) {
        const auto p=points->GetPoint(id);
        return detail::PolygonPoint{p[0],p[1],p[2]};
    };
    detail::PolygonPoint mean{};
    for(auto id:ids) {const auto p=get(id);for(int d=0;d<3;++d)mean[d]+=p[d]/ids.size();}
    bool unchanged=true;
    for(int d=0;d<3;++d) unchanged &= static_cast<float>(mean[d])==static_cast<float>(center[d]);
    if(unchanged) {source.srcPointIds=ids;return true;}
    const auto a=get(ids[0]);
    for(size_t i=1;i<ids.size();++i)for(size_t j=i+1;j<ids.size();++j)for(size_t k=j+1;k<ids.size();++k) {
        const auto b=get(ids[i]),c=get(ids[j]),d=get(ids[k]);
        const double det=detail::TetraDeterminant(a,b,c,d);
        if(!std::isfinite(det) || det==0)continue;
        std::vector<double> w{
            detail::TetraDeterminant(center,b,c,d)/det,
            detail::TetraDeterminant(a,center,c,d)/det,
            detail::TetraDeterminant(a,b,center,d)/det,
            detail::TetraDeterminant(a,b,c,center)/det};
        if(std::all_of(w.begin(),w.end(),[](double x){return std::isfinite(x) && x>=-1e-8 && x<=1+1e-8;})) {
            double sum=0;for(double& x:w){x=std::max(0.0,x);sum+=x;}for(double& x:w)x/=sum;
            source.srcPointIds={ids[0],ids[i],ids[j],ids[k]};source.weights=std::move(w);return true;
        }
    }
    return false;
}

template<typename ArrayType>
ArrayObject::Pointer CopyAttribute(AttributeSet::Attribute& attr, const ArrayObject::Pointer& inArray,
                                   AttributeSet::Pointer outData, igIndex inPointNum, igIndex outPointNum,
                                   igIndex outCellNum, const std::vector<NewPointSource>& newPointSources,
                                   const std::vector<igIndex>& originCells, double values[], double tmp[]) {
    auto outArray = ArrayType::New();
    outArray->SetName(inArray->GetName());
    outArray->SetDimension(inArray->GetDimension());
    const int dim = inArray->GetDimension();

    if (attr.attachmentType == IG_POINT) {
        outArray->Resize(outPointNum);

        const igIndex copyPointNum =
                std::min<igIndex>(inPointNum, static_cast<igIndex>(inArray->GetNumberOfElements()));
        for (igIndex pid = 0; pid < copyPointNum; ++pid) {
            inArray->GetElement(pid, values);
            outArray->SetElement(pid, values);
        }

        for (const auto& np: newPointSources) {
            if (np.outPointId < 0 || np.outPointId >= outPointNum) continue;
            const igIndex cnt = static_cast<igIndex>(np.srcPointIds.size());
            if (cnt <= 0) continue;

            for (int k = 0; k < dim; ++k) { values[k] = 0.0; }
            igIndex usedCount = 0;
            for (igIndex s = 0; s < cnt; ++s) {
                const igIndex srcId = np.srcPointIds[static_cast<size_t>(s)];
                if (srcId < 0 || srcId >= copyPointNum) continue;
                inArray->GetElement(srcId, tmp);
                const double weight = np.weights.empty() ? 1.0 : np.weights[static_cast<size_t>(s)];
                for (int k = 0; k < dim; ++k) { values[k] += weight * tmp[k]; }
                ++usedCount;
            }
            if (usedCount <= 0) continue;
            const double inv = np.weights.empty() ? 1.0 / static_cast<double>(usedCount) : 1.0;
            for (int k = 0; k < dim; ++k) { values[k] *= inv; }
            outArray->SetElement(np.outPointId, values);
        }

        outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
    } else if (attr.attachmentType == IG_CELL) {
        outArray->Resize(outCellNum);
        const igIndex copyCellNum = std::min<igIndex>(outCellNum, static_cast<igIndex>(originCells.size()));
        for (igIndex cid = 0; cid < copyCellNum; ++cid) {
            const igIndex srcCell = originCells[static_cast<size_t>(cid)];
            inArray->GetElement(srcCell, values);
            outArray->SetElement(cid, values);
        }
        outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
    } else {
        outData->AddAttribute(attr.type, attr.attachmentType, inArray, attr.GetDataRange());
    }
    return outArray;
}

} // namespace

bool MeshTetrahedralize::Execute() {
    m_failReason.clear();
    SetOutput(nullptr);
    const auto fail=[&](const std::string& reason) {m_failReason=reason;return false;};
    const auto cellFail=[&](IGsize id,const std::string& reason) {
        return fail("单元 " + std::to_string(id) + "（0 起始）：" + reason);
    };
    auto object=GetInput(0);
    if(!object) return fail("没有输入体网格。");
    UnstructuredMesh::Pointer input;
    VolumeMesh::Pointer volume;
    if(object->GetDataObjectType()==IG_UNSTRUCTURED_MESH)input=DynamicCast<UnstructuredMesh>(object);
    else if(object->GetDataObjectType()==IG_VOLUME_MESH)volume=DynamicCast<VolumeMesh>(object);
    else return fail("四面体化只接受体网格或包含体单元的非结构化网格。");

    // Reuse explicit CGNS NGON/NFACE connectivity: converting a large
    // polyhedral mesh to an intermediate mesh duplicates its connectivity.
    CellArray::Pointer faces,cellFaces;
    Points::Pointer points;
    AttributeSet::Pointer attributes;
    if(volume && volume->GetIsPolyhedronType()) {
        faces=volume->GetFaces();cellFaces=volume->GetVolumeFaces();
        points=volume->GetPoints();attributes=volume->GetAttributeSet();
    } else {
        if(volume) {
            input=UnstructuredMesh::New();input->GenerateFromVolumeMesh(volume);
        }
        if(!input) return fail("无法读取输入网格。");
        points=input->GetPoints();attributes=input->GetAttributeSet();
        if(!points)return fail("输入网格缺少点坐标。");
        faces=CellArray::New();cellFaces=CellArray::New();
        std::unordered_map<std::vector<igIndex>,igIndex,FaceKeyHash> faceLookup;
        for(IGsize ci=0;ci<input->GetNumberOfCells();++ci) {
            const igIndex* ids=nullptr;
            const int count=input->GetCellPointIds(ci,ids);
            const auto type=input->GetCellType(ci);
            std::vector<igIndex> fids;
            std::string faceReason;
            const auto addFace=[&](const igIndex* vertices,int size) {
                std::vector<igIndex> key(vertices,vertices+size);std::sort(key.begin(),key.end());
                if(key.front()<0 || key.back()>=points->GetNumberOfPoints() ||
                   std::adjacent_find(key.begin(),key.end())!=key.end()) {
                    faceReason="面顶点索引越界或重复。";return false;
                }
                const igIndex next=static_cast<igIndex>(faces->GetNumberOfCells());
                auto [iter,inserted]=faceLookup.emplace(key,next);
                if(inserted)faces->AddCellIds(vertices,size);
                else {
                    const igIndex* previous=nullptr;faces->GetCellIds(iter->second,previous);
                    int start=0;while(previous[start]!=vertices[0])++start;
                    bool forward=true,backward=true;
                    for(int j=0;j<size;++j) {
                        forward &= previous[(start+j)%size]==vertices[j];
                        backward &= previous[(start+size-j)%size]==vertices[j];
                    }
                    if(!forward && !backward){faceReason="共享面的顶点绕序不兼容。";return false;}
                }
                fids.push_back(iter->second);return true;
            };
            if(type==IG_POLYHEDRON) {
                if(count<1 || ids[0]<4)return cellFail(ci,"多面体连接数据不完整。");
                int offset=1;
                for(int f=0;f<ids[0];++f) {
                    if(offset>=count)return cellFail(ci,"多面体面连接越界。");
                    const int size=ids[offset++];
                    if(size<3 || size>count-offset)return cellFail(ci,"多面体面顶点数无效。");
                    if(!addFace(ids+offset,size))return cellFail(ci,faceReason);
                    offset+=size;
                }
                if(offset!=count)return cellFail(ci,"多面体连接包含多余数据。");
            } else {
                const auto addRegularFaces=[&](const auto& localFaces,int maxFaceSize,int expected) {
                    if(count!=expected)return false;
                    for(const auto& face:localFaces) {
                        igIndex vertices[4];const int size=face[maxFaceSize];
                        for(int j=0;j<size;++j)vertices[j]=ids[face[j]];
                        if(!addFace(vertices,size))return false;
                    }
                    return true;
                };
                bool valid=false;
                switch(type) {
                    case IG_TETRA:valid=addRegularFaces(Tetra::faces,Tetra::MaxFaceSize,4);break;
                    case IG_HEXAHEDRON:valid=addRegularFaces(Hexahedron::faces,Hexahedron::MaxFaceSize,8);break;
                    case IG_PRISM:valid=addRegularFaces(Prism::faces,Prism::MaxFaceSize,6);break;
                    case IG_PYRAMID:valid=addRegularFaces(Pyramid::faces,Pyramid::MaxFaceSize,5);break;
                    default:return cellFail(ci,"不支持的单元类型 " + std::to_string(type));
                }
                if(!valid)return cellFail(ci,faceReason.empty()?"单元顶点数与类型不一致。":faceReason);
            }
            cellFaces->AddCellIds(fids.data(),static_cast<int>(fids.size()));
        }
    }
    if(!points || !faces || !cellFaces || cellFaces->GetNumberOfCells()==0)
        return fail("输入网格没有完整体单元。");
    const IGsize nCells=cellFaces->GetNumberOfCells(),nFaces=faces->GetNumberOfCells();
    std::vector<uint8_t> faceUsers(nFaces,0);
    for(IGsize ci=0;ci<nCells;++ci) {
        const igIndex* ids=nullptr;const int n=cellFaces->GetCellIds(ci,ids);
        if(n<4)return cellFail(ci,"单元面数不足四个。");
        std::set<igIndex> unique;
        for(int j=0;j<n;++j) {
            if(ids[j]<0 || ids[j]>=nFaces || !unique.insert(ids[j]).second)
                return cellFail(ci,"面引用越界或重复。");
            if(++faceUsers[ids[j]]>2)return cellFail(ci,"共享面被超过两个体单元使用。");
        }
    }

    detail::PolygonEdges occupied;
    std::unordered_multimap<size_t,IGsize> faceDefinitions;
    for(IGsize fi=0;fi<nFaces;++fi) {
        if(!faceUsers[fi])continue;
        const igIndex* ids=nullptr;const int n=faces->GetCellIds(fi,ids);
        if(n<3)return fail("面 " + std::to_string(fi) + " 顶点不足三个。");
        if(volume && volume->GetIsPolyhedronType()) {
            // Explicit face IDs must identify the same shared interface once.
            // Otherwise two cells could use separate IDs for an identical face
            // and evade both shared-side and matching-triangulation checks.
            std::vector<igIndex> key(ids,ids+n);std::sort(key.begin(),key.end());
            const size_t hash=FaceKeyHash{}(key);const auto range=faceDefinitions.equal_range(hash);
            for(auto iter=range.first;iter!=range.second;++iter) {
                const igIndex* old=nullptr;const int count=faces->GetCellIds(iter->second,old);
                if(count!=n)continue;
                std::vector<igIndex> other(old,old+count);std::sort(other.begin(),other.end());
                if(other==key)return fail("面 " + std::to_string(fi) + " 与面 " + std::to_string(iter->second) + " 重复定义，无法保证共享接口。");
            }
            faceDefinitions.emplace(hash,fi);
        }
        for(int j=0;j<n;++j) {
            if(ids[j]<0 || ids[j]>=points->GetNumberOfPoints())return fail("面 " + std::to_string(fi) + " 顶点索引越界。");
            occupied.emplace(ids[j],ids[(j+1)%n]);
        }
    }
    faceDefinitions.clear();faceDefinitions.rehash(0);
    auto outPoints=Points::New();outPoints->DeepCopy(points);
    const auto get=[&](detail::PolygonId id) {
        const auto p=outPoints->GetPoint(static_cast<igIndex>(id));
        return detail::PolygonPoint{p[0],p[1],p[2]};
    };
    auto triFaces=CellArray::New();std::vector<IGsize> offsets(nFaces+1,0);
    std::vector<NewPointSource> newPointSources;
    for(IGsize fi=0;fi<nFaces;++fi) {
        offsets[fi]=triFaces->GetNumberOfCells();
        if(!faceUsers[fi]){offsets[fi+1]=offsets[fi];continue;}
        const igIndex* raw=nullptr;const int n=faces->GetCellIds(fi,raw);
        std::vector<detail::PolygonId> ids(raw,raw+n);
        std::vector<detail::PolygonPoint> p;for(auto id:ids)p.push_back(get(id));
        std::vector<detail::PolygonTriangle> tris;
        // A warped face has no unique planar diagonal. A shared face center
        // gives both cells the same symmetric piecewise-linear boundary and
        // avoids a locally good diagonal that destroys a neighboring kernel.
        detail::PolygonPoint center{},normal{},lo=p[0],hi=p[0];
        for(const auto& x:p)for(int d=0;d<3;++d) {
            center[d]+=x[d]/n;lo[d]=std::min(lo[d],x[d]);hi[d]=std::max(hi[d],x[d]);
        }
        for(int j=1;j+1<n;++j) {
            const auto c=detail::PolygonCross(detail::PolygonSubtract(p[j],p[0]),detail::PolygonSubtract(p[j+1],p[0]));
            for(int d=0;d<3;++d)normal[d]+=c[d];
        }
        const double normalLength=std::sqrt(detail::PolygonDot(normal,normal));
        const double diagonal=std::sqrt(detail::PolygonDot(detail::PolygonSubtract(hi,lo),detail::PolygonSubtract(hi,lo)));
        double warp=0;
        if(normalLength>0)for(const auto& x:p)
            warp=std::max(warp,std::abs(detail::PolygonDot(normal,detail::PolygonSubtract(x,center)))/normalLength);
        const bool warped=n>3 && warp>1e-5*diagonal;
        for(double& x:center)x=static_cast<float>(x);
        bool fanValid=normalLength>0 && std::isfinite(normalLength);
        for(int j=0;j<n && fanValid;++j) {
            const auto c=detail::PolygonCross(detail::PolygonSubtract(p[j],center),detail::PolygonSubtract(p[(j+1)%n],center));
            fanValid=detail::PolygonDot(c,normal)>0;
        }
        const bool faceCenter=(warped && fanValid) || !detail::TriangulatePolygon(ids,p,occupied,tris);
        if(faceCenter) {
            // Validate before adding a center: a concave face whose centroid
            // lies outside must not be filled by an overlapping fan.
            std::set<igIndex> unique(raw,raw+n);
            if(unique.size()!=static_cast<size_t>(n))return fail("面 " + std::to_string(fi) + " 包含重复顶点。");
            if(!fanValid)return fail("面 " + std::to_string(fi) + " 无法完整三角化，面可能退化或自交。");
            if(outPoints->GetNumberOfPoints()>=std::numeric_limits<igIndex>::max())return fail("新增点超出索引范围。");
            const igIndex centerId=outPoints->AddPoint(center[0],center[1],center[2]);
            newPointSources.push_back({centerId,std::vector<igIndex>(raw,raw+n),{}});
            tris.clear();for(int j=0;j<n;++j)tris.push_back({centerId,ids[j],ids[(j+1)%n]});
        }
        for(const auto& t:tris)triFaces->AddCellId3(t[0],t[1],t[2]);
        offsets[fi+1]=triFaces->GetNumberOfCells();
        if(fi%20000==0)UpdateProgress(0.1+0.35*double(fi)/std::max<IGsize>(nFaces,1));
    }
    occupied.clear();occupied.rehash(0);

    auto outCells=CellArray::New();std::vector<igIndex> originCells;
    std::vector<int8_t> faceSide(nFaces,0);
    for(IGsize ci=0;ci<nCells;++ci) {
        const igIndex* fids=nullptr;const int n=cellFaces->GetCellIds(ci,fids);
        std::vector<detail::PolygonTriangle> boundary;
        std::vector<size_t> faceStarts;
        std::set<igIndex> vertexIds;
        for(int f=0;f<n;++f) {
            faceStarts.push_back(boundary.size());
            for(IGsize t=offsets[fids[f]];t<offsets[fids[f]+1];++t) {
                const igIndex* ids=nullptr;triFaces->GetCellIds(t,ids);
                boundary.push_back({ids[0],ids[1],ids[2]});
            }
            const igIndex* ids=nullptr;const int count=faces->GetCellIds(fids[f],ids);
            vertexIds.insert(ids,ids+count);
        }
        detail::StarCell star;std::string reason;
        const bool passthrough=vertexIds.size()==4 && boundary.size()==4;
        if(!detail::PrepareStarCell(boundary,get,star,reason,!passthrough))return cellFail(ci,reason);
        for(int f=0;f<n;++f) {
            const int side=boundary[faceStarts[f]][1]==star.boundary[faceStarts[f]][1]?1:-1;
            const size_t end=f+1<n?faceStarts[f+1]:boundary.size();
            for(size_t t=faceStarts[f];t<end;++t)
                if((boundary[t][1]==star.boundary[t][1]?1:-1)!=side)
                    return cellFail(ci,"同一多边形面三角形的方向不一致。");
            if(faceSide[fids[f]]==side)return cellFail(ci,"共享面的两个体单元位于同侧或互相重叠，面 " + std::to_string(fids[f]));
            faceSide[fids[f]]=static_cast<int8_t>(side);
        }
        if(passthrough) {
            std::vector<igIndex> tet(vertexIds.begin(),vertexIds.end());
            if(detail::TetraDeterminant(get(tet[0]),get(tet[1]),get(tet[2]),get(tet[3]))<0)std::swap(tet[1],tet[2]);
            outCells->AddCellIds(tet.data(),4);originCells.push_back(ci);
        } else {
            if(outPoints->GetNumberOfPoints()>=std::numeric_limits<igIndex>::max())return cellFail(ci,"新增点超出索引范围。");
            NewPointSource source;
            if(!CenterAttributeWeights(std::vector<igIndex>(vertexIds.begin(),vertexIds.end()),outPoints.get(),star.center,source))
                return cellFail(ci,"无法为内部中心构造点属性插值权重。");
            const igIndex centerId=outPoints->AddPoint(star.center[0],star.center[1],star.center[2]);
            source.outPointId=centerId;newPointSources.push_back(std::move(source));
            for(const auto& t:star.boundary) {
                // Positive determinant, with consistent orientation across the
                // cone's internal interfaces and the cached shared boundary.
                outCells->AddCellId4(centerId,t[0],t[1],t[2]);originCells.push_back(ci);
            }
        }
        if(ci%20000==0)UpdateProgress(0.45+0.45*double(ci)/nCells);
    }
    auto out=VolumeMesh::New();out->SetName(object->GetName());
    out->SetPoints(outPoints);out->SetVolumes(outCells);
    auto outAttributes=AttributeSet::New();out->SetAttributeSet(outAttributes);
    if(attributes) {
        auto all=attributes->GetAllAttributes();
        for(IGsize ai=0;ai<all->GetNumberOfElements();++ai) {
            auto& attr=all->GetElement(ai);auto array=attr.pointer;
            if(attr.isDeleted || !array)continue;
            if(array->GetDimension()<=0)return fail("属性分量数无效。");
            if((attr.attachmentType==IG_POINT && array->GetNumberOfElements()<points->GetNumberOfPoints()) ||
               (attr.attachmentType==IG_CELL && array->GetNumberOfElements()<nCells))return fail("属性数组长度与输入网格不匹配。");
            std::vector<double> values(array->GetDimension()),tmp(array->GetDimension());
#define COPY_ATTRIBUTE(ArrayType) CopyAttribute<ArrayType>(attr,array,outAttributes,points->GetNumberOfPoints(),outPoints->GetNumberOfPoints(),outCells->GetNumberOfCells(),newPointSources,originCells,values.data(),tmp.data())
            switch(array->GetArrayType()) {
                case IG_FloatArray:COPY_ATTRIBUTE(FloatArray);break;
                case IG_DoubleArray:COPY_ATTRIBUTE(DoubleArray);break;
                case IG_IntArray:COPY_ATTRIBUTE(IntArray);break;
                case IG_UnsignedIntArray:COPY_ATTRIBUTE(UnsignedIntArray);break;
                case IG_CharArray:COPY_ATTRIBUTE(CharArray);break;
                case IG_UnsignedCharArray:COPY_ATTRIBUTE(UnsignedCharArray);break;
                case IG_ShortArray:COPY_ATTRIBUTE(ShortArray);break;
                case IG_UnsignedShortArray:COPY_ATTRIBUTE(UnsignedShortArray);break;
                case IG_LongLongArray:COPY_ATTRIBUTE(LongLongArray);break;
                case IG_UnsignedLongLongArray:COPY_ATTRIBUTE(UnsignedLongLongArray);break;
                default:return fail("不支持的属性数组类型。");
            }
#undef COPY_ATTRIBUTE
        }
    }
    SetOutput(out);UpdateProgress(1.0);return true;
}

MeshTetrahedralize::MeshTetrahedralize() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END

