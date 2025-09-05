#ifndef iGameIGCReader_h
#define iGameIGCReader_h

#include "../../Filters/iGameMeshCodec/iGameMeshDecoder.h"
#include "iGameFileReader.h"

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
};

IGAME_NAMESPACE_END
#endif
