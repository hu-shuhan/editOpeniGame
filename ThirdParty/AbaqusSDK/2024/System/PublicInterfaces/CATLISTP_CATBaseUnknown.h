/* -*-c++-*- */
#ifndef CATLISTP_CAT_BASEUNKNOWN_H
#define CATLISTP_CAT_BASEUNKNOWN_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
  * @CAA2Level L1
  * @CAA2Usage U1
  */

/**
 * @collection CATLISTP(CATBaseUnknown)
 * Collection class for CATBaseUnknown.
 * All the methods of collection classes are available.
 * Refer to the articles dealing with collections in the encyclopedia.
 */
#include "JS0CORBA.h"
class CATBaseUnknown;

#include "CATLISTP_Clean.h"

#include "CATLISTP_PublicInterface.h"

#include "CATLISTP_Declare.h"

#undef	CATCOLLEC_ExportedBy
#define	CATCOLLEC_ExportedBy	ExportedByJS0CORBA

CATLISTP_DECLARE( CATBaseUnknown )
// typedef CATLISTP(CATBaseUnknown) CATListPtrCATBaseUnknown;
#undef	CATCOLLEC_ExportedBy

#endif


