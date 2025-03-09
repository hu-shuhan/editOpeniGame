//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DistributedLoad_h
#define odb_DistributedLoad_h

// Begin local includes
#include <odb_Load.h>

// Forward declarations
class ddb_DistributedLoad;
class odb_PropagatingComponent;

class odb_DistributedLoad: public odb_Load
{
public:

   void setMagnitudeInStep(const odb_String& stepName, 
                            const odb_PropagatingComponent& mag);    

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_DistributedLoad();

    odb_DistributedLoad(const ddb_DistributedLoad& load);
    
};

#endif // odb_DistributedLoad_h
