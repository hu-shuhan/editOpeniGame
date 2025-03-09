//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_RigidBodyShortcut_h
#define odiK_RigidBodyShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <odiK_RigidBodyList.h>
// Forward declarations

ddr_SHORTCUT_DECL(odiK_RigidBody, odiK_RigidBodyShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(odiK_RigidBodyList, odiK_RigidBody, odiK_RigidBodyInRigidBodyListShortcut);

#endif

