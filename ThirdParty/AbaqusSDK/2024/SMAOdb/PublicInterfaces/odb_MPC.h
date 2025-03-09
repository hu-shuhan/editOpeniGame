//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
#ifndef odb_MPC_h
#define odb_MPC_h

// Begin local includes
#include <odb_Constraint.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_Mpc;
class odb_Set;
class odb_DatumCsys;

class odb_MPC: public odb_Constraint
{
public:

    // member access
    odb_Set surface() const;
    odb_Set refPoint() const;
    odb_DatumCsys csys() const;
    odb_String mpcType() const;
    odb_String userMode() const;
    int userType() const;

    // Options

private:

public:   // undocumented and unsupported
    static unsigned int typeIdentifier();
    odb_MPC( const odb_MPC& copy );
    odb_MPC& operator=( const odb_MPC& rhs );


    // Needed for container
    odb_MPC();

    odb_MPC(const ddb_Mpc& constraint);
    
};

#endif // odb_MPC_h
