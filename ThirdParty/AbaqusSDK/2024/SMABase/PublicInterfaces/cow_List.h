//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cow_List_h
#define cow_List_h

///////////////////////////////////////////////////////////////////////////////
//
// Description:
//
//    The cow_List class template provides an expandable list interface with 
//    blocked array performance. 
//

//
// Begin local includes
//
// #include <mem_AllocationOperators.h>
#include <SMABasCowAllocator.h>
#include <omi_types.h>
#include <omn_pragma.h>
#include <cow_Counter.h>
// #include <mem_C_Allocation.h>

enum { DEFAULT_BLOCKSHIFT =  4 };

//
// Allocator Class definition
//
class cow_allocator 
{
public:
  cow_allocator() {} 

  //  static void* allocate(size_t __n) {  
  //	return mem_Malloc_Handled(__n); 
  //  }
  //  static void  deallocate(void* __p) {  
  //	mem_Free(__p);
  //  }
};



//
// Forward Declarations
//
template <class Item> class cow_ListIter;


//
// Class definition
//

// A picture is worth lotsa comments:

/*
   Box #1           Box #2     Box #3        Box #4
+-----------+
|cow_List   |   
+-----------+   +--->+-+    +-------------+
|blockLimit |   |   0|*+--->|cow_ListBlock|
|blockSize  |   |    +-+    +-------------+
|array      +---+   1|*+    |blockSize    |  +-+
+-----------+        +-+    +T*           +->|T|0
                    2|*+    +-------------+  +-+
                     +-+                     |T|1
                    3|*+                     +-+
                     +-+                     |T|2
                    4|*+                     +-+
                     +-+                     |T|3
                    5|*+                     +-+
                     +-+                     |T|4
                     +.+                     +-+
         blockLimit-1|*|                     |T|blockSize-1
                     +-+                     +-+
*/

// Box #2 will grow vertically as needed. Box #4 is fixed in length
// (and == blockSize). Each cow_ListBlock (#3) is lazily allocated
// and then pointed to by a slot in Box #2. When Box #2 is copied, the
// new slots point to the old blocks until a Get is done, and then the
// block is copied.

//
// cow_ListBlock HELPER CLASS: this holds the array of T's
//

template <class Item>
class cow_ListBlock : public cow_Counter
{
public:

    cow_ListBlock(int sz);

    ~cow_ListBlock();

    // mutator
    Item& Get(int i) { return arr[i]; }

    // accessor
    const Item& ConstGet(int i) const { return arr[i]; }
    const Item* Items() const { return arr; }

private:
    Item      *arr;

    cow_ListBlock(const cow_ListBlock<Item>&);
    cow_ListBlock<Item>& operator=(const cow_ListBlock<Item>&);
};


// The following are necesary to avoid conflict with short list constructors.
enum cow_ListConstruct { cow_EstimateLength };
enum cow_ListFill { cow_Fill };

//
// This is the REAL CLASS you want to use
//

template <class Item>
class cow_List : public SMABasCowAllocator
{
    friend class cow_ListIter<Item>;

public:
    // Default constructor allocates nothing. Avoid it unless your list will 
    // always be small. 
    inline cow_List();

    // This is the preferred constructor. Be generous in your estimate. A
    // block length of roughly 1/16th of your estimate will be calculated.
    inline cow_List(cow_ListConstruct, int);

    // This is constructor initializes the list a uniform value. 
    inline cow_List(cow_ListFill, int length, const Item& item = Item());

    // Initialize with a native array. The second argument will be taken as
    // the estimated length, if none is provided.
    inline cow_List(const Item* C_arr, int C_arrLen, int estimate = 0);

    // Short list constructors -- for very short lists.
    cow_List(const Item&);
    cow_List(const Item&, const Item&);
    cow_List(const Item&, const Item&, const Item&);
    cow_List(const Item&, const Item&, const Item&, const Item&);
    cow_List(const Item&, const Item&, const Item&, const Item&, const Item&);

    // The Big Three must be implemented explicitly.
    ~cow_List(){
        if (refCount->Decrement()) {
            Resize(0);
            delete refCount;
        }
        else {
            length = 0;
            blockLimit = 0;
            blockShift = DEFAULT_BLOCKSHIFT;
            start = 0;
            array = 0;
        }
        refCount = 0;
    }

    cow_List(const cow_List<Item>& rhs): length(rhs.length),
          blockLimit(rhs.blockLimit), 
          blockShift(rhs.blockShift),
          start(rhs.start),
          array(rhs.array), 
          refCount(rhs.refCount)
    {
        refCount->Increment();
    }


    cow_List<Item>& operator=(const cow_List<Item>&);

    // Member Fuctions:

    // Release everthing -- revert to default object.
    cow_List<Item>&    Clear() { return Resize(0); }

    // Adding
    cow_List<Item>&    Append(const Item& item); 
    cow_List<Item>&    Concatenate(const cow_List<Item>& lst);
    cow_List<Item>&    InsertIndex(int, const Item&);
    cow_List<Item>&    Prepend(const Item& item); 
    cow_List<Item>&    Fill(int length, const Item& item = Item()); 

    // Removing
    cow_List<Item>&    RemoveIndex(int index);
    cow_List<Item>&    RemoveLast()  { return RemoveIndex(length-1); }
    cow_List<Item>&    RemoveFirst() { return RemoveIndex(0); }
    Item Pop();

    // Queries
    int   Length() const { return length; }
    bool  IsEmpty() const { return length == 0; }

    // Protection

    // This method throws an exception if the index is out-of-range.
    inline int CheckIndex(int index) const;

    // Accessors

    // Assert that the member exists.
    const Item&  ConstGet(int i) const { return UncheckedConstGet(CheckIndex(i)); }
    const Item&  First() const { return ConstGet(0); }
    const Item&  Last()  const { return ConstGet(length-1); }

    // Assume that the member exists.
    // Use with CAUTION.
    inline const Item& UncheckedConstGet(int i) const;

    // The index operator is provided for syntactic convenience.
    const Item& operator[](int i) const { return ConstGet(i); }

    // Mutator
    Item&        Get(int i);

    // The Resize method will expand or shrink the list. Expansion will do no
    // more than to increase the number of pointers to blocks. Shrinking will
    // destroy any items between the new and old length, and release any empty
    // blocks.
    cow_List<Item>& Resize(int newSize);

    // This method is provided for the corresponding constructor. It behaves
    // like an assignment if called directly.
    cow_List<Item>& SetFromArray(const Item* C_arr, int C_arrLen);

    // Like Python slice. end can be > length; it will be adjusted
    // The output includes start, but *not* end
    cow_List<Item> Slice(int start, int end) const;

    // Return a copy of the arraylist as a native C array. Also
    // returns length of the new C array as an inout.
    //
    // WARNING: Caller *must* delete!
    Item* GetCArray(int &len) const;


    // Equality Methods. The following methods require == operators 
    // on the template parameter.
    //
    // To instantiate these members include cow_ListEquality.T
    //
    bool IsMember(const Item &) const; // true iff item == an item in list.
    cow_List<Item>& RemoveMember(const Item& key); // remove first item that == key.
    int  FindMember(const Item&)  const;

    // The IsEqual method returns true iff each item AND order are equal.
    // Inline Equality and Inequality operators are provided for syntactic 
    // convenience.
    //
    bool IsEqual(const cow_List<Item>&) const;

    // Conditional append
    cow_List<Item>&  AppendUnlessMember(const Item&);


    // The Difference method computes the disjunction between two sets. An
    // object is in the resulting list if and only if it is in this list,
    // but not in the argument list. This method produces a new list.
    // An inline operator- is provided for syntactic convenience.
    //
    cow_List<Item> Difference(const cow_List<Item>&) const;

    // Comparison Methods. The following methods require == operators and
    // operator < on the template parameter.
    //
    // To instantiate these members include cow_ListCompare.T
    //
    cow_List<Item>& Sort(); // Sort the list in acsending order.

private:

    int  length;
    int  blockLimit;
    int  blockShift;
    int  start;
    cow_Counter   *refCount;
    cow_ListBlock<Item> **array;

    // Helpers
    void CopyArray();
    void Initialize(int);
    void InitializeBlock(const Item*,int,int);
    void IsolateRange(int,int);
    void Prefix();
    void RaiseOutOfRange(int) const;
    void Isolate(int index) { IsolateRange(index,index); }
    inline Item& UncheckedGet(int i);
    inline void  Put(int, const Item&);
    inline void  Destroy(int);

    // private member used in the sort algorithm. Done as a member so that
    // the template does not have to be instantiated separately
    static int SortCmp(const void *, const void *); 
};


//
// List Iterator
//
template <class Item>
class cow_ListIter : public mem_AllocationOperators
{
public:
    // Constructors & Destructors
    cow_ListIter(const cow_List<Item>&);

    // Member Fuctions:

    bool NotDone() const { return arrayIndex < arrayRange; }
    bool IsDone() const { return ! NotDone(); }

    // Accessing
    const Item& CurrentItem() const { return items[itemIndex]; }

    // return the array index of the current item
    int ArrayIndex() const { return arrayIndex-array.start; }


    cow_ListIter<Item>&  First();
    cow_ListIter<Item>&  Next();

// Data Members:
protected:

    cow_List<Item> array;
    const Item* items;
    int arrayRange;
    int blockSize;
    int arrayIndex; // index within the array [<arrayRange]
    int itemIndex;  // index within the block [<blockSize]
};


//
// Inlines
//


///////////////////////////////////////////////////////////////////////////////
// Constructors
//

template<class Item>
inline
cow_List<Item>::cow_List()
{
    Initialize(0);
}

template<class Item>
inline
cow_List<Item>::cow_List(cow_ListConstruct, int sz)
{
    Initialize(sz);
}

template<class Item>
inline
cow_List<Item>::cow_List(cow_ListFill, int sz, const Item& item)
{
    Initialize(sz);
    Fill(sz, item);
}

template<class Item>
inline
cow_List<Item>::cow_List(const Item* C_arr, int C_arrLen, int estimate)
{
    Initialize(estimate ? estimate : C_arrLen);
    SetFromArray(C_arr, C_arrLen);
}


template <class Item>
inline int cow_List<Item>::CheckIndex(int i) const
{
    if (0 > i || i >= length)
        RaiseOutOfRange(i);
    return i;
}

template <class Item>
inline const Item& cow_List<Item>::UncheckedConstGet(int i) const
{
    int k = start+i;
    int block = k >> blockShift;
    int index = k - (block << blockShift);

    return array[block]->ConstGet(index);
}

// Inline global operators. These must be global operators, rather
// than methods, to avoid explicit instantiation problems with the
// Intel compiler.

template <class Item>
inline bool operator==(const cow_List<Item>& a, const cow_List<Item>& b)
{
    return a.IsEqual(b);
}

template <class Item>
inline bool operator!=(const cow_List<Item>& a, const cow_List<Item>& b)
{
    return ! a.IsEqual(b);
}

template <class Item>
inline cow_List<Item> operator-(const cow_List<Item>& a, const cow_List<Item>& b)
{
    return a.Difference(b);
}


//
// Instantiation macros
//

//
// Instantiate the list template over item using the given name
//
#define COW_LIST_FWDL(ITEM,NAME) \
typedef cow_ListBlock<ITEM> NAME ## Blk; \
typedef cow_List<ITEM> NAME; \
typedef cow_ListIter<ITEM> NAME ## IT;

#define COW_LIST_ITER_FWDL(ITEM,NAME) COW_LIST_FWDL(ITEM,NAME)

#ifdef SMA_NO_TGEN
#define COW_LIST_EXTL(ITEM,NAME) \
extern template class cow_ListBlock<ITEM>; \
extern template class cow_List<ITEM>; \
extern template class cow_ListIter<ITEM>;
#else
#define COW_LIST_EXTL(ITEM,NAME)
#endif

#define COW_LIST_ITER_EXTL(ITEM,NAME) COW_LIST_EXTL(ITEM,NAME)

#define COW_LIST_BLT_EXTL(ITEM,NAME) \
COW_LIST_EXTL(ITEM,NAME) \
static const bool NAME ## _ist = true;

#define COW_LIST_ITER_BLT_EXTL(ITEM,NAME) COW_LIST_BLT_EXTL(ITEM,NAME)

#define COW_LIST_DECL(ITEM,NAME) \
COW_LIST_FWDL(ITEM,NAME) \
COW_LIST_EXTL(ITEM,NAME)

//
// The COW_LIST_ITER_* macros never behaved differently than the
// COW_LIST_* macros. They are maintained for temporary compatiblity.
//
#define COW_LIST_ITER_DECL(ITEM,NAME) COW_LIST_DECL(ITEM,NAME)

#define COW_LIST_BLT_DECL(ITEM,NAME) \
COW_LIST_FWDL(ITEM,NAME) \
COW_LIST_BLT_EXTL(ITEM,NAME)

#define COW_LIST_ITER_BLT_DECL(ITEM,NAME) COW_LIST_BLT_DECL(ITEM,NAME)


//
// Explicit instantiators for the template
//
//
#define COW_LIST_IMPL(ITEM,NAME) \
template class cow_ListBlock<ITEM>; \
template class cow_List<ITEM>; \
template class cow_ListIter<ITEM>;

#define COW_LIST_ITER_IMPL(ITEM,NAME) COW_LIST_IMPL(ITEM,NAME)

#define COW_LIST_BLT_IMPL(ITEM,NAME) \
COW_LIST_IMPL(ITEM,NAME) \
static_assert(cow_List_builtin); \
static_assert(cow_COW_builtin); \
static_assert(NAME ## _ist);

#define COW_LIST_ITER_BLT_IMPL(ITEM,NAME) COW_LIST_BLT_IMPL(ITEM,NAME)


#endif  // #ifdef cow_List_h
