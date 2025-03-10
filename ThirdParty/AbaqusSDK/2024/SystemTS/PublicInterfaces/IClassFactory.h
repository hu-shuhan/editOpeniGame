#ifndef INCL_IClassFactory_h
#define INCL_IClassFactory_h

/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#ifdef _WINDOWS_SOURCE

#include <unknwn.h>

#else /* _WINDOWS_SOURCE */

#include "IUnknown.h"
#include "CATBooleanDef.h"

/**
 * Definition of a boolean type used by IClassFactory.
 */
typedef int BOOL;

//
// COPYRIGHT DASSAULT SYSTEMES  1999
//=============================================================================
// Aug. 99   Creation
//=============================================================================
//
/**
 * IClassFactory objects factories are dedicated to instantiating components of
 * a specified CLSID.
 */
class IClassFactory : public IUnknown {

public:

	/**
	 * Creates an uninitialized object of a specified CLSID.
	 * @param iUnkOuter
	 *     Pointer to whether object is or is not part of an aggregate.
	 * @param riid
	 *     Reference to the identifier of the interface.
	 * @param oObject
	 *     Address of output variable that receives the interface pointer requested in riid.
	 */
	virtual HRESULT __stdcall CreateInstance(
                                    IUnknown * iUnkOuter,// Pointer to whether object is or isn't part of an aggregate
                                    REFIID riid,         // Reference to the identifier of the interface  
                                    void ** oObject      // Address of output variable that receives the interface pointer requested in riid.
                                            ) = 0;

	/**
	 * Used to keep an out-process server in memory.
	 * @param iLock
	 *     Increments or decrements the lock count.
	 */
	virtual HRESULT __stdcall LockServer(
                                     BOOL iLock          // Increments or decrements the lock count
                                        ) = 0;
};

/* 00000001-0000-0000-C000-000000000046 */
/**
 * IID of the interface IClassFactory.
 */
extern "C" const IID IID_IClassFactory;

#endif /* _WINDOWS_SOURCE */

#endif
