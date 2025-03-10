// COPYRIGHT DASSAULT SYSTEMES 2003
#ifndef CATERROR_H
#define CATERROR_H  42500

// COPYRIGHT DASSAULT SYSTEMS 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

//-----------------------------------------------------------------------------
// Abstract:	Base abstract class for error handling
//-----------------------------------------------------------------------------
// Usage:	This base abstract class implements all the required methods
//		The only method to be redefined is GetErrorTable which returns
//		a pointer to a table containing all the errors pertaining to
//		this class: this is automatically done by the msgp processor
//-----------------------------------------------------------------------------

#include <string.h>
#include "CATErrorDefs.h"
#include "CATErrDsc.h"
#include "CATErrParams.h"
#include "CATUnicodeString.h"
#include "CATAssert.h"

#ifndef _WINDOWS_SOURCE
#include "IErrorInfo.h"
#else 
#include "oaidl.h"
#include "oleauto.h"
#endif

// Error Key Format in message catalogue
// -------------------------------------
/** @nodoc */
#define MSG_CLE_PREFIX            "ERR_"    
/** @nodoc */
#define MSG_CLE_SUFFIX            "."
/** @nodoc */
#define MSG_CLE_SEPARATOR         "_"
/** @nodoc */
#define MSG_CLE_SUFFIX_ACTION     ".Request"	
/** @nodoc */
#define MSG_CLE_SUFFIX_CAUSE      ".Diagnostic"
/** @nodoc */
#define MSG_CLE_SUFFIX_SOLUTION   ".Advice"

// CATError Return Macros: Specify generic return code for method
// --------------------------------------------------------------
// return ok
/**
 * Method successful return code.
 * <br><b>Role</b>: Use it to successfully return from a method using the error manager.
 */
#define CATReturnSuccess             CATError::GetReturn(1,NULL,NULL,__FILE__,__LINE__) // S_OK

// return failed without a CATError
/**
 * Method failing return code.
 * <br><b>Role</b>: Use it to unsuccessfully return from a method using the error manager.
 */
#define CATReturnFailure             CATError::GetReturn(0,NULL,NULL,__FILE__,__LINE__) //E_UNEXPECTED

// return failed with a CATError
/**
 * Method failing return code returning an error class instance.
 * <br><b>Role</b>: Use it to unsuccessfully return from a method using the error manager,
 * if you want to associate the failure with an error class instance.
 */
#define CATReturnError(iptCATError)  CATError::GetReturn(2,iptCATError,NULL,__FILE__,__LINE__)

// CATError types
// --------------
/**
 * Error type.
 * @param CATErrorTypeCritical
 *   Critical error.
 *   A problem occurs that the method cannot handle.
 *   It returns as soon as the problem occurs.
 *   This is the default
 * @param CATErrorTypeFatal
 *   Fatal error.
 *   A problem occurs that the method cannot handle.
 *   It stops with no possible recovery.
 * @param CATErrorTypeWarning
 *   Warning.
 *   Something happens that doesn't stop the method.
 *   It completes and returns a warning.
 * @param CATErrorTypeInformation
 *   Information.
 *   The method successively completes.
 *   It returns information that can be handy
 */
enum CATErrorType { CATErrorTypeCritical = 1,
                    CATErrorTypeFatal = 2,
                    CATErrorTypeWarning = 3,
                    CATErrorTypeInformation = 4};


class CATError;
/**
 * Delete error chain.
 * @param errorPtr
 *   The first error to delete in the error chain
 */
extern ExportedByJS0ERROR void Flush (CATError* errorPtr);

class CATSysErrCx;
/*** 
 *** 
 ***/
enum  CATErrorNLSMsgSpecifier { CATErrorNLSSpcDefault=0, CATErrorNLSSpcRequest=1,CATErrorNLSSpcAdvice=2,CATErrorNLSSpcDiagnostic=3  };
//
// Abstract root class of all errors
//
/**
 * Base class for errors.
 * <b>Role</b>:
 * You cannot instantiate this class.
 * Derive it to create your own error class that refines the type of managed errors  using
 * @href CATDeclareError and @href CATImplementError macros.
 * 
 */
class ExportedByJS0ERROR CATError : public CATBaseUnknown {

  friend class CATSysErrCx;
  friend class CATErrorStreamer;
  friend class CATSysErrSet;
  public:
    /**
     * Delete error chain.
     * @param errorPtr
     *   The first error to delete in the error chain
     */
    friend ExportedByJS0ERROR void Flush (CATError* errorPtr);

    // Constructor 
    // -----------
	/**
	 * Constructs an error with a given error identifier and a given message.
	 * @param iMsgId
	 *   The message key
	 * @param iMsgCatalog
	 *   The message catalog in which the message is located
	 */
    CATError ( const char * iMsgId , const char * iMsgCatalog  );
	/** @nodoc */
    CATError (CATErrorId iId);
	/** @nodoc */
    CATError (CATErrorId iId, const char * iMsgId, const char * iMsgCatalog = NULL);
	/** @nodoc */
    CATError (const CATError & iError);

    // Destructor
    // ----------
    virtual ~CATError ();

    // General Information
    // -------------------
	/**
	 * Returns the error identifier.
	 */
    virtual CATErrorId GetId ();          // return the error id 
	/**
	 * Returns the error message key.
	 */
    virtual const char *GetMsgId ();      // return the message Id ( message key )
	/**
	 * Returns the error message catalog file name.
	 */
    virtual const char *GetMsgCatalog (); // return the message catalog
	/** @nodoc */
    const char *GetName ();               // reserved
    
    // File information where the error occurred
    // -----------------------------------------
	/**
	 * Retrieves the source file path name where the error occurs.
	 * @param oFile
	 *   The source file path name.
	 *   <br><b>Lifecycle rules deviation</b>: Do not delete this pointer.
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The source file path name is successively retrieved
	 *     <dt>E_FAIL  <dd>The source file path name can't be retrieved
	 *   </dl>
	 */
    virtual HRESULT GetSourceFileName(  char ** oFile );
    /**
	 * Sets the source file path name where the error occurs.
	 * @param iFile
	 *   The source file path name
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The source file path name is successively set
	 *     <dt>E_FAIL  <dd>The source file path name can't be set
	 *   </dl>
	 */
    virtual HRESULT SetSourceFileName( const char * iFile );

	/**
	 * Retrieves the line number in the source file where the error occurs.
	 * @param oLine
	 *   The line number
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The line number is successively retrieved
	 *     <dt>E_FAIL  <dd>The line number can't be retrieved
	 *   </dl>
	 */    
    virtual HRESULT GetSourceLineNumber(  int *oLine );
	/**
	 * Sets the line number in the source file where the error occurs.
	 * @param iLine
	 *   The line number
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The source file path name is successively set
	 *     <dt>E_FAIL  <dd>The source file path name can't be set
	 *   </dl>
	 */
    virtual HRESULT SetSourceLineNumber(  int iLine );
    
    
    // Get Errors messages
    // ------------------------------
	/** @nodoc */
    virtual const char *GetMessageText (int longMessage = 0); 	//reserved

   	/**
	 * Returns the error message.
	 * <br><b>Role</b>: The three parts Request, Diagnostic, and Advice
	 * of the error message are returned concatenated in a single message.
	 */
    virtual CATUnicodeString GetNLSMessage ();
   	/**
	 * Returns the Request part of the error message.
	 */
    virtual CATUnicodeString GetNLSRequest ();
   	/**
	 * Returns the Diagnostic part of the error message.
	 */
    virtual CATUnicodeString GetNLSDiagnostic ();
   	/**
	 * Returns the Advice part of the error message.
	 */
    virtual CATUnicodeString GetNLSAdvice ();

   	/**
	 * Returns the error source.
	 * <br><b>Role</b>: The error source is made of the concatenation of:
	 * <ul>
	 * <li>the source context. It is the name of the application
	 * that issues the error, usually CATIA
	 * <li>the error message catalog file name
	 * <li>the error message key in the error message catalog
	 * </ul>
	 * They are separated with the "-" (dash) character.
	 * <br>Example: CATIA - SystemError - KeyRangeERR_3000 
	 */
    virtual CATUnicodeString GetNLSSource ();
   	/** @nodoc */
    virtual CATUnicodeString GetNLSHelpFile ();
	/** @nodoc */
    virtual DWORD GetNLSHelpContext ();

        
    // outputs one the error messages
	/** @nodoc */
    virtual void Display ();
    // output the error history
	/** @nodoc */
    virtual void PrintHistory ();
    
    // Throw
    /** 
     * @nodoc 
     * The C++ attribute [[noreturn]] allows the compiler to perform additionnal optimizations, 
     * avoid spurious warnings of uninitialized variables and detect unreachable code.
     */
    [[noreturn]] void Throw (const char *loc, int line);
    
    // Set the error message parameters
    // --------------------------------
	/** @nodoc */
    void SetParameters (int numIParams, int I1, ...);		//reserved
	/** @nodoc */
    void SetParameters (int numRParams, double R1, ...);	//reserved
	/** @nodoc */
    void SetParameters (int numCParams, const char *C1, ...);	//reserved
    /**
	 * Sets the values of the error message parameters.
	 * <br><b>Role</b>: The error message can contain parameters that are valued at run time
	 * when the error occurs.
	 * Their values must first be converted as CATUnicodeString instances.
	 * This method has a variable number of parameters to match the error message
	 * parameter number.
	 * Use this method for a non-composite error message, that is, an error message without
	 * Request, Diagnostic, and Advice parts.
	 * @param iNumNLSParam
	 *   The number of parameters of the error message
	 * @param iUS1
	 *   The first parameter value converted as a CATUnicodeString instance
     */
    virtual void SetNLSParameters (int iNumNLSParam, const CATUnicodeString * iUS1, ...);
    /**
	 * Sets the values of the Request part of the error message parameters.
	 * <br><b>Role</b>: The Request part of error message can contain parameters
	 * that are valued at run time when the error occurs.
	 * Their values must first be converted as CATUnicodeString instances.
	 * This method has a variable number of parameters to match the error message
	 * parameter number.
	 * @param iNumNLSParam
	 *   The number of parameters of the error message
	 * @param iUS1
	 *   The first parameter value converted as a CATUnicodeString instance
     */
    virtual void SetNLSRequestParams (int iNumNLSParam, const CATUnicodeString * iUS1, ...);
    /**
	 * Sets the values of the Diagnostic part of the error message parameters.
	 * <br><b>Role</b>: The Diagnostic part of error message can contain parameters
	 * that are valued at run time when the error occurs.
	 * Their values must first be converted as CATUnicodeString instances.
	 * This method has a variable number of parameters to match the error message
	 * parameter number.
	 * @param iNumNLSParam
	 *   The number of parameters of the error message
	 * @param iUS1
	 *   The first parameter value converted as a CATUnicodeString instance
     */
    virtual void SetNLSDiagnosticParams (int iNumNLSParam, const CATUnicodeString * iUS1, ...);
    /**
	 * Sets the values of the Advice part of the error message parameters.
	 * <br><b>Role</b>: The Advice part of error message can contain parameters
	 * that are valued at run time when the error occurs.
	 * Their values must first be converted as CATUnicodeString instances.
	 * This method has a variable number of parameters to match the error message
	 * parameter number.
	 * @param iNumNLSParam
	 *   The number of parameters of the error message
	 * @param iUS1
	 *   The first parameter value converted as a CATUnicodeString instance
     */
    virtual void SetNLSAdviceParams (int iNumNLSParam, const CATUnicodeString * iUS1, ...);

	/**
	 * Sets the source context.
	 * <br><b>Role</b>: The source context is the name of the application
	 * that issues the error. It is CATIA by default.
	 * @param icontext
	 *   The source context
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The source context is successively set
	 *     <dt>E_FAIL  <dd>The source context can't be set
	 *   </dl>
	 */
    virtual HRESULT SetSourceContext( const char * icontext );
	/**
	 * Retrieves the source context.
	 * <br><b>Role</b>: The source context is the name of the application
	 * that issues the error.
	 * @param ocontext
	 *   The source context.
	 *   <br><b>Legal values</b>: It is CATIA by default.
	 * @return
	 *   An HRESULT.
	 *   <dl>
	 *     <dt>S_OK    <dd>The source context is successively retrieved
	 *     <dt>E_FAIL  <dd>The source context can't be retrieved
	 *   </dl>
	 */ 
    virtual HRESULT GetSourceContext( char **ocontext );
        
    // HRESULT management 
    // ------------------
	/**
	 * Returns the error return code for error propagation.
	 * <br><b>Legal values</b>: Valid HRESULT values.
	 */
    virtual HRESULT GetReturnCode();		// Return a COM compatible HRESULT :default E_FAIL
	/**
	 * Sets the error return code for error propagation.
	 * <br><b>Role</b>: Replaces the default return code (E_FAIL).
	 * This return code is used when the error needs to be propagated, that is,
	 * when the method in error returns using the @href CATReturnError macro.
	 * @param iHRESULTCode
	 *   The error code.
	 *   <br><b>Legal values</b>: It must be a valid HRESULT value.
	 */
    virtual void SetReturnCode(HRESULT iHRESULTCode);	// Modify HRESULT
    
    // Error type
    // ----------
	/**
	 * Returns the error type.
	 */
    virtual CATErrorType GetInformationOnErrorType ();            // Return the Error type : default Critical
	/**
	 * Sets the error type.
	 * @param iErrorType
	 *   The error type to set to the error
	 */
    virtual HRESULT SetInformationOnErrorType( CATErrorType iErrorType);    // Modify the Error type 
   
    // Errors Management
    // -----------------
    // Gets the CATError pointer set by CATSetLastError
	/**
	 * Returns a pointer to the last error class instance.
	 * @param iHR
	 *   The return code of the method in error
	 * @param iInterfaces
	 *   The pointer to the interface used to call the method in error.
	 *   <br><b>Legal values</b>: Set it to NULL in case of non-distribution, and
	 *   to the interface pointer used to call the method otherwise. 
	 */
    static CATError* CATGetLastError ( HRESULT iHR, IUnknown *iInterfaces = NULL );
    
    // Release the current CATError if one exists
	/**
	 * Cleans the last returned error.
	 * <br><b>Role</b>: Deletes the last returned error instance and cleans
	 * the error manager.
	 */
    static void CATCleanLastError ( );
     
    // Sets the CATError pointer. Do not use this directly, use CATError Return Macros
	/**
	 * Sets the error to return.
	 * <br><b>Role</b>: Once you have created the error class instance in the method in error,
	 * use CATSetLastError to pass it to the error manager. The error can then be retrieved
	 * by the caller using @href #CATGetLastError
	 * @param iInterfaces
	 *   <br><b>Legal values</b>: Set it always to NULL
	 */
    void CATSetLastError (IUnknown *iInterfaces = NULL );
      
    // Internal Return Code Managment 
    // -----------------------------
	/** @nodoc */
    static HRESULT GetReturn ( int , CATError *ierr, IUnknown *iinterface, const char *ifile, int iline );  //reserved
   
    // Internal COM Errors Management
    // ------------------------------
	/** @nodoc */
    IErrorInfo* SetErrorInfoFromCATError( IUnknown *iinterface ); //reserved
    
    // Internal bookeeping
    // -------------------
	/** @nodoc */
    static CATError *GetCurrentError ();
	/** @nodoc */
    static CATError *SetCurrentError (CATError *err);
	/** @nodoc */
    CATDeclareBaseErrorClass (CATError)
 
	/** @nodoc */
    //
    // GetNLSParametersArray allows to get an array of CATUnicodeString  with the parameters associated to the error
    // *  If the error is associated to a simple error message and
    //       iSpecs=CATErrorNLSSpcDefault
    //     the array returned will be  an array of *oCount +1 CATUnicodeString  .
    //     S_OK will be returned if the array is retreived or if there is not parameters associated to the error
    //
    // * If the error is associated to a multiple error message ( Request/ Advice / Diagnostics ) 
    //     and iSpec=CATErrorNLSSpcDefault
    //    if the three optional parameters oCountRq, oCountDiag and oCountAdv are NOT set the method will return S_FALSE
    //       no array of parameters will be given in this case 
    //    if the three option parameters oCountRq , oCountDiag and oCountAdv are  set to non null values the method then
    //     the array returned will be an array of *oCount +1 CATUnicodeString  .
    //     the first part of the array ( from 0 to oCountRq-1) will contain the parameters for the request                 
    //     the 2nd   part of the array ( from *oCountRq to *oCountRq+*oCountDiag-1) will contain the parameters for the diagnostic
    //     the 3nd   part of the array ( from *oCountRq+*oCountDiag to *oCount-1) will contain the parameters for the advice
    //     S_OK will be returned if the array is retreived or if there is not parameters associated to the error
    //
    // * If the error is associated to a multiple error message ( Request/ Advice / Diagnostics ) 
    //     and iSpec is not equal to CATErrorNLSSpcDefault
    //   Then the corresponding array of parameters will be returned
    //   
    HRESULT GetNLSParametersArray( CATUnicodeString **oArray, int *oTotalCount,CATErrorNLSMsgSpecifier iSpecs=CATErrorNLSSpcDefault, int *oCountRq=NULL, int *oCountDiag=NULL, int *oCountAdv=NULL);


    /** @nodoc Handle lifecycle of CATError during Throw - System internal use only!!! */
    class ExportedByJS0ERROR ThrowSmartPtr
    {
        /** @nodoc */
        CATError *m_ptr;
    public:
        /** @nodoc */
        ThrowSmartPtr(CATError *ipErr) noexcept;
        /** @nodoc */
        ThrowSmartPtr(ThrowSmartPtr &&) noexcept;
        /** @nodoc Still required by the C++ standard */
        ThrowSmartPtr(const ThrowSmartPtr &);
        /** @nodoc Not virtual by intent */
        ~ThrowSmartPtr();
        /** @nodoc cf. std::unique_ptr::get, with unambiguous naming */
        CATError * get_CATError() const noexcept;
        
    private:
        friend class CATXHContextCxx;
        friend struct CGMTkCompatCtx;
    #if defined(CATXHContextCxx) || defined(CGMTkCompatCtx)
    #error Do not attempt to bypass friendness!
    #endif
    };

  protected:
     
    // Internal Error Manager
    // ----------------------
	/** @nodoc */
	static void SetLastCATError ( CATError* ierr );
	/** @nodoc */
    static CATError* GetLastCATError ( HRESULT ihr );
    
    /** @nodoc */
#define CATError_OnBeforeThrow_Defined
    /** @nodoc Restricted usage + do not expect this function to be called in case of rethrow (CATRethrow) */
    virtual void OnBeforeThrow(const char * /*loc*/, int /*line*/) {}
    
	/** @nodoc */
    CATErrorId ExtractCATErrorId( CATUnicodeString& ); 
	/** @nodoc */
    const CATErrDsc *LookupEntry ();

	/** @nodoc */
    CATErrParams params;
	/** @nodoc */
    CATErrDsc   *tableEntry;
	/** @nodoc */
    CATErrorId   id;
	/** @nodoc */
    int          lineno;
	/** @nodoc */
    char         message[128];
	/** @nodoc */
    char         filename[128];
	/** @nodoc */
    CATString   *msgCatalog;
	/** @nodoc */
    CATString   *msgId;
	/** @nodoc */
    HRESULT    ReturnCode; 
	/** @nodoc */
    int          NLSParamCount;
	/** @nodoc */
    CATUnicodeString *NLSParams;
	/** @nodoc */
    CATUnicodeString *NLSParamsRequest;
	/** @nodoc */
    int          NLSParamCountRequest; /** @win64 fbq : 64-bit structure padding **/
	/** @nodoc */
    int          NLSParamCountDiagnostic;
	/** @nodoc */
    CATUnicodeString *NLSParamsDiagnostic;
	/** @nodoc */
    CATUnicodeString *NLSParamsAdvice;
	/** @nodoc */
    CATUnicodeString *msgcontext;
	/** @nodoc */
    int          NLSParamCountAdvice; /** @win64 fbq : 64-bit structure padding **/
    

   private:
  /**
   ** allows to stream the CATError into a transportable binary buffer  allowing to transport
   ** and build a proxy error  containing the error message associated to the original error 
   ** in another address space
   **
   ** The objective is not to keep all the informations stored in the error which may be meaningless
   ** in another address space but to allow to transport the associated error message on the other side
   ** In particular, the original class of the error will not be kept.
   **
   ** So please take note that only some attributes of the error are streamed.
   ** So if one build a CATError out of a streamed CATError , the built CATError will not 
   ** be exactly equivalent to the original CATError .
   ** The streamed attributes are : 
   **    - the attributes allowing to rebuild the NLS error messages associated to the error
   **    - the severity 
   **    - the associated HRESULT 
   **
   ** Informations which are not streamed
   **    - the original class of error is lost
   **    - the original class of error is lost
   **
   ** Parameters : 
   **    oBuffer : Stream will allocate the stream buffer and return of pointer on this buffer to the caller
   **              The caller of Stream is in charge of freeing this memory by using FreeStreamBuffer    
   **    oLen    :  oLen will be set to the lenght of the buffer returned by StreamBuffer and pointed to by oBuffer
   **
   ** Return code :
   **      S_OK if successful
   **      E_FAIL : if the streaming operation failed
   **
   **/
  /** @nodoc */
  HRESULT Stream(void **oBuffer,int *oLen );
  /*** 
   ***  FreeStreamBuffer  allows to free the oBuffer buffer returned by the Stream method
   ***/
  HRESULT FreeStreamBuffer( void *iBuffer);
  /***
   ***  Create an error initialising with the iBuffer information
   ***  iBuffer : input stream buffer 
   ***  iLen    : length of iBuffer
   ***  oError  : created error if successful 
   *** 
   ***  Return values :
   ***       S_OK if successful
   ***       ! S_OK if failed
   *** 
   ***/
  HRESULT BuildFromStream(const void *iBuffer,int iLen  );
  
 private:
    
    CATError *previousError;
    int msgtype;
    void  Init (CATErrorId iId, const char *imsgId, const char *imsgCatalog);
    CATUnicodeString *comhelpfile;
    DWORD comhelpcontext;
    CATErrorType errtype;
    char * sourcename;
   
    //
    CATError *m_NxInCx;
    CATError *m_PvInCx;
    CATSysErrCx *m_Cx;

    CATError *m_NxInSt;
};

	/** @nodoc */
[[noreturn]] ExportedByJS0ERROR void CATTerminate ();

	/** @nodoc */
ExportedByJS0ERROR CATTerminateFunction CATSetTerminate (CATTerminateFunction func);

#ifndef _WINDOWS_SOURCE
	/** @nodoc */
ExportedByJS0ERROR HRESULT SetErrorInfo ( long ival, IErrorInfo *iperrinfo );
	/** @nodoc */
ExportedByJS0ERROR HRESULT GetErrorInfo ( long ival, IErrorInfo**opperrinfo );
#endif

/** 
 * @nodoc This 'typedef' abstracts 'ThrowSmartPtr', which is an implementation detail
 * It is restricted for use with CATError-derived types. 
 */
template<typename _Ty, typename std::enable_if<std::is_base_of<CATError,_Ty>::value, bool>::type = true>
using CATException = typename _Ty::ThrowSmartPtr;

#endif  // CATERROR_H
