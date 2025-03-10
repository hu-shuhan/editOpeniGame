//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_Map2Obj_h
#define cls_Map2Obj_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//
//
// Modification history:
//      Rob deFriesse 8/99 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes
#include <mem_AllocationOperators.h>

#include <typ_typeTag.h>

#include <cow_COW.h>
#include <cow_Btree.h>

#include <cls_PC.h>

#include <cls_Uid.h>
#include <cls_AgentMapKey2Obj.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class K, class V, class C>
class cls_Map2ObjIter;

template <class K, class V, class C>
class cls_Map2Obj : public cls_PC
{
public:
  cls_Map2Obj();
  cls_Map2Obj(const cls_Map2Obj<K,V,C>& source);
  cls_Map2Obj (const cls_ReadVisitor& rv);
  virtual ~cls_Map2Obj();

  cls_Map2Obj<K,V,C>& operator= (const cls_Map2Obj<K,V,C>& source);

  virtual void DBWrite (const cls_WriteVisitor& rv) const;

  virtual typ_typeTag DynTypeId() const;
  static  typ_typeTag TypeId();
  static  void* Ctor (cls_ReadVisitor& rv);

  virtual void Flush() const;
  virtual void Detach();

  // Do-nothing unless file is read-only.
  int Snap(void) const;

  // Dirty members are not released.
  void Release(const K& key) const;
  void Release(void) const;

  // New members are dirty.  Duplicate replaces previous.
  bool Insert(const K& key, const V& val);
  bool Insert(const K& key, V* val);

  // Member does _not_ become dirty.
  const V& ConstGet(const K& key) const;

  // Member becomes dirty.
  V& Get(const K& key);

  bool Remove(const K& key);

  bool IsMember(const K& key) const;

  int Size() const;

  bool IsEmpty() const;

private:
  friend class cls_Map2ObjIter<K,V,C>;

  typedef cls_Map2Obj<K,V,C> MyClass;

  bool DoInsert(const K& key, const V& val);
  bool DoInsert(const K& key, V* val);

  cow_Btree<K,C>  m_Map;
  cls_AgentMapKey2Obj<K>* m_Agent;
  int m_Dirty;
  cls_Uid m_ClsUid;

  // If the COW does not have a value, the LoadCow method should be called 
  // to load the object and replace that object in the map
  const C& LoadCOW(const K&) const;

};

template <class K, class V, class C>
class cls_Map2ObjIter : public mem_AllocationOperators
{
public:
  cls_Map2ObjIter(const cls_Map2Obj<K,V,C>& map);

  inline void First() { m_Iter.First(); };
  inline void Next() { m_Iter.Next(); };
  inline bool IsDone() const { return m_Iter.IsDone(); };

  inline const K& CurrentKey() const { return m_Iter.CurrentKey(); };
  const V& CurrentValue() const ;

  // Copy constructors is provided so that 
  // Factory methods can create iterators (i.e. cls_Map2Obj<> CreateIterator())
  // However they do not copy the current state of the iterator, they 
  // reset the state. After copying you must call First on the iterator
  cls_Map2ObjIter(const cls_Map2ObjIter<K,V,C>&);

private:
  const cls_Map2Obj<K,V,C>& m_Map;
  cow_BtreeIterator<K,C> m_Iter;

  // Disabled.
  cls_Map2ObjIter();
  cls_Map2ObjIter<K,V,C>& operator= (const cls_Map2ObjIter<K,V,C>&);
};

#define CLS_MAP2OBJ_FWDL(KEY,VALUE,NAME) \
COW_BTREE_FWDL(KEY,VALUE ## COW,NAME ## MAP) \
typedef cls_Map2Obj<KEY,VALUE,VALUE ## COW> NAME; \
COW_COW2_FWDL(NAME ## MAP, cow_Virtual)

#ifdef SMA_NO_TGEN
#define CLS_MAP2OBJ_EXTL(KEY,VALUE,NAME) \
COW_BTREE_CMP_EXTL(KEY,VALUE ## COW,NAME ## MAP) \
extern template class cls_Map2Obj<KEY,VALUE,VALUE ## COW>; \
COW_COW2_EXTL(NAME ## MAP, cow_Virtual)
#else
#define CLS_MAP2OBJ_EXTL(KEY,VALUE,NAME) \
COW_BTREE_CMP_EXTL(KEY,VALUE ## COW,NAME ## MAP) \
COW_COW2_EXTL(NAME ## MAP, cow_Virtual)
#endif

#define CLS_MAP2OBJ_DECL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_FWDL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_EXTL(KEY,VALUE,NAME)

#define CLS_MAP2OBJ_IMPL(KEY,VALUE,NAME) \
COW_BTREE_CMP_IMPL(KEY,VALUE ## COW,NAME ## MAP) \
template class cls_Map2Obj<KEY,VALUE,VALUE ## COW>; \
COW_COW2_IMPL(NAME ## MAP, cow_Virtual)

#define CLS_MAP2OBJ_ITER_FWDL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_FWDL(KEY,VALUE,NAME) \
typedef cls_Map2ObjIter<KEY,VALUE,VALUE ## COW> NAME ## IT;

#ifdef SMA_NO_TGEN
#define CLS_MAP2OBJ_ITER_EXTL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_EXTL(KEY,VALUE,NAME) \
extern template class cls_Map2ObjIter<KEY,VALUE,VALUE ## COW>;
#else
#define CLS_MAP2OBJ_ITER_EXTL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_EXTL(KEY,VALUE,NAME)
#endif

#define CLS_MAP2OBJ_ITER_DECL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_ITER_FWDL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_ITER_EXTL(KEY,VALUE,NAME)

#define CLS_MAP2OBJ_ITER_IMPL(KEY,VALUE,NAME) \
CLS_MAP2OBJ_IMPL(KEY,VALUE,NAME) \
template class cls_Map2ObjIter<KEY,VALUE,VALUE ## COW>;


#define CLS_ATTR_MAPSTRING2OBJ(MEMBER,CLASS) CLS_ATTR_COLOBJ(MEMBER,CLASS,cls_ColMapString2ObjT)
#define CLS_ATTR_MAPSTRING2OBJ_I(MEMBER,CLASS) CLS_ATTR_COLOBJ_I(MEMBER,CLASS,cls_ColMapString2ObjT)

#define CLS_ATTR_MAPINT2OBJ(MEMBER,CLASS) CLS_ATTR_COLOBJ(MEMBER,CLASS,cls_ColMapInt2ObjT)
#define CLS_ATTR_MAPINT2OBJ_I(MEMBER,CLASS) CLS_ATTR_COLOBJ_I(MEMBER,CLASS,cls_ColMapInt2ObjT)


#endif
