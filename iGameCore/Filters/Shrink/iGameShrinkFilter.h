#ifndef iGameShrinkFilter_h
#define iGameShrinkFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"

#include <vector>

IGAME_NAMESPACE_BEGIN
class ShrinkFilter : public Filter {
public:
	I_OBJECT(ShrinkFilter);
	static Pointer New() { return new ShrinkFilter; }

	void SetShrinkFactor(double factor);
	double GetShrinkFactor() const;

	bool Execute() override;

protected:
	ShrinkFilter();
	~ShrinkFilter() override = default;

private:
	bool CopyPointAttributes(PointSet* pointSet, const std::vector<IGsize>& srcOfNew);
	static ArrayObject::Pointer CloneArray(ArrayObject::Pointer src,
	                                       const std::vector<IGsize>& srcOfNew);

	double m_ShrinkFactor{0.5};
};
IGAME_NAMESPACE_END
#endif
