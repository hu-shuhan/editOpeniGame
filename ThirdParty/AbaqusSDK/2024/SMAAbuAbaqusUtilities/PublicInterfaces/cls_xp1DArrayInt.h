//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xp1DArrayInt_h
#define cls_xp1DArrayInt_h

// Begin local includes

#include <cow_ListInt.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xp1DArrayInt : public cow_ListInt
{
public:
  inline cls_xp1DArrayInt() : m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xp1DArrayInt(int sz) : cow_ListInt(sz), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xp1DArrayInt& operator=(const cow_ListInt& rhs);

  cls_xp1DArrayInt(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

inline bool operator==(const cls_xp1DArrayInt& a, const cls_xp1DArrayInt& b) { return a.IsEqual(b); }
inline bool operator!=(const cls_xp1DArrayInt& a, const cls_xp1DArrayInt& b) { return !(a.IsEqual(b)); }

cls_xp1DArrayInt cls_xp1DArrayIntFromCow(const cow_ListInt& cow);

#define CLS_ATTR_xp1DArrayInt(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)
#define CLS_ATTR_xp1DArrayInt_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)

#endif
