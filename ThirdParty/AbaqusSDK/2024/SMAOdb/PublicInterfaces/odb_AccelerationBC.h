//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_AccelerationBC_h
#define odb_AccelerationBC_h

// Begin local includes
#include <odb_BC.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_AccelerationBC;
class odb_PropagatingComponent;

class odb_AccelerationBC: public odb_BC
{
public:
    
    void setValuesInStep( const odb_String& stepName,
                          const odb_PropagatingComponent& a1,
			  const odb_PropagatingComponent& a2,
			  const odb_PropagatingComponent& a3,
			  const odb_PropagatingComponent& ar1,
			  const odb_PropagatingComponent& ar2,
			  const odb_PropagatingComponent& ar3 );
    
private:

public:   // undocumented and unsupported

    // Needed for container
    odb_AccelerationBC();

    odb_AccelerationBC(const ddb_AccelerationBC& bc);
    
};

#endif // odb_AccelerationBC_h
