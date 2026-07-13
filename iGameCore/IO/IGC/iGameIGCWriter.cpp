#include "iGameIGCWriter.h"
#include "Log/iGameLogger.h"

IGAME_NAMESPACE_BEGIN

using EncoderType = MeshEncoderFilter<EncodeOutputBinaryArray>;

bool IGCWriter::GenerateBuffers()
{
    IGAME_CORE_INFO("[IGCWriter] GenerateBuffers begin dataObject={}",
                    static_cast<const void*>(m_DataObject.GetPointer()));
    if (!m_DataObject) {
        IGAME_CORE_ERROR("[IGCWriter] GenerateBuffers abort: input data object is null");
        return false;
    }

    if (!EncodeData()) {
        IGAME_CORE_ERROR("[IGCWriter] GenerateBuffers abort: EncodeData failed");
        return false;
    }

    const bool ok = GenerateOutput();
    IGAME_CORE_INFO("[IGCWriter] GenerateBuffers end success={} buffers={}",
                    ok, m_Buffers.size());
    return ok;
}

bool IGCWriter::EncodeData()
{
    IGAME_CORE_INFO("[IGCWriter] EncodeData begin");
    m_encoder = EncoderType::New();
    m_encoder->SetInput(0, m_DataObject);
    m_encoder->SetEncodeTrace(m_EncodeTrace);

    if (m_hasCodecParams) {
        m_encoder->SetCodecControlParams(m_CodecParams);
    } else {
        auto codecParams = EncoderType::GenerateDefaultCodecParams(m_DataObject);
        m_encoder->SetCodecControlParams(codecParams);
    }

    const bool ok = m_encoder->Execute();
    IGAME_CORE_INFO("[IGCWriter] EncodeData end success={} output={}",
                    ok,
                    static_cast<const void*>(m_encoder->GetOutput(0).GetPointer()));
    return ok;
}

bool IGCWriter::GenerateOutput()
{
    IGAME_CORE_INFO("[IGCWriter] GenerateOutput begin");
    const auto& encoderOutput =
        DynamicCast<EncodeOutputBinaryArray>(m_encoder->GetOutput(0));
    if (!encoderOutput || encoderOutput->GetSize() == 0) {
        IGAME_CORE_ERROR("[IGCWriter] GenerateOutput abort: encoder output is null or empty");
        return false;
    }

    IGsize totalSize = encoderOutput->GetSize();

    m_Buffers.resize(1, nullptr);
    m_Buffers[0] = CharArray::New();
    m_Buffers[0]->Resize(totalSize);

    char* dest = m_Buffers[0]->RawPointer();
    std::memcpy(dest, encoderOutput->GetData(), totalSize);
    IGAME_CORE_INFO("[IGCWriter] GenerateOutput end bytes={}", totalSize);
    return true;
}

IGAME_NAMESPACE_END
