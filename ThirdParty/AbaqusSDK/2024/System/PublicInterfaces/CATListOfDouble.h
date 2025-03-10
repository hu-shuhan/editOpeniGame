#ifndef CATListOfDouble_h
#define CATListOfDouble_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0RCDBL.h"
#include <stdlib.h>


/**
 * Class to implement a mutable list of double-precision floating-point numbers.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive lists of double. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. It also provides high level operations such as sorting,
 * comparison, etc... The first element has index 1.
 * <br>Use the type @href CATListOfDouble
 */
class ExportedByCO0RCDBL CATRawColldouble : public CATCollecRoot {
	public :
		/**
		 * Constructs an empty list of doubles.
		 * @param iInitAlloc
		 * The default capacity of the list.
		 */
		CATRawColldouble ( int iInitAlloc = 0 );

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The list to copy.
		 */
		CATRawColldouble ( const CATRawColldouble & iCopy );

		/**
		 * Constructs a list and initializes it with a C++ double array.
		 * @param iArray
		 * A C++ array of doubles used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of doubles used to initialize the list.
		 */
		CATRawColldouble ( double * iArray , int iSize );

		/**
		 * Destructor.
		 */
		virtual ~CATRawColldouble ( );

		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the list with 
		 * another list.
		 * @param iCopy
		 * The assigned list.
		 */
		CATRawColldouble & operator = ( const CATRawColldouble & iCopy );

		/**
		 * Appends a double to the list.
		 * @param iAdd
		 * The double to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( double iAdd );

		/**
		 * Appends the content of a double list.
		 * @param iConcat
		 * The floating-point number list to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( const CATRawColldouble & iConcat );

		/**
		 * Inserts a double within the list at the specified index.
		 * @param iIndex
		 * The index at which the double is to be inserted. 
		 * <br><b>Legal values</b>: Equals 1 if the double is to be
		 * inserted at the head of the list, and <tt>Size() + 1</tt> 
		 * if the double is to be inserted at the tail of the list.
		 * @param iAdd
		 * The double to insert.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int InsertAt ( int iIndex , double iAdd );

		/**
		 * Returns the size of the list. 
		 * @return the size of the list.
		 */
		int Size ( ) const { return _Size; }

		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is larger than the current
		 * size, the newly allocated slots contain random double values.
		 * @param iSize
		 * The desired size.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize );

		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is larger than the current
		 * size, the newly allocated slots contain <tt>iFiller</tt>.
		 * @param iSize
		 * The desired size.
		 * @param iFiller
		 * A double used to fill newly allocated slots.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize , double iFiller );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The double at index <tt>iIndex</tt>.
		 */
		double & operator[] ( int iIndex );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The double at index <tt>iIndex</tt>.
		 */
		double operator[] ( int iIndex ) const;

		/**
		 * Finds the first occurrence of a double from a given index.
		 * @param iLocate
		 * The double to locate.
		 * @param iIndex
		 * The index at which the search is to begin.
		 * @return 
		 * The index of the located double, or <tt>0</tt> if the list does not contain
		 * the specified double.
		 */
		int Locate ( double iLocate , int iIndex = 1 ) const;

		/**
		 * Removes the first occurrence of a double from the list.
		 * @param iRemove
		 * The double to remove.
		 * @return 
		 * The index of the removed double, or <tt>0</tt> if the list does not contain
		 * the specified double.
		 */
		int RemoveValue ( double iRemove );

		/**
		 * Removes all the values specifed in <tt>iSubstract</tt> from the list.
		 * @param iSubstract
		 * The list of doubles to remove.
		 * @return 
		 * The count of doubles removed from the list.
		 */
		int Remove ( const CATRawColldouble & iSubstract );

		/**
		 * Removes the double located at a given index.
		 * @param iIndex
		 * The list index of the double to remove.
		 */
		void RemovePosition ( int iIndex );

		/**
		 * Removes several doubles from the starting given index.
		 * @param iIndex
		 * The stating list index of the doubles to remove.
		 * @param iNbElem
		 * The number of doubles to remove.
		 * @return 
		 * The count of doubles removed from the list.
		 */
		int RemovePosition(int iIndex, unsigned int iNbElem);

		/**
		 * Removes all the elements from the list.
		 * @param iMH
		 * <br><b>Legal values</b>: Specifies whether the list capacity 
		 * should be shrunk to 0 (<tt>CATCollec::ReleaseAllocation</tt>)
		 *  or not (<tt>CATCollec::KeepAllocation</tt>).
		 */
		void RemoveAll ( CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation );

		/**
		 * Removes all the doubles equal to 0.0.
		 * @return 
		 * The count of null doubles removed from the list.
		 */
		int RemoveNulls ( );

		/**
		 * Removes all the duplicate occurrences of a double from the list and
		 * appends them to another list.
		 * @param ioExtract
		 * A list to which duplicate doubles are appended.
		 * <br><b>Legal values</b>: Any valid pointer to a <tt>CATListOfDouble</tt> 
		 * instance. The default value is <tt>NULL</tt>: this causes duplicate
		 * occurrences to simply be removed and not stored anywhere else.
		 * @return 
		 * The count of duplicate doubles removed from the list.
		 */
		int RemoveDuplicates ( CATRawColldouble * ioExtract = NULL );

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
		int operator == ( const CATRawColldouble & iRC ) const { 
			return ( * this != iRC ? 0 : 1 );
		}

		/**
		 * Inequality operator.
		 * @param iRC
		 * The list to test for inequality
		 * @return
		 * <tt>1</tt> if the two lists are not equal, <tt>0</tt> otherwise.
		 */
		int operator != ( const CATRawColldouble & iRC ) const;

		/**
		 * Compares two lists of doubles. 
		 * @param iRC1
		 * The first list
		 * @param iRC2
		 * The second list
		 * @return 
		 * <br><b>Legal values</b>: <tt>0</tt> if the lists are equal,
		 * <tt>-1</tt> if the first list is smaller than the second list
		 * (smaller means that the first list contains less elements than the second
		 * list or that <tt>iRC1[i] &lt; iRC2[i]</tt> for the first i where
		 * <tt>iRC1[i] != iRC2[i]</tt>), or <tt>1</tt> otherwise.
		 */
		static int Compare ( const CATRawColldouble & iRC1 , const CATRawColldouble & iRC2 );

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
		 * Fills a C++ array of doubles with elements from the list.
		 * @param ioArray
		 * The C++ array to fill.
		 * @param iMaxSize
		 * The size of the C++ array to fill.
		 */
		void FillArray ( double * ioArray , int iMaxSize ) const;

		/**
		 * Counts the occurrences of a double in the list.
		 * @param iTest
		 * The double for which the occurrences are to be counted.
		 * @return
		 * The number of occurrences of the double in the list.
		 */
		int NbOccur ( double iTest );

		/**
		 * Computes the intersection of two lists.
		 * @param iRC1
		 * The first list.
		 * @param iRC2
		 * The second list.
		 * @param ioResult
		 * A list to which elements in the intersection are appended.
		 */
		static void Intersection ( const CATRawColldouble & iRC1,
								   const CATRawColldouble & iRC2,
								   CATRawColldouble & ioResult );

	protected :
		/** @nodoc */
		void GetStorage( int iNbElem , double * & oBlock ) {
			oBlock = new double[iNbElem];
		}

		/** @nodoc */
		void FreeStorage( double * & ioBlock );

		/** @nodoc */
		double * Storage( ) {
			return _Block;
		}

		/** @nodoc */
		int _Size;

		/** @nodoc */
		int _MaxSize;

		/** @nodoc */
		double * _Block;

/** 
 * @nodoc 
 * The list has an initial internal static buffer of size <tt>CATListOfDoubleDefaultSize</tt>
 * it uses to store elements. This reduces the number of memory allocation calls for
 * small lists.
 */
#define CATListOfDoubleDefaultSize  3

/** @nodoc */
#define _LessFree
#ifdef _LessFree

	private:

		/** @nodoc */
		double _StaticBaseBlock[CATListOfDoubleDefaultSize];
#endif
};
 

#include "CATRCOLL_Misc.h"

#endif		/* CATListOfDouble_h */
