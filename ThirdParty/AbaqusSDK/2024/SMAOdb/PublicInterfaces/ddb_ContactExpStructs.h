//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_ContactExpStructs_h
#define ddb_ContactExpStructs_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_List.h>
#include <ddb_Set.h>
#include <ddb_InteractionProperty.h>
#include <cow_String.h>

// Forward declarations

struct ddb_RegionPairStruct : public mem_AllocationOperators
{
    ddb_Set region1;
    ddb_Set region2;
};

struct ddb_ContactPropertyAssignmentStruct : public mem_AllocationOperators
{
    ddb_Set region1;
    ddb_Set region2;
    ddb_InteractionProperty prop;
};

struct ddb_SurfaceThicknessAssignmentStruct : public mem_AllocationOperators
{
    ddb_Set region;
    double factor;
    cow_String thicknessConst;
    double thickness;
};

COW_LIST_DECL(ddb_RegionPairStruct, ddb_RegionPairList);
COW_LIST_DECL(ddb_ContactPropertyAssignmentStruct, ddb_ContactPropertyAssignmentList);
COW_LIST_DECL(ddb_SurfaceThicknessAssignmentStruct, ddb_SurfaceThicknessAssignmentList);

#endif
