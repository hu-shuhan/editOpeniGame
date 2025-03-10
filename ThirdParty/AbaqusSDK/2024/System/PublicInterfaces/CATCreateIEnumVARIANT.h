#ifndef CATCreateIEnumVARIANT_h
#define CATCreateIEnumVARIANT_h

// COPYRIGHT DASSAULT SYSTEMES 2002

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0COL.h"
#include "IUnknown.h"
#include "IEnumVARIANT.h"

/**
 * Creates a new object which implements @href IEnumVARIANT.
 * <b>Role</b>:
 * Use this function to instantiate a new object which 
 * implements the @href IEnumVARIANT interface and traverses
 * the supplied array of objects. Such objects are required
 * to implement the <tt>get__NewEnum</tt> property in Automation collections.
 * (see the <tt>CATIACollection</tt> IDL interface for a description of
 * the get__NewEnum property)
 * The IEnumVARIANT interface is used by VB in the <tt>ForEach</tt> 
 * construct to traverse collections of objects.
 * @param iElementArray
 * An array of elements to traverse during iteration (the array
 * is copied internally).
 * @param iArraySize
 * The size of the element array.
 * @param iElementType
 * The IID of the Automation interface characterizing the elements.
 * @param oEnumerator
 * The returned IEnumVARIANT-implementing object.
 */
HRESULT ExportedByJS0COL CATCreateIEnumVARIANT(
			IUnknown **iElementArray, 
			unsigned long iArraySize,
			const IID & iElementType,
			IEnumVARIANT*& oEnumerator);

#endif // CATCreateIEnumVARIANT_h

