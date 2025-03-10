//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_Uid_h
#define cls_Uid_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//   
// Modification history:
//      Rob deFriesse 4/2000 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes
#include <mem_AllocationOperators.h>

// End local includes

class cls_UidCatalogEntry;

class cls_Uid  : public mem_AllocationOperators
{
public:
  cls_Uid();
  cls_Uid(const cls_Uid& uid);
  cls_Uid(cls_UidCatalogEntry* entry);

  ~cls_Uid();

  static cls_Uid AllocateUid();

  void NewUid();

  cls_Uid& operator=(const cls_Uid& rhs);

  inline cls_UidCatalogEntry* GetCatalogEntry() const { return m_Entry; };

private:
  cls_UidCatalogEntry* m_Entry;
};

#endif
