#ifndef iGameConvertToCellDataFilter_h
#define iGameConvertToCellDataFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToCellDataFilter : public Filter {
public:
    I_OBJECT(ConvertToCellDataFilter);
    static Pointer New() { return new ConvertToCellDataFilter; }

    bool Execute() override;

protected:
    ConvertToCellDataFilter();
    ~ConvertToCellDataFilter() override = default;
};
IGAME_NAMESPACE_END
#endif
