//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xcMap_h
#define cls_xcMap_h

// Begin local includes

#include <cow_String.h>
#include <cow_Map.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>
#include <cls_FieldHandle.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class KEY, class CTYPE, class TYPE>
class cls_xcMap : public cow_Map<KEY,CTYPE>
{
public:
  inline cls_xcMap() : m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xcMap(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

#define CLS_xcMAP_DECL(K,C,T,N)\
typedef cls_xcMap<K,C,T> N;\
COW_MAP_DECL(K,C,N ## COWMAP);

#define CLS_xcMAP_DECL_ADD(K,C,T,N)\
typedef cls_xcMap<K,C,T> N;

#define CLS_xcMAP_ITER_DECL_ADD(K,C,T,N)\
CLS_xcMAP_DECL_ADD(K,C,T,N);

#define CLS_xcMAP_ITER_DECL(K,C,T,N)\
CLS_xcMAP_DECL(K,C,T,N);\
typedef cow_ConstMapIterator<K,C> N ## IT;

#define CLS_ATTR_xcMap(MEMBER) AddAttribute(#MEMBER,cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT) 
#define CLS_ATTR_xcMap_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER),cls_ObjectT,TYP_TIX_cls_Map,cls_ColMapString2ObjT)

#endif
