//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
/////////////////////////////////////////////
// DNOTE: This class represent a Many:One omi_UintTable.
/////////////////////////////////////////////

#ifndef omi_UintTable_h
#define omi_UintTable_h

// Includes

#include <assert.h>

// Begin local includes

#include <mem_AllocationOperators.h>
#include <omi_types.h>
#include <omi_BlkBitVectTrav.h>
#include <omi_BlockedArray.h>
#include <omi_UintTableBlock.h>
#include <SMABasAllocationTracking.h>
#include <SMABasOmiDefaultAllocator.h>
#include <SMABasSrmAllocator.h>
// End local includes


// Forward declarations

template <class Value> struct cow_Direct;
template <class Value, class ALLOCATOR >  class omi_UintTableIterator;


// Class definition

template <class Value, class ALLOCATOR >
class omi_UintTable
{
  friend struct cow_Direct<omi_UintTable<Value,ALLOCATOR > >;
  friend class omi_UintTableIterator<Value,ALLOCATOR >;

  public:

    omi_UintTable();

    /// Create a table of pre-determined size (to avoid incremental growth)
    omi_UintTable(size_t numEntries); 

    /// Create a table of pre-determined size (to avoid incremental growth),
    /// and populate it from given arrays of keys and values.
    omi_UintTable(size_t numEntries, const unsigned int* keys, const Value* values); 

    omi_UintTable(unsigned int high_bit, 
                  unsigned int mid_bit,  
                  unsigned int init_pointer_sz);

    ~omi_UintTable();

    ////////////////////
    // Basic
    bool Remove(unsigned int);
    bool Insert(unsigned int, const Value&);

    ////////////////////
    // Lookup a value:
    inline const Value& ConstGet(unsigned int key) const {
	return FindTableBlock(key)->ConstGet(key & keyModulus);
    }

    inline bool ConstGetValue(unsigned int key, Value &value ) const {
	 unsigned int keymod = key & keyModulus;
	 const omi_UintTableBlock<Value,ALLOCATOR >* tblk = FindTableBlock(key);
	 if( (tblk != 0) && tblk->IsMember(keymod) )
	 {
	      value = tblk->ConstGet(keymod);
	      return true;
	 }
	 return false;
    }

    ////////////////////
    // Access a value
    inline Value& Get(unsigned int key) {
	return FindTableBlock(key)->Get(key & keyModulus);
    }

    ////////////////////
    // Test membership
    inline bool IsMember(unsigned int key) const {
	  const omi_UintTableBlock<Value,ALLOCATOR >* tblk = FindTableBlock(key);
	  return (tblk != 0) ? tblk->IsMember(key & keyModulus) : false;
    }

    // Return Size of Map
    int Size() const { return tableSize; }

    // Is Map empty?
    bool IsEmpty() const { return tableSize == 0; }

    // Clear the Map
    void Clear();

    // Deep copy of this
    omi_UintTable<Value,ALLOCATOR >* Copy() const;

    omi_UintTableBlock<Value,ALLOCATOR >* FindTableBlock(unsigned int tkey) const {
	  unsigned int index = tkey >> keyShift;
	  return (index < currentBlockSize) ? tableBlocks[index] : 0;
    }

    unsigned int KeyModulus() const { return keyModulus; }

    static inline void* operator new    (size_t sz) { return ALLOCATOR::allocate(sz); }
    static inline void* operator new[]  (size_t sz) { return ALLOCATOR::allocate(sz); }
    static inline void operator delete  (void* ptr) { ALLOCATOR::deallocate(ptr); }
    static inline void operator delete  (void* ptr, void*) { }
    static inline void operator delete[](void* ptr) { ALLOCATOR::deallocate(ptr); }
    static inline void operator delete[](void* ptr, void*) { }

    virtual size_t Footprint() const;

  private:

    void Initialize( unsigned int high_bit, 
					 unsigned int mid_bit,  
					 unsigned int init_pointer_sz);

    omi_UintTable( const omi_UintTable<Value,ALLOCATOR >& table );
    omi_UintTable<Value,ALLOCATOR >& operator=( const omi_UintTable<Value,ALLOCATOR >& );

    //  -- Data Members --
    omi_UintTableBlock<Value,ALLOCATOR > **tableBlocks;
    int                      tableSize;
    int                      currentBlockSize;
    unsigned int             keyModulus;
    unsigned int             keyShift;
    unsigned int             initPointerSz;
    unsigned int             blockOrder;
};

template <class Value>
class omi_UintTableNode : public mem_AllocationOperators
{
  public:
    
    omi_UintTableNode(): key(0), value(0) {}
    omi_UintTableNode(unsigned int k, const Value* v): key(k), value(v) {}

    unsigned int  key;
    const Value*  value;
};

template <class Value, class ALLOCATOR >
class omi_UintTableIterator : public mem_AllocationOperators
{
  public:

  omi_UintTableIterator(const omi_UintTable<Value,ALLOCATOR >& table);
  omi_UintTableIterator(const omi_UintTable<Value,ALLOCATOR >& table,
						int (*cmpFun)(const void *, const void *));

    ~omi_UintTableIterator();

    void First();

    void Next();
    void Last();

    void Previous();

    const Value&   CurrentValue() const;

    unsigned int   CurrentKey() const;

    bool IsDone() const;

    bool NextBlock();
    bool PreviousBlock();

  private:

    int (*compar)(const void *, const void *);

    const omi_UintTable<Value,ALLOCATOR >& tableRef;
    unsigned int               currentKey;
    unsigned int               currentOffset;
    int                        currentIndex;
    const omi_UintTableBlock<Value,ALLOCATOR >* currentBlock;
    omi_BlkBitVectTrav*        currentBlockTrav;
    bool                       isDone;
    omi_UintTableNode<Value>** tmpArray;
    int                        tableSize;
};

#define OMI_UINTTABLE_FWDL( VALUE, NAME ) \
typedef omi_UintTable<VALUE,SMABasOmiDefaultAllocator> NAME; \
typedef omi_UintTableIterator<VALUE,SMABasOmiDefaultAllocator> NAME ## IT; \
typedef omi_UintTableIterator<VALUE,SMABasOmiDefaultAllocator> NAME ## SIT; \
OMI_BLOCKEDARRAY_FWDL(VALUE, NAME ## BLOCKEDARRAY ) \
OMI_TABLEBLOCK_FWDL(VALUE, NAME ## TABLEBLOCK)

#define OMI_UINTTABLE_ALLOCATOR_FWDL( VALUE, ALLOCATOR, NAME ) \
typedef omi_UintTableBlock<VALUE,ALLOCATOR> NAME ## BLK; \
typedef omi_UintTable<VALUE,ALLOCATOR> NAME; \
typedef omi_UintTableIterator<VALUE,ALLOCATOR> NAME ## IT; \
typedef omi_UintTableIterator<VALUE,ALLOCATOR> NAME ## SIT;

#ifdef SMA_NO_TGEN
#define OMI_UINTTABLE_EXTL( VALUE, NAME ) \
OMI_BLOCKEDARRAY_EXTL(VALUE,NAME) \
OMI_TABLEBLOCK_EXTL(VALUE, NAME ## TABLEBLOCK) \
extern template class omi_UintTable<VALUE,SMABasOmiDefaultAllocator>; \
extern template class omi_UintTableIterator<VALUE,SMABasOmiDefaultAllocator>; \
extern template class omi_UintTableNode<VALUE>;
#else
#define OMI_UINTTABLE_EXTL( VALUE, NAME ) \
OMI_BLOCKEDARRAY_EXTL(VALUE,NAME) \
OMI_TABLEBLOCK_EXTL(VALUE, NAME ## TABLEBLOCK)
#endif

#ifdef SMA_NO_TGEN
#define OMI_UINTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR) \
extern template class  omi_UintTableBlock<TYPE,ALLOCATOR >; \
extern template class  omi_UintTable<TYPE,ALLOCATOR >; \
extern template class  omi_UintTableIterator<TYPE,ALLOCATOR >;
#else
#define OMI_UINTTABLE_ALLOCATOR_EXTL(TYPE,ALLOCATOR)
#endif

#define OMI_UINTTABLE_BLT_EXTL( VALUE, NAME ) \
OMI_UINTTABLE_EXTL( VALUE, NAME ) \
static const bool NAME ## _ist = true;

#define OMI_UINTTABLE_DECL( VALUE, NAME ) \
OMI_UINTTABLE_FWDL( VALUE, NAME ) \
OMI_UINTTABLE_EXTL( VALUE, NAME )


#define OMI_UINTTABLE_ALLOCATOR_DECL( VALUE, ALLOCATOR, NAME ) \
typedef omi_UintTableBlock<VALUE,ALLOCATOR> NAME ## BLK; \
typedef omi_UintTable<VALUE,ALLOCATOR> NAME; \
typedef omi_UintTableIterator<VALUE,ALLOCATOR> NAME ## IT; \
typedef omi_UintTableIterator<VALUE,ALLOCATOR> NAME ## SIT;

#define OMI_UINTTABLE_BLT_DECL( VALUE, NAME ) \
OMI_UINTTABLE_FWDL( VALUE, NAME ) \
OMI_UINTTABLE_BLT_EXTL( VALUE, NAME )


#define OMI_UINTTABLE_IMPL( VALUE, NAME ) \
OMI_BLOCKEDARRAY_IMPL(VALUE,NAME) \
OMI_TABLEBLOCK_IMPL(VALUE, NAME ## TABLEBLOCK) \
template class omi_UintTable<VALUE,SMABasOmiDefaultAllocator>; \
template class omi_UintTableIterator<VALUE,SMABasOmiDefaultAllocator>; \
template class omi_UintTableNode<VALUE>; \
template void mem_deleteBuiltIn<omi_UintTableNode<VALUE> >(omi_UintTableNode<VALUE>**); 


#define OMI_UINTTABLE_ALLOCATOR_IMPL(TYPE,ALLOCATOR,NAME) \
template class  omi_UintTableBlock<TYPE,ALLOCATOR >; \
template class  omi_UintTable<TYPE,ALLOCATOR >; \
template class  omi_UintTableIterator<TYPE,ALLOCATOR >;

#define OMI_UINTTABLE_BLT_IMPL( VALUE, NAME ) \
OMI_UINTTABLE_IMPL( VALUE, NAME ) \
static_assert(NAME ## _ist);


#endif
