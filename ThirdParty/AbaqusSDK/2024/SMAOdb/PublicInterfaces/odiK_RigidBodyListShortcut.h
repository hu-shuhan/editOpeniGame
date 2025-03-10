//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_RigidBodyListShortcut_h
#define odiK_RigidBodyListShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <odiK_RigidBodyList.h>
// Forward declarations
class odiK_Assembly;
class odiK_BaseInstance;
class odiK_Part;

ddr_SHORTCUT_DECL(odiK_RigidBodyList, odiK_RigidBodyListShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_Assembly, odiK_RigidBodyList, odiK_RigidBodyListInAssemblyShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_BaseInstance, odiK_RigidBodyList, odiK_RigidBodyListInPartInstanceShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_Part, odiK_RigidBodyList, odiK_RigidBodyListInPartShortcut);

#endif

