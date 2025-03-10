//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_HistoryOutputRequest_h
#define odb_HistoryOutputRequest_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <odb_Union.h>
#include <ddb_HistoryOutputRequest.h>

class odb_HistoryOutputRequest : public mem_AllocationOperators
{
public:
    void deactivate(odb_String stepName);
    odb_HistoryOutputRequest();
    odb_HistoryOutputRequest(const ddb_HistoryOutputRequest& request);

    void setFrequencyInStep( const odb_String& stepName, int f );
    void setNumIntervalsInStep( const odb_String& stepName, int ni );
    void setTimeIntervalInStep( const odb_String& stepName, double ti );
    void setModesInStep( const odb_String& stepName, const odb_SequenceInt& m );
    void setVariablesInStep( const odb_String& stepName, const odb_SequenceString& vars );

private:
    ddb_HistoryOutputRequestCOW m_OutputRequest;
};

COW_COW_DECL(odb_HistoryOutputRequest);
#endif // odb_HistoryOutputRequest_h
