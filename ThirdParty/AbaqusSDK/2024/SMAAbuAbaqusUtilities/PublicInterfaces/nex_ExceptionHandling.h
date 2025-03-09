//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef  nex_ExceptionHandling_h
#define  nex_ExceptionHandling_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: nex_ExceptionHandling.h
//
// Description:
//
//    Preprocessor macros aiding exception handling.
//

// Begin local includes 

// HKS obj_ExceptionPtr handling macros

#define  HKS_RETHROW throw
#define  HKS_PROPAGATE(REF) REF.Propagate()

// Preserve file/line location where thrown...
#define  HKS_THROW(REF) REF.Throw(__FILE__,__LINE__)

#define  HKS_TRY try
#define  HKS_CATCHTYPE(CLASS) catch (const CLASS &)
#define  HKS_CATCHREF(CLASS,REF) catch (const CLASS & REF)
#define  HKS_CATCHALL catch (...)
#define  HKS_ENDTRY

#define  HKS_EXCEPTIONSPEC(TYPELIST) hks__exceptionspec_(TYPELIST)

#define TESTFUNC(what2throw,badreturn,callreturn) \
    if ((badreturn) == (callreturn)) HKS_THROW (what2throw); else

#endif // nex_ExceptionHandling_h
