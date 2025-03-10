//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xp1DArrayListString_h
#define cls_xp1DArrayListString_h

// Begin local includes

#include <cow_ListString.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xp1DArrayListString : public cow_ListString
{
public:
  inline cls_xp1DArrayListString() : m_ExternalUid(NULL) {};
  inline cls_xp1DArrayListString(int sz) : cow_ListString(cow_EstimateLength, sz), m_ExternalUid(NULL) {};

  // These constructors should be used when writing temporary objects, that have
  // their UIDs stored in the owner class.
  cls_xp1DArrayListString( const cls_Uid &uid )
      : m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  cls_xp1DArrayListString( const cls_Uid &uid, int sz)
      : cow_ListString(cow_EstimateLength, sz), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  cls_xp1DArrayListString( const cls_Uid &uid, const cow_ListString &lst )
      : cow_ListString(lst), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {}

  ~cls_xp1DArrayListString();

  cls_xp1DArrayListString& operator=(const cow_ListString& rhs);

  cls_xp1DArrayListString(const cls_ReadVisitor& rv, cls_Uid *externalUid = NULL);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
  cls_Uid *m_ExternalUid;
};

inline bool operator==(const cls_xp1DArrayListString& a, const cls_xp1DArrayListString& b) { return a.IsEqual(b); };
inline bool operator!=(const cls_xp1DArrayListString& a, const cls_xp1DArrayListString& b) { return !(a.IsEqual(b)); };

cls_xp1DArrayListString cls_xp1DArrayListStringFromCow(const cow_ListString& cow);

#define CLS_ATTR_xp1DArrayListString(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListStringT)
#define CLS_ATTR_xp1DArrayListString_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListStringT)

#endif
