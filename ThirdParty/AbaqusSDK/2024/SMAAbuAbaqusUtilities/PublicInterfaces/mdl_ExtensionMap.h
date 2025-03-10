//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mdl_ExtensionMap_h
#define mdl_ExtensionMap_h

// Begin local includes
#include <cow_Map.h>
#include <cow_MapUtils.h>
#include <cow_String.h>
#include <mdl_Extension.h>
#include <cow_List.h>
#include <cow_ListString.h>

COW_LIST_DECL(mdl_ExtensionCOW, mdl_ExtensionCOWLST);
COW_MAP_ITER_DECL(cow_String,mdl_ExtensionCOW,mdl_ExtensionMap);
COW_MAPUTILS_DECL(cow_String,mdl_ExtensionCOW);

#endif
