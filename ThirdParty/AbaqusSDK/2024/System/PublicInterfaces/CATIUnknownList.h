#ifndef INCL_CATIUnknownList_h
#define INCL_CATIUnknownList_h

// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATBaseUnknown.h"

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

/**
 * Interface to handle a list of IUnknown interface pointers.
 * <b>Role:</b> The first element in the list has index 0.
 * @see CATIUnknownListImpl
 */

class CATIUnknownList : public CATBaseUnknown {

  CATDeclareInterface;

	public:
  
		/**
		 * Returns the count of elements in the list.
		 * @param oCount
		 *   Count of elements
		 */
		virtual HRESULT Count(
			unsigned int *oCount
		) = 0;

		/**
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
		) = 0;

		/**
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
		) = 0;

		/**
		 * Removes the first occurence of an element from the list.
		 * @param iItem
		 *   Element to be removed.
		 * @return 
		 * If the element cannot be found in the list, E_FAIL
		 * is returned.
		 */
		virtual HRESULT Remove(
			IUnknown *iItem
		) = 0;

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
		) = 0;
};

/* 209f24f4-465c-11d3-b802-0008c70fc870 */
extern "C" const IID IID_CATIUnknownList;

#endif

