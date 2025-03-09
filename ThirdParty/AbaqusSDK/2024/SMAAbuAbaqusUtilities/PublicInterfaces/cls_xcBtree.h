//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef cls_xcBtree_h
#define cls_xcBtree_h

// Begin local includes

#include <cow_String.h>
#include <cow_Btree.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>
#include <cls_FieldHandle.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

#undef TYPENAME
#if defined(_WINDOWS) || defined(SMA_GNUC)
#define TYPENAME
#else
#define TYPENAME typename
#endif

template <class KEY, class CTYPE, class TYPE>
class cls_xcBtree : public cow_Btree<KEY,CTYPE>
{
public:
  inline cls_xcBtree() : m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xcBtree<KEY,CTYPE,TYPE>& operator=(const cow_Btree<KEY,CTYPE>& rhs);

  cls_xcBtree(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

template <class KEY, class CTYPE, class TYPE>
cls_xcBtree<KEY,CTYPE,TYPE> cls_xcBtreeFromCow(const cow_Btree<KEY,CTYPE>& cow, TYPE*);

#define CLS_xcBTREE_FWDL(K,C,T,N) \
typedef cls_xcBtree<K,C,T> N; \
COW_BTREE_FWDL(K,C,N ## COWBTREE)

#ifdef SMA_NO_TGEN
#define CLS_xcBTREE_EXTL(K,C,T,N) \
COW_BTREE_CMP_EXTL(K,C,N ## COWBTREE) \
extern template class cls_xcBtree<K,C,T>; \
extern template cls_xcBtree<K,C,T> cls_xcBtreeFromCow(const cow_Btree<K,C>& cow, T*);
#else
#define CLS_xcBTREE_EXTL(K,C,T,N) \
COW_BTREE_CMP_EXTL(K,C,N ## COWBTREE)
#endif

#define CLS_xcBTREE_DECL(K,C,T,N) \
CLS_xcBTREE_FWDL(K,C,T,N) \
CLS_xcBTREE_EXTL(K,C,T,N)

#define CLS_xcBTREE_FWDL_ADD(K,C,T,N) \
typedef cls_xcBtree<K,C,T> N;

#ifdef SMA_NO_TGEN
#define CLS_xcBTREE_EXTL_ADD(K,C,T,N) \
extern template class cls_xcBtree<K,C,T>;
#else
#define CLS_xcBTREE_EXTL_ADD(K,C,T,N)
#endif

#define CLS_xcBTREE_DECL_ADD(K,C,T,N) \
CLS_xcBTREE_FWDL_ADD(K,C,T,N) \
CLS_xcBTREE_EXTL_ADD(K,C,T,N)

#define CLS_xcBTREE_IMPL(K,C,T,N) \
COW_BTREE_CMP_IMPL(K,C,N ## COWBTREE) \
template class cls_xcBtree<K,C,T>; \
template cls_xcBtree<K,C,T> cls_xcBtreeFromCow(const cow_Btree<K,C>& cow, T*);


#define CLS_xcBTREE_IMPL_ADD(K,C,T,N) \
template class cls_xcBtree<K,C,T>;

#define CLS_ATTR_xcBtree(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT) 
#define CLS_ATTR_xcBtree_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT)

#define CLS_ATTR_xcBtreeInt2Obj(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapInt2ObjT) 
#define CLS_ATTR_xcBtreeInt2Obj_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapInt2ObjT)

#endif
