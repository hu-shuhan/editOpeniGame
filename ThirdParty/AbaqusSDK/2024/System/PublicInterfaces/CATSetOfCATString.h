#ifndef CATSetOfCATString_h
#define CATSetOfCATString_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CO0SETST.h"
#include "CATListPV.h"
class CATString;

/**
 * Class to implement a mutable ordered set of CATString.
 * <b>Role</b>: The purpose of this class is to let C++ classes easily pass and
 * receive sets of @href CATString. The class handles all the low-level
 * memory allocation tasks such as reallocating the set once it capacity has
 * been exceeded. Set elements are sorted by lexicographic order.
 * <br>Use the type @href CATSetOfCATString
 */ 
class ExportedByCO0SETST CATSetValCATString  : public CATCollecRoot {
	public:                                                     
		/**
		 * Constructs an empty set of CATString.
		 */
		CATSetValCATString ();  
		
		/**
		 * Constructs an empty set of CATString with a 
		 * specific initial capacity.
		 * @param iInitAlloc
		 * The default capacity of the set.
		 */
		CATSetValCATString ( int iInitAlloc );                             

		/** @nodoc */
		CATSetValCATString ( int iInitAlloc, int iExtensAlloc );           

		/**
		 * Copy constructor.
		 * @param iCopy
		 * The set to copy.
		 */
		CATSetValCATString ( const CATSetValCATString& iCopy );    
		
		/**
		 * Constructs a set and initializes it with a C++ CATString array.
		 * @param iArray
		 * A C++ array of @href CATString used to initialize the set.
		 * @param iSize
		 * The size of the C++ array of <tt>CATString</tt> used to initialize the set.
		 */
		CATSetValCATString ( CATString* iArray, int iSize );

		/**
		 * Constructs a set and initializes it with a C++ <tt>CATString</tt> pointer array.
		 * @param iArray
		 * A C++ array of <tt>CATString</tt> pointers used to initialize the set.
		 * @param iSize
		 * The size of the C++ array of <tt>CATString</tt> pointers used to initialize the set.
		 */
		CATSetValCATString ( CATString** iArray, int iSize );

		/**
		 * Destructor.
		 */
		~CATSetValCATString ();                                            
  									 
		/**
		 * Assignment operator. 
		 * <br><b>Role</b>: Overwrites the content of the set with 
		 * another set.
		 * @param iCopy
		 * The assigned set.
		 */
		CATSetValCATString& operator= ( const CATSetValCATString& iCopy );       
  									 
		/**
		 * Adds a <tt>CATString</tt> to the set if the set does not already contain it.
		 * @param iAdd
		 * The <tt>CATString</tt> to append.
		 */
		int  Add ( const CATString& iAdd );                                  

		/**
		 * Union with a set.
		 * <br><b>Role</b>: Adds all the elements from a <tt>CATString</tt> set 
		 * which are not already contained by the set.
		 * @param iConcat
		 * The <tt>CATString</tt> set to add.
		 */
		int  Add ( const CATSetValCATString& iConcat );
  									 
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
		 * The <tt>CATString</tt> at index <tt>iIndex</tt>.
		 */
		CATString operator[] ( int iIndex ) const;                             
  									 
		/**
		 * Finds the index of a <tt>CATString</tt> in the set.
		 * @param iLocate
		 * The <tt>CATString</tt> to locate.
		 * @return 
		 * <br><b>Legal values</b>: The index of the located <tt>CATString</tt>, or 
		 * <tt>0</tt> if the set does not contain
		 * the specified <tt>CATString</tt>.
		 */
		int  Locate ( const CATString& iLocate ) const ;
  									 
		/**
		 * Removes a <tt>CATString</tt> from the set.
		 * @param iRemove
		 * The <tt>CATString</tt> to remove.
		 * @return 
		 * <br><b>Legal values</b>: The index of the removed <tt>CATString</tt>, 
		 * or <tt>0</tt> if the set does not contain
		 * the specified <tt>CATString</tt>.
		 */
		int  RemoveValue ( const CATString& iRemove );

		/**
		 * Difference of two sets.
		 * <br><b>Role</b>: Removes all the values specifed in <tt>iSubstract</tt> from the set.
		 * @param iSubstract
		 * A set of <tt>CATString</tt> to remove.
		 * @return 
		 * The count of <tt>CATString</tt> removed from the set.
		 */
		int  Remove ( const CATSetValCATString& iSubstract );


		/**
		 * Removes the <tt>CATString</tt> located at a given index.
		 * @param iIndex
		 * The set index of the <tt>CATString</tt to remove.
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
		int operator == ( const CATSetValCATString& iLV ) const ;     		

		/**
		 * Inequality operator.
		 * @param iLV
		 * The set to test for inequality
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the two sets are not equal, 
		 * <tt>0</tt> otherwise.
		 */
		int operator != ( const CATSetValCATString& iLV ) const ;


		/**
		 * Less than or equal to operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than or 
		 * equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <= ( const CATSetValCATString& iLV ) const ;


		/**
		 * Greater than or equal to operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than 
		 * or equal to <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator >= ( const CATSetValCATString& iLV ) const ;

		/**
		 * Less than operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is less than 
		 * <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator <  ( const CATSetValCATString& iLV ) const ;

		/**
		 * Greater than operator.
		 * @param iLV
		 * The set to compare the receiver to.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> the receiver is greater than 
		 * <tt>iLV</tt>, <tt>0</tt> otherwise
		 */
		int operator >  ( const CATSetValCATString& iLV ) const ;
  									 
		/**
		 * Compares two sets of CATString. 
		 * @param iLV1
		 * the first set of <tt>CATString</tt>.
		 * @param iLV2
		 * the second set of <tt>CATString</tt>.
		 * @param iPFCompare
		 * A pointer to a function which compares two <tt>CATString</tt> and
		 * returns -1, 0 or 1 depending on the order of the strings.
		 * @return 
		 * <br><b>Legal values</b>: <tt>0</tt> if the sets are equal,
		 * <tt>-1</tt> if the first set is smaller than the second set
		 * (smaller means that the first set contains less elements than the second
		 * set or that <tt>iRC1[i] &lt; iRC2[i]</tt> for the first i where
		 * <tt>iRC1[i] != iRC2[i]</tt> and < is a lexicographic comparison
		 * of two strings), or <tt>1</tt> otherwise.
		 */
		static int  Compare ( const CATSetValCATString& iLV1,       
							  const CATSetValCATString& iLV2,       
							  int  (*iPFCompare) ( CATString*, CATString* ) );
  									 
		/**
		 * Replaces an element at a specified index with another <tt>CATString</tt> value. 
		 * @param iIndex
		 * index of the element to replace.
		 * @param iReplace
		 * The new <tt>CATString</tt> value.
		 */
		int  Replace  ( int  iIndex, const CATString& iReplace );                                      
  									 
		/**
		 * Fills a C++ array of CATString with elements from the set.
		 * <br><b>Role</b>: The array has to have the same size as the set.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array  ( CATString*  ioArray ) const ;

		/**
		 * Fills a C++ array of pointers to CATString with elements from the set.
		 * <br><b>Role</b>: The array has to have the same size as the set.
		 * @param ioArray
		 * The C++ array to fill.
		 */
		void Array  ( CATString** ioArray ) const ;                                          

		/**
		 * Test if the set contains the specified <tt>CATString</tt>.
		 * @param iTest
		 * The <tt>CATString</tt> to look up.
		 * @return
		 * <br><b>Legal values</b>: <tt>1</tt> if the set contains the
		 * specified <tt>CATString</tt>, <tt>0</tt> otherwise.
		 */
		int Contains ( const CATSetValCATString& iTest );

		/**
		 * Intersection of two sets.
		 * @param iL1
		 * The first set.
		 * @param iL2
		 * The second set.
		 * @param ioResult
		 * A set to which elements in the intersection are added.
		 */
		static void Intersection ( const CATSetValCATString& iL1,
								   const CATSetValCATString& iL2,
								   CATSetValCATString& ioResult );
	private:                                                     

		/** @nodoc */
		CATListPV _Sval ;                                           
};

#endif  /* CATSetOfCATString_h */
