
#ifndef CATVariant_h
#define CATVariant_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0CTYP.h"
#include "CATCORBAAny.h"

#ifndef _WINDOWS_SOURCE

#include "IUnknown.h"
#include "CATJHNTypeLib.h"
#include "CATBSTR.h"
#include "CATWTypes.h"

interface IDispatch;
interface IRecordInfo;

/** @nodoc */
typedef struct tagSAFEARRAY CATSafeArray;

/** @nodoc */
typedef short VARIANT_BOOL;

/** @nodoc */
typedef VARIANT_BOOL _VARIANT_BOOL;

/** @nodoc */
typedef union tagCY {
 struct {
   LONG Hi;
   ULONG Lo;
 } u ;
 LONGLONG int64;
} CY;

/** @nodoc */
struct tagDEC_1_ {
  BYTE scale;
  BYTE sign;
};

/** @nodoc */
struct tagDEC_2_ {
  ULONG Mid32; ULONG Lo32 ;
};

/** @nodoc */
typedef struct tagDEC {
  unsigned short wReserved;
  union {
    struct tagDEC_1_ n1 ;
    unsigned short signscale;
  };
  ULONG Hi32;
  union {
    struct tagDEC_2_ n2 ;
    ULONGLONG Lo64;
  } ;
} DECIMAL;

/** @nodoc */
typedef struct tagVARIANT VARIANT;

/** @nodoc */
typedef VARIANT *LPVARIANT;

/** @nodoc */
typedef VARIANT VARIANTARG;

/** @nodoc */
struct tagVARIANT_1_
{
  void * pvRecord;
  IRecordInfo *pRecInfo;
};

/** @nodoc */
struct tagVARIANT
 {
   VARTYPE vt;
   WORD wReserved1;
   WORD wReserved2;
   WORD wReserved3;
   union
     {
       LONG lVal;
       BYTE bVal;
       short iVal;
       float fltVal;
       double dblVal;
       VARIANT_BOOL boolVal;
       _VARIANT_BOOL vbool;
       LONG scode;
       CY cyVal;
       double date;
       CATBSTR bstrVal;
       IUnknown *punkVal;
       IDispatch *pdispVal;
       CATSafeArray *parray;
       BYTE *pbVal;
       short *piVal;
       LONG *plVal;
       float *pfltVal;
       double *pdblVal;
       VARIANT_BOOL *pboolVal;
       _VARIANT_BOOL *pbool;
       LONG *pscode;
       CY *pcyVal;
       double *pdate;
       CATBSTR *pbstrVal;
       IUnknown * *ppunkVal;
       IDispatch * *ppdispVal;
       CATSafeArray * *pparray;
       VARIANT *pvarVal;
       void * byref;
       char cVal;
       unsigned short uiVal;
       ULONG ulVal;
       int intVal;
       unsigned int uintVal;
       DECIMAL *pdecVal;
       char *pcVal;
       unsigned short *puiVal;
       ULONG *pulVal;
       int *pintVal;
       unsigned int *puintVal;
       struct tagVARIANT_1_ brecVal ;
     } ;
 };

/** @nodoc */
struct _tagVARIANT
{
  union
    {
      struct tagVARIANT n2 ;
      DECIMAL decVal;
    } ;
};

/** @nodoc */
#define V_VT(X)          ((X)->vt)

/** @nodoc */
#define V_I4(X)          ((X)->lVal)

/** @nodoc */
#define V_BSTR(X)          ((X)->bstrVal)

/** @nodoc */
#define V_I2(X)          ((X)->iVal)

/** @nodoc */
#define V_R4(X)          ((X)->fltVal)

/** @nodoc */
#define V_R8(X)          ((X)->dblVal)

/** @nodoc */
#define V_DISPATCH(X)          ((X)->pdispVal)

/** @nodoc */
#define V_BOOL(X)          ((X)->boolVal)

/** @nodoc */
#define V_UNKNOWN(X)          ((X)->punkVal)

/** @nodoc */
#define V_I1(X)          ((X)->cVal)

/** @nodoc */
#define V_UI1(X)          ((X)->bVal)

/** @nodoc */
#define V_UI2(X)          ((X)->uiVal)

/** @nodoc */
#define V_UI4(X)          ((X)->ulVal)

/** @nodoc */
#define V_DATE(X)          ((X)->date)

/** 
 * Defines the variant type to be used by Automation interfaces.
 */
typedef VARIANT CATVariant;

#else
#include "windows.h"

/** 
 * Defines the variant type to be used by Automation interfaces.
 */
typedef VARIANT CATVariant;

#endif

/** 
 * Function to invoke to free <tt>CATVariant</tt> types.
 */
ExportedByJS0CTYP HRESULT CATVariantClear(CATVariant * iVariant);

/** @nodoc */
ExportedByJS0CTYP HRESULT ConvertAnyToVariant(const any & iAny, CATVariant* oVariant);

/** @nodoc */
ExportedByJS0CTYP any * BuildAnyFromVariant (const CATVariant & iVariant);

#endif
