#ifndef MeshDecoder_h
#define MeshDecoder_h

#include "iGameFilter.h"
#include "iGameMacro.h"
#include <string>
#include "iGameThreadPool.h"
#include "meshoptimizer.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptDecoder.h"
#include <numeric>

#include "iGameMeshOptEncoder.h"

IGAME_NAMESPACE_BEGIN
class MeshDecoder : public Filter {
public:
    I_OBJECT(MeshDecoder);
    static Pointer New() { return new MeshDecoder; }

    MeshDecoder() {};

    bool Execute() override
    {
        if (!m_FilePath.empty() && !this->OpenStream(m_FilePath)) { return false; }
        // ---------------------------------------------------------
        
        // ½âÂë
        MeshOptParameters params;
        MeshOptDecoder decoder(this->m_BytestreamFile, params);
        DataObject::Pointer dataObj = decoder.Execute();
        this->SetNumberOfOutputs(1);
        this->SetOutput(dataObj);

        // ---------------------------------------------------------
        this->closeStream();

        return false;
    }

    DataObject::Pointer ReadFile(const std::string& filePath) {
        if (!this->OpenStream(filePath)) { return nullptr; }
        // ---------------------------------------------------------

        // ½âÂë
        MeshOptParameters params;
        MeshOptDecoder decoder(this->m_BytestreamFile, params);
        DataObject::Pointer dataObj = decoder.Execute();
        this->SetNumberOfOutputs(1);
        this->SetOutput(dataObj);

        // ---------------------------------------------------------
        this->closeStream();
        return dataObj;
    }

    void SetFilePath(const std::string& filePath) { 
        m_FilePath = filePath;
    }

private:
    std::ifstream m_BytestreamFile;
    std::string m_FilePath;

    bool OpenStream(std::string path)
    {
        this->m_BytestreamFile.open(path, std::ios::binary);
        if (!this->m_BytestreamFile.is_open()) {
            return false;
        }
        return true;
    }

    void closeStream()
    {
        this->m_BytestreamFile.clear();
        this->m_BytestreamFile.seekg(0, std::ios_base::end);
        //std::cout << "Total bitstream size " << this->m_BytestreamFile.tellg() << " B" << std::endl;
    }
};

IGAME_NAMESPACE_END
#endif