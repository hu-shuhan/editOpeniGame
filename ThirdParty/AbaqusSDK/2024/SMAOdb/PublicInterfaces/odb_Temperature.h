//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Temperature_h
#define odb_Temperature_h

// Begin local includes
#include <odb_GeneralFieldBase.h>

// Forward declarations
typedef ddb_GeneralFieldBase ddb_Temperature;

class odb_Temperature: public odb_GeneralFieldBase
{

public:   // Undocumented and unsupported

    // Needed for container
    odb_Temperature();

    odb_Temperature(const ddb_Temperature& predefinedField);

};

#endif // odb_Temperature_h
