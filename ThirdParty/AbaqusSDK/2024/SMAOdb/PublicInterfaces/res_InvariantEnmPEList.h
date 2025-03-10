//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_InvariantEnmPEList_h
#define res_InvariantEnmPEList_h

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_List.h>

#include <utf_ResEnum.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>
#include <cls_FieldHandle.h>

// End local includes

COW_LIST_DECL(res_InvariantEnmPE,res_IEPL);

class cls_ReadVisitor;
class cls_WriteVisitor;

class res_InvariantEnmPEList  : public mem_AllocationOperators
{
public:
  res_InvariantEnmPEList();
  res_InvariantEnmPEList(const cls_ReadVisitor& rv);

  void                      DBWrite(const cls_WriteVisitor& wv) const;

  // This list of methods may need to be extended.  I've only provided
  // what was used when this class was written.
  bool                      IsMember(const res_InvariantEnmPE& item) const;
  const res_InvariantEnmPE& ConstGet(unsigned int i) const;
  int                       Size() const;
  bool                      IsEmpty() const; 
  void                      Append(const res_InvariantEnmPE& enm); 
  void                      AppendUnlessMember(const res_InvariantEnmPE& enm);
  void                      ClearAll();

  bool                      operator==(const res_InvariantEnmPEList& rhs) const;

private:
  res_IEPL  m_List;
  cls_Uid   m_ClsUid;
};

#define RES_ATTR_InvariantEnmPEList(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT) 
#define RES_ATTR_InvariantEnmPEList_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListObjT)

#endif
