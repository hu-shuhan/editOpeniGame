//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_BaseInstanceShortcut_h
#define odiK_BaseInstanceShortcut_h

// Begin local includes
#include <odiK_InstanceContainerShortcut.h>

// Forward declarations
class odiK_BaseInstance;

ddr_SHORTCUT_DECL(odiK_BaseInstance, odiK_BaseInstanceShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(odiK_InstanceContainer, odiK_BaseInstance, odiK_BaseInstanceInInstanceContainerShortcut);

#endif
