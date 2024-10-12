#ifndef iGameModelClip_h
#define iGameModelClip_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"


IGAME_NAMESPACE_BEGIN
class ModelClip : public Filter {

public:
	I_OBJECT(ModelClip);
	static Pointer New() { return new ModelClip; }
	~ModelClip();

	bool Execute()override;

	bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
	bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
	bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);

	float GetCutValue(float x[3]) {
		return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
			this->m_Normal[2] * (x[2] - this->m_Origin[2]));
	}
	float GetCutValue(float x0, float x1, float x2) {
		return (this->m_Normal[0] * (x0 - this->m_Origin[0]) + this->m_Normal[1] * (x1 - this->m_Origin[1]) +
			this->m_Normal[2] * (x2 - this->m_Origin[2]));
	}
	void SetPlane(float o[3], float n[3]) {
		m_Normal[0] = n[0];
		m_Normal[1] = n[1];
		m_Normal[2] = n[2];
		m_Origin[0] = o[0];
		m_Origin[1] = o[1];
		m_Origin[2] = o[2];
	}
    void GetPlane(float o[3], float n[3]) {
        n[0]= m_Normal[0];
        n[1]= m_Normal[1];
        n[2]= m_Normal[2];
        o[0]= m_Origin[0];
        o[1]= m_Origin[1];
        o[2]= m_Origin[2];
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
protected:
	ModelClip();

	float m_CutPlane[4];
	float m_Normal[3];
	float m_Origin[3];
	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
	SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };
	VolumeMesh::Pointer m_VolumeMesh{ nullptr };
	bool m_Slice = false;

private:

};
IGAME_NAMESPACE_END
#endif