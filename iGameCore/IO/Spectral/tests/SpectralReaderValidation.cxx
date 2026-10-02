// Integration gap (2026-10-01): Jacobi DAT/Nektar files had no OpeniGame reader;
// XML was always sent to the spline-mode dialog. Verify endian handling, the
// two different modal orderings, nondegenerate prism sampling, compressed fields,
// curved geometry, FileIO dispatch and original coefficient retention.
// Fix commit: b1161467 (feat: add CPU spectral readers and sample datasets).
// Find the first commit with:
// git log --diff-filter=A --format="%h %s" -- iGameCore/IO/Spectral/tests/SpectralReaderValidation.cxx
#include "Spectral/iGameSpectralReaderCPU.h"
#include "Spectral/iGameSpectralMesh.h"
#include "iGameFileIO.h"
#include "iGameRenderWindow.h"
#include "iGameInteractor.h"
#include "iGameScene.h"
#include <GLFW/glfw3.h>
#include <zlib.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace iGame;
using namespace iGame::Spectral;
namespace {
void Require(bool ok,const std::string& message) {if(!ok) throw std::runtime_error(message);}
void Near(double actual,double expected,double tolerance=1e-10) {
    Require(std::abs(actual-expected)<tolerance,"numerical mismatch: "+std::to_string(actual)+" vs "+std::to_string(expected));
}
template<class T> void Append(std::vector<char>& bytes,T value,bool big) {
    char b[sizeof(T)];std::memcpy(b,&value,sizeof(T));
    if(big) std::reverse(b,b+sizeof(T));bytes.insert(bytes.end(),b,b+sizeof(T));
}
std::vector<char> Fixture(bool big,int type) {
    const char h[]="Finite Element Volume  ";std::vector<char> b(h,h+sizeof(h));
    Append<int>(b,1,big);Append<int>(b,1,big);Append<int>(b,type,big);Append<int>(b,0,big);
    Append<double>(b,0,big);Append<double>(b,0,big);
    const double hex[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    const double prism[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{-1,1,1},{0,0,0},{0,0,0}};
    for(int i=0;i<8;++i) for(int d=0;d<3;++d) Append<double>(b,type==1?hex[i][d]:prism[i][d],big);
    for(int d=0;d<3;++d) Append<int>(b,1,big);
    // Hex: 2 + 3*a + 5*b + 7*c + 11*a*b*c (k fastest).
    // Prism: the i=1,k=0 mode is a*(1-c), j fastest within i blocks.
    const double hexCoeffs[8]={2,7,5,0,3,0,0,11},prismCoeffs[6]={2,7,5,0,3,0};
    for(int i=0;i<(type==1?8:6);++i) Append<double>(b,type==1?hexCoeffs[i]:prismCoeffs[i],big);
    return b;
}
std::string CompressedField(const std::vector<double>& values) {
    uLongf size=compressBound(static_cast<uLong>(values.size()*8));std::vector<unsigned char> bytes(size);
    Require(compress(bytes.data(),&size,reinterpret_cast<const unsigned char*>(values.data()),static_cast<uLong>(values.size()*8))==Z_OK,"fixture compression failed");
    bytes.resize(size);const std::string table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;
    for(size_t i=0;i<bytes.size();i+=3) {
        unsigned x=unsigned(bytes[i])<<16;if(i+1<bytes.size()) x|=unsigned(bytes[i+1])<<8;if(i+2<bytes.size()) x|=bytes[i+2];
        out+=table[(x>>18)&63];out+=table[(x>>12)&63];out+=i+1<bytes.size()?table[(x>>6)&63]:'=';out+=i+2<bytes.size()?table[x&63]:'=';
    }
    return out;
}
void NektarFixture() {
    auto path=std::filesystem::current_path()/"spectral-validation-prism.xml";
    auto fld=path;fld.replace_extension(".fld");
    const std::string xml=R"(<NEKTAR><GEOMETRY DIM="3" SPACE="3"><VERTEX>
<V ID="0">-1 -1 -1</V><V ID="1">1 -1 -1</V><V ID="2">1 1 -1</V><V ID="3">-1 1 -1</V><V ID="4">-1 -1 1</V><V ID="5">-1 1 1</V></VERTEX>
<EDGE><E ID="0">0 1</E><E ID="1">1 2</E><E ID="2">2 3</E><E ID="3">3 0</E><E ID="4">0 4</E><E ID="5">1 4</E><E ID="6">2 5</E><E ID="7">3 5</E><E ID="8">4 5</E></EDGE>
<FACE><Q ID="0">0 1 2 3</Q><T ID="1">0 5 4</T><Q ID="2">1 6 8 5</Q><T ID="3">2 7 6</T><Q ID="4">3 7 8 4</Q></FACE>
<ELEMENT><R ID="7">0 1 2 3 4</R></ELEMENT></GEOMETRY></NEKTAR>)";
    std::ofstream(path)<<xml;
    std::vector<double> values={1,1,0,1,1,0,1,0,1,0,0,0};auto first=values;
    for(double x:first) values.push_back(2*x);
    std::ofstream(fld)<<"<NEKTAR><ELEMENTS FIELDS=\"p,q\" SHAPE=\"Prism\" BASIS=\"Modified_A,Modified_A,Modified_B\" NUMMODESPERDIR=\"UNIORDER:3,2,3\" ID=\"7\" COMPRESSED=\"B64Z-LittleEndian\">"<<CompressedField(values)<<"</ELEMENTS></NEKTAR>";
    // Older files use explicit LittleEndian text; current files may use LE.
    auto data=ReadNektar(path.string());Require(data.elements[0].id==7,"nonzero element ID lost");
    for(Vec3 p:{Vec3{-.5,.3,-.1},Vec3{-.9,-.5,1},Vec3{.9,.5,1}}) {
        Near(data.elements[0].Evaluate("p",p),1);Near(data.elements[0].Evaluate("q",p),2);
    }
    auto object=FileIO::ReadFile(fld.string());Require(object!=nullptr,"FLD FileIO dispatch failed");
    std::filesystem::remove(fld);
    auto reader=SpectralReaderCPU::New();reader->SetFilePath(path.string());Require(!reader->Execute(),"missing FLD companion accepted");
    std::filesystem::remove(path);
    std::cout<<"PASS Nektar prism, multi-field compression, nonzero IDs, singular vertex, FLD dispatch, missing companion rejection\n";
}
void SelfCheck() {
    for(bool big:{false,true}) for(int type:{1,3}) {
        auto bytes=Fixture(big,type);auto data=ReadJacobi(bytes.data(),bytes.size());
        Vec3 p{.2,-.3,.4};
        double expected=type==1?2+3*p[0]+5*p[1]+7*p[2]+11*p[0]*p[1]*p[2]:
            2+7*(.5+1.5*p[2])+5*p[1]+3*p[0]*(1-p[2]);
        Near(data.elements[0].Evaluate("scalar",p),expected);
        auto reader=SpectralReaderCPU::New();reader->SetMemoryBuffer(bytes.data(),bytes.size());
        reader->SetSamplingSubdivisions(2);Require(reader->Execute(),"synthetic DAT reader failed");
        auto mesh=DynamicCast<SpectralMesh>(reader->GetOutput());Require(mesh!=nullptr,"original expansions not retained");
        Require(mesh->GetNumberOfPoints()==(type==1?27:18),"sample point count");
        Require(mesh->GetNumberOfCells()==8,"sample cell count");
        Require(mesh->GetSpectralData().elements[0].fields.at("scalar").size()==(type==1?8:6),"coefficient retention");
        for(IGsize i=0;i<mesh->GetNumberOfCells();++i) {
            igIndex ids[8];mesh->GetCellPointIds(i,ids);
            // determinant of three incident edges: wedge ordering must not
            // collapse the apex into a degenerate hex or invert orientation.
            const auto a=mesh->GetPoint(ids[0]);
            auto x=mesh->GetPoint(ids[1])-a,y=mesh->GetPoint(ids[type==1?3:2])-a,z=mesh->GetPoint(ids[type==1?4:3])-a;
            double determinant=x[0]*(y[1]*z[2]-y[2]*z[1])-x[1]*(y[0]*z[2]-y[2]*z[0])+x[2]*(y[0]*z[1]-y[1]*z[0]);
            Require(determinant>0,"degenerate or inverted sample cell");
        }
        bytes.pop_back();reader->SetMemoryBuffer(bytes.data(),bytes.size());
        Require(!reader->Execute()&&reader->GetOutput()==nullptr,"truncated file leaked stale output");
    }
    // Nektar's i-fastest Modified_A ordering must reproduce this polynomial.
    Element e;e.nektar=true;e.modes={3,2,2};e.basis={Basis::ModifiedA,Basis::ModifiedA,Basis::ModifiedA};
    e.fields["p"]={1,2,4,1,2,4,1,2,4,1,2,4};
    Near(e.Evaluate("p",{.2,.3,.4}),1*(1-.2)/2+2*(1+.2)/2+4*(1-.2*.2)/4);
    NektarFixture();
    std::cout<<"PASS synthetic little/big endian hex/prism, modal ordering, topology, stale-output rejection\n";
}
std::string Utf8(const std::filesystem::path& path) {
    auto s=path.u8string();return std::string(s.begin(),s.end());
}
void CheckSamples(const std::filesystem::path& directory) {
    int count=0;
    for(const auto& entry:std::filesystem::directory_iterator(directory)) {
        auto path=entry.path();auto ext=path.extension().string();if(ext!=".dat"&&ext!=".xml") continue;
        auto reader=SpectralReaderCPU::New();reader->SetSamplingSubdivisions(2);
        auto object=reader->ReadFile(Utf8(path));auto mesh=DynamicCast<SpectralMesh>(object);
        Require(mesh&&mesh->GetNumberOfCells()>0,"sample failed: "+path.string());
        auto& data=mesh->GetSpectralData();Require(!data.elements.empty(),"empty original dataset");
        for(const auto& e:data.elements) for(const auto& f:data.fields) Require(std::isfinite(e.Evaluate(f,{.17,-.31,.49})),"non-finite sample evaluation");
        if(path.filename().string().rfind("Hex_01_01",0)==0) {
            const auto& e=data.elements[0];Vec3 p{.17,-.31,.49};auto xyz=e.Position(p);
            Near(e.Evaluate("p",p),xyz[0]*xyz[0]+xyz[1]*xyz[1]+xyz[2]*xyz[2],1e-8);
        }
        if(path.filename()=="Hex_CurvedFace.xml") Near(data.elements[0].Position({0,0,-1})[2],-.3);
        if(path.filename()=="Hex_Mushroom.xml") {
            auto xyz=data.elements[0].Position({-1,-1,0});Near(xyz[0],.3);Near(xyz[1],.3);Near(xyz[2],.5);
        }
        auto type=FileIO::GetFileType(Utf8(path));Require(type==(ext==".dat"?FileIO::SPECTRAL_DAT:FileIO::SPECTRAL_NEKTAR),"FileIO dispatch mismatch");
        if(ext==".xml") {
            auto fld=path;fld.replace_extension(".fld");auto other=ReadNektar(Utf8(fld));
            Near(other.elements[0].Evaluate(other.fields[0],{0,0,0}),data.elements[0].Evaluate(data.fields[0],{0,0,0}));
        }
        mesh->ConvertToDrawableData();Require(mesh->GetRenderableObject()!=nullptr,"drawable conversion failed");
        std::cout<<"PASS "<<path.filename().string()<<" elements="<<data.elements.size()<<" points="<<mesh->GetNumberOfPoints()<<" cells="<<mesh->GetNumberOfCells()<<'\n';++count;
    }
    Require(count>0,"sample directory contains no inputs");std::cout<<"PASS "<<count<<" spectral sample datasets\n";
}
void Render(const std::string& path,const std::string& snapshot) {
    auto object=FileIO::ReadFile(path);auto mesh=DynamicCast<SpectralMesh>(object);Require(mesh!=nullptr,"FileIO spectral read failed");
    Require(glfwInit()!=0,"GLFW initialization failed");
    if(!snapshot.empty()) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    auto window=RenderWindow::New();Require(window->GetRawWindowPtr()!=nullptr,"render context unavailable");
    window->SetSize(1000,760);window->SetTitle("OpeniGame - Spectral CPU");
    auto scene=Scene::New();window->SetScene(scene);scene->AddModel(object);
    mesh->ViewCloudPicture(scene,0,0);scene->ResetCameraView();scene->ResetCameraViewToIsometric();
    scene->SetColorBarVisible(true);
    if(mesh->GetSpectralData().elements[0].shape==Shape::Quadrilateral) scene->ResetCameraViewToPositiveZ();
    scene->SetAxesVisible(false);scene->SetCenterAxesVisible(false);
    auto interactor=Interactor::New();interactor->Initialize(scene);interactor->CreateDefaultStyle();window->SetInteractor(interactor);
    if(snapshot.empty()) window->Show();
    else {
        for(int i=0;i<4;++i) window->RenderOneFrame();
        // Scene::CaptureScreen's existing mirrored path assumes RGBA. Keep the
        // RGB capture unmirrored and reverse rows here to avoid that unrelated bug.
        auto pixels=scene->CaptureScreen(0,0,1000,760,GLFramebuffer::Type::RGB,false);
        Require(pixels.size()==1000*760*3,"capture failed");
        std::ofstream out(std::filesystem::u8path(snapshot),std::ios::binary);out<<"P6\n1000 760\n255\n";
        for(int row=759;row>=0;--row) out.write(reinterpret_cast<const char*>(pixels.data()+row*1000*3),1000*3);
        Require(bool(out),"snapshot write failed");
        std::cout<<"PASS rendered "<<path<<" -> "<<snapshot<<'\n';
    }
}
}
int main(int argc,char** argv) {
    try {
        if(argc>=3&&std::string(argv[1])=="--samples") CheckSamples(std::filesystem::u8path(argv[2]));
        else if(argc>=3&&std::string(argv[1])=="--render") Render(argv[2],argc>=4?argv[3]:"");
        else SelfCheck();
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
