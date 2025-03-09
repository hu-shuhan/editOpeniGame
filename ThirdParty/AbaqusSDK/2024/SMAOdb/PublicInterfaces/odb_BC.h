//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_BC_h
#define odb_BC_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>


// Forward declarations
class odb_Amplitude;
class ddb_BC;
COW_COW2_DECL(ddb_BC, cow_Virtual); 

class odb_BC  : public mem_AllocationOperators
{
public:
    void deactivate(odb_String stepName);
    void setAmplitudeInStep(const odb_String& stepName, 
                            const odb_Amplitude& ampl);

protected:
    ddb_BCCOW m_bc;

public:   // undocumented and unsupported

    // Needed for container
    odb_BC();

    odb_BC(const ddb_BC& bc);
    
};

COW_COW_DECL(odb_BC); 

#endif // odb_BC_h
