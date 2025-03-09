//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DisplacementBC_h
#define odb_DisplacementBC_h

// Begin local includes
#include <odb_BC.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_DisplacementBC;
class odb_PropagatingComponent;

class odb_DisplacementBC: public odb_BC
{
public:
    
    void setUInStep( const odb_String& stepName,
                     const odb_PropagatingComponent& u1,
                     const odb_PropagatingComponent& u2,
                     const odb_PropagatingComponent& u3,
                     const odb_PropagatingComponent& ur1,
                     const odb_PropagatingComponent& ur2,
                     const odb_PropagatingComponent& ur3 );
    
private:

public:   // undocumented and unsupported

    // Needed for container
    odb_DisplacementBC();

    odb_DisplacementBC(const ddb_DisplacementBC& bc);
    
};

#endif // odb_DisplacementBC_h
