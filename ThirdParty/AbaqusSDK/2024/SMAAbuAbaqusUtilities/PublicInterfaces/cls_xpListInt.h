//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xpListInt_h
#define cls_xpListInt_h

// Begin local includes

#include <cow_ListInt.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xpListInt : public cow_ListInt
{
public:
  inline cls_xpListInt() : m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xpListInt(const int& item) : cow_ListInt(item), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xpListInt& operator=(const cow_ListInt& rhs);

  cls_xpListInt(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

inline bool operator==(const cls_xpListInt& a, const cls_xpListInt& b) { return a.IsEqual(b); };
inline bool operator!=(const cls_xpListInt& a, const cls_xpListInt& b) { return !(a.IsEqual(b)); }

cls_xpListInt cls_xpListIntFromCow(const cow_ListInt& cow);

#define CLS_ATTR_xpListInt(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)
#define CLS_ATTR_xpListInt_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)

#endif
