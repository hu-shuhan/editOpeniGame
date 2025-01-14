#ifndef iGameOBJWriter_h
#define iGameOBJWriter_h

#include "iGameFileWriter.h"

IGAME_NAMESPACE_BEGIN

class OBJWriter : public FileWriter {
public:
	I_OBJECT(OBJWriter);
	static Pointer New() { return new OBJWriter; }

	bool GenerateBuffers() override;

	const void WritePointsToBuffer(CharArray::Pointer&);
	const void WriteFacesToBuffer(CharArray::Pointer&);
protected:
	OBJWriter() = default;
	~OBJWriter() override = default;
	SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };
};

IGAME_NAMESPACE_END
#endif