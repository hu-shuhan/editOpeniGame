#ifndef iGameQuickModelClip_h
#define iGameQuickModelClip_h

#include "iGameModelClip.h"


IGAME_NAMESPACE_BEGIN
class QuickModelClip : public ModelClip {

public:
	I_OBJECT(QuickModelClip);
	static Pointer New() { return new QuickModelClip; }
	~QuickModelClip();


	bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um)override;
	bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm)override;
	bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm)override;
	bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm)override;





protected:
	QuickModelClip();



private:

};
IGAME_NAMESPACE_END
#endif