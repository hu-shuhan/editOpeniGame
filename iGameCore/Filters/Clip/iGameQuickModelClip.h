#ifndef iGameQuickModelClip_h
#define iGameQuickModelClip_h

#include "iGameModelClip.h"


IGAME_NAMESPACE_BEGIN
class QuickModelClip : public ModelClip {

public:
	I_OBJECT(QuickModelClip);
	static Pointer New() { return new QuickModelClip; }
	~QuickModelClip();








protected:
	QuickModelClip();


	bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um)override;
	bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm)override;
	bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm)override;
private:

};
IGAME_NAMESPACE_END
#endif