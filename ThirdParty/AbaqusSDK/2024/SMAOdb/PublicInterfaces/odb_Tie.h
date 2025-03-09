//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Tie_h
#define odb_Tie_h

// Begin local includes
#include <odb_Constraint.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_Tie;
class odb_Set;

class odb_Tie: public odb_Constraint
{
public:

    // member access
    odb_Set master() const;
    odb_Set slave() const;
    bool adjust() const;
    double positionTolerance() const;
    bool tieRotations() const;
    odb_String positionToleranceMethod() const;
    odb_String constraintRatioMethod() const;
    double constraintRatio() const;
    odb_String constraintEnforcement() const;
    bool thickness() const;


    // Options

private:

public:   // undocumented and unsupported
    static unsigned int typeIdentifier();
    odb_Tie( const odb_Tie& copy );
    odb_Tie& operator=( const odb_Tie& rhs );


    // Needed for container
    odb_Tie();

    odb_Tie(const ddb_Tie& constraint);
    
};

#endif // odb_Tie_h
