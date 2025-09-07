#include "iGameIGCReader.h"

IGAME_NAMESPACE_BEGIN

bool IGCReader::Parsing()
{
    MeshDecoder::Pointer decoder = MeshDecoder::New();
    
    decoder->SetMemoryMappedPointers(this->FILESTART, 
                                   this->IS, 
                                   this->FILEEND, 
                                   this->m_FileSize);
    
    if (!decoder->Execute()) {
        return false;
    }
    
    m_DecodedOutput = decoder->GetOutput();
    
    return true;
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
