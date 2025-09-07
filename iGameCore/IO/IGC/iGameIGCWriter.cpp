#include "iGameIGCWriter.h"

IGAME_NAMESPACE_BEGIN

bool IGCWriter::GenerateBuffers()
{
    if (!m_DataObject) {
        return false;
    }

    m_encoder = MeshEncoder::New();
    m_encoder->SetInput(0, m_DataObject);

    if (m_hasUIParams) {
        m_encoder->SetUIControlParams(m_UIParams);
    } else {
        auto uiParams = MeshEncoder::GenUiControlParams(m_DataObject);
        m_encoder->SetUIControlParams(uiParams);
    }

    if (!m_encoder->Execute()) {
        return false;
    }

    const auto& encoderBuffers = m_encoder->GetBuffers();
    if (encoderBuffers.empty()) {
        return false;
    }

    IGsize totalSize = 0;
    for (const auto& buffer : encoderBuffers) {
        if (buffer) {
            totalSize += buffer->GetNumberOfValues();
        }
    }
    
    if (totalSize == 0) {
        return false;
    }

    m_Buffers.resize(1, nullptr);
    m_Buffers[0] = CharArray::New();
    m_Buffers[0]->Resize(totalSize);

    IGsize offset = 0;
    char* dest = m_Buffers[0]->RawPointer();
    
    for (const auto& buffer : encoderBuffers) {
        if (buffer && buffer->GetNumberOfValues() > 0) {
            IGsize bufferSize = buffer->GetNumberOfValues();
            std::memcpy(dest + offset, buffer->RawPointer(), bufferSize);
            offset += bufferSize;
        }
    }
    
    return true;
}

IGAME_NAMESPACE_END
