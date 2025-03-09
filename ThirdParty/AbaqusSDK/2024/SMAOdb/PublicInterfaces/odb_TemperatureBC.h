//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_TemperatureBC_h
#define odb_TemperatureBC_h

// Begin local includes
#include <odb_DistributedBC.h>

// Forward declarations
class ddb_TemperatureBC;

class odb_TemperatureBC: public odb_DistributedBC
{
public:
    
    void setDofInStep(const odb_String& stepName, int dof);

private:

public:   // Undocumented and unsupported

    // Needed for container
    odb_TemperatureBC();

    odb_TemperatureBC(const ddb_TemperatureBC& bc);

};

#endif // odb_TemperatureBC_h
