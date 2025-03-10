#ifndef CATCOMERRORS_H
#define CATCOMERRORS_H

// Copyright DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//-----------------------------------------------------------------------------
// Abstract:	The class of errors coming from COM/DCOM Error
//		 
//-----------------------------------------------------------------------------
#include "CATError.h"
#include "CATErrorDefs.h"

#ifndef _WINDOWS_SOURCE
#include "IErrorInfo.h"
/** @nodoc */
typedef GUID&  REFGUID;
#endif

/**
 * Class for COM errors.
 * <b>Role</b>: CATCOMErrors is dedicated to any COM/DCOM error.
 * You cannot instantiate it. You can just retrieve a pointer to such an error
 * using the error manager static @href CATError#CATGetLastError method.
 */
class ExportedByJS0ERROR CATCOMErrors : public CATError {

  public:
    
    CATDeclareError(CATCOMErrors,CATError)

	/** @nodoc */
    CATCOMErrors();
    virtual ~CATCOMErrors();  
     
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
    CATUnicodeString GetNLSSource ();
   	/**
	 * Returns the error message.
	 */
    CATUnicodeString GetNLSMessage ();
	/** @nodoc */
    CATUnicodeString GetNLSHelpFile ();
    /** @nodoc */
    DWORD GetNLSHelpContext ();
	/** @nodoc */
    HRESULT GetGUID (GUID*);
       
	/** @nodoc */
    HRESULT SetSource (CATUnicodeString*);
	/** @nodoc */
    HRESULT SetDescription (CATUnicodeString*);
	/** @nodoc */
    HRESULT SetHelpFile (CATUnicodeString*);
	/** @nodoc */
    HRESULT SetHelpContext (DWORD);
	/** @nodoc */
    HRESULT SetGUID (REFGUID);
    
    /** @nodoc */ 
    static CATCOMErrors* SetCATErrorFromIErrorInfo ( IUnknown *iinterfaces );
    
   private :
      
      
     HRESULT comhr;    
     CATUnicodeString *comsource;
     CATUnicodeString *comdescription;
     CATUnicodeString *comhelpfile;
     GUID comguid;
     DWORD comhelpcontext;
	 
     
              

};

#endif


