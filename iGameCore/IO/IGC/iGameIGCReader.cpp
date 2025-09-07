#include "iGameIGCReader.h"

IGAME_NAMESPACE_BEGIN

bool IGCReader::Parsing()
{
    return ParsingWithMemoryMapping();
}

bool IGCReader::ParsingWithMemoryMapping()
{
    MeshDecoder::Pointer decoder = MeshDecoder::New();

    decoder->SetMemoryMappingData(this->FILESTART, 
                                  this->IS, 
                                  this->FILEEND, 
                                  this->m_FileSize);
    
    if (!decoder->Execute()) {
        return false;
    }
    
    m_DecodedOutput = decoder->GetOutput();
    return true;
}

bool IGCReader::ParsingWithFilePath()
{
    MeshDecoder::Pointer decoder = MeshDecoder::New();

    EncodedMeshData::Pointer encodedData = CreateEncodedDataFromFile();
    if (!encodedData) {
        return false;
    }

    decoder->SetInput(0, encodedData);
    
    if (!decoder->Execute()) {
        return false;
    }
    
    m_DecodedOutput = decoder->GetOutput();
    return true;
}

EncodedMeshData::Pointer IGCReader::CreateEncodedDataFromFile()
{
    std::ifstream file(m_FilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << m_FilePath << std::endl;
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize)) {
        std::cerr << "Failed to read file: " << m_FilePath << std::endl;
        return nullptr;
    }
    file.close();

    EncodedMeshData::Pointer encodedData = EncodedMeshData::New();
    encodedData->m_Buffers.resize(fileSize);
    std::memcpy(encodedData->m_Buffers.data(), buffer.data(), fileSize);
    
    return encodedData;
}

bool IGCReader::CreateDataObject()
{
    if (m_DecodedOutput) {
        m_Output = m_DecodedOutput;
        return true;
    }
    return false;
}

IGAME_NAMESPACE_END
