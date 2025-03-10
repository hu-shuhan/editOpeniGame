#ifndef __CATInstantiateComponent
#define __CATInstantiateComponent

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "CATBaseUnknown.h"

/**
 * Creates an class instance by its name.
 * @param iname
 *   The class name for which an instance is requested.
 * @param iid
 *   The identifier of the interface which is queryied on the implementation.
 * @param oppv [out, IUnknown#Release]
 *   The address where the returned pointer to the interface is located.
 * @return
 *   <dl>
 *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
 *   <dt><tt>E_UNEXPECTED</tt>  <dd>for an unexpected failure
 *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
 *   </dl>
 */
ExportedByJS0CORBA HRESULT __stdcall CATInstantiateComponent(const char *iname,
                                                            const IID &iid,
                                                            void **oppv);

#endif
