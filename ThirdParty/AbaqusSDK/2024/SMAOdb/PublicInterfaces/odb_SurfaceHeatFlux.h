//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SurfaceHeatFlux_h
#define odb_SurfaceHeatFlux_h

// Begin local includes
#include <odb_DistributedLoad.h>

// Forward declarations
typedef ddb_DistributedLoad ddb_SurfaceHeatFlux;

class odb_SurfaceHeatFlux: public odb_DistributedLoad
{
public:

    // member access

    // Options

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_SurfaceHeatFlux();

    odb_SurfaceHeatFlux(const ddb_SurfaceHeatFlux& load);

};

#endif // odb_SurfaceHeatFlux_h
