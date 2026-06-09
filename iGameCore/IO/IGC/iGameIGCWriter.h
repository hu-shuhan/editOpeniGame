#ifndef iGameIGCWriter_h
#define iGameIGCWriter_h

#include "MeshCodec/iGameMeshEncoderFilter.h"
#include "iGameFileWriter.h"

IGAME_NAMESPACE_BEGIN

class IGCWriter : public FileWriter {
public:
    I_OBJECT(IGCWriter);
    static Pointer New() { return new IGCWriter; }

    bool GenerateBuffers() override;

    void SetCodecControlParams(const CodecControlParams& params) {
        m_hasCodecParams = true;
        m_CodecParams = params;
    }

    std::vector<std::pair<std::string, std::string>> GetReport() const {
        if (m_encoder) { return m_encoder->GetReport(); }
        return {};
    }

    void SetEncodeTrace(MeshEncodeTrace* trace) {
        m_EncodeTrace = trace;
    }

protected:
    IGCWriter() = default;
    ~IGCWriter() override = default;

private:
    bool m_hasCodecParams = false;
    CodecControlParams m_CodecParams;

    MeshEncoderFilter<EncodeOutputBinaryArray>::Pointer m_encoder;
    MeshEncodeTrace* m_EncodeTrace = nullptr;

    // helper methods
    bool EncodeData();
    bool GenerateOutput();
};

IGAME_NAMESPACE_END
#endif
