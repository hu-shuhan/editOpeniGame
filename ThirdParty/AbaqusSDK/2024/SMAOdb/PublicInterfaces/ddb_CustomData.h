//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_CustomData_h
#define ddb_CustomData_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_CustomData.h
//
// Description:
//

// Begin local includes
#include <mem_AllocationOperators.h>

#include <udd_CustomDataShortcut.h>

// Forward declarations

// Class definition

class ddb_CustomData : public mem_AllocationOperators
{
  public:
    ddb_CustomData(udd_CustomDataShortcut);
   ~ddb_CustomData();

    udd_CustomDataShortcut Shortcut() const { return m_shortcut; }

  private:

    udd_CustomDataShortcut m_shortcut;
};

// Define class handler
COW_COW_DECL(ddb_CustomData);

#endif 
