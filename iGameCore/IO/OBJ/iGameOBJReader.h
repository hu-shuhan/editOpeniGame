#ifndef iGameOBJReader_h
#define iGameOBJReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

class OBJReader : public FileReader {
public:
	I_OBJECT(OBJReader);
	static Pointer New() { return new OBJReader; }

	bool Parsing() override;

protected:
	OBJReader() = default;
	~OBJReader() override = default;

};

IGAME_NAMESPACE_END
#endif