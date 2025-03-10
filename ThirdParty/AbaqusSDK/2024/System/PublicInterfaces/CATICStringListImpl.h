#ifndef INCL_CATICStringListImpl_h
#define INCL_CATICStringListImpl_h

//
// COPYRIGHT DASSAULT SYSTEMES  1999
//

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Class which implements CATICStringList interface. 
 * @see CATICStringList
 */

#include "CATBaseUnknown.h"
#include "CATListOfCATString.h"
#include "ListImpl.h"

class ExportedByListImpl CATICStringListImpl : public CATBaseUnknown {
	protected:

		/** @nodoc */
		CATListOfCATString _strings;

	public:
		CATDeclareClass;

		/** Constructs an empty list of string. */
		CATICStringListImpl();

		/** 
		* Constructs a list of string. 
		* @param iStringsList
		*    List of string to initialize the list.
		*/
		CATICStringListImpl(CATListOfCATString iStringlist);

		~CATICStringListImpl();

        /** @nodoc */
		virtual HRESULT Count(
			unsigned int *oCount
		);
		        /** @nodoc */
		virtual HRESULT Item(
			const unsigned int iPosition,
			char **oItem
		);
		        /** @nodoc */
		virtual HRESULT Add(
			const unsigned int iPosition,
			const char *iItem
		);
		        /** @nodoc */
		virtual HRESULT Remove(
			const char *iItem
		);
};

/* e0eb860e-95f7-11d3-b80a-0008c70fc870 */
extern "C" const CLSID CLSID_CATICStringListImpl;

#endif
