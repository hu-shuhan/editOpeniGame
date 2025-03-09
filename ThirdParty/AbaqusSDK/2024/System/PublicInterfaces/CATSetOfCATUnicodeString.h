#ifndef CATSetOfCATUnicodeString_h
#define CATSetOfCATUnicodeString_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0SETST.h"
#include "CATListPV.h"
class CATUnicodeString;

/**
 * Class to implement a mutable ordered set of CATUnicodeStrings.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive sets of @href CATUnicodeString. The class handles all the low-level
 * memory allocation tasks such as reallocating the list once it capacity has
 * been exceeded. Set elements are sorted by lexicographic order.
 * <br>Use the @href CATSetOfCATUnicodeString
 */ 
class ExportedByCO0SETST CATSetValCATUnicodeString  : public CATCollecRoot {
	public:
		
		/**
		 * Constructs an empty set of <tt>CATUnicodeStrings</tt>.
		 */
		CATSetValCATUnicodeString ();                                             

		/**
		 * Constructs an empty set of <tt>CATUnicodeStrings</tt> with a 
		 * specific initial capacity.
		 * @param iInitAlloc
		 * The default capacity of the set.
		 */
		CATSetValCATUnicodeString ( int iInitAlloc );                             

		/** @nodoc */
		CATSetValCATUnicodeString ( int iInitAlloc, int iExtensAlloc );           

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The set to copy.
		 */
		CATSetValCATUnicodeString ( const CATSetValCATUnicodeString& iCopy );                 

		/**
		 * Constructs a set and initializes it with a C++ <tt>CATUnicodeString</tt> array.
		 * @param iArray
		 * A C++ array of <tt>CATUnicodeStrings</tt> used to initialize the set.
		 * @param iSize
		 * The size of the C++ array of <tt>CATUnicodeStrings</tt> used to initialize the set.
		 */
		CATSetValCATUnicodeString ( CATUnicodeString* iArray, int iSize );

		/**
		 * Constructs a set and initializes it with a C++ <tt>CATUnicodeString</tt> pointer array.
		 * @param iArray
		 * A C++ array of <tt>CATUnicodeString</tt> pointers used to initialize the set.
		 * @param iSize
		 * The size of the C++ array of <tt>CATUnicodeString</tt> pointers used to initialize the set.
		 */
		CATSetValCATUnicodeString ( CATUnicodeString** iArray, int iSize );

		/**
		 * Destructor.
		 */
		~CATSetValCATUnicodeString ();                                            
  									 
		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the set with 
		 * another set.
		 * @param iCopy
		 * The assigned set.
		 */
		CATSetValCATUnicodeString& operator= ( const CATSetValCATUnicodeString& iCopy );       
  									 
		/**
		 * Adds a <tt>CATUnicodeString</tt> to the set if the set does not already contain it.
		 * @param iAdd
		 * The <tt>CATUnicodeString</tt> to append.
		 */
		int  Add ( const CATUnicodeString& iAdd );                                  

		/**
		 * Union with a set.
		 * <br><b>Role</b>: Adds all the elements from a <tt>CATUnicodeString</tt> set 
		 * which are not already contained by the set.
		 * @param iConcat
		 * The <tt>CATUnicodeString</tt> set to add.
		 */
		int  Add ( const CATSetValCATUnicodeString& iConcat );
  									 
		/**
		 * Returns the size of the set. 
		 * @return the size of the set.
		 */
		int  Size () const ;                                         
  									 
		/**
		 * Subscripting operator.
		 * @param iIndex
		 * The index of the element.
		 * @return 
		 * The <tt>CATUnicodeString</tt> at index <tt>iIndex</tt>.
		 */
		CATUnicodeString operator[] ( int iIndex ) const;                             
  									 
		/**
		 * Finds the index of a <tt>CATUnicodeString</tt> in the set.
		 * @param iLocate
		 * The <tt>CATUnicodeString</tt> to locate.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located <tt>CATUnicodeString</tt>, or 
		 * <tt>0</tt> if the set does not contain
		 * the specified <tt>CATUnicodeString</tt>.
		 */
		int  Locate ( const CATUnicodeString& iLocate ) const ;
  									 
		/**
		 * Removes a <tt>CATUnicodeString</tt> from the set.
		 * @param iRemove
		 * The <tt>CATUnicodeString</tt> to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed <tt>CATUnicodeString</tt>, 
		 * or <tt>0</tt> if the set does not contain
		 * the specified <tt>CATUnicodeString</tt>.
		 */
		int  RemoveValue ( const CATUnicodeString& iRemove );                                     

		/**
		 * Difference of two sets.
		 * <br><b>Role</b>: Removes all the values specifed in <tt>iSubstract</tt> from the set.
		 * @param iSubstract
		 * A set of <tt>CATUnicodeStrings</tt> to remove.
		 * @return 
		 * The count of <tt>CATUnicodeStrings</tt> removed from the set.
		 */
		int  Remove ( const CATSetValCATUnicodeString& iSubstract );

		/**
		 * Removes the <tt>CATUnicodeString</tt> located at a given index.
		 * @param iIndex
		 * The set index of the <tt>CATUnicodeString</tt to remove.
		 */
		void RemovePosition ( int iIndex );

		/**
		 * Removes all the elements from the set.
		 * @param iMH
		 * <br><b>Legal values</b>: Specifies whether the set capacity 
		 * should be shrunk to 0 
		 * (<tt>CATCollec::ReleaseAllocation</tt>) or not
		 * (<tt>CATCollec::KeepAllocation</tt>).
		 */
		void RemoveAll ( CATCollec::MemoryHandling iMH              
  				  = CATCollec::ReleaseAllocation );          
  									 
		/**
		 * Equality operator. 
		 * <br><b>Role</b>: Two sets are equal if they contain the same
		 * elements in the same order.
		 * @param iLV
		 * The set to test for equality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two sets are equal, 
		 * <tt>0</tt> otherwise
		 */
		int operator == ( const CATSetValCATUnicodeString& iLV ) const ;          

		/**
		 * Inequality operator.
		 * @param iLV
		 * The set to test for inequality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two sets are not equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator != ( const CATSetValCATUnicodeString& iLV ) const ;

		/**
		 * Less than or equal to operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than or 
		 * equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <= ( const CATSetValCATUnicodeString& iLV ) const ;

		/**
		 * Greater than or equal to operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than 
		 * or equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator >= ( const CATSetValCATUnicodeString& iLV ) const ;

		/**
		 * Less than operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than 
		 * <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <  ( const CATSetValCATUnicodeString& iLV ) const ;

		/**
		 * Greater than operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than 
		 * <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator >  ( const CATSetValCATUnicodeString& iLV ) const ;
  									 
		/**
		 * Compares two sets of <tt>CATUnicodeStrings</tt>. 
		 * @param iLV1
		 * the first set of <tt>CATUnicodeStrings</tt>.
		 * @param iLV2
		 * the second set of <tt>CATUnicodeStrings</tt>.
		 * @param iPFCompare
		 * A pointer to a function which compares two <tt>CATUnicodeStrings</tt> and
		 * returns -1, 0 or 1 depending on the order of the strings.
		 * @return 
		 * <br><b>Legal values</b>: <tt>0</tt> if the sets are equal,
		 * <tt>-1</tt> if the first set is smaller than the second set
		 * (smaller means that the first set contains less elements than the second
		 * set or that <tt>iRC1[i] &lt; iRC2[i]</tt> for the first i where
		 * <tt>iRC1[i] != iRC2[i]</tt> and < is a lexicographic comparison
		 * of two strings), or <tt>1</tt> otherwise.
		 */
		static int  Compare ( const CATSetValCATUnicodeString& iLV1,       
							  const CATSetValCATUnicodeString& iLV2,       
							  int  (*iPFCompare) ( CATUnicodeString*, CATUnicodeString* ) );
  									 
		/**
		 * Replaces an element at a specified index with another <tt>CATUnicodeString</tt> value. 
		 * @param iIndex
		 * index of the element to replace.
		 * @param iReplace
		 * The new <tt>CATUnicodeString</tt> value.
		 */
		int  Replace  ( int  iIndex, const CATUnicodeString& iReplace );                                      
  									 
		/**
		 * Fills a C++ array of <tt>CATUnicodeStrings</tt> with elements from the set.
		 * <br><b>Role</b>: The array has to have the same size as the set.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array  ( CATUnicodeString*  ioArray ) const ;

		/**
		 * Fills a C++ array of pointers to <tt>CATUnicodeStrings</tt> with elements from the set.
		 * <br><b>Role</b>: The array has to have the same size as the set.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array  ( CATUnicodeString** ioArray ) const ;                                          

		/**
		 * Test if the set contains the specified <tt>CATUnicodeString</tt>.
		 * @param iTest
		 * The <tt>CATUnicodeString</tt> to look up.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the set contains the
		 * specified <tt>CATUnicodeString</tt>, <tt>0</tt> otherwise.
		 */
		int Contains ( const CATSetValCATUnicodeString& iTest );

		/**
		 * Intersection of two sets.
		 * @param iL1
		 * The first set.
		 * @param iL2
		 * The second set.
		 * @param ioResult
		 * A set to which elements in the intersection are added.
		 */
		static void Intersection ( const CATSetValCATUnicodeString& iL1,
								   const CATSetValCATUnicodeString& iL2,
								   CATSetValCATUnicodeString& ioResult );

	private:                                                     

		/** @nodoc */
		CATListPV _Sval ;                                           
};


#endif  /* CATSetOfCATUnicodeString_h */
