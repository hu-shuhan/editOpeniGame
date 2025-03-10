//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Gravity_h
#define odb_Gravity_h

// Begin local includes
#include <odb_VectorLoad.h>

// Forward declarations
typedef ddb_VectorLoad ddb_Gravity;

class odb_Gravity: public odb_VectorLoad
{
public:

    // member access

    // Options

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_Gravity();

    odb_Gravity(const ddb_Gravity& load);
};

#endif // odb_Gravity_h
