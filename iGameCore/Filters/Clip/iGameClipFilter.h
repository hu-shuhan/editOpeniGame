#ifndef iGameClipFilter_h
#define iGameClipFilter_h

#include "iGameQuickModelClip.h"
IGAME_NAMESPACE_BEGIN
class ClipFilter : public Filter {

public:
	I_OBJECT(ClipFilter);
	static Pointer New() { return new ClipFilter; }
	~ClipFilter();

	bool Execute() override;
	//Returns the converted output mesh, resulting in an unstructured mesh
	UnstructuredMesh::Pointer GetClipMesh() { return DynamicCast<UnstructuredMesh>(m_Clippper->GetOutput()); };


	void SetClipMethod(ModelClip::ClipMethod CM) { this->m_Clippper->SetClipMethod(CM); };

	void SetPlane(float o[3], float n[3]) { this->m_Clippper->SetPlane(o, n); };
	void SetPlane(double o[3], double n[3]) { this->m_Clippper->SetPlane(o, n); };
	void GetPlane(float o[3], float n[3]) { this->m_Clippper->GetPlane(o, n); };
	void GetPlane(double o[3], double n[3]) { this->m_Clippper->GetPlane(o, n); };

protected:
	ClipFilter();



private:
	ModelClip::Pointer m_Clippper{ nullptr };
};
IGAME_NAMESPACE_END
#endif