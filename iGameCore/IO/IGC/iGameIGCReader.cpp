#include "iGameIGCReader.h"
#include "Log/iGameLogger.h"
#include "MeshCodec/DecodeInput/iGameDecodeInputBinaryArray.h"
#include "MeshCodec/DecodeInput/iGameDecodeInputMappedMemory.h"
#include "MeshCodec/DecodeOutput/iGameGenericDecodeOutput.h"

IGAME_NAMESPACE_BEGIN

using DecoderOutputType = GenericDecodeOutput<DataObject::Pointer>;

bool IGCReader::Parsing() { return ParsingWithMemoryMapping(); }

bool IGCReader::ParsingWithMemoryMapping() {
    auto decoderInput = DecodeInputMappedMemory::New(this->FILESTART, this->IS, this->FILEEND, this->m_FileSize);

    auto adapter = std::make_unique<MeshDecodeAdapterToDataObject>();

    auto decoder = MeshDecoderFilter<DecoderOutputType>::New();
    decoder->SetAdapter(std::move(adapter));
    decoder->SetIGCLayout(m_IGCLayout);
    decoder->SetInput(0, decoderInput);

    if (!decoder->Execute()) { return false; }

    m_DecodedOutput = DynamicCast<DecoderOutputType>(decoder->GetOutput())->GetOutput();
    return true;
}

bool IGCReader::ParsingWithFilePath() {
    auto decoderInput = CreateDecoderInputFromFile();
    if (!decoderInput) { return false; }

    auto adapter = std::make_unique<MeshDecodeAdapterToDataObject>();

    auto decoder = MeshDecoderFilter<DecoderOutputType>::New();
    decoder->SetAdapter(std::move(adapter));
    decoder->SetIGCLayout(m_IGCLayout);
    decoder->SetInput(0, decoderInput);

    if (!decoder->Execute()) { return false; }

    m_DecodedOutput = DynamicCast<DecoderOutputType>(decoder->GetOutput())->GetOutput();
    return true;
}

DecodeInputBinaryArray::Pointer IGCReader::CreateDecoderInputFromFile() {
    std::ifstream file(m_FilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        IGAME_CORE_ERROR("Failed to open file: {}", m_FilePath);
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    m_FileBuffer.resize(fileSize);
    if (!file.read(reinterpret_cast<char*>(m_FileBuffer.data()), fileSize)) {
        IGAME_CORE_ERROR("Failed to read file: {}", m_FilePath);
        return nullptr;
    }
    file.close();

    return DecodeInputBinaryArray::New(m_FileBuffer.data(), m_FileBuffer.size());
}

bool IGCReader::CreateDataObject() {
    if (m_DecodedOutput) {
        m_Output = m_DecodedOutput;
        return true;
    }
    return false;
}

IGAME_NAMESPACE_END
