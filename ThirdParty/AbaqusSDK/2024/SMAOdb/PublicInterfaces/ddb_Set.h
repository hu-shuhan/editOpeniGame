//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_Set_h
#define ddb_Set_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_Set.h
//
// Description:  Root model object on ddb
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ListInt.h>
#include <rgnC_SetDataShortcut.h>
#include <odiC_Enums.h>
#include <rgnC_ResEnum.h>
#include <cow_COW.h>
#include <cow_List.h>

// Forward declarations
class ddb_Subset;
class rgnC_SetData;

// Class definition

class ddb_Set  : public mem_AllocationOperators
{
public:
     ddb_Set(const rgnC_SetDataShortcut& shortcut);
     ddb_Set();

     int numSubsets() const;
     ddb_Subset subSet(int subsetIndex) const;
     ddb_Subset subSet(int partIndex, int classIndex, res_SetTypeEnm type) const;

     res_SetTypeEnm setType() const;
     bool internalSet() const;
     bool unsortedSet() const;

     cow_String setLabel() const;

     const rgnC_SetData& set() const;

     rgnC_SetDataShortcut Shortcut() const { return shortcut; }

     int setIndex() const;

private:
     rgnC_SetDataShortcut shortcut;
};

COW_COW_DECL(ddb_Set);

COW_LIST_DECL(ddb_Set, ddb_LocalSetList);
#endif // ddb_Set_h
