#include "iGameSpectralReaderCPU.h"
#include "iGameSpectralMesh.h"
#include <filesystem>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>

IGAME_NAMESPACE_BEGIN
bool SpectralReaderCPU::Execute() {
    m_Output=nullptr;SetOutput(0,nullptr);m_SpectralData={};
    bool success=false;
    try {
        if(m_Subdivisions<0||m_Subdivisions>128) throw std::runtime_error("Spectral sampling subdivisions must be 0..128");
        success=Parsing()&&CreateDataObject();
        if(success) {
            auto path=std::filesystem::u8path(m_FilePath);
            auto name=path.stem().u8string();
            m_Output->SetName(std::string(name.begin(),name.end()));
            m_Output->GetProperties()->AddProperty(Variant::String,"FilePath")->SetValue(m_FilePath);
            m_Output->GetProperties()->AddProperty(Variant::LongLong,"FileSize")->SetValue(static_cast<long long>(m_FileSize));
            SetOutput(0,m_Output);
        }
    } catch(const std::exception& error) {
        success=false;
        IGAME_CORE_ERROR("[SpectralReaderCPU] {}: {}",m_FilePath,error.what());
        m_Output=nullptr;SetOutput(0,nullptr);m_SpectralData={};
    }
    m_Progress=0;m_ProgressShift=0;m_ProgressScale=1;
    if(m_ProgressObserver) {m_ProgressObserver->UpdateProgress(0);m_ProgressObserver->UpdateText("");}
    return success;
}
bool SpectralReaderCPU::Parsing() {
    if(m_UseMemoryBuffer) {
        m_FileSize=m_MemoryBufferSize;
        m_SpectralData=Spectral::ReadJacobi(m_MemoryBuffer,m_MemoryBufferSize);
    } else {
        auto path=std::filesystem::u8path(m_FilePath);
        m_FileSize=static_cast<size_t>(std::filesystem::file_size(path));
        auto ext=path.extension().string();
        std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return std::tolower(c);});
        if(ext==".xml"||ext==".fld") m_SpectralData=Spectral::ReadNektar(m_FilePath);
        else {
            std::ifstream in(path,std::ios::binary);
            if(!in) throw std::runtime_error("Cannot open spectral file");
            std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});
            m_SpectralData=Spectral::ReadJacobi(bytes.data(),bytes.size());
        }
    }
    UpdateProgress(0.3);return true;
}
bool SpectralReaderCPU::CreateDataObject() {
    // igIndex is signed and the renderer uses 32-bit indices. Preflight before
    // allocating a large sampled mesh; never silently reduce requested quality.
    size_t count=0;
    for(const auto& e:m_SpectralData.elements) {
        size_t n=m_Subdivisions?m_Subdivisions:std::max(2,*std::max_element(e.modes.begin(),e.modes.end()));
        count+=e.shape==Spectral::Shape::Prism?(n+1)*(n+1)*(n+2)/2:
            (n+1)*(n+1)*(e.shape==Spectral::Shape::Quadrilateral?1:n+1);
    }
    if(count>20000000||count>static_cast<size_t>(std::numeric_limits<igIndex>::max()))
        throw std::runtime_error("Spectral display mesh exceeds 20 million points; reduce sampling subdivisions");
    auto mesh=SpectralMesh::New();auto points=Points::New();points->Reserve(count);
    std::vector<DoubleArray::Pointer> fields;
    for(const auto& name:m_SpectralData.fields) {
        auto array=DoubleArray::New();array->SetName(name);array->SetDimension(1);array->Reserve(count);
        mesh->GetAttributeSet()->AddScalar(IG_POINT,array);fields.push_back(array);
    }
    auto elementIds=IntArray::New();elementIds->SetName("SpectralElementId");elementIds->SetDimension(1);
    Spectral::Sample(m_SpectralData,m_Subdivisions,
        [&](const Spectral::Vec3& p,const std::vector<double>& values) {
            double xyz[3]={p[0],p[1],p[2]};points->AddPoint(xyz);
            for(size_t i=0;i<fields.size();++i) fields[i]->AddValue(values[i]);
        },
        [&](Spectral::Shape shape,const std::vector<size_t>& indices,int id) {
            igIndex ids[8];for(size_t i=0;i<indices.size();++i) ids[i]=static_cast<igIndex>(indices[i]);
            mesh->AddCell(ids,static_cast<int>(indices.size()),shape==Spectral::Shape::Hexahedron?IG_HEXAHEDRON:
                          shape==Spectral::Shape::Prism?IG_PRISM:IG_QUAD);
            elementIds->AddValue(id);
        });
    mesh->SetPoints(points);mesh->GetAttributeSet()->AddScalar(IG_CELL,elementIds);
    // The extracted rendering shell shares this mapper. Initialise it from the
    // completed field array so it never starts with an unrelated default range.
    if(!fields.empty()) mesh->GetColorMapper()->InitRange(fields[0],0);
    mesh->SetSpectralData(std::move(m_SpectralData));mesh->SetViewStyle(IG_SURFACE);
    m_Output=mesh;UpdateProgress(1);return true;
}
IGAME_NAMESPACE_END
