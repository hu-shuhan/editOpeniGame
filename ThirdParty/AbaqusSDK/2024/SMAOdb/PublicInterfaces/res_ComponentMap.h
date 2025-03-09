//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_ComponentMap_h
#define res_ComponentMap_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
//#include <cow_Map.h>
#include <cow_String.h>
#include <utf_ResEnum.h>
#include <typ_typeTag.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>

#include <res_ComponentEnmPEMap.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class res_ComponentMap  : public mem_AllocationOperators
{
  friend class res_ComponentMapIT;

public:
  inline res_ComponentMap() : m_ClsUid(cls_Uid::AllocateUid()) {};
     
  inline bool Remove(const cow_String& key) { return map.Remove(key); };
  inline bool Insert(const cow_String& key, const res_ComponentEnm& value) { return map.Insert(key, res_ComponentEnmPE(value)); };

  inline res_ComponentEnm ConstGet(const cow_String& key) const { return map.ConstGet(key); };
  //inline res_ComponentEnm Get(const cow_String& key) { return map.Get(key).GetValue(); };

  inline bool IsMember(const cow_String& key) const { return map.IsMember(key); };

  inline int Size() const { return map.Size(); };

  //inline bool IsEmpty() const { return map.IsEmpty(); };

  //inline void Clear() { map.Clear(); };

  virtual typ_typeTag DynTypeId() const;
  static typ_typeTag TypeId();

  // Database interface
  res_ComponentMap(const cls_ReadVisitor& rv);
  static void* Ctor(cls_ReadVisitor& rv);
  static void CowDtor(cls_IntCOW* cow);
  virtual void DBWrite(const cls_WriteVisitor& wv) const;

  static void InitializeMetadata(cls_ClassRegistrar& reg);

  bool operator==( const res_ComponentMap  &rhs ) const;

private:
     res_ComponentEnmPEMap map;

     cls_Uid m_ClsUid;
};

COW_COW_DECL(res_ComponentMap);

class res_ComponentMapIT  : public mem_AllocationOperators
{
public:
     res_ComponentMapIT(const res_ComponentMap& compMap) : iter(compMap.map) {};
     
     void First() { iter.First(); };
     void Next() { iter.Next(); };
     bool IsDone() const { return iter.IsDone(); };

     res_ComponentEnm CurrentValue() const { return res_ComponentEnm(iter.CurrentValue()); };
     const cow_String& CurrentKey() const { return iter.CurrentKey(); };

private:
     res_ComponentEnmPEMapIT iter;
};

#endif
