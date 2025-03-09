//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xp2DArrayDouble_h
#define cls_xp2DArrayDouble_h

// Begin local includes

#include <cow_2DArrayDouble.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xp2DArrayDouble : public cow_2DArrayDouble
{
public:
  inline cls_xp2DArrayDouble(int r, int c) : cow_2DArrayDouble(r, c), m_ExternalUid(NULL) {};

  // This constructor should be used when writing temporary objects, that have
  // their UIDs stored in the owner class.
  cls_xp2DArrayDouble(const cls_Uid &uid, int r, int c)
      : cow_2DArrayDouble(r, c), m_ClsUid(uid), m_ExternalUid(const_cast<cls_Uid*>(&uid)) {};

  cls_xp2DArrayDouble();
  ~cls_xp2DArrayDouble();

  cls_xp2DArrayDouble& operator=(const cow_2DArrayDouble& rhs);

  cls_xp2DArrayDouble(const cls_ReadVisitor& rv, cls_Uid *externalUid = NULL);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
  cls_Uid *m_ExternalUid;
};

cls_xp2DArrayDouble cls_xp2DArrayDoubleFromCow(const cow_2DArrayDouble& cow);

#define CLS_ATTR_xp2DArrayDouble(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColList2DDoubleT)
#define CLS_ATTR_xp2DArrayDouble_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColList2DDoubleT)

#endif
