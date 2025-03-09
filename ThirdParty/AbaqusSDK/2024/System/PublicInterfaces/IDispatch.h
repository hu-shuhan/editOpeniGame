
// COPYRIGHT DASSAULT SYSTEMES 2000


/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

#ifndef __IDispatch
#define __IDispatch

#include "IUnknown.h"

/** @nodoc */
typedef wchar_t CATWCHAR_T;

#ifndef _WINDOWS_SOURCE

//
// Uuid of IDispatch
//
extern const GUID IID_IDispatch;

// forward
interface ITypeInfo;

/** @nodoc */
typedef struct tagVARIANT VARIANT;
/** @nodoc */
typedef struct tagDISPPARAMS DISPPARAMS;
/** @nodoc */
typedef struct tagEXCEPINFO EXCEPINFO;

//
// Definition of the IDispatch interface for UNIX
//
/**
 * Base interface for all Automation interfaces.
 * <b>Role</b>: All Automation interfaces derive from <tt>IDispatch</tt> which replaces for UNIX the
 * native Microsoft(R) <tt>IDispatch</tt> interface.
 * This interface supplies basic methods to be Microsoft(R) Automation compliant.
 */
interface IDispatch : public IUnknown
{
	//
	// Give the number of TypeInfo that the object provides
    //
	/** @nodoc */
	virtual HRESULT __stdcall GetTypeInfoCount(unsigned int *oNbOfTypeInfo)
									    = 0;
	
	//
	// Retrieves the type information for an object, which can 
	// then be used to get the type information for an interface. 
	//
    /** @nodoc */
	virtual HRESULT __stdcall GetTypeInfo(unsigned int iIndex,
					      ULONG iLangId, 
					      ITypeInfo **oPtTypeInfo)=0;
	
	//
	// Maps a single member and an optional set of argument names
	// to a corresponding set of integer DISPIDs
	//
    /** @nodoc */
	virtual HRESULT __stdcall GetIDsOfNames(const IID & forFutur,
						CATWCHAR_T **iArrayOfNames,
						unsigned int iNbNames,
						ULONG iLangId,
						LONG *oArrayOflong)=0;
	
	// 
	// Provides access to properties and methods exposed by an object.
	//
	/** @nodoc */
	virtual HRESULT __stdcall Invoke(LONG ilongMember, const IID & forFutur,
					 ULONG iLangId,
					 unsigned short iFlags,
					 DISPPARAMS *iPdisparams,
					 VARIANT *oPvaresult,
					 EXCEPINFO *oPexcepinfo,
					 unsigned int *oPuArgErr)=0;
};

#endif //_WINDOWS_SOURCE

#endif //__IDispatch

