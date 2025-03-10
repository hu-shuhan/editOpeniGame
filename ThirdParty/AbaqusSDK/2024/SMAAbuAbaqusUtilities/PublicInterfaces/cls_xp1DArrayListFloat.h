//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xp1DArrayListFloat_h
#define cls_xp1DArrayListFloat_h

// Begin local includes

#include <cow_ListFloat.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xp1DArrayListFloat : public cow_ListFloat
{
public:
  inline cls_xp1DArrayListFloat() : m_ExternalUid(NULL) {};
  inline cls_xp1DArrayListFloat(int sz) : cow_ListFloat(cow_EstimateLength, sz), m_ExternalUid(NULL) {};

  // These constructors should be used when writing temporary objects, that have
  // their UIDs stored in the owner class.
  cls_xp1DArrayListFloat( const cls_Uid &uid )
      : m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  cls_xp1DArrayListFloat( const cls_Uid &uid, int sz)
      : cow_ListFloat(cow_EstimateLength, sz), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  cls_xp1DArrayListFloat( const cls_Uid &uid, const cow_ListFloat &lst )
      : cow_ListFloat(lst), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {}

  ~cls_xp1DArrayListFloat();

  cls_xp1DArrayListFloat& operator=(const cow_ListFloat& rhs);

  cls_xp1DArrayListFloat(const cls_ReadVisitor& rv, cls_Uid *externalUid = NULL);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
  cls_Uid *m_ExternalUid;
};

inline bool operator==(const cls_xp1DArrayListFloat& a, const cls_xp1DArrayListFloat& b) { return a.IsEqual(b); };
inline bool operator!=(const cls_xp1DArrayListFloat& a, const cls_xp1DArrayListFloat& b) { return !(a.IsEqual(b)); };

cls_xp1DArrayListFloat cls_xp1DArrayListFloatFromCow(const cow_ListFloat& cow);

#define CLS_ATTR_xp1DArrayListFloat(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListFloatT)
#define CLS_ATTR_xp1DArrayListFloat_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListFloatT)

#endif
