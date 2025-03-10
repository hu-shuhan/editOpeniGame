#ifndef CATAssert_H
#define CATAssert_H     42400
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#include "JS0ERROR.h" 
#include <stdlib.h>

/**
 * no doc
 * enum return by CATSysLogAbendCallback and specifying the behaviour to take
 *  for CATSysLogAbend
 *  CATSysLogAbdAbort  : session will abort
 *  CATSysLogAbdIgnore : problem will be logged and session will continue
 *  CATSysLogAbdDebug  : debug 
 **/
enum CATSysLogAbendBehaviour {  CATSysLogAbdAbort, CATSysLogAbdIgnore, CATSysLogAbdDebug };
/**
 * no doc
 *
 *  if iAppIsBatch == 0 means applications runs in interactive mode
 *                      else means applications runs in batche mode
 *
 *   if  iInOdt != 0   means applications runs in odt mode
 *
 */
typedef  CATSysLogAbendBehaviour( *CATSysLogAbendCbk) ( int  iAppIsBatch ,  int iInOdt, const char *iCause );
/**
 * no doc
 */
ExportedByJS0ERROR CATSysLogAbendCbk CATSysSetLogAbendCallback ( CATSysLogAbendCbk iCallback);
/** 
 * no doc
 * CATSysLogAbend permits to do the following
 * Args
 * iCause      : Cause of the exception
 *
 *  - log the error in industrial stage
 *  - immediately abort in development stage
 */
ExportedByJS0ERROR void CATSysLogAbend( const char *iCause=NULL );

/** 
 * no doc
 * CATSysLogError permits to do the following
 * Args
 * iCause      : Cause of the exception
 *
 *  - log the error in industrial stage , no abend
 */
ExportedByJS0ERROR void CATSysLogError( const char *iCause=NULL);

/** 
 * no doc
 * CATFatalErrorSimple permits to raise an exception when no better 
 * error handling may be taken
 * Args
 * iFile       : File (facultative)
 * iLine       : Line ( facultative)
 */
[[noreturn]] ExportedByJS0ERROR void CATFatalErrorSimple(const char * iFile=NULL, int iLine=0);

/** 
 * no doc
 * CATSysCheckFailureSimple permits to raise an exception only in development stage
 *
 * Args
 * iFile       : File (facultative)
 * iLine       : Line ( facultative)
 */
ExportedByJS0ERROR int CATSysCheckFailureSimple( const char * iFile=NULL, int iLine=0);

/** 
 * no doc
 * CATFatalError permits to raise an exception when no better 
 * error handling may be taken
 * Args
 * iCause      : Cause of the exception
 * iSoftwareId : Identifier of the software which detected the error
 * ikey        : Key of specific NLS error message
 * iCatalog    : NLS catalog name
 * iFile       : File (facultative)
 * iLine       : Line ( facultative)
 */
[[noreturn]] ExportedByJS0ERROR void CATFatalError( const char *iCause = NULL,
                                   const char *iSoftwareId  = NULL,
                                   const char *iKey= NULL,
                                   const char *iCatalog= NULL,
                                   const char * iFile=NULL,
                                   int iLine=0);

/** 
 * no doc
 * CATSysCheckFailure permits to raise an exception only in development
 * stage
 * CATSysCheckFailure should be used only through the CATSysCheckExpression
 * macro
 *
 * Args
 * iCause      : Cause of the exception
 * iSoftwareId : Identifier of the software which detected the error
 * ikey        : Key of specific NLS error message
 * iCatalog    : NLS catalog name
 * iFile       : File (facultative)
 * iLine       : Line ( facultative)
 */
ExportedByJS0ERROR int CATSysCheckFailure( const char *iCause = NULL,
                                   const char *iSoftwareId  = NULL,
                                   const char *iKey= NULL,
                                   const char *iCatalog= NULL,
                                   const char * iFile=NULL,
                                   int iLine=0);


#include <assert.h>
/** 
 * no doc
 * CATAssert permits to check an expression 
 *  In case the assertion fails, CATAssert will have the following behaviour
 *    in development stage, immediately terminate the program  
 *    in industrial stage, throws a recoverable error         
 */
#ifdef _MOBILE_SOURCE
# define CATAssert(exp)   (void) ( (exp) ? (void)0 : CATFatalErrorSimple(__FILE__,__LINE__) )
#else // _MOBILE_SOURCE
# define CATAssert(exp)   (void) ( (exp) ? (void)0 : CATFatalError( "AssertionFailed:"#exp,NULL,NULL,NULL,__FILE__,__LINE__) )
#endif  // _MOBILE_SOURCE

/** 
 * no doc
 * CATSysCheckExpression permits to check an expression
 *   with the following behaviour:
 *   - if the expression is true, CATSysCheckExpression returns -1,
 *   - if the expression is false:
 *     + in development stage, immediately terminates the program,
 *     + in industrial stage, returns 0.
 * Usage:
 *   if ( !CATSysCheckExpression(iPtr) ) return E_INVALIDARG;
 */
#ifdef _MOBILE_SOURCE
# define CATSysCheckExpression(exp)   ((int) ( (exp) ? -1 : CATSysCheckFailureSimple(__FILE__,__LINE__) ) )
#else // _MOBILE_SOURCE
# define CATSysCheckExpression(exp)   ((int) ( (exp) ? -1 : CATSysCheckFailure( "ExpressionCheck:"#exp,NULL,NULL,NULL,__FILE__,__LINE__) ) )
#endif  // _MOBILE_SOURCE

#endif
