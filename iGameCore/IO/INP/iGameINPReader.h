//还没有完全实现
#ifndef iGameINPReader_h
#define iGameINPReader_h

#include "iGameFileReader.h"
#include <iGameVolumeMesh.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN

class INPReader :public FileReader {
public:
	I_OBJECT(INPReader);
	static INPReader* New() { return new INPReader; }
	~INPReader();

	bool Parsing() override;
	bool CreateDataObject() override;
	
	bool ChangeTypeToMytype(char* Type, int& vcnt, int& MyType);
	DataObject::Pointer GetOutput() override;

protected:
	INPReader();
private:
	Points::Pointer m_Points{ nullptr };
	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
	StructuredMesh::Pointer m_StructuredMesh{ nullptr };
	VolumeMesh::Pointer m_VolumeMesh{ nullptr };
	DataObject::Pointer m_ParentObject{ nullptr };
	int m_DataObjectType = IG_NONE;
};

IGAME_NAMESPACE_END
#endif