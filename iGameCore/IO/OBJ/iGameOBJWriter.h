#ifndef iGameOBJWriter_h
#define iGameOBJWriter_h

#include "iGameFileWriter.h"
#include <vector>
#include <cstdint>

IGAME_NAMESPACE_BEGIN

class OBJWriter : public FileWriter {
public:
	I_OBJECT(OBJWriter);
	static Pointer New() { return new OBJWriter; }

	bool GenerateBuffers() override;

	// 将OBJ内容直接写入内存，避免磁盘IO
	bool WriteToMemory(DataObject::Pointer dataObject, std::vector<uint8_t>& outData);

	const void WritePointsToBuffer(CharArray::Pointer&);
	const void WriteFacesToBuffer(CharArray::Pointer&);
protected:
	OBJWriter() = default;
	~OBJWriter() override = default;
	SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };
};

IGAME_NAMESPACE_END
#endif