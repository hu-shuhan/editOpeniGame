#ifndef _CATBaseObject_h
#define _CATBaseObject_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "CATIABase.h"
#include "CATUnicodeString.h"
#include "JS0LOGRP.h"

/**
 * Adaptor class for the CATIABase Interface.
 * <b>Role</b>: 
 * This class provides a default implementation for the <tt>CATIABase</tt> interface. 
 * All implementations of Automation interfaces being derived from <tt>CATIABase</tt> should inherit 
 * from <tt>CATBaseObject</tt>.
 * @see CATIABase
 */
class ExportedByJS0LOGRP CATBaseObject : public CATBaseUnknown {

	public:

		/**
		 * Constructs a CATBaseObject with the given parent.
		 * @param iParent
		 *   The parent of the CATBaseObject
		 */
		CATBaseObject(CATBaseDispatch *iParent);

		/**
		 * Default constructor.
		 */
		CATBaseObject();

		/**
		 * Sets the parent of the CATBaseObject.
		 * @param iParent
		 *   The parent of the CATBaseObject
		 */
		void SetParent(CATBaseDispatch *iParent);
		
		/**
		 * @nodoc
		 */
		~CATBaseObject();

		/**
		 * @nodoc
		 */
		virtual HRESULT ChangeComponentState(	
			ComponentState iFromState,
			ComponentState iToState,
			const CATSysChangeComponentStateContext * iContext);

		/**
		 * Returns the application.
		 * See @href CATIABase#get_Application for a complete description of this method.
		 * @param oApplication
		 *   The returned application
		 */
		virtual HRESULT  get_Application(CATIAApplication *& oApplication);
		
		/**
		 * Returns the parent of the object.
		 * See @href CATIABase#get_Parent for a complete description of this method.
		 * All implementations must override this default implementation.
		 * @param oParent
		 *   The returned parent
		 */
		virtual HRESULT __stdcall get_Parent(CATBaseDispatch *& oParent);

		/**
		 * Returns the name of the object.
		 * See @href CATIABase#get_Name for a complete description of this method.
		 * All implementations must override this default implementation.
		 * @param oNameBSTR
		 *   The returned name
		 */
		virtual HRESULT  get_Name(CATBSTR & oNameBSTR);
		
		/**
		 * Sets the name of the object.
		 * See @href CATIABase#put_Name for a complete description of this method.
		 * All implementations must override this default implementation.
		 * @param iNameBSTR
		 *   The name to set
		 */
		virtual HRESULT  put_Name(const CATBSTR & iNameBSTR);

		/**
		 * @nodoc
		 */
		virtual HRESULT  SetName(const CATUnicodeString & iName);

		/**
		 * Retrieves an object with the specified name.
		 * See @href CATIABase#GetItem for a complete description of this method.
		 * All implementations must override this default implementation.
		 * @param iNameBSTR
		 *   The specified name of the object to retrieve
		 * @param oObject
		 *   The returned object
		 */
		virtual HRESULT GetItem(const CATBSTR & iNameBSTR, CATBaseDispatch *& oObject);

		/**
		 * @nodoc
		 */
                static HRESULT __stdcall sGetApplication(CATIAApplication *& oApplication);

		/**
		 * @nodoc
		 */
                static HRESULT __stdcall sComputeName(CATBaseDispatch* iObject, CATUnicodeString& oName);

		/**
		 * @nodoc
		 */
                static HRESULT __stdcall sGetItem(CATBaseDispatch* iTarget,
                                                  const CATBSTR & iName, 
                                                  CATBaseDispatch *& oObject);
  
		/**
		 * @nodoc
		 */
		static long nbvar;

	protected:
  
		/**
		 * @nodoc
		 */
		CATBaseObject & operator=(CATBaseObject & iObject);

		/**
		 * @nodoc
		 */
		CATBaseObject(const CATBaseObject & iObject);

		/**
		 * The stored parent.
		 */
		CATBaseDispatch *_parent;

		/**
		 * The name of the object.
		 */
		CATUnicodeString _name;
};

#endif // _CATBaseObject_h
