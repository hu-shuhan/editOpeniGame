//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_PredefinedField_h
#define odb_PredefinedField_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>


// Forward declarations
class odb_Amplitude;
class ddb_PredefinedField;
COW_COW_DECL(ddb_PredefinedField);

class odb_PredefinedField : public mem_AllocationOperators
{

public:
    void setAmplitudeInStep(const odb_String& stepName, 
                            const odb_Amplitude& ampl);

protected:

    ddb_PredefinedFieldCOW m_predefinedField;

public:   // Undocumented and unsupported

    // Needed for container
    odb_PredefinedField();

    odb_PredefinedField(const ddb_PredefinedField& field);

};

COW_COW_DECL(odb_PredefinedField);

#endif // odb_PredefinedField_h
