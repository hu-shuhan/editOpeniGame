#if defined(CGNS_ENABLE)
#ifndef iGameCGNSReader_h
#define iGameCGNSReader_h

#include <iGameFilter.h>
#include <string>
#include <iGameMacro.h>
#include <iGameVolumeMesh.h>
#include <iGameStructuredMesh.h>
#include <iGameUnStructuredMesh.h>
#include <cgns_io.h>
#include <cgnslib.h>
#include <unordered_set>
IGAME_NAMESPACE_BEGIN
class  iGameCGNSReader : public Filter {

public:
	I_OBJECT(iGameCGNSReader);
	static 	iGameCGNSReader* New() { return new iGameCGNSReader; }

	~iGameCGNSReader();
	DataObject::Pointer ReadFile(std::string name);

	void GenStructuredCellConnectivities(cgsize_t cellDim, cgsize_t* size);
	void ReadPointCoordinates(int pointNum, int positionDim, int index_file, int index_base, int index_zone, cgsize_t* size);
	void ReadUnstructuredCellConnectivities(int index_file, int index_base, int  index_zone, cgsize_t cellNum);
	void ReadFields(int index_file, int index_base, int  index_zone, int index_sol, ZoneType_t zoneType, int celldim, GridLocation_t location, cgsize_t* size);

	void ChangeMixElementToMyCell(std::vector<cgsize_t>, int);

	DataObject::Pointer GetOutput() {
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
private:
	iGameCGNSReader();
	Points::Pointer m_Points{ nullptr };
	StructuredMesh::Pointer m_StructuredMesh{ nullptr };
	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
	VolumeMesh::Pointer m_VolumeMesh{ nullptr };
	AttributeSet::Pointer m_AttributeSet{ nullptr };
	DataObject::Pointer m_ParentObject{ nullptr };
	igIndex m_DataObjectType = IG_NONE;
	std::unordered_set<std::string >BoundryNames;
};

IGAME_NAMESPACE_END
#endif // iGameCGNSReader_h
#endif