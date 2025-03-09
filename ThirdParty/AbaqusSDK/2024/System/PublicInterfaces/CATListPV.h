#ifndef	CATListPV_h
#define	CATListPV_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0LSTPV.h"
#include <stdlib.h>

/**
 * Class to implement a mutable list of <tt>void *</tt> pointers.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive lists of <tt>void *</tt> pointers. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. It also provides high level operations such as sorting,
 * comparison, etc... The first element has index 1.
 * <br>Use the type @href CATListPV
 */
class ExportedByCO0LSTPV CATRawCollPV : public CATCollecRoot {
	public:

		/**
		 * Constructs an empty list of <tt>void *</tt> pointers.
		 * @param iInitAlloc
		 * The default capacity of the list.
		 */
		CATRawCollPV ( int iInitAlloc = 0 );

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The list to copy.
		 */
		CATRawCollPV ( const CATRawCollPV & iCopy );

		/**
		 * Constructs a list and initializes it with a C++ <tt>void *</tt> pointer array.
		 * @param iArray
		 * A C++ array of <tt>void *</tt> pointers used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of <tt>void *</tt> pointers used to initialize the list.
		 */
		CATRawCollPV ( void** iArray , int iSize );

		/**
		 * Destructor.
		 */
		virtual ~CATRawCollPV ( );

		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the list with 
		 * another list.
		 * @param iCopy
		 * The assigned list.
		 */
		CATRawCollPV & operator = ( const CATRawCollPV & iCopy );

		/**
		 * Appends a <tt>void *</tt> pointer to the list.
		 * @param iAdd
		 * The <tt>void *</tt> pointer to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( void *iAdd );

		/**
		 * Appends the content of a <tt>void *</tt> pointer list.
		 * @param iConcat
		 * The <tt>void *</tt> pointer list to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( const CATRawCollPV & iConcat );

		/**
		 * Inserts a <tt>void *</tt> pointer within the list at the specified index.
		 * @param iIndex
		 * The index at which the <tt>void *</tt> pointer is to be inserted. 
		 * <br><b>Legal values</b>: Equals 1 of the <tt>void *</tt> pointer is
		 * to be inserted at the head of the list, and <tt>Size() + 1</tt> if the
		 * <tt>void *</tt> pointer is to be inserted at the tail of the list.
		 * @param iAdd
		 * The <tt>void *</tt> pointer to insert.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int InsertAt ( int iPos , void *iAdd );

		/**
		 * Returns the size of the list. 
		 * @return the size of the list.
		 */
		inline int Size ( ) const;

		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is
		 * larger than the current size, the newly allocated slots contain random
		 * <tt>void *</tt> pointer values.
		 * @param iSize
		 * The desired size.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize );

		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is
		 * larger than the current size, the newly allocated slots contain
		 * <tt>iFiller</tt>.
		 * @param iSize
		 * The desired size.
		 * @param iFiller
		 * A <tt>void *</tt> pointer used to fill newly allocated slots.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize , void *iFiller );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The <tt>void *</tt> pointer at index <tt>iIndex</tt>.
		 */
		void *& operator[] ( int iPos );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The <tt>void *</tt> pointer at index <tt>iIndex</tt>.
		 */
		void * operator[] ( int iPos ) const;

		/**
		 * Finds the first occurrence of a <tt>void *</tt> pointer from a given index.
		 * @param iLocate
		 * The <tt>void *</tt> pointer to locate.
		 * @param iIndex
		 * The index at which the search is to begin.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located <tt>void *</tt> pointer, or 
		 * <tt>0</tt> if the list does not contain
		 * the specified <tt>void *</tt> pointer.
		 */
		int Locate ( void *iLocate , int iFrom = 1 ) const;

		/**
		 * Removes the first occurrence of a <tt>void *</tt> pointer from the list.
		 * @param iRemove
		 * The <tt>void *</tt> pointer to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed <tt>void *</tt> pointer, 
		 * or <tt>0</tt> if the list does not contain
		 * the specified <tt>void *</tt> pointer.
		 */
		int  RemoveValue ( void *iRemove );

		/**
		 * Removes all the values specifed in <tt>iSubstract</tt> from the list.
		 * @param iSubstract
		 * A list of <tt>void *</tt> pointers to remove.
		 * @return 
		 * The count of <tt>void *</tt> pointers removed from the list.
		 */
		int  Remove ( const CATRawCollPV & iSubstract );

		/**
		 * Removes the <tt>void *</tt> pointer located at a given index.
		 * @param iIndex
		 * The list index of the <tt>void *</tt> pointer to remove.
		 */
		void RemovePosition ( int iPos );

		/**
		 * Removes several <tt>void *</tt> pointers from the starting given index.
		 * @param iIndex
		 * The stating list index of the <tt>void *</tt> pointers to remove.
		 * @param iNbElem
		 * The number of <tt>void *</tt> pointers to remove.
		 * @return 
		 * The count of <tt>void *</tt> pointers removed from the list.
		 */
		int RemovePosition(int iIndex, unsigned int iNbElem);

		/**
		 * Removes all the elements from the list.
		 * @param iMH
		 * <br><b>Legal values</b>: Specifies whether the list capacity 
		 * should be shrunk to 0 
		 * (<tt>CATCollec::ReleaseAllocation</tt>) or not
		 * (<tt>CATCollec::KeepAllocation</tt>).
		 */
		void RemoveAll ( CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation );

		/**
		 * Removes all the <tt>void *</tt> pointers equal to <tt>NULL</tt>.
		 * @return 
		 * The count of null <tt>void *</tt> pointers removed from the list.
		 */
		int  RemoveNulls ( );

		/**
		 * Removes all the duplicate occurrences of a <tt>void *</tt> pointer from the list and
		 * appends them to another list.
		 * @param ioExtract
		 * A list to which duplicate <tt>void *</tt> pointers are appended.
		 * @return 
		 * The count of duplicate <tt>void *</tt> pointers removed from the list.
		 */
		int  RemoveDuplicates ( CATRawCollPV * ioExtract = NULL );

		/**
		 * Equality operator.
		 * <br><b>Role</b>: Two lists are equal if they contain the same
		 * elements in the same order.
		 * @param iRC
		 * The list to test for equality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two lists are equal, 
		 * <tt>0</tt> otherwise
		 */
		inline int operator == ( const CATRawCollPV & iRC ) const;

		/**
		 * Inequality operator.
		 * @param iRC
		 * The list to test for inequality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two lists are not equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator != ( const CATRawCollPV & iRC ) const;

		/**
		 * Compares two lists of <tt>void *</tt> pointers. 
		 * @param iRC1
		 * The first list
		 * @param iRC2
		 * The second list
		 * @return 
		 * <br><b>Legal values</b>: <tt>0</tt> if the lists are equal,
		 * <tt>-1</tt> if the first list is smaller 
		 * than the second list
		 * (smaller means that the first list contains less elements than the second
		 * list or that <tt>iRC1[i] &lt; iRC2[i]</tt> for the first i where
		 * <tt>iRC1[i] != iRC2[i]</tt>), or <tt>1</tt> otherwise.
		 */
		static int Compare ( const CATRawCollPV & iRC1 , const CATRawCollPV & iRC2, 
				     int (*iPFCompare) (const void*, const void*) );

		/**
		 * Swaps two list elements. 
		 * @param iIndex1
		 * index of the first element to swap.
		 * @param iIndex2
		 * index of the second element to swap.
		 */
		void Swap ( int iPos1 , int iPos2 );

		/**
		 * Sorts the list using the quicksort algorithm.
		 */
		void QuickSort (int (*iPFCompare) (const void*, const void*));
        
        /** @nodoc deprecated, use QuickSort */
		void QuickSort_s (int (*iPFCompare) (const void *const*, const void *const*));

		/**
		 * Fills a C++ array of <tt>void *</tt> pointers with elements from the list.
		 * @param ioArray
		 * The C++ array to fill.
		 * @param iMaxSize
		 * The size of the C++ array to fill.
		 */
		void FillArray ( void** ioArray , int iMaxSize ) const;

		/**
		 * Counts the occurrences of a <tt>void *</tt> pointer in the list.
		 * @param iTest
		 * The <tt>void *</tt> pointer for which the occurrences are to be counted.
		 * @return
		 * The count of occurrences of the <tt>void *</tt> pointer in the list.
		 */
		int NbOccur ( void *iTest );

		/**
		 * Computes the intersection of two lists.
		 * @param iRC1
		 * The first list.
		 * @param iRC2
		 * The second list.
		 * @param ioResult
		 * A list to which elements in the intersection are appended.
		 */
		static void Intersection ( const CATRawCollPV & iRC1,
					   const CATRawCollPV & iRC2,
					   CATRawCollPV & ioResult );

  protected:
		/** @nodoc */
		void GetStorage(int iNbElem, void **& oBlock);

		/** @nodoc */
		void FreeStorage( void **& ioBlock );

		/** @nodoc */
		inline void ** Storage( );

		/** @nodoc */
		int _Size;

		/** @nodoc */
		int _MaxSize;

		/** @nodoc */
		void ** _Block;
};
inline int CATRawCollPV::Size ( ) const
{
  return _Size;
}
inline int CATRawCollPV::operator == ( const CATRawCollPV & iRC ) const
{
  return ( *this != iRC ? 0 : 1 ); 
}
inline void ** CATRawCollPV::Storage( )
{
  return _Block; 
}

#include "CATRCOLL_Misc.h"

#endif		/* CATListPV_h */
