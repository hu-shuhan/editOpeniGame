//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_HistoryOutputRequest_h
#define ddb_HistoryOutputRequest_h

// Begin local includes
#include <ddb_OutputRequest.h>
#include <korC_HistoryOutputShortcut.h>
#include <cow_COW.h>

// Forward declarations
class korC_HistoryOutput;

ddb_OUTPUT_REQUEST_DECL(korC_HistoryOutputShortcut, korC_HistoryOutput, ddb_HistoryOutputRequest);

COW_COW_DECL( ddb_HistoryOutputRequest )
#endif
