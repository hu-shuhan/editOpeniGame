//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//             High performance integer mapped dictionary                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef omi_IntTable_h
#define omi_IntTable_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_UintTable.h>
// End local includes

// Forward declarations

template <class Value> struct cow_Direct;
template <class Value, class ALLOCATOR > class omi_IntTableIterator;



// Class definition

template <class Value, class ALLOCATOR >
class omi_IntTable : public mem_AllocationOperators
{
  friend struct cow_Direct<omi_IntTable<Value,ALLOCATOR > >;
  friend class omi_IntTableIterator<Value,ALLOCATOR >;

  public:

  inline omi_IntTable():
    negative(new omi_UintTable<Value,ALLOCATOR >()),
    positive(new omi_UintTable<Value,ALLOCATOR >()){}

    /// Create a table of pre-determined size (to avoid incremental growth),
    /// and populate it from given array of keys and values. 
    /// Use only with positive keys.
    omi_IntTable(size_t numEntries, const int* keys, const Value* values); 

    omi_IntTable(unsigned int high_bit,
         unsigned int mid_bit,  
         unsigned int init_pointer_sz);

    virtual ~omi_IntTable(){
      delete negative;
      delete positive;
    }

    ////////////////////
    // Basic
    bool Remove(int key) {
    return (key < 0) ? negative->Remove(-key) : positive->Remove(key);
    }

    bool Insert(int key, const Value& v) {
    return (key < 0) ? negative->Insert(-key, v) : positive->Insert(key, v);
    }

    ////////////////////
    // Lookup a value:
    const Value& ConstGet(int key) const {
    return (key < 0) ? negative->ConstGet(-key) : positive->ConstGet(key);
    }
    bool ConstGetValue(int key, Value& val) const {
    return (key < 0) ? negative->ConstGetValue(-key, val) : 
        positive->ConstGetValue(key, val);
    }

    ////////////////////
    // Access a value
    Value& Get(int key) const {
    return (key < 0) ? negative->Get(-key) : positive->Get(key);
    }

    ////////////////////
    // Test membership
    bool IsMember(int key) const {
    return (key < 0) ? negative->IsMember(-key) : positive->IsMember(key);
    }

    // Return Size of Map
    int Size() const { return negative->Size() + positive->Size(); }
    int SizeOfNegatives() const { return negative->Size(); }

    // Is Map empty?
    bool IsEmpty() const { return negative->IsEmpty() && positive->IsEmpty(); }

    // Clear the Map
    void Clear() { negative->Clear(); positive->Clear(); }

    // Deep copy 
    omi_IntTable<Value,ALLOCATOR >* Copy() const;

    size_t Footprint() const; // return currently held memory, in bytes

  protected:

    omi_IntTable(const omi_IntTable<Value,ALLOCATOR >& table);
    omi_IntTable<Value,ALLOCATOR >& operator=( const omi_IntTable<Value,ALLOCATOR >& );

    omi_UintTableBlock<Value,ALLOCATOR >* FindTableBlock(int tkey) const;

    // -- DATA Members --    
    omi_UintTable<Value,ALLOCATOR >* negative;
    omi_UintTable<Value,ALLOCATOR >* positive;
};

template <class Value, class ALLOCATOR >
class omi_IntTableNode: public omi_UintTableNode<Value>
{
  public:
    
    omi_IntTableNode() {}
    omi_IntTableNode(int k, const Value* v);
};


// template <typename ALLOCATOR >
template <class Value, class ALLOCATOR >
class omi_IntTableIterator : public mem_AllocationOperators
{
  public:

  // omi_IntTableIterator(const omi_IntTable<Value,SMABasOmiDefaultAllocator>& table);
  // omi_IntTableIterator(const omi_IntTable<Value,SMABasOmiDefaultAllocator>& table,
  //                       int (*cmpFun)(const void *, const void *));

   omi_IntTableIterator(const omi_IntTable<Value,ALLOCATOR >& table);
   omi_IntTableIterator(const omi_IntTable<Value,ALLOCATOR >& table,
						int (*cmpFun)(const void *, const void *));

    void  First();
    void  Last();
    void  Next();

    const Value& CurrentValue() const;
    int          CurrentKey() const;

    bool         IsDone() const;

  private:

    int                          numNegative;
    int                          totalNegative;
    omi_UintTableIterator<Value,ALLOCATOR > niter;
    omi_UintTableIterator<Value,ALLOCATOR > piter;

    int (*compar)(const void *, const void *);
};

#define OMI_INTTABLE_FWDL( VALUE, NAME ) \
OMI_UINTTABLE_FWDL(VALUE, NAME ## UINTTABLE) \
typedef omi_IntTable<VALUE,SMABasOmiDefaultAllocator> NAME; \
typedef omi_IntTableIterator<VALUE,SMABasOmiDefaultAllocator> NAME ## IT; \
typedef omi_IntTableIterator<VALUE,SMABasOmiDefaultAllocator> NAME ## SIT;

#define OMI_INTTABLE_ALLOCATOR_FWDL( VALUE, ALLOCATOR, NAME ) \
OMI_UINTTABLE_ALLOCATOR_FWDL(VALUE,ALLOCATOR, NAME ## UINTTABLE) \
typedef omi_IntTable<VALUE,ALLOCATOR > NAME; \
typedef omi_IntTableIterator<VALUE,ALLOCATOR > NAME ## IT; \
typedef omi_IntTableIterator<VALUE,ALLOCATOR > NAME ## SIT;

#ifdef SMA_NO_TGEN
#define OMI_INTTABLE_EXTL( VALUE, NAME ) \
OMI_UINTTABLE_EXTL(VALUE, NAME ## UINTTABLE) \
extern template class omi_IntTable<VALUE,SMABasOmiDefaultAllocator>; \
extern template class omi_IntTableIterator<VALUE,SMABasOmiDefaultAllocator>; 
#else
#define OMI_INTTABLE_EXTL( VALUE, NAME ) \
OMI_UINTTABLE_EXTL(VALUE, NAME ## UINTTABLE)
#endif

#ifdef SMA_NO_TGEN
#define OMI_INTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR) \
OMI_UINTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR) \
extern template class  omi_IntTable<TYPE,ALLOCATOR >; \
extern template class  omi_IntTableIterator<TYPE,ALLOCATOR >;
#else
#define OMI_INTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR) \
OMI_UINTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR)
#endif

#define OMI_INTTABLE_BLT_EXTL( VALUE, NAME ) \
OMI_INTTABLE_EXTL( VALUE, NAME ) \
static const bool NAME ## _ist = true;

#define OMI_INTTABLE_DECL( VALUE, NAME ) \
OMI_INTTABLE_FWDL( VALUE, NAME ) \
OMI_INTTABLE_EXTL( VALUE, NAME )

// The following macro DECLARES omi_IntTableVALUE class and its iterators
// using a user-provided ALLOCATOR

#define OMI_INTTABLE_ALLOCATOR_DECL( VALUE, ALLOCATOR, NAME ) \
OMI_UINTTABLE_ALLOCATOR_DECL(VALUE,ALLOCATOR, NAME ## UINTTABLE) \
typedef omi_IntTable<VALUE,ALLOCATOR > NAME; \
typedef omi_IntTableIterator<VALUE,ALLOCATOR > NAME ## IT; \
typedef omi_IntTableIterator<VALUE,ALLOCATOR > NAME ## SIT;

#define OMI_INTTABLE_BLT_DECL( VALUE, NAME ) \
OMI_INTTABLE_FWDL( VALUE, NAME ) \
OMI_INTTABLE_BLT_EXTL( VALUE, NAME )

#define OMI_INTTABLE_IMPL( VALUE, NAME ) \
OMI_UINTTABLE_IMPL(VALUE, NAME ## UINTTABLE) \
template class omi_IntTable<VALUE,SMABasOmiDefaultAllocator>; \
template class omi_IntTableIterator<VALUE,SMABasOmiDefaultAllocator>; 

// The following macro INSTANTIATES omi_IntTableVALUE class and its iterators
// using a user-provided ALLOCATOR

#define OMI_INTTABLE_ALLOCATOR_IMPL(TYPE,ALLOCATOR,NAME) \
OMI_UINTTABLE_ALLOCATOR_IMPL(TYPE,ALLOCATOR,NAME) \
template class  omi_IntTable<TYPE,ALLOCATOR >; \
template class  omi_IntTableIterator<TYPE,ALLOCATOR >;

#define OMI_INTTABLE_BLT_IMPL( VALUE, NAME ) \
OMI_INTTABLE_IMPL( VALUE, NAME ) \
static_assert(NAME ## _ist);


#endif
