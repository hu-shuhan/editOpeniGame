#ifndef IGAME_DECIMATE_POLYLINE_FILTER_H
#define IGAME_DECIMATE_POLYLINE_FILTER_H

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
class DecimatePolylineFilter : public Filter {
public:
    I_OBJECT(DecimatePolylineFilter);
    static Pointer New() { return new DecimatePolylineFilter; }
    
    bool Execute() override { return false; }

protected:
    DecimatePolylineFilter() {};
    ~DecimatePolylineFilter() override = default;


private:

};
IGAME_NAMESPACE_END
#endif