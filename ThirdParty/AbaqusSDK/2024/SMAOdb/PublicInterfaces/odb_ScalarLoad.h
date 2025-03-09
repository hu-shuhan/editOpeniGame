//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ScalarLoad_h
#define odb_ScalarLoad_h

// Begin local includes
#include <odb_Load.h>

// Forward declarations
class ddb_ScalarLoad;
class odb_PropagatingComponent;

class odb_ScalarLoad: public odb_Load
{
public:

    void setMagnitudeInStep(const odb_String& stepName, 
                            const odb_PropagatingComponent& mag);    

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_ScalarLoad();

    odb_ScalarLoad(const ddb_ScalarLoad& load);
    
};

#endif // odb_ScalarLoad_h
