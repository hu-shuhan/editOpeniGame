//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddr_ExtensionTable_h
#define ddr_ExtensionTable_h

// Begin local includes
#include <cls_xcMap.h>
#include <mdl_Extension.h>
#include <mdl_ExtensionMap.h>
#include <mdl_OdbExtensionTable.h>
#include <cow_List.h>

CLS_xcMAP_DECL_ADD(cow_String,mdl_ExtensionCOW,mdl_Extension,ddr_ExtensionMap);

MDL_ODBEXTENSIONTABLE_DECL(mdl_Extension, ddr_ExtensionTable);

#endif
