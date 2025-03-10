#ifndef mdl_MapString2Obj_h
#define mdl_MapString2Obj_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ListString.h>
#include <cow_ArrayCOW.h>
#include <cow_Map.h>
#include <cow_List.h>
#include <cow_MapUtils.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>

// End local includes

#define COW_ARRAYCOWV_FWDL(ITEM,NAME) \
COW_ARRAYCOW2_FWDL(ITEM, cow_Virtual) \
typedef ITEM ## COW NAME;

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class ITEM>
class mdl_MapString2ObjIT;

template <class ITEM>
class mdl_MapString2Obj : public mem_AllocationOperators
{
public:
  mdl_MapString2Obj();
  ~mdl_MapString2Obj();

  // Interface
  bool Remove(const cow_String&);
  bool Insert(const cow_String&, const ITEM&);
  bool Insert(const cow_String&, ITEM*);
  bool IsMember(const cow_String&) const;
  bool IsEmpty() const;
  void Clear();
  int Size() const;

  ITEM& Get(const cow_String &);
  const ITEM& ConstGet(const cow_String&) const;

  cow_ListString Keys() const;

  COW_ARRAYCOWV_FWDL(ITEM,ItemCOW);
  COW_MAP_FWDL(cow_String,ItemCOW,ItemMAP);
  ItemMAP& GetMap();
  const ItemMAP& ConstGetMap() const;

  mdl_MapString2Obj(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

  friend class mdl_MapString2ObjIT<ITEM>;

private:
  ItemMAP map;
  cls_Uid m_ClsUid;
  mutable bool recalcKeys;
  mutable cow_List<cow_String> keys;
};

template <class ITEM>
class mdl_MapString2ObjIT : public mem_AllocationOperators
{
public:
     mdl_MapString2ObjIT(const mdl_MapString2Obj<ITEM>&);
     void First() { iter.First(); }
     void Next() { iter.Next(); }
     bool IsDone() const { return iter.IsDone(); }

     const cow_String& CurrentKey() const { return iter.CurrentItem(); }
     const ITEM& CurrentValue() const;
private:
     const mdl_MapString2Obj<ITEM> m;
     cow_ListIter<cow_String> iter;
};

// Macros for instantiation
#define MDL_MAP_STRING_2_OBJ_FWDL(ITEM,NAME) \
COW_LIST_ITER_FWDL(ITEM ## COW, NAME ## COWLIST) \
COW_MAP_ITER_FWDL(cow_String,ITEM ## COW,NAME ## MAP) \
COW_MAPUTILS_FWDL(cow_String,ITEM ## COW) \
typedef mdl_MapString2Obj<ITEM> NAME; \
typedef mdl_MapString2ObjIT<ITEM> NAME ## IT;

#ifdef SMA_NO_TGEN
#define MDL_MAP_STRING_2_OBJ_EXTL(ITEM,NAME) \
COW_LIST_ITER_EXTL(ITEM ## COW,NAME ## COWLIST) \
COW_MAP_ITER_EXTL(cow_String,ITEM ## COW,NAME ## MAP) \
COW_MAPUTILS_EXTL(cow_String,ITEM ## COW) \
template <> void mdl_MapString2Obj<ITEM>::DBWrite(const cls_WriteVisitor& wv) const; \
extern template class mdl_MapString2Obj<ITEM>; \
extern template class mdl_MapString2ObjIT<ITEM>;
#else
#define MDL_MAP_STRING_2_OBJ_EXTL(ITEM,NAME) \
COW_LIST_ITER_EXTL(ITEM ## COW,NAME ## COWLIST) \
COW_MAP_ITER_EXTL(cow_String,ITEM ## COW,NAME ## MAP) \
COW_MAPUTILS_EXTL(cow_String,ITEM ## COW)
#endif

#define MDL_MAP_STRING_2_OBJ_DECL(ITEM,NAME) \
MDL_MAP_STRING_2_OBJ_FWDL(ITEM,NAME) \
MDL_MAP_STRING_2_OBJ_EXTL(ITEM,NAME)

#ifdef SMA_NO_TGEN
#define MDL_MAP_STRING_2_OBJ_IMPL(ITEM,NAME) \
COW_LIST_ITER_IMPL(ITEM ## COW,NAME ## COWLIST) \
COW_MAP_ITER_IMPL(cow_String,ITEM ## COW,NAME ## MAP) \
COW_MAPUTILS_IMPL(cow_String,ITEM ## COW) \
template <> \
void mdl_MapString2Obj<ITEM>::DBWrite(const cls_WriteVisitor& wv) const \
{ \
    wv.StartWrite(TYP_TIX_cls_Map, m_ClsUid); \
    cls_WriteMapString2Obj* writer = wv.MakeWriteMapString2Obj(); \
                                                                    \
    mdl_MapString2ObjIT<ITEM> iter(*this); \
    for(iter.First(); !iter.IsDone(); iter.Next()) \
        iter.CurrentValue().DBWrite(cls_WriteVisitor(cls_WriteRequestMapString2Obj(writer, iter.CurrentKey()))); \
                                                                    \
    if (writer->NeedsDeletion()) \
        delete writer; \
} \
template class mdl_MapString2Obj<ITEM>; \
template class mdl_MapString2ObjIT<ITEM>;
#else
#define MDL_MAP_STRING_2_OBJ_IMPL(ITEM,NAME) \
COW_LIST_ITER_IMPL(ITEM ## COW,NAME ## COWLIST) \
COW_MAP_ITER_IMPL(cow_String,ITEM ## COW,NAME ## MAP) \
COW_MAPUTILS_IMPL(cow_String,ITEM ## COW) \
template class mdl_MapString2Obj<ITEM>; \
template class mdl_MapString2ObjIT<ITEM>; \
template <> \
void mdl_MapString2Obj<ITEM>::DBWrite(const cls_WriteVisitor& wv) const \
{ \
    wv.StartWrite(TYP_TIX_cls_Map, m_ClsUid); \
    cls_WriteMapString2Obj* writer = wv.MakeWriteMapString2Obj(); \
                                                                    \
    mdl_MapString2ObjIT<ITEM> iter(*this); \
    for(iter.First(); !iter.IsDone(); iter.Next()) \
        iter.CurrentValue().DBWrite(cls_WriteVisitor(cls_WriteRequestMapString2Obj(writer, iter.CurrentKey()))); \
                                                                    \
    if (writer->NeedsDeletion()) \
        delete writer; \
}
#endif


#define MDL_ATTR_MapString2Obj(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT) 
#define MDL_ATTR_MapString2Obj_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT)

#endif
