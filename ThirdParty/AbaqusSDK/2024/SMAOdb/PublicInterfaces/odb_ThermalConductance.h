//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ThermalConductance_h
#define odb_ThermalConductance_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>

// Forward declarations
class ddb_ThermalConductance;
COW_COW_DECL(ddb_ThermalConductance);

class odb_ThermalConductance : public mem_AllocationOperators
{
public:

    // member access

    // Options

private:
    ddb_ThermalConductanceCOW thermalConductance;

public:   // Undocumented and unsupported

    // Needed for container
    odb_ThermalConductance();

    odb_ThermalConductance(const ddb_ThermalConductance& nb);

};

#endif // odb_ThermalConductance_h
