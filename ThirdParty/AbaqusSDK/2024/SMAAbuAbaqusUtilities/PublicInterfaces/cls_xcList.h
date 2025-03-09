//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cls_xcList_h
#define cls_xcList_h

// Begin local includes

#include <cow_List.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class CTYPE, class TYPE>
class cls_xcList : public cow_List<CTYPE>
{
public:
  inline cls_xcList() : m_ClsUid(cls_Uid::AllocateUid()) {}
  inline cls_xcList(const CTYPE& item) : cow_List<CTYPE>(item), m_ClsUid(cls_Uid::AllocateUid()) {}
  inline cls_xcList(unsigned int sz)
    : cow_List<CTYPE>(cow_EstimateLength, sz) {};

  cls_xcList<CTYPE,TYPE>& operator=(const cow_List<CTYPE>& rhs);

  cls_xcList(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

  inline void NewUid() { m_ClsUid.NewUid(); }

private:
  cls_Uid m_ClsUid;
};

template <class CTYPE, class TYPE>
inline bool operator==(const cls_xcList<CTYPE,TYPE>& a, const cls_xcList<CTYPE,TYPE>& b) { return a.IsEqual(b); };

template <class CTYPE, class TYPE>
inline bool operator!=(const cls_xcList<CTYPE,TYPE>& a, const cls_xcList<CTYPE,TYPE>& b) { return !(a.IsEqual(b)); };

template <class CTYPE, class TYPE>
cls_xcList<CTYPE,TYPE> cls_xcListFromCow(const cow_List<CTYPE>& cow, TYPE*);

#define CLS_xcLIST_DECL(C,T,N)\
typedef cls_xcList<C,T> N;\
COW_LIST_DECL(C,N ## BB);

#define CLS_xcLIST_DECL_ADD(C,T,N)\
typedef cls_xcList<C,T> N;\
COW_LIST_DECL_ADD(C,N ## BB);

#define CLS_xcLIST_ITER_DECL(C,T,N)\
typedef cls_xcList<C,T> N;\
COW_LIST_ITER_DECL(C,N ## XX);\
typedef N ## XXIT N ## IT;

#define CLS_xcLIST_IMPL(C,T,N)\
template class cls_xcList<C,T>;\
template cls_xcList<C,T> cls_xcListFromCow(const cow_List<C>& cow, T*);\
COW_LIST_IMPL(C,N ## BB);

#define CLS_xcLIST_IMPL_ADD(C,T,N)\
template class cls_xcList<C,T>;\
COW_LIST_IMPL_ADD(C,N);

#define CLS_xcLIST_ITER_IMPL(C,T,N)\
template class cls_xcList<C,T>;\
template cls_xcList<C,T> cls_xcListFromCow(const cow_List<C>& cow, T*);\
COW_LIST_ITER_IMPL(C,N ## XX)

#define CLS_ATTR_xcList(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT)
#define CLS_ATTR_xcList_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT)

#endif
