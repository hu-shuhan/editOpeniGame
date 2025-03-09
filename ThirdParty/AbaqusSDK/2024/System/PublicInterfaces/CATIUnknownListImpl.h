#ifndef INCL_CATIUnknownListImpl_h
#define INCL_CATIUnknownListImpl_h

//
// COPYRIGHT DASSAULT SYSTEMES  1999
//

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Class which implements CATIUnknownList interface. 
 * @see CATIUnknownList
 */

#include "CATIUnknownList.h"
#include "CATLISTPIUnknown.h"
#include "ListImpl.h"

class ExportedByListImpl CATIUnknownListImpl : public CATIUnknownList {

	    /** @nodoc */
		CATDeclareClass;

	public:

		/** Constructor. */
		CATIUnknownListImpl();

		/** 
		* Constructor with a list. 
		* @param iObjects
		*    List of IUnknown
		*/
		CATIUnknownListImpl(CATLISTP(IUnknown) iObjects);

		virtual ~CATIUnknownListImpl();

		/**
		 * @nodoc
		 * Returns the count of elements in the list.
		 * @param oCount
		 *   Count of elements
		 */
		virtual HRESULT Count(
			unsigned int *oCount
		);

		/**
		 * @nodoc
		 * Returns the iPosition-th element.
		 * @param iPosition
		 *   Element to be returned.
		 * @param oItem
		 *   Returned element.
		 * @return
		 * If iPosition does not respect respect the following rule:
		 * 0 &lt;= position &lt; Count, E_FAIL is returned.
		 */
		virtual HRESULT Item(
			const unsigned int iPosition,
			IUnknown **oItem
		);

		/**
		 * @nodoc
		 * Adds an element at a given position in the list.
		 * @param iPosition
		 *   Position of the added element in the list.
		 * @param iItem
		 *   Element to be added.
		 * @return
		 * If iPosition does not respect respect the following rule:
		 * 0 &lt;= position &lt; Count, E_FAIL is returned.
		 * The element to add must not be a NULL reference or
		 * E_POINTER will be returned.
		 */
		virtual HRESULT Add(
			const unsigned int iPosition,
			IUnknown *iItem
		);

		/**
		 * @nodoc
		 * Removes the first occurence of an element from the list.
		 * @param iItem
		 *   Element to be removed.
		 * @return 
		 * If the element cannot be found in the list, E_FAIL
		 * is returned.
		 */
		virtual HRESULT Remove(
			IUnknown *iItem
		);

		/**
		 * Removes the occurence of an element which position is indicated
		 * by iPosition from the list.
		 * @param iPosition
		 *   position of the element to be removed.
		 * @return 
		 * @return
		 * If iPosition does not respect respect the following rule:
		 * 0 &lt;= position &lt; Count, E_FAIL is returned.
		 */
		virtual HRESULT Remove (
			const unsigned int iPosition
		);

	protected:
		CATLISTP(IUnknown) _objects;
};

/** @nodoc */
/* df648466-95f7-11d3-b80a-0008c70fc870 */
extern "C" const CLSID CLSID_CATIUnknownListImpl;

#endif
