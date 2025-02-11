#ifndef iGameModelClip_h
#define iGameModelClip_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameCellClip.h"

IGAME_NAMESPACE_BEGIN
class ModelClip : public Filter {

public:
	I_OBJECT(ModelClip);
	static Pointer New() { return new ModelClip; }
	~ModelClip();

	bool Execute()override;
	//Returns the converted output mesh, resulting in an unstructured mesh
	UnstructuredMesh::Pointer GetClipMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); };



	enum ClipMethod {
		IG_PLANE,
		IG_SCALAR
	};
	void SetClipMethod(ClipMethod CM);

	void SetPlane(float o[3], float n[3]);
	void SetPlane(double o[3], double n[3]);
	void SetIsoScalarData(ArrayObject::Pointer array, double value, int dimension = 0);
	void GetPlane(float o[3], float n[3]);
	void GetPlane(double o[3], double n[3]);
	void SetIsSlice(bool s);
	bool GetIsSlice();

protected:
	ModelClip();

	ClipMethod m_ClipMethod = IG_PLANE;
	double m_CutPlane[4];
	double m_Normal[3];
	double m_Origin[3];

	ArrayObject::Pointer m_SelectedScalar{ nullptr };
	int m_SeletectDimension = -1;
	double m_IsoValue = 0.0;

	bool m_Slice = false;
	bool m_InsideOut = true;

	void ComputePointValueAndCellVisible(Points::Pointer, CellArray::Pointer, DoubleArray::Pointer, CharArray::Pointer);
	void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData, AttributeSet::Pointer outData,
		std::vector<CellClip::InterpolateEdge>OriginEdge, std::vector<igIndex> OriginCell);
	virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
	virtual bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
	virtual bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);
	virtual bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
private:
	double GetCutValue(float x[3]);
	double GetCutValue(double x[3]);
	double GetCutValue(float x0, float x1, float x2);
	double GetCutValue(double x0, double x1, double x2);
	double GetCutValue(Point x);
	double GetPointValue(igIndex pId, Points::Pointer points);
};
IGAME_NAMESPACE_END
#endif