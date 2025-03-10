//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_ComponentEnmPEMap_h
#define res_ComponentEnmPEMap_h

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_Map.h>

#include <utf_ResEnum.h>

#include <cls_Uid.h>
#include <cls_TypeTable.h>
#include <cls_CollectionType.h>
#include <cls_FieldHandle.h>

// End local includes

COW_MAP_ITER_DECL(cow_String,res_ComponentEnmPE,res_CEPM);

class cls_ReadVisitor;
class cls_WriteVisitor;

class res_ComponentEnmPEMap  : public mem_AllocationOperators
{
  friend class res_ComponentEnmPEMapIT;

public:
  inline res_ComponentEnmPEMap() : m_ClsUid(cls_Uid::AllocateUid()) {};

  res_ComponentEnmPEMap(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

  // This list of methods may need to be extended.  I've only provided
  // what was used when this class was written.
  bool IsMember(const cow_String& key) const;
  const res_ComponentEnmPE& ConstGet(const cow_String& key) const;

  bool Remove(const cow_String& key);
  bool Insert(const cow_String& key, const res_ComponentEnmPE& value);

  inline int Size() const { return m_Map.Size(); };

private:
  res_CEPM m_Map;
  cls_Uid m_ClsUid;
};

class res_ComponentEnmPEMapIT  : public mem_AllocationOperators
{
public:
     res_ComponentEnmPEMapIT(const res_ComponentEnmPEMap& map) : m_Iter(map.m_Map) {};
     
     void First();
     void Next() { m_Iter.Next(); };
     bool IsDone() const { return m_Iter.IsDone(); };

     const res_ComponentEnmPE& CurrentValue() const { return m_Iter.CurrentValue(); };
     const cow_String& CurrentKey() const { return m_Iter.CurrentKey(); };

private:
     res_CEPMIT m_Iter;
};

#define RES_ATTR_ComponentEnmPEMap(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_Map, cls_ColMapString2ObjT) 
#define RES_ATTR_ComponentEnmPEMap_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_Map, cls_ColMapString2ObjT)

#endif
