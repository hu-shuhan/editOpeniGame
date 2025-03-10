//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef omi_UintTableBlock_h
#define omi_UintTableBlock_h

#if defined (_WINDOWS_SOURCE)
#pragma warning (disable: 4584)
#endif


//
// Includes
//
#include <assert.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_types.h>
#include <omi_BlockedArray.h>
#include <omi_BlkBitVect.h>

//
// Forward declarations
//
template <class Value, class ALLOCATOR>  class omi_IntTableIterator;
template <class Value, class ALLOCATOR>  class omi_UintTableIterator;

//
// Class definition
//

/////////////////////////////////////////////
// DNOTE: This class represent a Many:One omi_UintTableBlock.
/////////////////////////////////////////////

template <class Value, class ALLOCATOR>
class omi_UintTableBlock 
    : private omi_BlockedArray<Value>
{

    friend class omi_IntTableIterator<Value,ALLOCATOR>;
    friend class omi_UintTableIterator<Value,ALLOCATOR>;

public:

    omi_UintTableBlock( unsigned int init_pointer_sz, unsigned int block_size );

    omi_UintTableBlock( const omi_UintTableBlock<Value,ALLOCATOR>& tableblock );

    ~omi_UintTableBlock();

    ////////////////////
    // Basic
    inline bool Remove(unsigned int key)
    {
	unsigned int cnt = tableMask.BitCount();
	tableMask.UnsetBit(key);
	return (cnt != tableMask.BitCount());
    }
    inline bool Insert(unsigned int key, const Value &val)
    {
	unsigned int cnt = tableMask.BitCount();
	tableMask.SetBit(key);	
	this->SetCell( key, val );
	return (cnt != tableMask.BitCount());
    }

    ////////////////////
    // Lookup a value:
    // This method is not key safe!!! don't call with !IsMember(key)
    inline const Value& ConstGet(unsigned int key) const
    {
	return omi_BlockedArray<Value>::blockPointers[key>>omi_BlockedArray<Value>::blockOrder][key&omi_BlockedArray<Value>::blockModulus];
    }

    ////////////////////
    // Access a value
    // This method is not key safe!!! don't call with !IsMember(key)
    inline Value& Get(unsigned int key)
    {
	return omi_BlockedArray<Value>::blockPointers[key>>omi_BlockedArray<Value>::blockOrder][key&omi_BlockedArray<Value>::blockModulus];
    }

    ////////////////////
    // Test membership
    inline bool IsMember(unsigned int key) const
    {
	return tableMask.GetBit(key);
    }

    // Return Size of Map
    int  Size() const { return tableMask.BitCount(); }

    // Is Map empty?
    bool IsEmpty() const { return tableMask.BitCount() == 0; }

    // Clear the Map
    void Clear();

    virtual size_t Footprint() const; /// return the amount of memory held, in bytes

    static inline void* operator new    (size_t sz) { return ALLOCATOR::allocate(sz); }
    static inline void* operator new[]  (size_t sz) { return ALLOCATOR::allocate(sz); }
    static inline void operator delete  (void* ptr) { ALLOCATOR::deallocate(ptr); }
    static inline void operator delete  (void* ptr, void*) { }
    static inline void operator delete[](void* ptr) { ALLOCATOR::deallocate(ptr); }
    static inline void operator delete[](void* ptr, void*) { }

private:

    omi_BlkBitVect tableMask;
};

#define OMI_TABLEBLOCK_FWDL( VALUE, NAME ) \
typedef omi_UintTableBlock<VALUE,SMABasOmiDefaultAllocator> NAME;

#ifdef SMA_NO_TGEN
#define OMI_TABLEBLOCK_EXTL( VALUE, NAME ) \
extern template class omi_UintTableBlock<VALUE,SMABasOmiDefaultAllocator>;
#else
#define OMI_TABLEBLOCK_EXTL( VALUE, NAME )
#endif

#define OMI_TABLEBLOCK_DECL( VALUE, NAME ) \
OMI_TABLEBLOCK_FWDL( VALUE, NAME ) \
OMI_TABLEBLOCK_EXTL( VALUE, NAME )

#define OMI_TABLEBLOCK_IMPL( VALUE, NAME ) \
template class omi_UintTableBlock<VALUE,SMABasOmiDefaultAllocator>; \
template void mem_deleteBuiltIn<omi_UintTableBlock<VALUE,SMABasOmiDefaultAllocator> >(omi_UintTableBlock<VALUE,SMABasOmiDefaultAllocator>**);


#endif
