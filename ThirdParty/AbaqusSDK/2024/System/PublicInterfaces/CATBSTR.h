
#ifndef CATBSTR_h
#define CATBSTR_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0CTYP.h"
#include "CATCORBAAny.h"
#include "CATBSTRDef.h"

#ifndef _WINDOWS_SOURCE

#include "CATCORBASequence.h"
/** @nodoc */
typedef unsigned short  WCHAR;
DEF_SEQ(_SEQUENCE_CATBSTR, WCHAR);

#endif

/** 
 * Function to invoke to free <tt>CATBSTR</tt> types.
 */
ExportedByJS0CTYP void CATFreeString(CATBSTR iBstr);

/** @nodoc */
ExportedByJS0CTYP HRESULT BuildAnyFromBSTR(any & ioAny, CATBSTR iBstr);

/** @nodoc */
ExportedByJS0CTYP HRESULT ConvertAnyToBSTR(any iAny, CATBSTR & ioBstr);

#endif
