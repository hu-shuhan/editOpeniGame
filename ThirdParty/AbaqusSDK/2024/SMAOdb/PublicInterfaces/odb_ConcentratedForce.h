//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ConcentratedForce_h
#define odb_ConcentratedForce_h

// Begin local includes
#include <odb_VectorLoad.h>
#include <odb_Types.h>

// Forward declarations
class ddb_ConcentratedForce;

class odb_ConcentratedForce: public odb_VectorLoad
{
public:

    // member access

    // Options

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_ConcentratedForce();

    odb_ConcentratedForce(const ddb_ConcentratedForce& load);
    
};

#endif // odb_ConcentratedForce_h
