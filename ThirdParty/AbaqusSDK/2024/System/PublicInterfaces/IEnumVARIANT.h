#ifndef IEnumVARIANT_h
#define IEnumVARIANT_h

// COPYRIGHT DASSAULT SYSTEMES 2002

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifndef _WINDOWS_SOURCE

#include "IUnknown.h"
#include "CATVariant.h"

extern IID IID_IEnumVARIANT;

/**
 * Interface to traverse CATVariant collections.
 * <br><b>Role</b>:
 * The <tt>IEnumVARIANT</tt> interface is used by VB in the <tt>For Each</tt> 
 * instruction to traverse collections of objects. See
 * @href CATCreateIEnumVARIANT for a description of a function
 * which instantiates a object which implements <tt>IEnumVARIANT</tt>
 */
interface IEnumVARIANT : public IUnknown {
	public:
  
	/**
	 * @nodoc
	 * Returns the next iCelt elements.
	 * @param iCelt
	 * The number of elements to return.
	 * @param oRgvar
	 * an initialized CATVariant C++ array in which the resulting elements will be stored.
	 * @param oCeltFetched
	 * The number of elements returned.
	 */
	virtual __stdcall HRESULT Next(
		ULONG iCelt, 
		CATVariant *oRgvar, 
		ULONG *oCeltFetched) = 0;
	
	/**
	 * @nodoc
	 * Skips the current position to current + iCelt.
	 * @param iCelt
	 * The number of elements to skip.
	 */
	virtual __stdcall HRESULT Skip(
		ULONG iCelt) = 0;
    
	/**
	 * @nodoc
	 * Sets the current position to the first element.
	 */
	virtual __stdcall HRESULT Reset() = 0;

    /**
	 * @nodoc
	 * Gets a clone of IEnumVARIANT.
	 * @param oEnum
	 * The cloned IEnumVARIANT.
	 */
	virtual __stdcall HRESULT Clone(
		IEnumVARIANT **oEnum) = 0;	
};

#else // _WINDOWS_SOURCE
#include <oaidl.h>
#endif //_WINDOWS_SOURCE

#endif //IEnumVARIANT_h
