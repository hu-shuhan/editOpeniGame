//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_ListObj_h
#define cls_ListObj_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//
//
// Modification history:
//      Rob deFriesse 8/99 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes


#include <typ_typeTag.h>

#include <cow_COW.h>
#include <cow_List.h>

#include <cls_PC.h>

#include <cls_Uid.h>
#include <cls_AgentListObj.h>

// End local includes

class cls_ReadVisitor;
class cls_WriteVisitor;

template <class V, class C>
class cls_ListObj : public cls_PC
{
public:
  cls_ListObj();
  cls_ListObj(const cls_ListObj<V,C>& source);
  cls_ListObj (const cls_ReadVisitor& rv);
  virtual ~cls_ListObj();

  void Presize(int size);   

  cls_ListObj<V,C>& operator= (const cls_ListObj<V,C>& source);

  virtual void DBWrite (const cls_WriteVisitor& rv) const;

  virtual typ_typeTag DynTypeId() const;
  static  typ_typeTag TypeId();
  static  void* Ctor (cls_ReadVisitor& rv);

  virtual void Flush() const;
  virtual void Detach();

  int Snap(void) const;
  void Release(int index) const;
  void Release(void) const;

  int Size() const;

  void Append(const V& item);
  void Append(V* item);

  const V& ConstGet(int index) const;
  const C& ConstGetCOW(int index) const;

  V& Get(int index);

private:
  typedef cls_ListObj<V,C> MyClass;

  cow_List<C> m_List;

  int m_Dirty;

  cls_AgentListObj* m_Agent;

  cls_Uid m_ClsUid;
};


#define CLS_LISTOBJ_DECL(VALUE,NAME) \
COW_LIST_DECL(VALUE ## COW,NAME ## LIST); \
typedef cls_ListObj<VALUE,VALUE ## COW> NAME;

#define CLS_LISTOBJ_IMPL(VALUE,NAME) \
COW_LIST_IMPL(VALUE ## COW,NAME ## LIST); \
template class cls_ListObj<VALUE,VALUE ## COW>;

#define CLS_ATTR_LISTOBJ(MEMBER,CLASS) CLS_ATTR_COLOBJ(MEMBER,CLASS,cls_ColListObjT)
#define CLS_ATTR_LISTOBJ_I(MEMBER,CLASS) CLS_ATTR_COLOBJ_I(MEMBER,CLASS,cls_ColListObjT)

#endif
