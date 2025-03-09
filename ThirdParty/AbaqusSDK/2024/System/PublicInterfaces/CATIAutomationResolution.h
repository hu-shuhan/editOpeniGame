#ifndef CATIAutomationResolution_h
#define CATIAutomationResolution_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#include "JS0LOGRP.h"
#include "CATBaseUnknown.h"

class CATIScriptMethodCall;
class CATBaseDispatch;

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0LOGRP IID IID_CATIAutomationResolution;
#else
extern "C" const IID IID_CATIAutomationResolution;
#endif

/**
 * Interface for the resolution mechanism of the macros generation.
 * <b>Role:</b>
 * This interface must be implemented by objects that want to specify a custom
 * access path in the Automation container-content hierarchy.
 * <br>
 * It contains a single method which generates a <tt>CATIScriptMethodCall</tt>.
 * This generated object describes how to access the object from its container.
 * @see CATIScriptManager, CATIScriptJournal, CATIScriptMethodCall
 */
class ExportedByJS0LOGRP CATIAutomationResolution : public CATBaseUnknown {
  CATDeclareInterface;

  public :

  	/**
		 * Retrieves the resolution of the object.
  	 * <br><b>Role:</b>
		 * Retrieves a <tt>CATIScriptMethodCall</tt> which describes how 
		 * to access the object from its container.
		 * @param oResolution
		 * The returned method call that describes the resolution of the object.
		 * @param iObjectToResolve
		 * The object to resolve.
		 * This parameter can be used to delegate the resolution of an object to another one.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT GetResolution(CATIScriptMethodCall *& oResolution, CATBaseDispatch * iObjectToResolve) = 0;

};


#endif
