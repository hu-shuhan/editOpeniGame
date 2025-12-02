#ifndef iGameIGCReader_h
#define iGameIGCReader_h

#include "../../Filters/MeshCodec/DecodeInput/iGameDecodeInputBinaryArray.h"
#include "MeshCodec/iGameMeshDecoderFilter.h"
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
    std::vector<unsigned char> m_FileBuffer;

    // 分离的解析方法
    bool ParsingWithMemoryMapping();
    bool ParsingWithFilePath();
    DecodeInputBinaryArray::Pointer CreateDecoderInputFromFile();
};

IGAME_NAMESPACE_END
#endif
