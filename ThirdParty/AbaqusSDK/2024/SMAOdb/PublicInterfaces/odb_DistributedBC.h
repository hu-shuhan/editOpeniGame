//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DistributedBC_h
#define odb_DistributedBC_h

// Begin local includes
#include <odb_BC.h>

// Forward declarations
class ddb_DistributedBC;
class odb_PropagatingComponent;

class odb_DistributedBC: public odb_BC
{
public:

   void setMagnitudeInStep(const odb_String& stepName, 
                            const odb_PropagatingComponent& mag);    

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_DistributedBC();

    odb_DistributedBC(const ddb_DistributedBC& bc);
    
};

#endif // odb_DistributedBC_h
