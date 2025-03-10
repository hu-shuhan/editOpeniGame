#include "CATMacForCodeActivation_required.h"
#ifndef __CATMacForCodeActivation
#define __CATMacForCodeActivation
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2012

#include "CATIAV5Level.h"  // CATIAR427
#include "DSYSysMacForBuildtimeWarning.h"   // DSY_SYS_STATIC_WARNING



#if defined(REMOVE_USELESS_INCLUDE)
/** @nodoc */
#define CATICREATEINSTANCE_IS_NOT_AUTHORIZED
#endif



#if defined(CATOMStaticAssertLevel)
#if (CATOMStaticAssertLevel >= 1)

#if !defined(CATOMStaticCheck_ImplementItfBase_IsAssert)
/** @nodoc */
#define CATOMStaticCheck_ImplementItfBase_IsAssert      1
#endif
#if !defined(CATOMStaticCheck_ImplementClassBase_IsAssert)
/** @nodoc */
#define CATOMStaticCheck_ImplementClassBase_IsAssert    1
#endif
#if !defined(CATOMStaticCheck_ImplementClassIsCBU_IsAssert)
/** @nodoc */
#define CATOMStaticCheck_ImplementClassIsCBU_IsAssert   1
#endif
#if !defined(CATOMStaticCheck_ImplementVarBase_IsAssert)
/** @nodoc */
#define CATOMStaticCheck_ImplementVarBase_IsAssert   1
#endif

#endif
#endif  // CATOMStaticAssertLevel


#if !defined(CATOMStaticCheck_ImplementItfBase_IsAssert)
/** @nodoc Default value */
#define CATOMStaticCheck_ImplementItfBase_IsAssert    0
#endif


#if (CATOMStaticCheck_ImplementItfBase_IsAssert != 0) || defined(REMOVE_USELESS_INCLUDE) || defined(CATIAR427)
/** @nodoc */
#define CATOMStaticCheck_ImplementItfBase(expr, msg)   static_assert(expr, msg)
#else
/** @nodoc */
#define CATOMStaticCheck_ImplementItfBase(expr, msg)   DSY_SYS_STATIC_WARNING(expr, msg)
#endif


#if !defined(CATOMStaticCheck_ImplementClassBase_IsAssert)
/** @nodoc Default value */
#define CATOMStaticCheck_ImplementClassBase_IsAssert    0
#endif

#if (CATOMStaticCheck_ImplementClassBase_IsAssert != 0) || defined(REMOVE_USELESS_INCLUDE) || defined(CATIAR427)
/** @nodoc */
#define CATOMStaticCheck_ImplementClassBase(expr, msg)   static_assert(expr, msg)
#else
/** @nodoc */
#define CATOMStaticCheck_ImplementClassBase(expr, msg)   DSY_SYS_STATIC_WARNING(expr, msg)
#endif


#if !defined(CATOMStaticCheck_ImplementClassIsCBU_IsAssert)
/** @nodoc Default value */
#define CATOMStaticCheck_ImplementClassIsCBU_IsAssert    0
#endif

#if (CATOMStaticCheck_ImplementClassIsCBU_IsAssert != 0) || defined(REMOVE_USELESS_INCLUDE) || defined(CATIAR427)
/** @nodoc */
#define CATOMStaticCheck_ImplementClassIsCBU(expr, msg)     static_assert(expr, msg)
#else
/** @nodoc */
#define CATOMStaticCheck_ImplementClassIsCBU(expr, msg)     DSY_SYS_STATIC_WARNING(expr, msg)
#endif


#if !defined(CATOMStaticCheck_ImplementVarBase_IsAssert)
/** @nodoc Default value */
#define CATOMStaticCheck_ImplementVarBase_IsAssert    0
#endif

#if (CATOMStaticCheck_ImplementVarBase_IsAssert != 0) || defined(REMOVE_USELESS_INCLUDE) || defined(CATIAR427)
/** @nodoc */
#define CATOMStaticCheck_ImplementVarBase(expr, msg)    static_assert(expr, msg)
#else
/** @nodoc */
#define CATOMStaticCheck_ImplementVarBase(expr, msg)    DSY_SYS_STATIC_WARNING(expr, msg)
#endif

#endif // __CATMacForCodeActivation
