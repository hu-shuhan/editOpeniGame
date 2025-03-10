//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Load_h
#define odb_Load_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>


// Forward declarations
class odb_Amplitude;
class ddb_Load;
COW_COW2_DECL(ddb_Load, cow_Virtual); 

class odb_Load : public mem_AllocationOperators
{
public:
    void deactivate(odb_String stepName);
    void setAmplitudeInStep(const odb_String& stepName, 
                            const odb_Amplitude& ampl);

protected:
    ddb_LoadCOW m_load;

public:   // undocumented and unsupported

    // Needed for container
    odb_Load();

    odb_Load(const ddb_Load& load);
    
};

COW_COW_DECL(odb_Load); 

#endif // odb_Load_h
