//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_BodyHeatFlux_h
#define odb_BodyHeatFlux_h

// Begin local includes
#include <odb_DistributedLoad.h>

// Forward declarations
typedef ddb_DistributedLoad ddb_BodyHeatFlux;

class odb_BodyHeatFlux: public odb_DistributedLoad
{
public:

    // member access

    // Options

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_BodyHeatFlux();

    odb_BodyHeatFlux(const ddb_BodyHeatFlux& load);

};

#endif // odb_BodyHeatFlux_h
