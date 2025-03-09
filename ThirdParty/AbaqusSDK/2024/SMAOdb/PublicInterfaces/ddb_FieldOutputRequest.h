//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_FieldOutputRequest_h
#define ddb_FieldOutputRequest_h

// Begin local includes
#include <ddb_OutputRequest.h>
#include <korC_FieldOutputShortcut.h>
#include <cow_COW.h>

// Forward declarations
class korC_FieldOutput;

ddb_OUTPUT_REQUEST_DECL(korC_FieldOutputShortcut, korC_FieldOutput, ddb_FieldOutputRequest);

COW_COW_DECL( ddb_FieldOutputRequest)

#endif
