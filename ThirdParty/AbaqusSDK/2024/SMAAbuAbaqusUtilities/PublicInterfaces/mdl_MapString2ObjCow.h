//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mdl_MapString2ObjCow_h
#define mdl_MapString2ObjCow_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <mdl_Extension.h>
#include <mdl_Repository.h>
#include <omi_types.h>
#include <cow_String.h>
#include <cls_xcBtree.h>
#include <cow_List.h>
#include <cow_ArrayCOW.h>
#include <cow_CountedHld.h>
#include <cow_Copy.h>
#include <cow_BtreeUtils.h>
#include <cow_ListString.h>
#include <cow_BtreeStringUint.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class T> class mdl_MapString2ObjCowIT;
template <class T> class cls_AbsTypeAdapter;
template <class T> class cls_TypeAdapter;

template <class ITEM>
class mdl_MapString2ObjCow : public mdl_Repository
{
public:
  mdl_MapString2ObjCow();
  mdl_MapString2ObjCow( const cow_String &_path);
  ~mdl_MapString2ObjCow();

  // Interface

  bool Remove(const cow_String&);
  bool Insert(const cow_String&, const ITEM&);
  bool Insert(const cow_String&, ITEM*); // adopt insert method
  bool IsMember(const cow_String&) const;
  bool IsEmpty() const;
  void Clear();
  int Size() const;

  cls_xcBtree<cow_String, cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, ITEM>& GetMap();
  const cls_xcBtree<cow_String, cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, ITEM>& ConstGetMap() const;

  ITEM& Get(const cow_String &);
  const ITEM& ConstGet(const cow_String&) const;

  cow_List<cow_String> Keys() const;

  virtual mdl_Extension* Copy() const;
  virtual mdl_Extension* DeepCopy() const;

  // Timestamp
  void UpdateTimeStamp( const cow_String& aa );
  double GetTimeStamp() const { return _timestamp; }
  void ResetDeltaList() { deltaTree.Clear(); }
  cow_ListString DeltaList() ;

  // Messaging/database interface
  virtual typ_typeTag     DynTypeId() const;
  static typ_typeTag      TypeId();

  // Used to read/write as a plain Map collection
  mdl_MapString2ObjCow(const cls_ReadVisitor& rv, bool wrapped );
  void DBWrite(const cls_WriteVisitor& wv, bool wrapped) const;
  void NewUid();

  // Regular persistence
public:
  static void InitializeMetadata(cls_ClassRegistrar& reg);

protected:
  // MapString2ObjCow should only be loaded as an object through the base class.
  mdl_MapString2ObjCow(const cls_ReadVisitor& rv );
  void DBWrite(const cls_WriteVisitor& wv) const;
  friend class cls_AbsTypeAdapter<mdl_MapString2ObjCow<ITEM> >; // for DBWrite
  friend class cls_TypeAdapter<mdl_MapString2ObjCow<ITEM> >;    // for constructor

private:

  static const char *DBClassName();

  // This method can be moved to the public interface if needed.
  bool Insert(const cow_String& key,
              const cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >& valCow);

  cls_xcBtree<cow_String, cow_ArrayCOW<ITEM, cow_Virtual<ITEM> >, ITEM> map;
  cls_Uid m_ClsUid;
  mutable bool recalcKeys;
  mutable cow_List<cow_String> keys;

  double _timestamp; // container timestamp;
  cow_BtreeStringUint deltaTree; // List of stale objects
  cow_String clientPath;
};

// Iterator class to hide the cows

template <class ITEM>
class mdl_MapString2ObjCowIT : public mem_AllocationOperators
{
public:

  mdl_MapString2ObjCowIT(const mdl_MapString2ObjCow<ITEM>&m)
      : iter(m.ConstGetMap()) {}

  void First() { iter.First(); }
  void Next() { iter.Next(); }
  bool IsDone() const { return iter.IsDone(); }

  const cow_String& CurrentKey() const { return iter.CurrentKey(); }
  const ITEM& CurrentValue() const {return iter.CurrentValue().ConstGet();}

private:

  cow_BtreeIterator<cow_String, cow_ArrayCOW<ITEM, cow_Virtual<ITEM> > >   iter;
};


#define MDL_MAP_STRING_2_OBJ_COW_DECL(ITEM,NAME)                \
    COW_LIST_ITER_DECL(ITEM ## COW,NAME ## COWLIST);            \
    CLS_xcBTREE_DECL(cow_String,ITEM ## COW,ITEM,NAME ## MAP);  \
    COW_BTREEUTILS_DECL(cow_String,ITEM ## COW);                        \
    typedef mdl_MapString2ObjCow<ITEM> NAME;                    \
    typedef mdl_MapString2ObjCowIT<ITEM> NAME ## IT;

#define MDL_MAP_STRING_2_OBJ_COW_IMPL(ITEM,NAME)                        \
    template <> typ_typeTag                                             \
    mdl_MapString2ObjCow<ITEM>::TypeId() { return TYP_TIX_ ## NAME; }   \
    template <> const char *                                            \
    mdl_MapString2ObjCow<ITEM>::DBClassName() { return #NAME; }         \
    COW_LIST_ITER_IMPL(ITEM ## COW,NAME ## COWLIST);                    \
    CLS_xcBTREE_IMPL(cow_String,ITEM ## COW,ITEM,NAME ## MAP);          \
    COW_BTREEUTILS_IMPL(cow_String,ITEM ## COW);                        \
    CSL_TYPEADAPTER_SPECIALIZE(NAME);                                   \
    template class mdl_MapString2ObjCow<ITEM>;                          \
    template class mdl_MapString2ObjCowIT<ITEM>;                        \
    template class cls_AbsTypeAdapter<NAME>;                            \
    template class cls_TypeAdapter<NAME>

#define MDL_ATTR_MapString2ObjCow(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT)
#define MDL_ATTR_MapString2ObjCow_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT)

#endif
