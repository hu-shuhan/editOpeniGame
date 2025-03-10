//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Sequence_h
#define odb_Sequence_h

//
// This is a wrapper around cow_List*.
//
// ITEM - basic class of items; 
// LIST - list of items of type ITEM, mainly cow_List*
//

// Begin local includes
#include <omi_types.h>
#include <omn_pragma.h>
// End local includes

template <class ITEM, class LIST>
class odb_Sequence: public LIST
{
public:

    // Allocates no blocks; AllocatedSize() on this returns 0
    odb_Sequence();
    odb_Sequence(const LIST &);
    odb_Sequence(const ITEM * array, int arrayLen);
    // This constructor estimates the array with have 'length' items.
    odb_Sequence(int size);

    // Returns # of items inserted into the array
    int size() const;

    // Returns allocated Size
    int allocatedSize() const;

    // Is the list Empty (length == 0)
    bool isEmpty() const;
    
    // Grow sequence to *at least* the passed in size
    bool grow(int newSize);

    void append(const ITEM & item);

    // append the passed in list to this
    void append(const odb_Sequence & list);

    // Is the item in the list? Returns true/false
    bool isMember(const ITEM & item) const;

    // Find the first occurrence.  Returns size() on failure.
    int findFirst(const ITEM & item) const;

    bool isEqual(const odb_Sequence & rhs) const;

    // Mutator. A common idiom is Get(7) = x;
    // Precondition: you can't Get past what you've appended
    ITEM & get(int i);
    
    // Accessor. Precondition: you can't Get past what 
    // you've appended
    const ITEM & constGet(int i) const;

    // Accessor that assumes the array is full.
    // Use with CAUTION.
    const ITEM & operator[](int i) const;

    // Like Python slice. Last can be > length; it will be adjusted
    // The output includes start, but *not* end
    // The returned arraylist is constructed via the default constructor
    // and the results are appended to it.
    odb_Sequence<ITEM,LIST> slice(int first, 
                                  int last) const;

    // Insert v at the ith position. Returns true/false
    // Subsequent elements are moved down
    bool insert(int i, const ITEM & item);

    // Remove the ith element. Returns true/false
    // Subsequent elements are moved up
    bool remove(int i);

    // Clear the whole arraylist
    void clearAll();
};

// Comparison operators. 

template <class ITEM, class LIST>
inline bool operator==(const odb_Sequence<ITEM,LIST>& a, 
		       const odb_Sequence<ITEM,LIST>& b)
{ return a.isEqual(b); }

template <class ITEM, class LIST>
inline bool operator!=(const odb_Sequence<ITEM,LIST>& a, 
		       const odb_Sequence<ITEM,LIST>& b)
{ return !(a.isEqual(b)); }

// Use this macros in .h file
#define ODB_SEQUENCE_DECL(ITEM,LIST,NAME) \
typedef odb_Sequence<ITEM,LIST> NAME;

// Use this macros in .C file. Remember to include .T files!
#define ODB_SEQUENCE_IMPL(ITEM,LIST) \
template class odb_Sequence<ITEM,LIST>;

#endif  // #ifdef odb_Sequence_h
