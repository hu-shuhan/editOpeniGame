#ifndef CATBaseDispatch_h
#define CATBaseDispatch_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "JS0DSPA.h"
#include "CATBaseUnknown.h"
#include "CATSTGErrors.h"
#include "CATBSTR.h"

class CATInternalDispatch;
extern ExportedByJS0DSPA const GUID CLSID_CATBaseDispatch;


/** @nodoc */
#define IID_CATBaseDispatch CLSID_CATBaseDispatch

/**
 * Base class for Automation interfaces.
 * <b>Role</b>: All Automation interfaces must inherit from
 * the <tt>CATBaseDispatch</tt> interface. They usually do not
 * inherit directly from <tt>CATBaseDispatch</tt>, but rather
 * from one of its subclasses: <tt>CATIABase</tt> individual objects 
 * or <tt>CATIACollection</tt> for collection objects.
 * Some methods may however have arguments of type <tt>CATBaseDispatch</tt>
 * when they accept both individual objects or collection objects.
 * The interface provides no functionalities per se.
 */ 
class ExportedByJS0DSPA CATBaseDispatch : public CATBaseUnknown {
	CATDeclareClass;

	public:
		/**
		 * Retrieves a pointer to a given interface.
		 * @param iid
		 *   The interface identifier for which a pointer is requested.
		 * @param ppv
		 *   The address where the returned pointer to the interface is located.
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
		virtual HRESULT __stdcall QueryInterface(const IID &iid, void **ppv);

		/** @nodoc */
		CATBaseDispatch();

		/** @nodoc */
		virtual ~CATBaseDispatch();

		private:
		/** @nodoc */
		CATInternalDispatch	*_pAggregatedIDispatch;

};

/** @nodoc */
CATDeclareHandler(CATBaseDispatch, CATBaseUnknown);

#endif
