#ifndef iGameIGCWriter_h
#define iGameIGCWriter_h

#include "iGameFileWriter.h"
#include "iGameMeshCodec/iGameMeshEncoder.h"

IGAME_NAMESPACE_BEGIN

class IGCWriter : public FileWriter {
public:
    I_OBJECT(IGCWriter);
    static Pointer New() { return new IGCWriter; }

    bool GenerateBuffers() override;

    void SetUIControlParams(const UIControlParams& params) {
        m_hasUIParams = true;
        m_UIParams = params;
    }

    std::vector<std::pair<std::string, std::string>> GetReport() const {
        if (m_encoder) {
            return m_encoder->GetReport();
        }
        return {};
    }

protected:
    IGCWriter() = default;
    ~IGCWriter() override = default;
    
private:
    bool m_hasUIParams = false;
    UIControlParams m_UIParams;

    MeshEncoder::Pointer m_encoder;
};

IGAME_NAMESPACE_END
#endif