#ifndef CATListOfFloat_h
#define CATListOfFloat_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0RCFLT.h"
#include <stdlib.h>


/**
 * Class to implement a mutable list of single-precision floating-point numbers.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive lists of floats. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. It also provides high level operations such as sorting,
 * comparison, etc... The first element has index 1.
 *<br>Use the type @href CATListOfFloat
 */
class ExportedByCO0RCFLT CATRawCollfloat : public CATCollecRoot {
	public:
		/**
		 * Constructs an empty list of floats.
		 * @param iInitAlloc
		 * The default capacity of the list.
		 */
		CATRawCollfloat ( int iInitAlloc = 0 );

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The list to copy.
		 */
		CATRawCollfloat ( const CATRawCollfloat & iCopy );

		/**
		 * Constructs a list and initializes it with a C++ float array.
		 * @param iArray
		 * A C++ array of floats used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of floats used to initialize the list.
		 */
		CATRawCollfloat ( float * iArray , int iSize );

		/**
		 * Destructor.
		 */
		virtual ~CATRawCollfloat ( );

		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the list with 
		 * another list.
		 * @param iCopy
		 * The assigned list.
		 */
		CATRawCollfloat & operator = ( const CATRawCollfloat & iCopy );

		/**
		 * Appends a float to the list.
		 * @param iAdd
		 * The float to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( float iAdd );

		/**
		 * Appends the content of a float list.
		 * @param iConcat
		 * The float list to append.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Append ( const CATRawCollfloat & iConcat );

		/**
		 * Inserts a float within the list at the specified index.
		 * @param iIndex
		 * The index at which the float is to be inserted. 
		 * <br><b>Legal values</b>: Equals 1 of the float is
		 * to be inserted at the head of the list, and <tt>Size() + 1</tt> if the
		 * float is to be inserted at the tail of the list.
		 * @param iAdd
		 * The float to insert.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int InsertAt ( int iIndex , float iAdd );

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
		 * float values.
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
		 * A float used to fill newly allocated slots.
		 * @return
		 * <tt>0</tt> upon sucess, <tt>1</tt> if there is no more free memory.
		 */
		int Size ( int iSize , float iFiller );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The float at index <tt>iIndex</tt>.
		 */
		float & operator[] ( int iIndex );

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The float at index <tt>iIndex</tt>.
		 */
		float operator[] ( int iIndex ) const;

		/**
		 * Finds the first occurrence of a float from a given index.
		 * @param iLocate
		 * The float to locate.
		 * @param iIndex
		 * The index at which the search is to begin.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located float, or 
		 * <tt>0</tt> if the list does not contain
		 * the specified float.
		 */
		int Locate ( float iLocate , int iIndex = 1 ) const;

		/**
		 * Removes the first occurrence of a float from the list.
		 * @param iRemove
		 * The float to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed float, 
		 * or <tt>0</tt> if the list does not contain
		 * the specified float.
		 */
		int RemoveValue ( float iRemove );

		/**
		 * Removes all the values specifed in <tt>iSubstract</tt> from the list.
		 * @param iSubstract
		 * A list of floats to remove.
		 * @return 
		 * The count of floats removed from the list.
		 */
		int Remove ( const CATRawCollfloat & iSubstract );

		/**
		 * Removes the float located at a given index.
		 * @param iIndex
		 * The list index of the float to remove.
		 */
		void RemovePosition ( int iIndex );

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
		 * Removes all the floats equal to 0.0.
		 * @return 
		 * The count of null floats removed from the list.
		 */
		int RemoveNulls ( );

		/**
		 * Removes all the duplicate occurrences of a float from the list and
		 * appends them to another list.
		 * @param ioExtract
		 * A list to which duplicate floats are appended.
		 * @return 
		 * The count of duplicate floats removed from the list.
		 */
		int RemoveDuplicates ( CATRawCollfloat * ioExtract = NULL );

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
		int operator == ( const CATRawCollfloat & iRC ) const { 
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
		int operator != ( const CATRawCollfloat & iRC ) const;

		/**
		 * Compares two lists of floats. 
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
		static int Compare ( const CATRawCollfloat & iRC1 , const CATRawCollfloat & iRC2 );

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
		 * Fills a C++ array of floats with elements from the list.
		 * @param ioArray
		 * The C++ array to fill.
		 * @param iMaxSize
		 * The size of the C++ array to fill.
		 */
		void FillArray ( float * ioArray , int iMaxSize ) const;

		/**
		 * Counts the occurrences of a float in the list.
		 * @param iTest
		 * The float for which the occurrences are to be counted.
		 * @return
		 * The count of occurrences of the float in the list.
		 */
		int NbOccur ( float iTest );

		/**
		 * Computes the intersection of two lists.
		 * @param iRC1
		 * The first list.
		 * @param iRC2
		 * The second list.
		 * @param ioResult
		 * A list to which elements in the intersection are appended.
		 */
		static void Intersection ( const CATRawCollfloat & iRC1,
								   const CATRawCollfloat & iRC2,
								   CATRawCollfloat & ioResult );

	protected:
		/** @nodoc */
		void GetStorage( int iNbElem , float * & oBlock ) {
			oBlock = new float[iNbElem];
		}

		/** @nodoc */
		void FreeStorage( float * & ioBlock );

		/** @nodoc */
		float * Storage( ) {
			return _Block;
		}

		/** @nodoc */
		int _Size;

		/** @nodoc */
		int _MaxSize;

		/** @nodoc */
		float * _Block;

/** 
 * @nodoc 
 * The list has an initial internal static buffer of size <tt>CATListOfFloatDefaultSize</tt>
 * it uses to store elements. This reduces the number of memory allocation calls for
 * small lists.
 */
#define CATListOfFloatDefaultSize  10

/** @nodoc */
#define _LessFree
#ifdef _LessFree

	private:

		/** @nodoc */
		float _StaticBaseBlock[CATListOfFloatDefaultSize];

#endif
};



#include "CATRCOLL_Misc.h"
#endif		/* CATListOfFloat_h */

