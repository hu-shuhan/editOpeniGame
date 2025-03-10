#ifndef  CATHashTable_h
#define  CATHashTable_h

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

// COPYRIGHT DASSAULT SYSTEMES 2000

typedef unsigned int CATHashKey; 

#include  "CxxSupport.h"
#include  "CATCollec.h"
#include  "CATHashCodeCollec.h"
#include  "CATListPV.h"
#include  "CO0HTAB.h"

// forward declarations
class CATHashTable ;
#ifdef _CAT_ANSI_STREAMS
/** @c++ansi aew 2004-08-02.20:05:14 [Replace forward declaration of standard streams with iosfwd.h] **/
 #include "iosfwd.h" 
#else //!_CAT_ANSI_STREAMS 
class ostream ;
#endif //_CAT_ANSI_STREAMS

/**
* Class to define a hash table.
* <b>Role</b>:The class collection you will obtain will gain benefits of C++
* strong type checking, but its designed to reduce executable code size and
* to fasten heavily used operations. This table is not ordered, contains
* no duplicate elements and no null elements. 
* <br>It is up to you to provide the dimension, the hash-code function and the
* compare function for each instance of hash table. Only the underlying 
* infrastructure is offered. 
*/
class ExportedByCO0HTAB CATHashTable : public CATHashCodeCollec
{
  public :

	/**
	* Constructs an empty hash table.
	* <br><b>Role</b>:Constructs an empty hash table of dimension iDimension.
	* <br>The hash table contains X buckets, X is defined from iDimension. An item
	* is inserted in a bucked according to its key hash value. A bucket can contain
	* several items with not the same key.
	* @param iPFH
	*  Pointeur on a global hashing function. This function must returned a
	*  key of each element of the table.
	* @param iPFC
    *  Pointeur on a global comparison function. This function must return 0
	*  if both instance of the table are equal.
	* @param iDimension
	*  Helps specifying the number of bucket (cells receiving 0 to N items) and
	*  this number will not vary up to the destruction of the hash table.
	*  <br><b>Default value</b>:10
	*/
    CATHashTable ( CATCollec::PFHash iPFH, CATCollec::PFCompare iPFC, int iDimension = 10 );
	
	/** @nodoc */
	CATHashTable ( CATCollec::PFHash iPFH, CATCollec::PFCompare iPFC, int iDimension, void * iOptions );

	/** 
	* Constructs a new hash table from an another one.
	* <br><b>Role</b>: This new table as same characteristics and
	* contents is copied.
	* @param iCopy
	* The hash table to duplicate
	*/
    CATHashTable ( const CATHashTable& iCopy );
	
    ~CATHashTable ();

    /** @nodoc */
    CATHashTable& operator = ( const CATHashTable& iCopy );
	
#if defined(DS_CXX11_SUPPORT_MOVE_SEMANTIC)
	/** @nodoc Move constructor. */
    CATHashTable(CATHashTable&& iRValue);
	
    /** @nodoc Move assignment operator. */
    CATHashTable& operator=(CATHashTable&& iRValue);
#endif  // DS_CXX11_SUPPORT_MOVE_SEMANTIC

	/** 
	* Inserts an item in the hash table.
	* <br><b>Role</b>:Inserts an item according to its key in the table. The key
	* is computed with the provided hash-code function with iAdd as argument.
    * @param iAdd
	*  Item to insert 
	* @return
	* If the key already exists, nothing is inserted and 0 is returned else iAdd is
	* inserted in a bucket and 1 is returned.	
	*/
    int Insert ( void* iAdd);

	/** 
	* Returns the number of bucket in the hash table.
	* <br><b>Role</b>:This number is also the maximum number of items without
	* collisions.
	*/
    int Dimension () const;

	/** 
	* @nodoc 
	* Warning, this method modifies the internal data structure of the hashtable then all 
	* relative information that has been saved should be recomputed (e.g. buckets or positions 
	* index, count of unused buckets, the dimension).
	*/
	void Rebuild(int iNewDimension); 
    
	/**
	* Returns the count of item in the hash table..
    */
    int Size () const;

	/**
	* Returns the count of item not directly accessible in a hash table.
	* <br><b>Role</b>: Item directly accessible are those at the first position 
	* in a bucket.
	*/
    int Collisions () const;

	/**
	* Returns the count of unused buckets in an hash table.
	*/
    int Unused () const;

	/**
	* Returns the item with has a given hash value key.
	* @param iKey
	* Hash value key.
	* @return 
	* The item which has iKey hash table key value, or NULL if not found.
	*/
    void* KeyLocate ( CATHashKey iKey ) const ;

	/**
	* Returns the item with has a given hash value key.
	* @param iKey
	*  Hash value key.
	* @param oBucket
	* the bucket of the item
	* @param oPosition
	* The position of this item in the bucket.
	* @return 
	* The item which has iKey hash table key value, or NULL if not found.
	*/
    void* KeyLocatePosition ( CATHashKey iKey, int& oBucket, int& oPosition ) const ;

	/** 
	* Returns an item from a key corresponding to an another item.
	* @param iLocate
	* We used it's key .
	* @return 
	* The item having a key corresponding to iLocate key's or NULL if not found.
	*/
    void* Locate ( void* iLocate ) const ;

	/** 
	* Returns an item from a key corresponding to an another item.
	* @param iLocate
	*  The element to find in the table.
	* @param oBucket
	*  The bucket of the returned item
	* @param oPosition
	*  The position in the bucket of the returned item
	* @return 
	* The item having a key corresponding to iLocate key's or NULL if not found.
	*/
    void* LocatePosition ( void* iLocate, int& oBucket, int& oPosition ) const ;

	/** 
	* Retrieves an item from its Bucket and Position in a hash table.
	* @param iBucket
	* The bucket of the returned item
	* @param iPosition
	* The position in the bucket of the returned item
	* @return 
	* Returns the item having position descibed by the couple of integer (iBucket,iPosition) 
	*/
    void* Retrieve ( int iBucket, int iPosition ) const ;

	/** @nodoc */
    void* operator[] ( const CATHashCodeIter& iIter ) const ; 


	/** @nodoc */
    void* NextItem( int&  ioBucket, int& ioPosition ) const ;

	/**
	* Returns the next item given a position in a hash table.
	* <br><b>Role</b>:Iterator of the hash table. To start a loop on all items set 
	* ioBucket to 1 and ioPosition to 0. If the returned value is NULL, you have reached 
	* the end of the table.
	* @param ioBucket
	* The bucket used to find the next item
	* @param ioPosition
	* The position in the  ioBucket used to find the next item
	* @return 
	* The next item next to the given position (iBucket, iPosition).
	*/
    void* Next    ( int&  ioBucket, int& ioPosition ) const ;

	/**
	* Returns the next item given a key in a hash table.
	* <br><b>Role</b>:Iterator of the hash table. To start a loop on all items set 
	* iFrom to NULL. If the returned value is NULL, you have reached 
	* the end of the table.
	* @param iFrom
	* The returned value is the next item of iFrom
	* @return 
	* The next item after iFrom
	*/
    void* Next    ( void* iFrom ) const ;


	/** 
	* Removes an item in the hash table.
	* @param iRemove
	* The element to remove
	* @return 
	*  The element removed or NULL if not exists
	*/
    void* Remove  ( void* iRemove );

	/** 
	* Removes an item in the hash table.
	* @return 
	*  The element removed or NULL if not exists
	*/
    void* RemovePosition ( int iBucket, int iPosition );


	/** 
	* Removes all elements in the hash table.
	*/
    void  RemoveAll ();


	/** 
	* @nodoc 
    * the size of lists is the first differentiator,
    * then each element is compared one to one (ordered test)
	*/
    int operator == ( const CATHashTable& iHT ) const ;

	/** @nodoc */
    int operator != ( const CATHashTable& iHT ) const ;


	/** @nodoc 
	* Apply 
	*/
    typedef int (*PtrGlbFunct) (void*) ;

	/** @nodoc */
    int ApplyGlobalFunct ( PtrGlbFunct iPF  ,
                           void* iFrom = 0  ,
                           void** iPLast = 0,
                           int* iPRC = 0    ) const ;

  private :

    // DATA MEMBERS
    void *_impl;
	int   _Flags;
};

//
// Macro to help iterations on hash tables
//
#include  "CATHashCodeIter.h"

/** @nodoc */
#define CATHTAB_START_LOOP( H, ITR )                 \
{                                                    \
  CATHashCodeIter ITR = (H).CreateIterator();        \
  while ( ITR++ != NULL )

#define CATHTAB_END_LOOP   }

#endif                /* CATHashTable_h */


