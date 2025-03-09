//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xpListDouble_h
#define cls_xpListDouble_h

// Begin local includes

#include <cow_ListDouble.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xpListDouble : public cow_ListDouble
{
public:
  inline cls_xpListDouble() : m_ClsUid(cls_Uid::AllocateUid()) {};
  inline cls_xpListDouble(int sz) : cow_ListDouble(sz), m_ClsUid(cls_Uid::AllocateUid()) {};

  cls_xpListDouble& operator=(const cow_ListDouble& rhs);

  cls_xpListDouble(const cls_ReadVisitor& rv);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  // cow_ListDouble m_Array;
  cls_Uid m_ClsUid;
};

inline bool operator==(const cls_xpListDouble& a, const cls_xpListDouble& b) { return a.IsEqual(b); }
inline bool operator!=(const cls_xpListDouble& a, const cls_xpListDouble& b) { return !(a.IsEqual(b)); }

cls_xpListDouble cls_xpListDoubleFromCow(const cow_ListDouble& cow);

#define CLS_ATTR_xpListDouble(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListDoubleT)
#define CLS_ATTR_xpListDouble_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListDoubleT)

#endif
