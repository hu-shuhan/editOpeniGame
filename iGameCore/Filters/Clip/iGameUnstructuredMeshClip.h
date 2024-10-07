#ifndef iGameUnstructuredMeshClip_h
#define iGameUnstructuredMeshClip_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"


IGAME_NAMESPACE_BEGIN
class UnstructuredMeshClip : public Filter {

public:
	I_OBJECT(UnstructuredMeshClip);
	static Pointer New() { return new UnstructuredMeshClip; }
	~UnstructuredMeshClip();

	bool Execute()override;

	float GetCutValue(float x[3]) {
		return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
			this->m_Normal[2] * (x[2] - this->m_Origin[2]));
	}
	float GetCutValue(float x0, float x1, float x2) {
		return (this->m_Normal[0] * (x0 - this->m_Origin[0]) + this->m_Normal[1] * (x1 - this->m_Origin[1]) +
			this->m_Normal[2] * (x2 - this->m_Origin[2]));
	}
	void SetPlane(float n[3], float o[3]) {
		m_Normal[0] = n[0];
		m_Normal[1] = n[1];
		m_Normal[2] = n[2];
		m_Origin[0] = o[0];
		m_Origin[1] = o[1];
		m_Origin[2] = o[2];
	}
protected:
	UnstructuredMeshClip();

	float m_CutPlane[4];
	float m_Normal[3];
	float m_Origin[3];
	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
};
IGAME_NAMESPACE_END
#endif