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

	bool ExecuteTest(DataObject::Pointer);
	bool ExecuteTest2(UnstructuredMesh::Pointer um);

	bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
	bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
	bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);
	bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);

	void ComputePointValueAndCellVisible(Points::Pointer, CellArray::Pointer, FloatArray::Pointer, CharArray::Pointer);
	void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData, AttributeSet::Pointer outData,
		std::vector<CellClip::InterpolateEdge>OriginEdge, std::vector<igIndex> OriginCell);

	enum ClipMethod {
		IG_PLANE,
		IG_SCALAR
	};
	void SetClipMethod(ClipMethod CM) {
		this->m_ClipMethod = CM;
	}
	float GetCutValue(float x[3]) {
		return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
			this->m_Normal[2] * (x[2] - this->m_Origin[2]));
	}
	float GetCutValue(float x0, float x1, float x2) {
		return (this->m_Normal[0] * (x0 - this->m_Origin[0]) + this->m_Normal[1] * (x1 - this->m_Origin[1]) +
			this->m_Normal[2] * (x2 - this->m_Origin[2]));
	}
	float GetCutValue(Point x) {
		return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
			this->m_Normal[2] * (x[2] - this->m_Origin[2]));
	}
	float GetPointValue(igIndex pId, Points::Pointer points) {
		switch (m_ClipMethod)
		{
		case iGame::ModelClip::IG_PLANE:
			return this->GetCutValue(points->GetPoint(pId));
			break;
		case iGame::ModelClip::IG_SCALAR:
			return this->m_SelectedScalar->GetElementValue(pId, m_SeletectDimension) - m_IsoValue;
			break;
		default:
			break;
		}
		return -1;
	}
	void SetPlane(float o[3], float n[3]) {
		double sum = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (sum < 1e-40) { sum = 1e-40; };
		m_Normal[0] = n[0] / sum;
		m_Normal[1] = n[1] / sum;
		m_Normal[2] = n[2] / sum;
		m_Origin[0] = o[0];
		m_Origin[1] = o[1];
		m_Origin[2] = o[2];
		this->SetClipMethod(IG_PLANE);
	}
	void SetPlane(double o[3], double n[3]) {
		double sum = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (sum < 1e-40) { sum = 1e-40; };
		m_Normal[0] = n[0] / sum;
		m_Normal[1] = n[1] / sum;
		m_Normal[2] = n[2] / sum;
		m_Origin[0] = o[0];
		m_Origin[1] = o[1];
		m_Origin[2] = o[2];
		this->SetClipMethod(IG_PLANE);
	}
	void SetIsoScalarData(ArrayObject::Pointer array, float value, int dimension = -1) {
		this->m_SelectedScalar = array;
		this->m_SeletectDimension = dimension;
		this->m_IsoValue = value;
		this->SetClipMethod(IG_SCALAR);
		this->m_Slice = true;
	}
	void GetPlane(float o[3], float n[3]) {
		n[0] = m_Normal[0];
		n[1] = m_Normal[1];
		n[2] = m_Normal[2];
		o[0] = m_Origin[0];
		o[1] = m_Origin[1];
		o[2] = m_Origin[2];
	}
	void GetPlane(double o[3], double n[3]) {
		n[0] = m_Normal[0];
		n[1] = m_Normal[1];
		n[2] = m_Normal[2];
		o[0] = m_Origin[0];
		o[1] = m_Origin[1];
		o[2] = m_Origin[2];
	}
	void SetIsSlice(bool s) {
		this->m_Slice = s;
	}
	bool GetIsSlice() {
		return this->m_Slice;
	}

protected:
	ModelClip();

	ClipMethod m_ClipMethod = IG_PLANE;
	float m_CutPlane[4];
	float m_Normal[3];
	float m_Origin[3];

	ArrayObject::Pointer m_SelectedScalar{ nullptr };
	int m_SeletectDimension = -1;
	float m_IsoValue = 0.0;

	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
	SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };
	VolumeMesh::Pointer m_VolumeMesh{ nullptr };
	bool m_Slice = false;

private:

};
IGAME_NAMESPACE_END
#endif