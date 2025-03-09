#ifndef __CATICreateInstance
#define __CATICreateInstance

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#include "JS0CORBA.h"
#include "CATBaseUnknown.h"

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0CORBA IID IID_CATICreateInstance;
#else
extern "C" const IID IID_CATICreateInstance;
#endif


/**
 * Interface used to create an implementation instance.
 * CATICreateInstance must be implemented by a <tt>CodeExtension</tt> of the component to create.
 */
class ExportedByJS0CORBA CATICreateInstance : public CATBaseUnknown
{
   CATDeclareInterface;
   public :
/**
 * Creates an instance.
 * @param oPPV
 *  The address of the pointer to the requested object.
 * @return
 *   An HRESULT.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>S_OK</dt>
 *     <dd>The component is successfully created.</dd>
 *     <dt>E_OUTOFMEMORY </dt> 
 *     <dd>The component allocation failed</dd>
 *   </dl>
 */
      virtual HRESULT __stdcall CreateInstance(void **oPPV) = 0;
};

#endif
