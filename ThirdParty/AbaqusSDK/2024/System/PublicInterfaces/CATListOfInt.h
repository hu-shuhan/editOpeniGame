#ifndef CATListOfInt_h
#define CATListOfInt_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0RCINT.h"
#include <stdlib.h>


/**
 * Class to implement a mutable list of 32-bit integers.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive lists of integers. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. It also provides high level operations such as sorting,
 * comparison, etc... The first element has index 1.
 * <br>Use the type @href CATListOfInt
 */
class ExportedByCO0RCINT CATRawCollint : public CATCollecRoot {
	public:

		/**
		 * Constructs an empty list of ints.
		 * @param iInitAlloc
		 * The default capacity of the list.
		 */
		CATRawCollint ( int iInitAlloc = 0 );

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The list to copy.
		 */
		CATRawCollint ( const CATRawCollint & iCopy );

		/**
		 * Constructs a list and initializes it with a C++ int array.
		 * @param iArray
		 * A C++ array of ints used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of ints used to initialize the list.
		 */
		CATRawCollint ( int * iArray , int iSize );

		/**
		 * Destructor.
		 */
		virtual ~CATRawCollint ( );

		/**
		 * Assignment operator. Overwrites the content of the list by 
		 * copying into it another list.
		 * @param iCopy
		 * The assigned list.
		 */
		CATRawCollint & operator = ( const CATRawCollint & iCopy );

		/**
		 * Appends an int to the list.
		 * @param iAdd
		 * The int to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( int iAdd );

		/**
		 * Appends the content of an int list.
		 * @param iConcat
		 * The int list to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( const CATRawCollint & iConcat );

		/**
		 * Inserts an int within the list at the specified index.
		 * @param iIndex
		 * The index at which the int is to be inserted. 
		 * <br><b>Legal values</b>: Equals 1 of the int is
		 * to be inserted at the head of the list, and <tt>Size() + 1</tt> if the
		 * int is to be inserted at the tail of the list.
		 * @param iAdd
		 * The int to insert.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int InsertAt ( int iIndex , int iAdd );

		/**
		 * Returns the size of the list. 
		 * @return the size of the list.
		 */
		int Size ( ) const {
			return _Size;
		}

		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is
		 * larger than the current size, the newly allocated slots contain random
		 * int values.
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
		 * An int used to fill newly allocated slots.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize , int iFiller );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The int at index <tt>iIndex</tt>.
		 */
		int & operator[] ( int iIndex );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The int at index <tt>iIndex</tt>.
		 */
		int operator[] ( int iIndex ) const;

		/**
		 * Finds the first occurrence of an int from a given index.
		 * @param iLocate
		 * The int to locate.
		 * @param iIndex
		 * The index at which the search is to begin.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located int, or 
		 * <tt>0</tt> if the list does not contain
		 * the specified int.
		 */
		int Locate ( int iLocate , int iIndex = 1 ) const;

		/**
		 * Removes the first occurrence of an int from the list.
		 * @param iRemove
		 * The int to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed int, 
		 * or <tt>0</tt> if the list does not contain
		 * the specified int.
		 */
		int RemoveValue ( int iRemove );

		/**
		 * Removes all the values specifed in <tt>iSubstract</tt> from the list.
		 * @param iSubstract
		 * A list of ints to remove.
		 * @return 
		 * The count of ints removed from the list.
		 */
		int Remove ( const CATRawCollint & iSubstract );

		/**
		 * Removes the int located at a given index.
		 * @param iIndex
		 * The list index of the int to remove.
		 */
		void RemovePosition ( int iIndex );

		/**
		 * Removes several ints from the starting given index.
		 * @param iIndex
		 * The stating list index of the ints to remove.
		 * @param iNbElem
		 * The number of ints to remove.
		 * @return 
		 * The count of ints removed from the list.
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
		 * Removes all the ints equal to 0.
		 * @return 
		 * The count of null ints removed from the list.
		 */
		int RemoveNulls ( );

		/**
		 * Removes all the duplicate occurrences of an int from the list and
		 * appends them to another list.
		 * @param ioExtract
		 * A list to which duplicate ints are appended.
		 * @return 
		 * The count of duplicate ints removed from the list.
		 */
		int RemoveDuplicates ( CATRawCollint * ioExtract = NULL );

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
		int operator == ( const CATRawCollint & iRC ) const { 
			return ( * this != iRC ? 0 : 1 );
		}

		/**
		 * Inequality operator.
		 * @param iRC
		 * The list to test for inequality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two lists are not equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator != ( const CATRawCollint & iRC ) const;

		/**
		 * Compares two lists of ints. 
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
		static int Compare ( const CATRawCollint & iRC1 , const CATRawCollint & iRC2 );

		/**
		 * Swaps two list elements. 
		 * @param iIndex1
		 * index of the first element to swap.
		 * @param iIndex2
		 * index of the second element to swap.
		 */
		void Swap ( int iIndex1 , int iIndex2 );

		/**
		 * Sorts the list using the quicksort algorithm.
		 */
		void QuickSort ( );

		/**
		 * Fills a C++ array of ints with elements from the list.
		 * @param ioArray
		 * The C++ array to fill.
		 * @param iMaxSize
		 * The size of the C++ array to fill.
		 */
		void FillArray ( int * ioArray , int iMaxSize ) const;

		/**
		 * Counts the occurrences of an int in the list.
		 * @param iTest
		 * The int for which the occurrences are to be counted.
		 * @return
		 * The count of occurrences of the int in the list.
		 */
		int NbOccur ( int iTest );

		/**
		 * Computes the intersection of two lists.
		 * @param iRC1
		 * The first list.
		 * @param iRC2
		 * The second list.
		 * @param ioResult
		 * A list to which elements in the intersection are appended.
		 */
		static void Intersection ( const CATRawCollint & iRC1 , const CATRawCollint & iRC2 , CATRawCollint & ioResult );

	protected:

		/** @nodoc */
		void GetStorage( int iNbElem , int * & oBlock ) {
			oBlock = new int[iNbElem];
		}

		/** @nodoc */
		void FreeStorage( int * & ioBlock );

		/** @nodoc */
		int * Storage( ) {
				return _Block;
		}

		/** @nodoc */
		int _Size;

		/** @nodoc */
		int _MaxSize;

		/** @nodoc */
		int * _Block;

/** 
 * @nodoc 
 * The list has an initial internal static buffer of size <tt>CATListOfIntDefaultSize</tt>
 * it uses to store elements. This reduces the number of memory allocation calls for
 * small lists.
 */
#define CATListOfIntDefaultSize  10

/** @nodoc */
#define _LessFree
#ifdef _LessFree

	private:

		/** @nodoc */
		int _StaticBaseBlock[CATListOfIntDefaultSize];
#endif
};
 

#include "CATRCOLL_Misc.h"

#endif		/* CATListOfInt_h */
