#ifndef iGameMeshDecodedDataObject_h
#define iGameMeshDecodedDataObject_h

#include "iGameDataObject.h"
#include "iGameMeshCodecParamSet.h"

IGAME_NAMESPACE_BEGIN

class MeshDecodedDataObject : public DataObject {
public:
    I_OBJECT(MeshDecodedDataObject);
    static Pointer New() { return new MeshDecodedDataObject; }

    IGenum GetDataObjectType() const override { return IG_MESH_DECODED_DATA; }

    void SetMeshData(DataObject::Pointer meshData) { 
        m_MeshData = meshData; 
    }

    DataObject::Pointer GetMeshData() const { 
        return m_MeshData; 
    }

    void SetUIControlParams(const UIControlParams& params) { 
        m_UIParams = params; 
        m_HasUIParams = true; 
    }

    const UIControlParams& GetUIControlParams() const { 
        return m_UIParams; 
    }

    bool HasUIControlParams() const { 
        return m_HasUIParams; 
    }

    void ClearUIControlParams() { 
        m_HasUIParams = false; 
    }

    void SetFilePath(const std::string& filePath) { 
        m_FilePath = filePath; 
    }

    const std::string& GetFilePath() const { 
        return m_FilePath; 
    }

    bool HasFilePath() const { 
        return !m_FilePath.empty(); 
    }

    bool IsValid() const { 
        return m_MeshData != nullptr; 
    }

protected:
    MeshDecodedDataObject() = default;
    ~MeshDecodedDataObject() override = default;

private:
    DataObject::Pointer m_MeshData = nullptr;
    UIControlParams m_UIParams;
    bool m_HasUIParams = false;
    std::string m_FilePath;
};

IGAME_NAMESPACE_END
#endif