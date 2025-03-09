//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_RotationalBodyForce_h
#define odb_RotationalBodyForce_h

// Begin local includes
#include <odb_ScalarLoad.h>

// Forward declarations
class ddb_RotationalBodyForce;

class odb_RotationalBodyForce: public odb_ScalarLoad
{
public:

    // member access

    // Options

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_RotationalBodyForce();

    odb_RotationalBodyForce(const ddb_RotationalBodyForce& load);
};

#endif // odb_RotationalBodyForce_h
