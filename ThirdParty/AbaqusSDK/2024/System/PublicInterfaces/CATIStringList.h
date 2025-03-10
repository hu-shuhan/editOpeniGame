#ifndef INCL_CATIStringList_h
#define INCL_CATIStringList_h

// COPYRIGHT DASSAULT SYSTEMES  1999

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

#include "IUnknown.h"
#include <wchar.h>


/**
 * Interface to handle a list of NLS strings.
 * <b>Role</b>:
 * The first element in the list has index 0.
 * @see CATICStringList, CATIStringListImpl
 */

class CATIStringList : public IUnknown {
	public:
  
		/**
		 * Returns the count of elements in the list.
		 * @param oCount
		 *   Number of elements
		 */
		virtual HRESULT Count(
			unsigned int *oCount
		) = 0;

		/**
		 * Returns the iPosition-th element.
		 * If iPosition does not respect respect the following rule:
		 * 0 &lt;= position &lt;= Count, E_FAIL is returned.
		 * @param iPosition
		 * @param iPosition
		 *   Element to be returned.
		 * @param oItem
		 *   Returned element.
		 */
		virtual HRESULT Item(
			const unsigned int iPosition,
			wchar_t **oItem
		) = 0;

		/**
		 * Adds an element at a given position in the list.
		 * If iPosition does not respect respect the following rule:
		 * 0 &lt;= position &lt;= Count, E_FAIL is returned.
		 * @param iPosition
		 *   Position of the added element in the list.
		 * @param iItem
		 *   Element to be added.
		 */
		virtual HRESULT Add(
			const int iPosition,
			const wchar_t *iItem
		) = 0;

		/**
		 * Removes the first occurence of an element from the list.
		 * If the element cannot be found in the list, E_FAIL
		 * is returned.
		 * @param iItem
		 *   Element to be removed.
		 */
		virtual HRESULT Remove(
			const wchar_t *iItem
		) = 0;
};

/* 188534ea-466e-11d3-b802-0008c70fc870 */
extern "C" const IID IID_CATIStringList;

#endif

