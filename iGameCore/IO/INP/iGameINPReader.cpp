#include "iGameINPReader.h"
#include "Log/iGameLogger.h"
IGAME_NAMESPACE_BEGIN

INPReader::INPReader()
{

}
INPReader::~INPReader()
{

}
bool INPReader::CreateDataObject() {
	if (!this->GetOutput())return false;
	m_Output = this->GetOutput();
	return true;
}
bool INPReader::Parsing()
{

	char buffer[256];
	while (1) {
		if (!this->ReadString(buffer))break;
		this->LowerCase(buffer);
		if (!strncmp(buffer, "*part,", 6)) {
			if (!this->ReadString(buffer) || strncmp(LowerCase(buffer), "name=", 5)) {
				igError("read part name error!");
				return false;
			}
			char partName[256];
			for (int i = 5;; i++) {
				partName[i - 5] = buffer[i];
				if (buffer[i] == '\0' || buffer[i] == '\r' || buffer[i] == '\n')break;
			}
            IGAME_CORE_DEBUG("Read part with name = {}", partName);
		}
		//read nodes
		else if (!strncmp(buffer, "*node", 5)) {
			igIndex id = 0;
			Point p;
			m_Points = Points::New();
			while (this->Read(&id))
			{
				for (int i = 0; i < 3; i++) {
					this->IS++;
					this->Read(&p[i]);
				}
				m_Points->AddPoint(p);
				while (*this->IS == ' ' || *this->IS == '\n' || *this->IS == '\r')this->IS++;
				if (*this->IS == '*')break;
			}
		}
		//read elements
		else if (!strncmp(buffer, "*element,", 9)) {
			if (!this->ReadString(buffer) || strncmp(LowerCase(buffer), "type=", 5)) {
				igError("read part elements type error!");
				return false;
			}
			char Type[256];
			for (int i = 5;; i++) {
				Type[i - 5] = buffer[i];
				if (buffer[i] == '\0')break;
			}
			this->IS -= 5;
			this->ReadLine(buffer);
			igIndex id = 0;
			igIndex vhs[64];
			int cellVcnt = 0;
			m_UnstructuredMesh = UnstructuredMesh::New();
			m_UnstructuredMesh->SetPoints(m_Points);
			m_DataObjectType = IG_UNSTRUCTURED_MESH;
			int iGType = IG_NONE;
			if (!ChangeTypeToMytype(Type, cellVcnt, iGType)) {
				igError("Type Error!");
				return false;
			}
			while (this->Read(&id)) {
				for (int i = 0; i < cellVcnt; i++) {
					this->IS++;
					this->Read(&vhs[i]);
					vhs[i]--;
				}
				m_UnstructuredMesh->AddCell(vhs, cellVcnt, iGType);
				while (*this->IS == ' ' || *this->IS == '\n' || *this->IS == '\r')this->IS++;
				if (*this->IS == '*')break;
			}
		}
	}
	return true;
}
bool  INPReader::ChangeTypeToMytype(char* Type, int& vcnt, int& MyType)
{
	//Shell Elements
	if (!strncmp(Type, "s3", 2))
	{
		vcnt = 3;
		MyType = IG_TRIANGLE;
		return true;
	}
	if (!strncmp(Type, "s4", 2))
	{
		vcnt = 4;
		MyType = IG_QUAD;
		return true;
	}
	if (!strncmp(Type, "s8", 2))
	{
		vcnt = 8;
		MyType = IG_QUADRATIC_QUAD;
		return true;
	}
	//Continuum Elements 
	if (!strncmp(Type, "c3d8", 4))
	{
		vcnt = 8;
		MyType = IG_HEXAHEDRON;
		return true;
	}
	if (!strncmp(Type, "c3d6", 4))
	{
		vcnt = 6;
		MyType = IG_PRISM;
		return true;
	}
	if (!strncmp(Type, "c3d5", 4))
	{
		vcnt = 5;
		MyType = IG_PYRAMID;
		return true;
	}
	if (!strncmp(Type, "c3d4", 4))
	{
		vcnt = 4;
		MyType = IG_TETRA;
		return true;
	}
	if (!strncmp(Type, "c3d10", 5))
	{
		vcnt = 10;
		MyType = IG_QUADRATIC_TETRA;
		return true;
	}
	if (!strncmp(Type, "c3d20", 5))
	{
		vcnt = 20;
		MyType = IG_QUADRATIC_HEXAHEDRON;
		return true;
	}
	//Beam Elements
	if (!strncmp(Type, "b31", 3))
	{
		vcnt = 2;
		MyType = IG_LINE;
		return true;
	}
	if (!strncmp(Type, "b32", 3))
	{
		vcnt = 3;
		MyType = IG_QUADRATIC_EDGE;
		return true;
	}
	//Membrane Elements
	if (!strncmp(Type, "m3d4", 4))
	{
		vcnt = 4;
		MyType = IG_QUAD;
		return true;
	}
	//Plane Stress Elements
	if (!strncmp(Type, "cps4", 4))
	{
		vcnt = 4;
		MyType = IG_QUAD;
		return true;
	}
	if (!strncmp(Type, "cps8", 4))
	{
		vcnt = 8;
		MyType = IG_QUADRATIC_QUAD;
		return true;
	}
	//Plane Strain Elements
	if (!strncmp(Type, "cpe4", 4))
	{
		vcnt = 4;
		MyType = IG_QUAD;
		return true;
	}
	if (!strncmp(Type, "cpe8", 4))
	{
		vcnt = 8;
		MyType = IG_QUADRATIC_QUAD;
		return true;
	}
	return false;
}
DataObject::Pointer INPReader::GetOutput()
{
	switch (this->m_DataObjectType)
	{
	case IG_NONE:
		return nullptr;
	case IG_STRUCTURED_MESH:
		return m_StructuredMesh;
	case IG_UNSTRUCTURED_MESH:
		return m_UnstructuredMesh;
	case IG_VOLUME_MESH:
		return m_VolumeMesh;
	case IG_MULTIBLOCK_MESH:
		return m_ParentObject;
	default:
		return nullptr;
	}
}
IGAME_NAMESPACE_END
