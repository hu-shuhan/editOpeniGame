//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_TangentialBehavior_h
#define odb_TangentialBehavior_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
#include <odb_Types.h>

// Forward declarations
class ddb_TangentialBehavior;
class odb_Union;
class odb_Set;
class odb_DatumCsys;
class odb_ContactProperty;
class ddb_TangentialBehavior;
COW_COW_DECL(ddb_TangentialBehavior);

class odb_TangentialBehavior : public mem_AllocationOperators
{
public:

    // member access
    odb_String formulation() const;
    odb_String directionality() const;
    bool slipRateDependency() const;
    bool pressureDependency() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String exponentialDecayDefinition() const;
    odb_SequenceSequenceDouble table() const;
    double shearStressLimit() const;
    odb_String maximumElasticSlip() const;
    double fraction() const;
    double absoluteDistance() const;
    double elasticSlipStiffness() const;
    int nStateDependentVars() const;
    bool useProperties() const;
    bool hasValue() const;

    // Options

private:
    ddb_TangentialBehaviorCOW tangentialBehavior;

public:   // undocumented and unsupported

    // Needed for container
    odb_TangentialBehavior();

    odb_TangentialBehavior(const ddb_TangentialBehavior& tb);
    
};

#endif // odb_TangentialBehavior_h
