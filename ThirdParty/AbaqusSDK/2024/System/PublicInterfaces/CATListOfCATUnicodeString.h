#ifndef	CATListOfCATUnicodeString_h
#define	CATListOfCATUnicodeString_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CO0LSTST.h"
#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CATListPV.h"

class CATUnicodeString;

/**
 * This class implements a mutable list of <tt>CATUnicodeStrings</tt>.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive lists of <tt>CATUnicodeString</tt>. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. It also provides high level operations such as sorting,
 * comparison, etc... The first element has index 1.
 * <br>Use the type @href CATListOfCATUnicodeString
 */
class ExportedByCO0LSTST CATListValCATUnicodeString : public CATCollecRoot  {                        
	public :                                                      
		/**
		 * Constructs an empty list of <tt>CATUnicodeStrings</tt>.
		 */
		CATListValCATUnicodeString ();                                           

		/**
		 * Constructs an empty list of <tt>CATUnicodeStrings</tt> with a specified initial capacity.
		 * @param iInitAlloc
		 * The default capacity of the list.
		 */
		CATListValCATUnicodeString ( int iInitAlloc );                           

		/** @nodoc */
		CATListValCATUnicodeString ( int iInitAlloc, int iExtensAlloc );         

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The list to copy.
		 */
		CATListValCATUnicodeString ( const CATListValCATUnicodeString& iCopy );

		/**
		 * Constructs a list and initializes it with a C++ <tt>CATUnicodeString</tt> array.
		 * @param iArray
		 * A C++ array <tt>CATUnicodeStrings</tt> used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of <tt>CATUnicodeStrings</tt> used to initialize the list.
		 */
		CATListValCATUnicodeString ( CATUnicodeString*  iArray, int iSize );

		/**
		 * Constructs a list and initializes it with a C++ pointer to <tt>CATUnicodeString</tt> array.
		 * @param iArray
		 * A C++ array of pointers to <tt>CATUnicodeStrings</tt> used to initialize the list.
		 * @param iSize
		 * The size of the C++ array of <tt>CATUnicodeStrings</tt> used to initialize the list.
		 */
		CATListValCATUnicodeString ( CATUnicodeString** iArray, int iSize );             

		/**
		 * Destructor.
		 */
		~CATListValCATUnicodeString ();                                          
                                                    
		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the list with 
		 * another list.
		 * @param iCopy
		 * The assigned list.
		 */
		CATListValCATUnicodeString& operator= ( const CATListValCATUnicodeString& iCopy );    
                                                    
		/**
		 * Appends a <tt>CATUnicodeString</tt> to the list.
		 * @param iAdd
		 * The <tt>CATUnicodeString</tt> to append.
		 */
		void Append ( const CATUnicodeString& iAdd );

		/**
		 * Appends the content of a <tt>CATUnicodeString</tt> list.
		 * @param iConcat
		 * The <tt>CATUnicodeString</tt> list to append.
		 */
		void Append ( const CATListValCATUnicodeString& iConcat );                              
                                                    
		/**
		 * Inserts a <tt>CATUnicodeString</tt> after the specified index.
		 * @param iIndex
		 * The index of the element.
		 * @param iAdd
		 * The element to insert.
		 */
		void InsertAfter  ( int iIndex, const CATUnicodeString& iAdd );

		/**
		 * Inserts a <tt>CATUnicodeString</tt> before the specified index.
		 * @param iIndex
		 * The index of the element.
		 * @param iAdd
		 * The element to insert.
		 */
		void InsertBefore ( int iIndex, const CATUnicodeString& iAdd );

		/** @nodoc */
		void InsertAt     ( int iIndex, const CATUnicodeString& iAdd );
                                                    
		/**
		 * Returns the size of the list. 
		 * @return the size of the list.
		 */
		int Size () const ; 
		
		/**
		 * Forces the size of the list to an arbitrary size. 
		 * <br><b>Role</b>: If <tt>iSize</tt> is
		 * larger than the current size, the newly allocated slots contain
		 * <tt>iFiller</tt>.
		 * @param iSize
		 * The desired size.
		 * @param iFiller
		 * A <tt>CATUnicodeString</tt> used to fill newly allocated slots.
		 */
		void Size ( int iSize, const CATUnicodeString* iFiller = NULL );                                        
                                                    
		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The <tt>CATUnicodeString</tt> at index <tt>iIndex</tt>.
		 */
		CATUnicodeString& operator[] ( int iIndex );                                 

		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The <tt>CATUnicodeString</tt> at index <tt>iIndex</tt>.
		 */
		const CATUnicodeString& operator[] ( int iIndex ) const;                     
                                                    
		/**
		 * Finds the first occurrence of a <tt>CATUnicodeString</tt> from a given index.
		 * @param iLocate
		 * The <tt>CATUnicodeString</tt> to locate.
		 * @param iIndex
		 * The index at which the search is to begin.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located <tt>CATUnicodeString</tt>, 
		 * or <tt>0</tt> if the list does not contain
		 * the specified <tt>CATUnicodeString</tt>.
		 */
		int Locate ( const CATUnicodeString& iLocate, int iIndex = 1 ) const ;                                     
                                                    
		/**
		 * Removes all the elements from the list.
		 * @param iMH
		 * <br><b>Legal values</b>: Specifies whether the list capacity should be 
		 * shrunk to 0 (<tt>CATCollec::ReleaseAllocation</tt>) or not
		 * (<tt>CATCollec::KeepAllocation</tt>).
		 */
		void RemoveAll ( CATCollec::MemoryHandling iMH              
				  = CATCollec::ReleaseAllocation );         

		/**
		 * Removes the first occurrence of a <tt>CATUnicodeString</tt> from the list.
		 * @param iRemove
		 * The <tt>CATUnicodeString</tt> to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed <tt>CATUnicodeString</tt>, 
		 * or <tt>0</tt> if the list does not contain the specified <tt>CATUnicodeString</tt>.
		 */
		int RemoveValue ( const CATUnicodeString& iRemove );

		/**
		 * Removes all the values specifed in <tt>iSubstract</tt> from the list.
		 * @param iSubstract
		 * A list of <tt>CATUnicodeStrings</tt> to remove.
		 * @return 
		 * The count of <tt>CATUnicodeStrings</tt> removed from the list.
		 */
		int Remove ( const CATListValCATUnicodeString& iSubstract );

		/**
		 * Removes the <tt>CATUnicodeString</tt> located at a given index.
		 * @param iIndex
		 * The list index of the <tt>CATUnicodeString</tt> to remove.
		 */
		void RemovePosition ( int iIndex );

		/**
		 * Removes all the duplicate occurrences of a <tt>CATUnicodeString</tt> from the list and
		 * appends them to another list.
		 * @param ioExtract
		 * A list to which duplicate <tt>CATUnicodeStrings</tt> are appended.
		 */
		void RemoveDuplicates ( CATListValCATUnicodeString& ioExtract );

		/**
		 * Removes all the duplicate occurrences of a <tt>CATUnicodeString</tt> from the list.
		 * @return 
		 * The count of duplicate <tt>CATUnicodeStrings</tt> removed from the list.
		 */
		int RemoveDuplicates ();
                                                    
		/**
		 * Equality operator. 
		 * <br><b>Role</b>: Two lists are equal if they contain the same
		 * elements in the same lexicographic order.
		 * @param iLV
		 * The list to test for equality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two lists are equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator == ( const CATListValCATUnicodeString& iLV ) const ;

		/**
		 * Inequality operator.
		 * @param iLV
		 * The list to test for inequality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two lists are not equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator != ( const CATListValCATUnicodeString& iLV ) const ;

		/**
		 * Less than or equal to operator.
		 * @param iLV
		 * The list to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than or 
		 * equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <= ( const CATListValCATUnicodeString& iLV ) const ;

		/**
		 * Greater than or equal to operator.
		 * @param iLV
		 * The list to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than 
		 * or equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator >= ( const CATListValCATUnicodeString& iLV ) const ;

		/**
		 * Less than operator.
		 * @param iLV
		 * The list to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than 
		 * <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <  ( const CATListValCATUnicodeString& iLV ) const ;

		/**
		 * Greater than operator.
		 * @param iLV
		 * The list to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than <tt>iLV</tt>, 
		 * <tt>0</tt> otherwise
		 */
		int operator >  ( const CATListValCATUnicodeString& iLV ) const ;                                  
                                                    
		/**
		 * Compares two lists of <tt>CATUnicodeStrings</tt>. 
		 * @param iLV1
		 * the first list of <tt>CATUnicodeStrings</tt>.
		 * @param iLV2
		 * the second list of <tt>CATUnicodeStrings</tt>.
		 * @param iPFCompare
		 * <br><b>Legal values</b>: A pointer to a function which compares 
		 * two <tt>CATUnicodeStrings</tt> and returns -1, 0 or 1 depending on 
		 * the order of the strings.
		 * @return 
		 * <br><b>Legal values</b>: <tt>0</tt> if the lists are equal,
		 * <tt>-1</tt> if the first list is smaller than the second list
		 * (smaller means that the first list contains less elements than the second
		 * list or that <tt>iRC1[i] &lt; iRC2[i]</tt> for the first i where
		 * <tt>iRC1[i] != iRC2[i]</tt> and < is a lexicographic comparison
		 * of two strings), or <tt>1</tt> otherwise.
		 */
		static int Compare ( const CATListValCATUnicodeString& iLV1,      
			 const CATListValCATUnicodeString& iLV2,      
			 int (*iPFCompare)( CATUnicodeString*, CATUnicodeString* ) );                             
                                                    
		/**
		 * Replaces an element at a specified index with another <tt>CATUnicodeString</tt> value. 
		 * @param iIndex
		 * index of the element to replace.
		 * @param iReplace
		 * The new <tt>CATUnicodeString</tt> value.
		 */
		void Replace ( int iIndex, const CATUnicodeString& iReplace );

		/**
		 * Swaps two list elements. 
		 * @param iIndex1
		 * index of the first element to swap.
		 * @param iIndex2
		 * index of the second element to swap.
		 */
		void Swap ( int iIndex1, int iIndex2 );

		/**
		 * Sorts the list using the quicksort algorithm.
		 * @param iPFCompare
		 * A pointer to a function which compares two <tt>CATUnicodeStrings</tt> and
		 * returns -1, 0 or 1 depending on the order of the strings.
		 */
		void QuickSort ( int (*iPFCompare)( CATUnicodeString*, CATUnicodeString* ) );                                
                                                    
		/**
		 * Fills a C++ array of <tt>CATUnicodeStrings</tt> with elements from the list.
		 * <br><b>Role</b>: The array has to have the same size as the list.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array ( CATUnicodeString*  ioArray ) const ;

		/**
		 * Fills a C++ array of pointers to <tt>CATUnicodeStrings</tt> with elements from the list.
		 * <br><b>Role</b>: The array has to have the same size as the list.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array ( CATUnicodeString** ioArray ) const ;

		/**
		 * Counts the occurrences of a <tt>CATUnicodeString</tt> in the list.
		 * @param iTest
		 * The <tt>CATUnicodeString</tt> for which the occurrences are to be counted.
		 * @return
		 * The count of occurrences of the <tt>CATUnicodeString</tt> in the list.
		 */
		int NbOccur ( const CATUnicodeString& iTest );

		/**
		 * Computes the intersection of two lists.
		 * @param iL1
		 * The first list.
		 * @param iL2
		 * The second list.
		 * @param ioResult
		 * A list to which elements in the intersection are appended.
		 */
		static void Intersection ( const CATListValCATUnicodeString& iL1, 
						   const CATListValCATUnicodeString& iL2,
						   CATListValCATUnicodeString& ioResult );                                      

	private :                                                     
		/** @nodoc */
		CATListPV _Lval;                                            
};

#endif

