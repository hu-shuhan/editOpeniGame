//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_InstanceContainerFwd_h
#define odiK_InstanceContainerFwd_h

//
// Begin local includes
//
#include <mdl_MapString2Obj.h>
#include <ddr_Shortcut.h>

// Forward declarations
class odiK_BaseInstance;

COW_ARRAYCOW2_FWDL(odiK_BaseInstance, cow_Virtual);
MDL_MAP_STRING_2_OBJ_FWDL(odiK_BaseInstance, odiK_InstanceContainer);

ddr_SHORTCUT_FWDL(odiK_BaseInstance, odiK_BaseInstanceShortcut);
ddr_DICTIONARY_SHORTCUT_FWDL(odiK_InstanceContainer, odiK_BaseInstance, odiK_BaseInstanceInInstanceContainerShortcut);

#endif
