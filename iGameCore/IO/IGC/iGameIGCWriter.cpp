#include "iGameIGCWriter.h"
#include "../../Filters/iGameMeshCodec/iGameMeshLoomEncoder.h"
#include "../../Filters/iGameMeshCodec/iGameMeshDecodedDataObject.h"
#include <fstream>

IGAME_NAMESPACE_BEGIN

bool IGCWriter::GenerateBuffers() {
    return true;
}

bool IGCWriter::WriteToFile(std::string saveFilePath, DataObject::Pointer dataObj, UIControlParams uiConParams) {
    if (!dataObj) {
        return false;
    }
    
    if (saveFilePath.empty()) {
        return false;
    }
    
    auto decodedData = MeshDecodedDataObject::New();
    decodedData->SetMeshData(dataObj);
    decodedData->SetUIControlParams(uiConParams);
    decodedData->SetFilePath(saveFilePath);
    
    auto encoder = MeshLoomEncoder::New();
    encoder->SetInput(decodedData);
    
    if (!encoder->Execute()) {
        return false;
    }
    
    return true;
}

IGAME_NAMESPACE_END
