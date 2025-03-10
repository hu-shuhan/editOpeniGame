
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L0
 * @CAA2Usage U5
 */

#ifndef CATIBaseAccess_H
#define CATIBaseAccess_H

#include "JS0INF.h"
#include "CATBaseUnknown.h"

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0INF IID IID_CATIBaseAccess ;
#else
extern "C" const IID IID_CATIBaseAccess ;
#endif

class CATBaseDispatch;
class CATIABase;

/**
 * Interface used to retrieve the accurate CATBaseDispatch or CATIABase interface on a component.
 * Implements this interface if your component implements several times the 
 * CATBaseDispatch or CATIABase interface to give the right implementation to the client.
 */
class ExportedByJS0INF CATIBaseAccess: public CATBaseUnknown
{
  CATDeclareInterface;

  public:

    /**
		 * Retrieves the accurate CATBaseDispatch or CATIABase interface on the current component.
		 * @param oBasePointer
		 * The returned interface pointer.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
     */
    virtual HRESULT GiveAccurateExposedInterface(CATBaseDispatch** oBasePointer)=0;
};

/** @nodoc */
CATDeclareHandler( CATIBaseAccess, CATBaseUnknown );

/**
 * Retrieves the accurate CATIABase interface on the component iComponent.
 * The method applies the following algorithm :
 *  Does iComponent offer the interface CATIBaseAccess ?
 *    - if yes, returns the result of the CATIBaseAccess method GiveAccurateExposedInterface if this result is a kind of CATIABase
 *    - else returns the result of the QueryInterface on iComponent for the interface CATIABase
 * @param iComponent
 * The component to test.
 * @param oBasePointer
 * The returned interface pointer.
 * @return
 *   An HRESULT value. 
		 *   <br><b>Legal values</b>:
		 *   <dl>
		 *     <dt>S_OK</dt>
		 *     <dd>The query succeeds.</dd>
		 *     <dt>E_NOINTERFACE </dt>
		 *     <dd>The interface does not exist.</dd>
		 *     <dt>E_FAIL </dt>
		 *     <dd>The object is not valid.</dd>
		 *     <dt>E_OUTOFMEMORY </dt>
		 *     <dd>One memory allocation fails</bb>
		 *     <dt>E_UNEXPECTED </dt>
		 *     <dd>The query fails for any other reason</dd>
		 *   </dl>
 */
ExportedByJS0INF HRESULT AccurateQueryInterfaceOnCATIABase(IUnknown * iComponent, CATIABase** oBasePointer);

#endif
