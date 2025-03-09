#ifndef INCL_CATIStringListImpl_h
#define INCL_CATIStringListImpl_h

//
// COPYRIGHT DASSAULT SYSTEMES  1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Class which implements CATIStringList interface. 
 * @see CATIStringList
 */

#include "CATBaseUnknown.h"
#include "CATListOfCATUnicodeString.h"
#include "ListImpl.h"
#include <wchar.h>

class ExportedByListImpl CATIStringListImpl : public CATBaseUnknown {

	protected:
		/** @nodoc */
		CATListOfCATUnicodeString _strings;

	public:
		/** @nodoc */
		CATDeclareClass;

		/** Constructs an empty list of CATUnicodeString. */
		CATIStringListImpl();


		/** Constructs a list of CATUnicodeString. 
		* @param iStringsList
		*    List of NLS string to initialize the list.
		*/

		CATIStringListImpl(CATListOfCATUnicodeString iStringsList);

		virtual ~CATIStringListImpl();

        /** @nodoc */
		virtual HRESULT Count(
			unsigned int *oCount
		);

		/** @nodoc */
		virtual HRESULT Item(
			const unsigned int iPosition,
			wchar_t **oItem
		);

		/** @nodoc */
		virtual HRESULT Add(
			const unsigned int iPosition,
			const wchar_t *iItem
		);

		/** @nodoc */
		virtual HRESULT Remove(
			const wchar_t *iItem
		);
};

/** @nodoc */
/* e06ac712-95f7-11d3-b80a-0008c70fc870 */
extern "C" const CLSID CLSID_CATIStringListImpl;

#endif
