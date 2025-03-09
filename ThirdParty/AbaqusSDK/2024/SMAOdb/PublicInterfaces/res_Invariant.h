//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_Invariant_h
#define res_Invariant_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
//#include <cow_List.h>
#include <utf_ResEnum.h>
#include <typ_typeTag.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>

#include <res_InvariantEnmPEList.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class res_Invariant  : public mem_AllocationOperators
{
public:
  inline res_Invariant() : m_ClsUid(cls_Uid::AllocateUid()) {};

  inline int Size() const { return invariant.Size(); };
  inline bool IsEmpty() const { return invariant.IsEmpty(); };

  inline void Append(const res_InvariantEnm& enm) { res_InvariantEnmPE temp(enm); invariant.Append(temp); };
  //inline void Append(const res_Invariant& list) { invariant.Append(inv.invariant); };

  void AppendUnlessMember(const res_InvariantEnm& enm);
  void AppendUnlessMember(const res_Invariant& list);

  inline bool IsMember(const res_InvariantEnm& inv) const { return invariant.IsMember(res_InvariantEnmPE(inv)); };
  //inline void RemoveMember(const res_InvariantEnm& inv) { invariant.RemoveMember(res_InvariantEnmPE(inv)); };
  
  inline void ClearAll() { invariant.ClearAll(); };

  //inline res_InvariantEnm Get(int i) { return invariant.Get(i).GetValue(); };
  inline res_InvariantEnm ConstGet(int i) const { return invariant.ConstGet(i); };

  inline cow_String Label(int i) const { return  res_InvariantEnmPE(invariant.ConstGet(i)).GetString(); };

  virtual typ_typeTag DynTypeId() const;
  static typ_typeTag TypeId();

  // Database methods
  res_Invariant(const cls_ReadVisitor& rv);
  static void* Ctor(cls_ReadVisitor& rv);
  static void CowDtor(cls_IntCOW* cow);
  virtual void DBWrite(const cls_WriteVisitor& wv) const;
  static void InitializeMetadata(cls_ClassRegistrar& reg);
     
  inline bool operator==(const res_Invariant &rhs) const { return this == &rhs || invariant == rhs.invariant; };

private:
     res_InvariantEnmPEList invariant;

     cls_Uid m_ClsUid;
};
     
COW_COW_DECL(res_Invariant);

#endif
     
