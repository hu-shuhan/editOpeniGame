#ifndef iGameIGCReader_h
#define iGameIGCReader_h

#include "iGameFileReader.h"
#include "../../Filters/iGameMeshCodec/iGameMeshEncodedDataObject.h"
#include "../../Filters/iGameMeshCodec/iGameMeshDecodedDataObject.h"
#include "../../Filters/iGameMeshCodec/iGameMeshLoomDecoder.h"

IGAME_NAMESPACE_BEGIN

class IGCReader : public FileReader {
public:
	I_OBJECT(IGCReader);
	static Pointer New() { return new IGCReader; }

    bool Execute() override;
    bool Parsing() override;

protected:
	IGCReader() = default;
	~IGCReader() override = default;
};

IGAME_NAMESPACE_END
#endif
