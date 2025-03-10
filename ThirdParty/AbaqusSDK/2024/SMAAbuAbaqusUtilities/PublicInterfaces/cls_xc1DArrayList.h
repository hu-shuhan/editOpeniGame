//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xc1DArrayList_h
#define cls_xc1DArrayList_h

// Begin local includes

#include <typ_typeTag.h>

#include <cow_List.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class CTYPE, class TYPE>
class cls_xc1DArrayList : public cow_List<CTYPE>
{
public:
  inline cls_xc1DArrayList()
    : cow_List<CTYPE>(), m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xc1DArrayList(unsigned int sz)
    : cow_List<CTYPE>(cow_EstimateLength, sz), m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xc1DArrayList(const CTYPE* C_arr,
                           int C_arrLen,
                           int maxSize = 0)
    : cow_List<CTYPE>(C_arr, C_arrLen, maxSize), m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xc1DArrayList(const cow_List<CTYPE>& src)
    : cow_List<CTYPE>(src), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xc1DArrayList(const cls_ReadVisitor& rv);

  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

#define CLS_xc1DARRAYLIST_DECL(C,T,N)\
typedef cls_xc1DArrayList<C,T> N;\
COW_LIST_DECL(C,N ## BB)

#define CLS_xc1DARRAYLIST_DECL_ADD(C,T,N)\
typedef cls_xc1DArrayList<C,T> N;

#define CLS_xc1DARRAYLIST_IMPL(C,T,N)\
template class cls_xc1DArrayList<C,T>;\
COW_LIST_IMPL(C,N ## BB)

#define CLS_xc1DARRAYLIST_IMPL_ADD(C,T,N)\
template class cls_xc1DArrayList<C,T>;

#define CLS_ATTR_xc1DArrayList(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_List,cls_ColListObjT) 
#define CLS_ATTR_xc1DArrayList_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_List,cls_ColListObjT)

#endif
