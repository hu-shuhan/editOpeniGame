#ifndef iGameIGCWriter_h
#define iGameIGCWriter_h

#include "iGameFileWriter.h"
#include "../Filters/iGameMeshCodec/iGameMeshCodecParamSet.h"

IGAME_NAMESPACE_BEGIN

class IGCWriter : public FileWriter {
public:
    I_OBJECT(IGCWriter);
    static Pointer New() { return new IGCWriter; }

    bool GenerateBuffers() override;
    bool WriteToFile(std::string saveFilePath, DataObject::Pointer dataObj, UIControlParams uiConParams);

protected:
    IGCWriter() = default;
    ~IGCWriter() override = default;
};

IGAME_NAMESPACE_END
#endif