//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xo1DArrayList_h
#define cls_xo1DArrayList_h

// Begin local includes

#include <cow_List.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class TYPE>
class cls_xo1DArrayList : public cow_List<TYPE>
{
public:
  inline cls_xo1DArrayList()
    : m_ExternalUid(NULL) {};
  inline cls_xo1DArrayList(unsigned int sz)
    : cow_List<TYPE>(cow_EstimateLength, sz), m_ExternalUid(NULL) {};
  inline cls_xo1DArrayList(const TYPE* C_arr,
                           int C_arrLen,
                           int maxSize = 0)
    : cow_List<TYPE>(C_arr, C_arrLen, maxSize), m_ExternalUid(NULL) {};
  inline cls_xo1DArrayList(const cow_List<TYPE>& src)
    : cow_List<TYPE>(src), m_ExternalUid(NULL) {};

  // These constructors should be used when writing temporary objects, that have
  // their UIDs stored in the owner class.
  cls_xo1DArrayList( const cls_Uid &uid )
    : m_ClsUid(uid), m_ExternalUid(const_cast<cls_Uid*>(&uid)) {};
  cls_xo1DArrayList( const cls_Uid &uid, unsigned int sz )
    : cow_List<TYPE>(cow_EstimateLength, sz), m_ClsUid(uid), m_ExternalUid(const_cast<cls_Uid*>(&uid)) {};
  cls_xo1DArrayList( const cls_Uid &uid,
                     const TYPE* C_arr,
                     int C_arrLen,
                     int maxSize = 0 )
    : cow_List<TYPE>(C_arr, C_arrLen, maxSize), m_ClsUid(uid), m_ExternalUid(const_cast<cls_Uid*>(&uid)) {};
  cls_xo1DArrayList( const cls_Uid &uid, const cow_List<TYPE>& src )
    : cow_List<TYPE>(src), m_ClsUid(uid), m_ExternalUid(const_cast<cls_Uid*>(&uid)) {};

  ~cls_xo1DArrayList() {
    if (m_ExternalUid) *m_ExternalUid = m_ClsUid; }

  cls_xo1DArrayList(const cls_ReadVisitor& rv, cls_Uid *externalUid = NULL);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
  cls_Uid *m_ExternalUid;
};

#define CLS_xo1DARRAYLIST_DECL(T,N)\
typedef cls_xo1DArrayList<T> N;\
COW_LIST_DECL(T,N ## BB);

#define CLS_xo1DARRAYLIST_DECL_ADD(T,N)\
typedef cls_xo1DArrayList<T> N;

#define CLS_xo1DARRAYLIST_IMPL(T,N)\
template class cls_xo1DArrayList<T>;\
COW_LIST_IMPL(T,N ## BB);

#define CLS_xo1DARRAYLIST_IMPL_ADD(T,N)\
template class cls_xo1DArrayList<T>;

#define CLS_ATTR_xo1DArrayList(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_List,cls_ColListObjT) 
#define CLS_ATTR_xo1DArrayList_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_List,cls_ColListObjT)

#endif
