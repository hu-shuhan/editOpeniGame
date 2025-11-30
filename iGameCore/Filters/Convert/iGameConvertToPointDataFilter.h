#ifndef iGameConvertToPointDataFilter_h
#define iGameConvertToPointDataFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToPointDataFilter : public Filter {
public:
    I_OBJECT(ConvertToPointDataFilter);
    static Pointer New() { return new ConvertToPointDataFilter; }

    bool Execute() override;

protected:
    ConvertToPointDataFilter();
    ~ConvertToPointDataFilter() override = default;
};
IGAME_NAMESPACE_END
#endif
