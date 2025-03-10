//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xpListString_h
#define cls_xpListString_h

// Begin local includes

#include <cow_ListString.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xpListString : public cow_ListString
{
public:
  inline cls_xpListString() : m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xpListString(const cow_String& item) : cow_ListString(item), m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xpListString(const cow_ListString& list) : cow_ListString(list), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xpListString& operator=(const cow_ListString& rhs);

  cls_xpListString(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
};

inline bool operator==(const cls_xpListString& a, const cls_xpListString& b) { return a.IsEqual(b); };
inline bool operator!=(const cls_xpListString& a, const cls_xpListString& b) { return !(a.IsEqual(b)); }

cls_xpListString cls_xpListStringFromCow(const cow_ListString& cow);

#define CLS_ATTR_xpListString(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListStringT)
#define CLS_ATTR_xpListString_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListStringT)

#endif
