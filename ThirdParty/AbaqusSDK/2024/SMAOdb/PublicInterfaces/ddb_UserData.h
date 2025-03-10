//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_UserData_h
#define ddb_UserData_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_UserData.h
//
// Description:  Root userData object on ddb
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <ddb_UserXYDataContainer.h>
#include <ddb_AnnotationContainer.h>
#include <udd_UserDataShortcut.h>

// Forward declarations

// Class definition

class ddb_UserData  : public mem_AllocationOperators
{
  public:
    ddb_UserData(udd_UserDataShortcut);
   ~ddb_UserData();

    ddb_UserXYDataContainer userXYData();
    ddb_AnnotationContainer annotation();

    udd_UserDataShortcut Shortcut() const { return shortcut; }

  private:

    udd_UserDataShortcut shortcut;
};

// Define class handler
COW_COW_DECL(ddb_UserData);

#endif 
