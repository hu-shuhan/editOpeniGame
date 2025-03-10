#ifndef IUnknown_H
#define IUnknown_H

// COPYRIGHT DASSAULT SYSTEMES  1999

/**
 * @CAA2Level L1
 * @CAA2Usage U5 
 */


#ifndef _WINDOWS_SOURCE

#include "CATSysErrorDef.h"
#include "CATSysDataType.h"

/**
 * Return type for the QueryInterface method, and for most interface methods. 
 * <br><b>Role</b>: This type allows you to analyze the result of a call for
 * the <tt>QueryInterface</tt> method and for most interface methods. 

 * <ol>
 * <li>In the case of the <tt>QueryInterface</tt> method </li>
 * <br><br>To test the result, always use the macros @href SUCCEEDED and @href FAILED.
 * <pre>
 *
 *  IMyInterface * pIMyInterface = NULL ;
 *  HRESULT rc = pIMyOtherInterface->QueryInterface(IID_IMyInterface,(void**) &pIMyInterface);
 * 
 *  //Test with SUCCEEDED macro:
 *  
 *  if ( SUCCEEDED(rc) )
 *  {
 *      // In this case, rc is <b>S_OK</b>
 *      // pIMyInterface is valid 
 *      ....
 *  }
 *
 *  //Test with FAILED macro
 *
 *  if ( FAILED(rc) )
 *  {
 *      // In this case, rc is <b>E_NOINTERFACE</b> or <b>E_UNEXPECTED</b>
 *      // pIMyInterface is not valid 
 *  }
 *
 * </pre>
 * <li>In the case of an interface method </li>
 * <br><br>You can use the  macros @href SUCCEEDED and @href FAILED or test directly
 * the value to have more details. Refer to the method documentation to know about its 
 * possible HRESULT values.
 * <pre>
 *  IMyInterface * pIMyInterface = NULL ;
 *  HRESULT rc = pIMyOtherInterface->QueryInterface(IID_IMyInterface,(void**) &pIMyInterface);
 *  if ( SUCCEEDED(rc) )
 *  {
 *      // rc = S_OK
 *      rc = pIMyInterface->Method();
 *      
 *      // Test with SUCCEEDED macro
 *
 *      if ( SUCCEEDED(rc) )
 *      {
 *         // In this case, rc is <b>S_OK</b> or <b>S_FALSE</b>   
 *         // You can directly test the rc value  
 *         if ( S_FALSE == rc )
 *         {
 *                ...
 *         }else
 *         {
 *                ....
 *         }
 *      }
 *
 *      // Test with FAILED macro
 *
 *      if ( FAILED(rc) )
 *      {
 *         // In this case, rc is <b>E_xxxxx</b> 
 *         // You can directly test the rc value, 
 *         if ( E_FAIL == rc )
 *         {
 *               ....
 *         }
 *         ....
 *      }
 *  }
 * </pre> 
 * </ol>
 * The possible values are:
 * @param S_OK
 * Method succeeded. In some contexts, it means additionally that the function returns
 * a boolean true.
 * @param S_FALSE
 * Method succeeded. In some contexts, it means additionally that the function returns
 * a boolean false.
 * @param E_FAIL
 * Unspecified failure.
 * @param E_NOINTERFACE
 * Component does not support the requested interface.
 * @param E_UNEXPECTED
 * For an unexpected failure.
 * @param E_NOTIMPL 
 * Method is not implemented.
 * @param E_OUTOFMEMORY
 * Required memory can not be allocated.
 */

/**
 * The structure to accommodate a Globally Unique Identifier.
 */
typedef struct  _GUID
{
   DWORD Data1;
   WORD Data2;
   WORD Data3;
   BYTE Data4[8];
} GUID;
/**
 * Interface Globally Unique Identifier.
 */
typedef GUID IID;
/**
 * Class Globally Unique Identifier.
 */
typedef GUID CLSID;
/**
 * A type for struct to be Microsoft(R) compatible.
 */
#define interface struct
/**
 * Reference to IID.
*/
#define REFIID const IID &
/**
 * Microsoft(R)-specific extension to the compiler. Set to NULL for UNIX.
 */
#define __stdcall


/**
 * GUID of the class IUnknown
 */
extern const IID IID_IUnknown;

/**
 * Base interface for all CAA interfaces.
 * <b>Role</b>: All CAA interfaces derive from <tt>IUnknown</tt> which replaces for UNIX the
 * native Microsoft(R) <tt>IUnknown</tt> interface.
 * This interface supplies the three basic methods @href #QueryInterface, @href #AddRef and
 * @href #Release to be COM (Microsoft(R) Component Object Model) compliant.
 */
interface IUnknown
{
/**
 *
 * Returns a pointer to a given interface.
 * @param iIID
 *   The interface identifier for which a pointer is requested.
 * @param oPPV
 *   The address where the returned pointer to the interface is located.
 * @return
 *   <dl>
 *   <dt><tt>S_OK</tt>          <dd>If the query succeeds
 *   <dt><tt>E_NOINTERFACE</tt> <dd>If the interface does not exist
 *   <dt>Other                  <dd>If the query fails for any other reason.
 *   </dl>
 */
   virtual HRESULT __stdcall QueryInterface(const IID &iIID, void **oPPV) = 0;
/**
 * Increments the reference count for the given interface.
 * @return The reference count value.
 * <br>This information is meant to be used for diagnostic/testing purposes only, because, in certain situations, the value may be unstable.
 */
   virtual ULONG __stdcall AddRef() = 0;
/**
 * Decrements the reference count for the given interface.
 * @return The reference count value.
 * <br>This information is meant to be used for diagnostic/testing purposes only, because, in certain situations, the value may be unstable.
 */
   virtual ULONG __stdcall Release() = 0;
};

#else // _WINDOWS_SOURCE

#pragma once

#if defined(_MFC_VER) && _MFC_VER>=0x0800

#pragma warning(push)
#pragma warning(disable:4530) 
#include <objbase.h>
#pragma warning(pop)

#else

#include <objbase.h>

#endif

#include "CATSysErrorDef.h"
#include "CATSysDataType.h"

#endif // _WINDOWS_SOURCE

#endif // IUnknown_H
