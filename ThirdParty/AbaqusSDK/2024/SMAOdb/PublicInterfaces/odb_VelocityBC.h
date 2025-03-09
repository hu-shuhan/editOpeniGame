//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_VelocityBC_h
#define odb_VelocityBC_h

// Begin local includes
#include <odb_BC.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_VelocityBC;
class odb_PropagatingComponent;

class odb_VelocityBC: public odb_BC
{
public:
    
    void setValuesInStep( const odb_String& stepName,
                          const odb_PropagatingComponent& v1,
			  const odb_PropagatingComponent& v2,
			  const odb_PropagatingComponent& v3,
			  const odb_PropagatingComponent& vr1,
			  const odb_PropagatingComponent& vr2,
			  const odb_PropagatingComponent& vr3 );
    
private:

public:   // undocumented and unsupported

    // Needed for container
    odb_VelocityBC();

    odb_VelocityBC(const ddb_VelocityBC& bc);
    
};

#endif // odb_VelocityBC_h
