
#ifndef CATSafeArray_h
#define CATSafeArray_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0CTYP.h"
#include "CATBaseUnknown.h"
#include "CATBSTR.h"
#include "CATVariant.h"
#include "CATCORBASequence.h"
#include "CATCORBAAny.h"


#ifdef _WINDOWS_SOURCE

#include "oaidl.h"
/**
 * Type to implement <tt>SAFEARRAY</tt> types on NT.
 * Use the conversions methods provided in <tt>CATAutoConversions.h</tt>
 * to use this type.
*/
typedef SAFEARRAY CATSafeArray;

#else
/** @nodoc */
DEF_SEQ(_SEQUENCE_dimensions, unsigned long);

/** @nodoc */
typedef  _SEQUENCE_dimensions  dimensions;

#include "CATNTTypes.h"
#include "TypeDesc.h"
#include "ArrayDesc.h"

/** @nodoc */
typedef struct tagSAFEARRAY {
  unsigned short cDims;
  unsigned short fFeatures;
  ULONG cbElements;
  ULONG cLocks;
  void * pvData;
  SAFEARRAYBOUND rgsabound[ 1 ];
} SAFEARRAY;

/** @nodoc */
typedef SAFEARRAY CATSafeArray;

#endif

/** 
 * Defines a <tt>CATSafeArrayVariant</tt> type to be used by Automation interfaces.
 * <tt>CATSafeArrayVariant</tt> are one-dimensional arrays of <tt>CATVariants</tt>.
 */
typedef CATSafeArray  CATSafeArrayVariant;

/** 
 * Function to invoke to free <tt>CATSafeArrayVariant</tt> types.
 */
ExportedByJS0CTYP HRESULT FreeVariantSafeArray(CATSafeArrayVariant * iArray);

/** @nodoc */
ExportedByJS0CTYP CATSafeArrayVariant * BuildFromVariantArray(CATVariant *iTab, unsigned long idim);

/** @nodoc */
ExportedByJS0CTYP HRESULT ConvertToVariantArray(CATSafeArrayVariant * iArray, CATVariant * &oTab, unsigned long &oDim);

/** @nodoc */
ExportedByJS0CTYP HRESULT InsertInVariantArray(long numtab, CATSafeArrayVariant & Tableau, CATVariant & new_elem);

/** @nodoc */
ExportedByJS0CTYP long GetVariantArraySize(CATSafeArrayVariant * iArray);



#endif // CATSafeArray_h





