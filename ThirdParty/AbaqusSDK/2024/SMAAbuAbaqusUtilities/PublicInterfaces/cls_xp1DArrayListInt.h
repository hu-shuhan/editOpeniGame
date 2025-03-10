//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_xp1DArrayListInt_h
#define cls_xp1DArrayListInt_h

// Begin local includes

#include <cow_ListInt.h>

#include <cls_TypeTable.h>
#include <cls_Uid.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

class cls_xp1DArrayListInt : public cow_ListInt
{
public:
  inline cls_xp1DArrayListInt() : m_ExternalUid(NULL) {};
  inline cls_xp1DArrayListInt(int sz) : cow_ListInt(cow_EstimateLength, sz), m_ExternalUid(NULL) {};

  // These constructors should be used when writing temporary objects, that have
  // their UIDs stored in the owner class.
  inline cls_xp1DArrayListInt( const cls_Uid &uid )
      : m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  inline cls_xp1DArrayListInt( const cls_Uid &uid, int sz)
      : cow_ListInt(cow_EstimateLength, sz), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {};
  cls_xp1DArrayListInt( const cls_Uid &uid, const cow_ListInt &lst )
      : cow_ListInt(lst), m_ClsUid(uid), m_ExternalUid( const_cast<cls_Uid*>(&uid) ) {}

  ~cls_xp1DArrayListInt();

  cls_xp1DArrayListInt& operator=(const cow_ListInt& rhs);

  cls_xp1DArrayListInt(const cls_ReadVisitor& rv, cls_Uid *externalUid = NULL);
  void DBWrite(const cls_WriteVisitor& wv) const;

private:
  cls_Uid m_ClsUid;
  cls_Uid *m_ExternalUid;
};

inline bool operator==(const cls_xp1DArrayListInt& a, const cls_xp1DArrayListInt& b) { return a.IsEqual(b); };
inline bool operator!=(const cls_xp1DArrayListInt& a, const cls_xp1DArrayListInt& b) { return !(a.IsEqual(b)); };

cls_xp1DArrayListInt cls_xp1DArrayListIntFromCow(const cow_ListInt& cow);

#define CLS_ATTR_xp1DArrayListInt(MEMBER) AddAttribute(#MEMBER, cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)
#define CLS_ATTR_xp1DArrayListInt_I(MEMBER) AddAttribute(cls_FieldHandle(#MEMBER,e_ ## MEMBER), cls_ObjectT, TYP_TIX_cls_List, cls_ColListIntT)

#endif
