#ifndef CATBaseCollection_h
#define CATBaseCollection_h

// COPYRIGHT DASSAULT SYSTEMES 1998

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "CATBaseUnknown.h"
class CATBaseDispatch;
class CATIAApplication;
#include "CATBSTR.h"
#include "JS0COL.h"

/**
 * Adaptor class for the CATIACollection Interface.
 * <b>Role</b>: 
 * This class provides a partial default implementation for the <tt>CATIACollection</tt> interface. 
 * All implementations of Automation interfaces being derived from <tt>CATIACollection</tt> should inherit 
 * from <tt>CATBaseCollection</tt>.
 * @see CATIACollection
 */
class ExportedByJS0COL CATBaseCollection : public CATBaseUnknown {
	
	public :
		
	/**
	 * Returns the application.
	 * See @href CATIACollection#get_Application for a complete description of this method.
	 * @param oApplication
	 *   The returned application
	 */
	virtual HRESULT __stdcall get_Application(CATIAApplication *&oApplication);
	
	/**
	 * Returns the parent of the object.
	 * See @href CATIACollection#get_Parent for a complete description of this method.
	 * All implementations must override this default implementation.
	 * @param oParent
	 *   The returned parent
	 */
	virtual HRESULT __stdcall get_Parent(CATBaseDispatch *&oParent);
	
	/**
	 * Retrieves an object with the specified name.
	 * See @href CATIACollection#GetItem for a complete description of this method.
	 * All implementations must override this default implementation.
	 * @param iName
	 *   The specified name of the object to retrieve
	 * @param oItem
	 *   The returned object
	 */
	virtual HRESULT __stdcall GetItem(const CATBSTR & iName, CATBaseDispatch *& oItem);
		
	/**
	 * Default constructor.
	 */
	CATBaseCollection();

	/**
	 * Copy constructor.
	 * @param iSrc
	 *   The object to copy
	 */
	CATBaseCollection(const CATBaseCollection & iSrc);

	virtual ~CATBaseCollection();
};

#endif //CATBaseCollection_h


