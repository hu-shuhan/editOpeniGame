//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_RestartRequest_h
#define odb_RestartRequest_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>
// End local includes

class ddb_Restart;
COW_COW_DECL(ddb_Restart);

// Forward declarations

class odb_RestartRequest : public mem_AllocationOperators
{
public:   // undocumented and unsupported
    odb_RestartRequest();
    odb_RestartRequest(const ddb_Restart &restart );

    ddb_Restart restart() const;
protected:
    ddb_RestartCOW m_restart;
};

COW_COW_DECL(odb_RestartRequest);

#endif // odb_RestartRequest_h
