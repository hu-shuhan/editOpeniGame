//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
#ifndef odb_ShellSolidCoupling_h
#define odb_ShellSolidCoupling_h

// Begin local includes
#include <odb_Constraint.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_ShellSolidCoupling;
class odb_Set;
class odb_DatumCsys;

class odb_ShellSolidCoupling: public odb_Constraint
{
public:

    // member access
    odb_Set shellEdge() const;
    odb_Set solidFace() const;
    //    kcoC_PosToleranceEnm positionToleranceMethod;
    odb_String positionToleranceMethod() const;
    double positionTolerance() const;
    //    kcoC_DefaultOrValueEnm influenceDistanceMethod() const;
    odb_String influenceDistanceMethod() const;
    double influenceDistance() const;


    // Options

private:

public:   // undocumented and unsupported
    static unsigned int typeIdentifier();
    odb_ShellSolidCoupling( const odb_ShellSolidCoupling& copy );
    odb_ShellSolidCoupling& operator=( const odb_ShellSolidCoupling& rhs );


    // Needed for container
    odb_ShellSolidCoupling();

    odb_ShellSolidCoupling(const ddb_ShellSolidCoupling& constraint);
    
};

#endif // odb_ShellSolidCoupling_h
