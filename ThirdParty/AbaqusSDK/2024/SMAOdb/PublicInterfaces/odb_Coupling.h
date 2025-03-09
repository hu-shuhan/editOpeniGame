//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
#ifndef odb_Coupling_h
#define odb_Coupling_h

// Begin local includes
#include <odb_Constraint.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_Coupling;
class odb_Set;
class odb_DatumCsys;

class odb_Coupling: public odb_Constraint
{
public:

    // member access
    odb_Set surface() const;
    odb_Set refPoint() const;
    double influenceRadius() const;
    odb_String couplingType() const;
    odb_DatumCsys csys() const;
    bool u1() const;
    bool u2() const;
    bool u3() const;
    bool ur1() const;
    bool ur2() const;
    bool ur3() const;
    bool adjust() const;
    //    kcoC_WeightingMethodEnm weightingMethod;
    odb_String weightingMethod() const;
    odb_Set couplingNodes() const;

    // Options

private:

public:   // undocumented and unsupported
    static unsigned int typeIdentifier();
    odb_Coupling( const odb_Coupling& copy );
    odb_Coupling& operator=( const odb_Coupling& rhs );


    // Needed for container
    odb_Coupling();

    odb_Coupling(const ddb_Coupling& constraint);
    
};

#endif // odb_Coupling_h
