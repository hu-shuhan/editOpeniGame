#include "iGameSpectralData.h"
#include <tinyxml2.h>
#include <zlib.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace iGame::Spectral {
namespace {
using namespace tinyxml2;
void Check(bool ok,const std::string& message) { if(!ok) throw std::runtime_error("Nektar++: "+message); }
std::string ReadText(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);
    Check(bool(in),"cannot open companion file: "+path.string());
    return std::string(std::istreambuf_iterator<char>(in),{});
}
void Load(XMLDocument& doc,const std::filesystem::path& path) {
    auto text=ReadText(path);
    Check(doc.Parse(text.data(),text.size())==XML_SUCCESS,"invalid XML file: "+path.string());
    Check(doc.FirstChildElement("NEKTAR")!=nullptr,"missing NEKTAR root: "+path.string());
}
std::string Attr(const XMLElement* e,const char* name) {
    auto a=e?e->Attribute(name):nullptr;
    Check(a!=nullptr,std::string("missing attribute ")+name);
    return a;
}
int Id(const XMLElement* e,const char* name="ID") {
    int id=-1; Check(e&&e->QueryIntAttribute(name,&id)==XML_SUCCESS&&id>=0,"invalid ID attribute"); return id;
}
std::vector<int> Integers(const std::string& text) {
    // Supports Nektar's comma/space lists, ID ranges (0-3), and C[0-3].
    std::vector<int> values;
    for(size_t p=0;p<text.size();) {
        if(!std::isdigit(static_cast<unsigned char>(text[p]))) {++p;continue;}
        size_t end=p;
        while(end<text.size()&&std::isdigit(static_cast<unsigned char>(text[end]))) ++end;
        int start=std::stoi(text.substr(p,end-p)); p=end;
        int stop=start;
        if(p<text.size()&&text[p]=='-') {
            end=++p;
            while(end<text.size()&&std::isdigit(static_cast<unsigned char>(text[end]))) ++end;
            Check(end>p,"invalid ID range"); stop=std::stoi(text.substr(p,end-p));p=end;
        }
        Check(stop>=start&&stop-start<1000000,"invalid ID range");
        for(int n=start;n<=stop;++n) values.push_back(n);
    }
    return values;
}
std::vector<std::string> Strings(std::string text) {
    std::replace(text.begin(),text.end(),',',' ');
    std::istringstream in(text);std::vector<std::string> out;std::string s;
    while(in>>s) out.push_back(s);return out;
}
std::vector<Vec3> Coordinates(const XMLElement* e) {
    Check(e&&e->GetText(),"missing coordinate text (compressed geometry is unsupported)");
    std::istringstream in(e->GetText());std::vector<Vec3> points;Vec3 p;
    while(in>>p[0]) {
        Check(bool(in>>p[1]>>p[2]),"invalid coordinate triple");
        for(double x:p) Check(std::isfinite(x),"non-finite geometry coordinate");
        points.push_back(p);
    }
    Check(in.eof(),"invalid coordinate text");return points;
}
std::vector<unsigned char> Base64(const std::string& text) {
    const std::string table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<unsigned char> out;unsigned value=0;int bits=0;bool padding=false;
    for(unsigned char c:text) {
        if(std::isspace(c)) continue;
        if(c=='=') {padding=true;continue;}
        Check(!padding,"invalid base64 padding");
        auto index=table.find(static_cast<char>(c));Check(index!=std::string::npos,"invalid base64 data");
        value=(value<<6)|static_cast<unsigned>(index);bits+=6;
        if(bits>=8) {bits-=8;out.push_back(static_cast<unsigned char>(value>>bits));}
    }
    Check(bits!=6,"invalid base64 length");return out;
}
std::vector<double> FieldValues(const XMLElement* e,size_t count) {
    Check(e->GetText()!=nullptr,"empty field payload");
    auto compressed=Base64(e->GetText());
    Check(count>0&&count<=std::numeric_limits<uLongf>::max()/8,"invalid field size");
    std::vector<unsigned char> bytes(count*8);
    uLongf size=static_cast<uLongf>(bytes.size());
    Check(uncompress(bytes.data(),&size,compressed.data(),static_cast<uLong>(compressed.size()))==Z_OK
          &&size==bytes.size(),"field decompression failed or coefficient count mismatch");
    bool big=false;
    if(auto s=e->Attribute("COMPRESSED")) {
        std::string format=s;
        Check(format.rfind("B64Z-",0)==0,
              "unsupported compression format "+format);
        big=format.find("BigEndian")!=std::string::npos||format=="B64Z-BE";
        bool little=format.find("LittleEndian")!=std::string::npos||format=="B64Z-LE";
        Check(big!=little,"missing or ambiguous field byte order");
    }
    // Older Nektar fixtures omit COMPRESSED and contain little-endian doubles.
    bool swap=big!=(std::endian::native==std::endian::big);
    std::vector<double> out(count);
    for(size_t i=0;i<count;++i) {
        auto p=bytes.data()+8*i;if(swap) std::reverse(p,p+8);
        std::memcpy(&out[i],p,8);Check(std::isfinite(out[i]),"non-finite field coefficient");
    }
    return out;
}
using Edges=std::map<int,std::array<int,2>>;
std::vector<int> Cycle(const std::vector<int>& edgeIds,const Edges& edges) {
    Check(edgeIds.size()==3||edgeIds.size()==4,"only triangle/quad faces are supported");
    auto first=edges.at(edgeIds[0]);std::vector<int> out{first[0],first[1]};
    std::set<int> used{edgeIds[0]};
    while(out.size()<edgeIds.size()) {
        bool found=false;
        for(int id:edgeIds) if(!used.count(id)) {
            auto e=edges.at(id);
            int next=e[0]==out.back()?e[1]:(e[1]==out.back()?e[0]:-1);
            if(next>=0&&next!=out.front()) {out.push_back(next);used.insert(id);found=true;break;}
        }
        Check(found,"face edges do not form a closed polygon");
    }
    bool closes=false;
    for(int id:edgeIds) if(!used.count(id)) {
        auto e=edges.at(id);
        closes=(e[0]==out.back()&&e[1]==out.front())||(e[1]==out.back()&&e[0]==out.front());
    }
    Check(closes,"face is not closed");return out;
}
std::vector<double> Nodes(const XMLElement* e,int n) {
    Check(n>=2&&n<=65,"curved geometry order outside supported range");
    auto type=Attr(e,"TYPE");std::vector<double> nodes(n);
    if(type=="PolyEvenlySpaced") {
        for(int i=0;i<n;++i) nodes[i]=-1+2.0*i/(n-1);
    } else if(type=="GaussLobattoLegendre") {
        nodes.front()=-1;nodes.back()=1;
        for(int i=1;i<n-1;++i) {
            double x=-std::cos(3.14159265358979323846*i/(n-1));
            for(int it=0;it<40;++it) {
                double f=Jacobi(n-2,1,1,x),df=(n+1)/2.0*Jacobi(n-3,2,2,x),step=f/df;
                x-=step;if(std::abs(step)<1e-14) break;
            }
            nodes[i]=x;
        }
    } else Check(false,"unsupported curved point type "+type);
    return nodes;
}
}
bool IsNektarFile(const std::string& path) {
    try {
        XMLDocument doc;auto text=ReadText(std::filesystem::u8path(path));
        return doc.Parse(text.data(),text.size())==XML_SUCCESS&&doc.FirstChildElement("NEKTAR")!=nullptr;
    } catch(...) {return false;}
}
Data ReadNektar(const std::string& selectedPath) {
    auto path=std::filesystem::u8path(selectedPath);
    auto xml=path;xml.replace_extension(".xml");
    auto fld=path;fld.replace_extension(".fld");
    auto extension=path.extension().string();
    std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return std::tolower(c);});
    if(extension==".xml") xml=path;else if(extension==".fld") fld=path;
    XMLDocument meshDoc,fieldDoc;Load(meshDoc,xml);Load(fieldDoc,fld);
    auto root=meshDoc.FirstChildElement("NEKTAR");auto geometry=root->FirstChildElement("GEOMETRY");
    Check(geometry!=nullptr,"geometry XML has no GEOMETRY section");
    int dimension=0;geometry->QueryIntAttribute("DIM",&dimension);
    Check(dimension==2||dimension==3,"only 2D quad and 3D hex/prism geometry are supported");
    auto vertex=geometry->FirstChildElement("VERTEX"),edge=geometry->FirstChildElement("EDGE"),element=geometry->FirstChildElement("ELEMENT");
    Check(vertex&&edge&&element,"missing VERTEX, EDGE or ELEMENT section");
    for(const auto* section:{geometry,vertex}) for(auto a=section->FirstAttribute();a;a=a->Next()) {
        std::string name=a->Name();
        Check(name.find("SCALE")==std::string::npos&&name.find("MOVE")==std::string::npos,
              "geometry coordinate transforms must be applied before import");
    }
    Check(!vertex->Attribute("COMPRESSED")&&!edge->Attribute("COMPRESSED")&&!element->Attribute("COMPRESSED"),
          "compressed mesh topology is unsupported");
    std::map<int,Vec3> vertices;Edges edges;std::map<int,std::vector<int>> faces;
    for(auto v=vertex->FirstChildElement();v;v=v->NextSiblingElement()) {
        auto xyz=Coordinates(v);Check(xyz.size()==1,"vertex must have one coordinate triple");
        Check(vertices.emplace(Id(v),xyz[0]).second,"duplicate vertex ID");
    }
    for(auto e=edge->FirstChildElement();e;e=e->NextSiblingElement()) {
        Check(e->GetText(),"empty edge");auto ids=Integers(e->GetText());Check(ids.size()==2,"invalid edge endpoints");
        Check(vertices.count(ids[0])&&vertices.count(ids[1])&&ids[0]!=ids[1],"unknown or repeated edge vertex");
        Check(edges.emplace(Id(e),std::array<int,2>{ids[0],ids[1]}).second,"duplicate edge ID");
    }
    if(auto face=geometry->FirstChildElement("FACE")) {
        Check(!face->Attribute("COMPRESSED"),"compressed faces are unsupported");
        for(auto f=face->FirstChildElement();f;f=f->NextSiblingElement()) {
            Check((std::string(f->Name())=="Q"||std::string(f->Name())=="T")&&f->GetText(),"only triangle/quad faces supported");
            auto ids=Integers(f->GetText());Cycle(ids,edges);
            Check(faces.emplace(Id(f),std::move(ids)).second,"duplicate face ID");
        }
    }
    std::map<int,Curve> curvedEdges,curvedFaces;
    if(auto curved=geometry->FirstChildElement("CURVED")) {
        for(auto c=curved->FirstChildElement();c;c=c->NextSiblingElement()) {
            Curve curve;curve.face=std::string(c->Name())=="F";
            Check(curve.face||std::string(c->Name())=="E","unsupported curved entity");
            curve.points=Coordinates(c);int count=0;c->QueryIntAttribute("NUMPOINTS",&count);
            Check(count>0&&size_t(count)==curve.points.size(),"curved point count mismatch");
            int n=curve.face?static_cast<int>(std::sqrt(count)):count;
            Check(!curve.face||n*n==count,"curved face must have a square tensor point grid");
            curve.nodes=Nodes(c,n);
            auto& target=curve.face?curvedFaces:curvedEdges;
            Check(target.emplace(Id(c,curve.face?"FACEID":"EDGEID"),std::move(curve)).second,"duplicate curved entity");
        }
    }
    Data data;std::map<int,size_t> byId;
    for(auto el=element->FirstChildElement();el;el=el->NextSiblingElement()) {
        Element e;e.id=Id(el);e.nektar=true;
        std::string type=el->Name();Check((dimension==2&&type=="Q")||(dimension==3&&(type=="H"||type=="R")),"only quad/hex/prism elements supported");
        Check(el->GetText(),"empty element topology");auto refs=Integers(el->GetText());
        e.shape=dimension==2?Shape::Quadrilateral:(type=="R"?Shape::Prism:Shape::Hexahedron);
        std::vector<int> localVertices,localEdges;
        if(dimension==2) {localVertices=Cycle(refs,edges);localEdges=refs;}
        else {
            bool prism=e.shape==Shape::Prism;
            Check(refs.size()==(prism?5:6),"wrong element face count");localVertices=Cycle(faces.at(refs[0]),edges);
            Check(localVertices.size()==4,"first hex/prism face must be a quad");
            std::set<int> uniqueEdges;
            for(int f:refs) for(int id:faces.at(f)) uniqueEdges.insert(id);
            Check(uniqueEdges.size()==(prism?9:12),"wrong element edge count");localEdges.assign(uniqueEdges.begin(),uniqueEdges.end());
            if(prism) {
                for(const auto pair: {std::array<int,2>{0,1},std::array<int,2>{3,2}}) {
                    int apex=-1;
                    for(int id:refs) {
                        auto cycle=Cycle(faces.at(id),edges);
                        if(cycle.size()!=3) continue;
                        if(std::find(cycle.begin(),cycle.end(),localVertices[pair[0]])==cycle.end()||
                           std::find(cycle.begin(),cycle.end(),localVertices[pair[1]])==cycle.end()) continue;
                        for(int v:cycle) if(v!=localVertices[pair[0]]&&v!=localVertices[pair[1]]) apex=v;
                    }
                    Check(apex>=0,"prism triangular apex missing");localVertices.push_back(apex);
                }
            } else for(int i=0;i<4;++i) {
                int opposite=-1;
                for(int id:localEdges) {
                    auto ed=edges.at(id);int v=ed[0]==localVertices[i]?ed[1]:(ed[1]==localVertices[i]?ed[0]:-1);
                    if(v>=0&&std::find(localVertices.begin(),localVertices.begin()+4,v)==localVertices.begin()+4) {
                        Check(opposite<0,"ambiguous hex vertical edge");opposite=v;
                    }
                }
                Check(opposite>=0,"hex vertical edge missing");localVertices.push_back(opposite);
            }
            Check(std::set<int>(localVertices.begin(),localVertices.end()).size()==(prism?6:8),"element vertices are not distinct");
        }
        auto local=[&](int global) {
            auto it=std::find(localVertices.begin(),localVertices.end(),global);
            Check(it!=localVertices.end(),"curve does not belong to element");return int(it-localVertices.begin());
        };
        for(size_t i=0;i<localVertices.size();++i) e.vertices[i]=vertices.at(localVertices[i]);
        for(int id:localEdges) if(curvedEdges.count(id)) {
            auto c=curvedEdges.at(id);c.corners[0]=local(edges.at(id)[0]);c.corners[1]=local(edges.at(id)[1]);
            e.curves.push_back(std::move(c));
        }
        if(dimension==3) for(int id:refs) if(curvedFaces.count(id)) {
            auto c=curvedFaces.at(id);auto cycle=Cycle(faces.at(id),edges);
            for(int i=0;i<4;++i) c.corners[i]=local(cycle[i]);e.curves.push_back(std::move(c));
        }
        // A 2D curved face is referenced by its element ID.
        if(dimension==2&&curvedFaces.count(e.id)) {
            auto c=curvedFaces.at(e.id);c.corners={0,1,2,3};e.curves.push_back(std::move(c));
        }
        Check(e.shape!=Shape::Prism||e.curves.empty(),"curved prism geometry is unsupported");
        Check(byId.emplace(e.id,data.elements.size()).second,"duplicate element ID");
        data.elements.push_back(std::move(e));
    }
    Check(!data.elements.empty(),"no supported elements found");
    for(auto block=fieldDoc.FirstChildElement("NEKTAR")->FirstChildElement("ELEMENTS");block;block=block->NextSiblingElement("ELEMENTS")) {
        auto names=Strings(Attr(block,"FIELDS"));Check(!names.empty(),"no field names");
        if(data.fields.empty()) data.fields=names;else Check(data.fields==names,"field names differ between blocks");
        auto ids=Integers(Attr(block,"ID"));Check(!ids.empty(),"field has no element IDs");
        auto basisNames=Strings(Attr(block,"BASIS"));Check(int(basisNames.size())==dimension,"basis dimension mismatch");
        std::array<Basis,3> bases{Basis::ModifiedA,Basis::ModifiedA,Basis::Legendre};
        for(int d=0;d<dimension;++d) {
            if(basisNames[d]=="Modified_A") bases[d]=Basis::ModifiedA;
            else if(basisNames[d]=="Ortho_A") bases[d]=Basis::OrthoA;
            else if(basisNames[d]=="Modified_B"&&d==2) bases[d]=Basis::ModifiedB;
            else Check(false,"unsupported basis "+basisNames[d]);
        }
        auto modesText=Attr(block,"NUMMODESPERDIR");
        bool uniform=modesText.rfind("UNIORDER:",0)==0,mixed=modesText.rfind("MIXORDER:",0)==0;
        Check(uniform||mixed,"unknown mode order encoding");auto modes=Integers(modesText);
        Check(modes.size()==size_t(dimension)*(uniform?1:ids.size()),"mode count mismatch");
        std::vector<size_t> counts;size_t total=0;
        for(size_t i=0;i<ids.size();++i) {
            Check(byId.count(ids[i]),"field references unknown element ID");auto& e=data.elements[byId.at(ids[i])];
            Check(Attr(block,"SHAPE")== (dimension==2?"Quadrilateral":(e.shape==Shape::Prism?"Prism":"Hexahedron")),"field shape mismatch");
            e.basis=bases;size_t count=1;
            for(int d=0;d<dimension;++d) {
                int m=modes[(uniform?0:i*dimension)+d];Check(m>0&&m<=65,"mode count outside 1..65");e.modes[d]=m;count*=m;
            }
            if(e.shape==Shape::Prism) {
                Check(bases[0]==Basis::ModifiedA&&bases[1]==Basis::ModifiedA&&bases[2]==Basis::ModifiedB,
                      "prism requires Modified_A,Modified_A,Modified_B");
                Check(e.modes[0]<=e.modes[2],"prism first mode count exceeds third mode count");
                count=0;for(int i=0;i<e.modes[0];++i) count+=size_t(e.modes[1])*(e.modes[2]-i);
            } else for(int d=0;d<dimension;++d) Check(bases[d]!=Basis::ModifiedB,"Modified_B requires a prism");
            counts.push_back(count);total+=count;
        }
        Check(total<=size_t(512)*1024*1024/8/names.size(),"field block exceeds 512 MiB limit");
        auto values=FieldValues(block,total*names.size());size_t start=0;
        for(const auto& name:names) for(size_t i=0;i<ids.size();++i) {
            auto& fields=data.elements[byId.at(ids[i])].fields;Check(!fields.count(name),"duplicate field block for element");
            fields[name]=std::vector<double>(values.begin()+start,values.begin()+start+counts[i]);start+=counts[i];
        }
    }
    Check(!data.fields.empty(),"no ELEMENTS field blocks in companion FLD");
    for(const auto& e:data.elements) for(const auto& name:data.fields) Check(e.fields.count(name),"field missing for element "+std::to_string(e.id));
    return data;
}
}
