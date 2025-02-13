#ifndef iGameContourFilter_h
#define iGameContourFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameCellContour.h"
IGAME_NAMESPACE_BEGIN

class ContourFilter : public Filter {

public:
	I_OBJECT(ContourFilter);
	static Pointer New() { return new ContourFilter; }
	~ContourFilter();

	bool Execute()override;

	//Returns the converted output mesh, resulting in an unstructured mesh
	UnstructuredMesh::Pointer GetContourMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); };

	// To set equivalent data, you need to enter the corresponding equivalent data and data objects
	// the dimension means extract the dimension of datas, dimension = 0 means the first dimension
	void SetIsoScalarData(ArrayObject::Pointer array, double value, int dimension = 0);

	//used to show slice
    bool SetPlane(double o[3],double n[3]);
protected:
	ContourFilter();

	DoubleArray::Pointer m_PointValues{nullptr};
	ArrayObject::Pointer m_SelectedScalar{nullptr};
	double m_SeletectDimension{0.0};

	double m_ContourValue{0.0};

	virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
	virtual bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
	virtual bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);
	virtual bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
	void ComputePointValueAndCellVisible(Points::Pointer, CellArray::Pointer, DoubleArray::Pointer, CharArray::Pointer);
	void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData, AttributeSet::Pointer outData,
		std::vector<CellContour::InterpolateEdge>OriginEdge, std::vector<igIndex> OriginCell);
private:

};
IGAME_NAMESPACE_END
#endif