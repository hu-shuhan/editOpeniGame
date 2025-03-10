#ifndef INCL_CATLISTPIUnknown_h
#define INCL_CATLISTPIUnknown_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * @collection CATLISTP(IUnknown)
 * Collection class for pointers to <tt>IUnknown</tt>.
 * All the methods of pointer collection classes are available.
 * Refer to the articles dealing with collections in the encyclopedia.
 * @see IUnknown
 */

// Clean previous functions requests
#include  <CATLISTP_Clean.h>

// Specify the function to take into account.
#include <CATLISTP_AllFunct.h>

// Get macros
#include  <CATLISTP_Declare.h>

// Define the NT DLL export macro
#include "ConnectionPts.h"
#undef	CATCOLLEC_ExportedBy
#define	CATCOLLEC_ExportedBy ExportedByConnectionPts

// Declare the CATLISTP(IUnknown) type.
#include "IUnknown.h"
CATLISTP_DECLARE( IUnknown )

typedef CATLISTP(IUnknown) CATListOfIUnknown;

#endif
