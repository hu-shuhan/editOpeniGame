//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mdl_MapInt2ObjCow_h
#define mdl_MapInt2ObjCow_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <mdl_Extension.h>
#include <mdl_Repository.h>
#include <omi_types.h>
#include <cow_IntTable.h>
#include <cow_ArrayCOW.h>
#include <cow_CountedHld.h>
#include <cow_Copy.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>

// End local includes


class cls_ReadVisitor;
class cls_WriteVisitor;

template <class ITEM> class mdl_MapInt2ObjCowIT;

template <class ITEM>
class mdl_MapInt2ObjCow : public mdl_Repository
{
public:
   mdl_MapInt2ObjCow();
  ~mdl_MapInt2ObjCow();    

  // Interface

  bool Remove(int);
  bool Insert(int, const ITEM&);
  bool Insert(int, ITEM*); // adopt insert method
  bool IsMember(int) const;
  bool IsEmpty() const;
  void Clear();
  int Size() const;

  cow_IntTable<cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, SMABasOmiDefaultAllocator>& GetMap();
  const cow_IntTable<cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, SMABasOmiDefaultAllocator >& ConstGetMap() const { return map;}

  ITEM& Get(int);
  const ITEM& ConstGet(int) const;
    
  virtual mdl_Extension* Copy() const;

  // Timestamp
  void UpdateTimeStamp();
  double GetTimeStamp() const { return _timestamp; }

  // Messaging/database interface
  virtual typ_typeTag     DynTypeId() const;
  static typ_typeTag      TypeId();


  mdl_MapInt2ObjCow(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;
  void NewUid();

  friend class mdl_MapInt2ObjCowIT<ITEM>;

private:
  // This method can be moved to the public interface if needed.
  bool Insert(int key,
              const cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >& valCow);

  cow_IntTable<cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, SMABasOmiDefaultAllocator > map;
  cls_Uid m_ClsUid;

  double _timestamp; // container timestamp;
};

// Iterator class to hide the cows

template <class ITEM>
class mdl_MapInt2ObjCowIT : public mem_AllocationOperators
{
public:

  mdl_MapInt2ObjCowIT(const mdl_MapInt2ObjCow<ITEM>&);

  void First() { iter.First(); }
  void Next() { iter.Next(); }
  bool IsDone() const { return iter.IsDone(); }

  int CurrentKey() const { return iter.CurrentKey(); }
  const ITEM& CurrentValue() const { return iter.CurrentValue().ConstGet(); }

private:

  cow_IntTableIterator<cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >,SMABasOmiDefaultAllocator > iter;
};



#define MDL_MAP_INT_2_OBJ_COW_DECL(ITEM,NAME) \
COW_INTTABLE_ITER_DECL(ITEM ## COW,NAME ## MAP);\
typedef mdl_MapInt2ObjCow<ITEM> NAME;\
typedef mdl_MapInt2ObjCowIT<ITEM> NAME ## IT;

#define MDL_MAP_INT_2_OBJ_COW_IMPL(ITEM,NAME) \
COW_INTTABLE_ITER_IMPL(ITEM ## COW,NAME ## MAP);\
template <> typ_typeTag \
mdl_MapInt2ObjCow<ITEM>::TypeId() { return TYP_TIX_ ## NAME; }\
template class mdl_MapInt2ObjCowIT<ITEM>;\
template class mdl_MapInt2ObjCow<ITEM>;




#define MDL_ATTR_MapInt2ObjCow(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapInt2ObjT) 
#define MDL_ATTR_MapInt2ObjCow_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapInt2ObjT)

#endif
