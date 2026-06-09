#include "iGameIGCWriter.h"

IGAME_NAMESPACE_BEGIN

using EncoderType = MeshEncoderFilter<EncodeOutputBinaryArray>;

bool IGCWriter::GenerateBuffers()
{
    if (!m_DataObject) {
        return false;
    }

    if (!EncodeData()) {
        return false;
    }

    return GenerateOutput();
}

bool IGCWriter::EncodeData()
{
    m_encoder = EncoderType::New();
    m_encoder->SetInput(0, m_DataObject);
    m_encoder->SetEncodeTrace(m_EncodeTrace);

    if (m_hasCodecParams) {
        m_encoder->SetCodecControlParams(m_CodecParams);
    } else {
        auto codecParams = EncoderType::GenerateDefaultCodecParams(m_DataObject);
        m_encoder->SetCodecControlParams(codecParams);
    }

    return m_encoder->Execute();
}

bool IGCWriter::GenerateOutput()
{
    const auto& encoderOutput =
        DynamicCast<EncodeOutputBinaryArray>(m_encoder->GetOutput(0));
    if (!encoderOutput || encoderOutput->GetSize() == 0) {
        return false;
    }

    IGsize totalSize = encoderOutput->GetSize();

    m_Buffers.resize(1, nullptr);
    m_Buffers[0] = CharArray::New();
    m_Buffers[0]->Resize(totalSize);

    char* dest = m_Buffers[0]->RawPointer();
    std::memcpy(dest, encoderOutput->GetData(), totalSize);
    
    return true;
}

IGAME_NAMESPACE_END
