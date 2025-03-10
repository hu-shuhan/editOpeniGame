//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FieldOutputRequest_h
#define odb_FieldOutputRequest_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <odb_Union.h>
#include <ddb_FieldOutputRequest.h>
// Forward declarations
//class ddb_FieldOutputRequest;
//COW_COW2_DECL(ddb_FieldOutputRequest, cow_Virtual);

class odb_FieldOutputRequest : public mem_AllocationOperators
{
public:   // undocumented and unsupported

    void deactivate(odb_String stepName);
    // Needed for container
    odb_FieldOutputRequest();
    odb_FieldOutputRequest(const ddb_FieldOutputRequest& request);

    void setFrequencyInStep( const odb_String& stepName, int f );
    void setNumIntervalsInStep( const odb_String& stepName, int ni );
    void setTimeIntervalInStep( const odb_String& stepName, double ti );
    void setModesInStep( const odb_String& stepName, const odb_SequenceInt& m );
    void setVariablesInStep( const odb_String& stepName, const odb_SequenceString& vars );
    void setTimeMarksInStep( const odb_String& stepName, bool tm );

private:
    ddb_FieldOutputRequestCOW m_OutputRequest;
};

COW_COW_DECL(odb_FieldOutputRequest);

#endif // odb_FieldOutputRequest_h
