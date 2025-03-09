//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef cow_IntTable_h
#define cow_IntTable_h

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
#include <omi_IntTable.h>

template <class ITEM,class ALLOCATOR> class cow_IntTableIterator;


template <class ITEM, class ALLOCATOR>
class cow_IntTable : public mem_AllocationOperators
{
  friend class cow_IntTableIterator<ITEM,ALLOCATOR>;

public:
  inline cow_IntTable(): tbl(new omi_IntTable<ITEM,ALLOCATOR>) {};

  inline bool  Remove(const int& key) { return tbl.Get().Remove(key); };
  inline bool  Insert(const int& key, const ITEM& value) { return tbl.Get().Insert(key, value); };
  inline const ITEM& ConstGet(const int& key) const { return tbl.ConstGet().ConstGet(key); };
  inline       ITEM& Get(const int& key) { return tbl.Get().Get(key); };
  inline bool  IsMember(const int& key) const { return tbl.ConstGet().IsMember(key); };
  inline int   Size() const { return tbl.ConstGet().Size(); };
  inline bool  IsEmpty() const { return tbl.ConstGet().IsEmpty(); };
  inline void  Clear() { tbl.Get().Clear(); };

private:
  cow_COW<omi_IntTable<ITEM,ALLOCATOR>, cow_Direct<omi_IntTable<ITEM,ALLOCATOR> > > tbl;
};


template <class ITEM,class ALLOCATOR>
class cow_IntTableIterator : public mem_AllocationOperators
{
public:
  inline cow_IntTableIterator(const cow_IntTable<ITEM,ALLOCATOR>& t)
	: iter(t.tbl.ConstGet()) 
  {};

  inline void First() { iter.First(); };
  inline void Last() { iter.Last(); };
  inline void Next() { iter.Next(); };
  inline bool IsDone() const { return iter.IsDone(); };
  inline const ITEM&  CurrentValue() const { return iter.CurrentValue(); };
  inline int          CurrentKey()   const { return iter.CurrentKey(); };

private:
  omi_IntTableIterator<ITEM,ALLOCATOR> iter;
};

#define COW_INTTABLE_DECL(ITEM,NAME) \
  typedef cow_IntTable<ITEM,SMABasOmiDefaultAllocator> NAME; \
  OMI_INTTABLE_DECL(ITEM, NAME ## OMI);			\
  COW_COW_DECL(NAME ## OMI);

#define COW_INTTABLE_ITER_DECL(ITEM,NAME)       \
  COW_INTTABLE_DECL(ITEM,NAME);					\
  typedef cow_IntTableIterator<ITEM,SMABasOmiDefaultAllocator> NAME ## IT;

#define COW_INTTABLE_SORTED_ITER_DECL(ITEM,NAME) \
  COW_INTTABLE_ITER_DECL(ITEM,NAME);			 \
  typedef NAME ## IT NAME ## SIT;

#define COW_INTTABLE_IMPL(ITEM,NAME) \
  OMI_INTTABLE_IMPL(ITEM, NAME ## OMI);			\
  COW_COW_IMPL(NAME ## OMI);					\
  template class cow_IntTable<ITEM,SMABasOmiDefaultAllocator>; 


#define COW_INTTABLE_ITER_IMPL(ITEM,NAME) \
  COW_INTTABLE_IMPL(ITEM,NAME);					\
  template class cow_IntTableIterator<ITEM,SMABasOmiDefaultAllocator>;

#define COW_INTTABLE_SORTED_ITER_IMPL(ITEM,NAME) COW_INTTABLE_ITER_IMPL(ITEM,NAME)

#endif  // cow_IntTable_h
