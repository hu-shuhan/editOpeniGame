/* -*-c++-*- */
#ifndef CATLISTV_CAT_BASEUNKNOWN_H
#define CATLISTV_CAT_BASEUNKNOWN_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
  * @CAA2Level L1
  * @CAA2Usage U1
  */

/**
 * @collection CATLISTV(CATBaseUnknown_var)
 * Collection class for CATBaseUnknown.
 * All the methods of handlers collection classes are available.
 * Refer to the articles dealing with collections in the encyclopedia.
 */
#include "JS0CORBA.h"

#include "CATBaseUnknown_var.h"

#include "CATLISTHand_Clean.h"
#include "CATLISTHand_AllFunct.h"

#include "CATLISTHand_Declare.h"

#undef	CATCOLLEC_ExportedBy
#define	CATCOLLEC_ExportedBy	ExportedByJS0CORBA

CATLISTHand_DECLARE( CATBaseUnknown_var )

#undef	CATCOLLEC_ExportedBy

#endif


