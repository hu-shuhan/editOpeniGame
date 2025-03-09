//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xoList_h
#define cls_xoList_h

// Begin local includes

#include <cow_List.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class T>
class cls_xoList : public cow_List<T>
{
public:
  inline cls_xoList() : m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xoList(const T& item) : cow_List<T>(item), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xoList<T>& operator=(const cow_List<T>& rhs);

  cls_xoList(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

  inline void NewUid() { m_ClsUid.NewUid(); };

private:
  cls_Uid m_ClsUid;
};

template <class T>
inline bool operator==(const cls_xoList<T>& a, const cls_xoList<T>& b) { return a.IsEqual(b); };

template <class T>
inline bool operator!=(const cls_xoList<T>& a, const cls_xoList<T>& b) { return !(a.IsEqual(b)); };

template <class T>
cls_xoList<T> cls_xoListFromCow(const cow_List<T>& cow);

#define CLS_xoLIST_DECL(T,N)\
typedef cls_xoList<T> N;\
COW_LIST_DECL(T,N ## BB);

#define CLS_xoLIST_DECL_ADD(T,N)\
typedef cls_xoList<T> N;\
COW_LIST_DECL_ADD(T,N ## BB);

#define CLS_xoLIST_ITER_DECL(T,N)\
typedef cls_xoList<T> N;\
COW_LIST_ITER_DECL(T,N ## XX);\
typedef N ## XXIT N ## IT;

#define CLS_xoLIST_IMPL(T,N)\
template class cls_xoList<T>;\
template cls_xoList<T> cls_xoListFromCow(const cow_List<T>& cow);\
COW_LIST_IMPL(T,N ## BB);

#define CLS_xoLIST_IMPL_ADD(T,N)\
template class cls_xoList<T>;\
COW_LIST_IMPL_ADD(T,N);

#define CLS_xoLIST_ITER_IMPL(T,N)\
template class cls_xoList<T>;\
template cls_xoList<T> cls_xoListFromCow(const cow_List<T>& cow);\
COW_LIST_ITER_IMPL(T,N ## XX)

#define CLS_ATTR_xoList(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT)
#define CLS_ATTR_xoList_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT)

#endif
