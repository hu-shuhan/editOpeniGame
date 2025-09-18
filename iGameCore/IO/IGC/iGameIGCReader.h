#ifndef iGameIGCReader_h
#define iGameIGCReader_h

#include "../../Filters/iGameMeshCodec/iGameEncodedMeshData.h"
#include "../../Filters/iGameMeshCodec/iGameMeshDecoder.h"
#include "iGameFileReader.h"
#include <fstream>
#include <iostream>

IGAME_NAMESPACE_BEGIN

class IGCReader : public FileReader {
public:
	I_OBJECT(IGCReader);
	static Pointer New() { return new IGCReader; }

    bool Parsing() override;
    bool CreateDataObject() override;

protected:
	IGCReader() = default;
	~IGCReader() override = default;

private:
    DataObject::Pointer m_DecodedOutput;
    
    // 分离的解析方法
    bool ParsingWithMemoryMapping();
    bool ParsingWithFilePath();
    EncodedMeshData::Pointer CreateEncodedDataFromFile();
};

IGAME_NAMESPACE_END
#endif
