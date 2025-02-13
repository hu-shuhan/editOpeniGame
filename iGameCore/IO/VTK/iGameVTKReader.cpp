#include "iGameVTKReader.h"
#include "iGameByteSwap.h"
#include "iGameUnstructuredMesh.h"
IGAME_NAMESPACE_BEGIN

bool VTKReader::CreateDataObject() {
	if (!this->GetOutput())return false;
	m_Output = this->GetOutput();
	return true;
}
bool VTKReader::Parsing() {
	int i, PointsNum = 0, CellsNum = 0;
	char line[256];
	int npts, size = 0, ncells = 0;
	int piece = 0, numPieces = 0, tmp = 0;
	IntArray::Pointer Types;
	IntArray::Pointer Cells;
	ArrayObject::Pointer CellsId;
	ArrayObject::Pointer CellsConnect;
	UnstructuredMesh::Pointer mesh = UnstructuredMesh::New();
	m_Output = mesh;
	ReadHeader();

	if (!this->ReadString(line)) {
		
		igError("Data file ends prematurely!");
		return false;
	}

	if (!strncmp(this->LowerCase(line), "dataset", 7)) {
		// Make sure we're reading right type of geometry
		//
		if (!this->ReadString(line)) {
			igError("Data file ends prematurely!");
			return false;
		}
		if (strncmp(this->LowerCase(line), "unstructured_grid", 17) == 0) {
			// Read unstructured grid specific stuff
			this->ReadUnstructuredGrid();
		}
		else if (strncmp(this->LowerCase(line), "polydata", 8) == 0) {
			// Read polydata
			this->ReadSurfaceMesh();
		}
		else if (strncmp(this->LowerCase(line), "structured_points", 17) == 0) {
			// Read structured grid
			this->ReadStructuredGrid();
		}
		else if (strncmp(this->LowerCase(line), "structured_grid", 15) == 0) {
			// Read structured grid
			this->ReadStructuredGrid();
		}
		else {
			igError("Cannot read dataset type: " << line);
			return false;
		}
	}
	else if (!strncmp(line, "point_data", 10)) {
		igDebug("No geometry defined in data file!");
		if (!this->Read(&PointsNum)) {
			igError("Cannot read point data!");
			return false;
		}
		this->ReadPointData(PointsNum);
	}
	else {
		igError("Unrecognized keyword: " << line);
	}
	if (m_Data.Data) {
		this->GetOutput()->SetAttributeSet(m_Data.GetData());
	}
	this->UpdateProgress(1.0);
	//igDebug("Read " << this->DataSet->GetNumberOfPoints() << " points,"
	//	<< this->DataSet->GetNumberOfCells() << " cells.\n");
	return true;
}

DataObject::Pointer VTKReader::GetOutput()
{
	switch (this->m_DataObjectType)
	{
	case IG_NONE:
		return nullptr;
	case IG_STRUCTURED_MESH:
		return m_StructuredMesh;
	case IG_UNSTRUCTURED_MESH:
		return m_UnstructuredMesh;
	case IG_SURFACE_MESH:
		return m_SurfaceMesh;
	default:
		return nullptr;
	}
}
IGAME_NAMESPACE_END