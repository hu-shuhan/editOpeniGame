#include "iGameIGCWriter.h"

IGAME_NAMESPACE_BEGIN

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
    m_encoder = MeshEncoder::New();
    m_encoder->SetInput(0, m_DataObject);

    if (m_hasUIParams) {
        m_encoder->SetUIControlParams(m_UIParams);
    } else {
        auto uiParams = MeshEncoder::GenUiControlParams(m_DataObject);
        m_encoder->SetUIControlParams(uiParams);
    }

    return m_encoder->Execute();
}

bool IGCWriter::GenerateOutput()
{
    const auto& encodedData = DynamicCast<EncodedMeshData>(m_encoder->GetOutput(0));
    if (!encodedData || encodedData->m_Buffers.empty()) {
        return false;
    }

    IGsize totalSize = encodedData->m_Buffers.size();
    if (totalSize == 0) {
        return false;
    }

    m_Buffers.resize(1, nullptr);
    m_Buffers[0] = CharArray::New();
    m_Buffers[0]->Resize(totalSize);

    char* dest = m_Buffers[0]->RawPointer();
    std::memcpy(dest, encodedData->m_Buffers.data(), totalSize);
    
    return true;
}

IGAME_NAMESPACE_END
