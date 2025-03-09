//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- mode: c++ -*- //
#ifndef odb_DictionaryInt_h
#define odb_DictionaryInt_h

#if defined(HKS_NTI)
#pragma warning (disable: 4584)
#endif



// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Exceptions.h>
#include <cow_IntTable.h>
#include <atr_translate.h>




// Forward Declarations

template <class VALUE, class ALLOCATOR> class odb_DictionaryIntIterator;
template <class Value, class ALLOCATOR> class odb_DictionaryInt;




template <class VALUE, class ALLOCATOR>
class odb_DictionaryInt : public mem_AllocationOperators,
                          private cow_IntTable<VALUE,ALLOCATOR>
{
  public:

    VALUE& operator[](int id) 
    { 
        return get(id); 
    }

    int size() const 
    { 
      return cow_IntTable<VALUE,ALLOCATOR>::Size(); 
    }

    VALUE& get(int id) 
    { 
      if (cow_IntTable<VALUE,ALLOCATOR>::IsMember(id))
		return cow_IntTable<VALUE,ALLOCATOR>::Get(id);
      throw odb_Exception(odb_TEXT_MESSAGE, atr("odb_Dictionary key error"));
    }

    const VALUE& constGet(int id) const 
    { 
      if (cow_IntTable<VALUE,ALLOCATOR>::IsMember(id))
        return cow_IntTable<VALUE,ALLOCATOR>::ConstGet(id);
      throw odb_Exception(odb_TEXT_MESSAGE, atr("odb_Dictionary key error"));
    }

    bool isMember(int id) const { return cow_IntTable<VALUE,ALLOCATOR>::IsMember(id); }
    bool isEmpty() const { return cow_IntTable<VALUE,ALLOCATOR>::IsEmpty(); }

    void insert(int id, const VALUE& value) 
    { 
      if (!cow_IntTable<VALUE,ALLOCATOR>::Insert(id,value))
        throw odb_Exception(odb_TEXT_MESSAGE, atr("odb_Dictionary insert failure"));
    }

    void yank(int id) 
    { 
      if (!cow_IntTable<VALUE,ALLOCATOR>::Remove(id))
        throw odb_Exception(odb_TEXT_MESSAGE, atr("odb_Dictionary insert failure"));
    }
  
  
  private:
  
  friend class odb_DictionaryIntIterator<VALUE,ALLOCATOR>;
};

template <class VALUE, class ALLOCATOR>
class odb_DictionaryIntIterator : private cow_IntTableIterator<VALUE,ALLOCATOR>
{
public:
  odb_DictionaryIntIterator(const odb_DictionaryInt<VALUE,ALLOCATOR> &container)
	:cow_IntTableIterator<VALUE,ALLOCATOR>(container) 
  { }

  void first() { cow_IntTableIterator<VALUE,ALLOCATOR>::First(); }

  void next()  { cow_IntTableIterator<VALUE,ALLOCATOR>::Next(); }

  bool isDone() const { return cow_IntTableIterator<VALUE,ALLOCATOR>::IsDone();}

  const VALUE& currentValue() { return cow_IntTableIterator<VALUE,ALLOCATOR>::CurrentValue();}

  int          currentKey()   { return cow_IntTableIterator<VALUE,ALLOCATOR>::CurrentKey();} 

};

#define ODB_INTCONTAINER_IMPL(VALUE, NAME) \
COW_INTTABLE_IMPL(VALUE,NAME ## BT); \
template class odb_DictionaryInt<VALUE,SMABasOmiDefaultAllocator>;			\
template class odb_DictionaryIntIterator<VALUE,SMABasOmiDefaultAllocator>;

#define ODB_INTCONTAINER_DECL( VALUE, NAME) \
COW_INTTABLE_DECL(VALUE,NAME ## BT); \
typedef odb_DictionaryInt<VALUE,SMABasOmiDefaultAllocator> NAME; \
typedef odb_DictionaryIntIterator<VALUE,SMABasOmiDefaultAllocator> NAME  ## IT;

#define ODB_NEWINTCONTAINER_IMPL( VALUE, NAME) \
COW_INTTABLE_IMPL(VALUE,NAME ## BT); \
template class odb_DictionaryInt<VALUE,SMABasOmiDefaultAllocator>; \
template class odb_DictionaryIntIterator<VALUE,SMABasOmiDefaultAllocator>;

#define ODB_NEWINTCONTAINER_DECL( VALUE, NAME) \
COW_INTTABLE_DECL(VALUE,NAME ## BT); \
typedef odb_DictionaryInt<VALUE,SMABasOmiDefaultAllocator> NAME ## IMPL; \
typedef odb_DictionaryIntIterator<VALUE,SMABasOmiDefaultAllocator> NAME  ## IT; 

#endif // odb_DictionaryInt_h


