//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kinC_InteractionShortcut_h
#define kinC_InteractionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kinC_InteractionContainer.h>

// Forward declarations
class kinC_Interaction;

ddr_SHORTCUT_DECL(kinC_Interaction, kinC_InteractionShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(kinC_InteractionContainer, kinC_Interaction, kinC_InteractionInInteractionContainerShortcut);

#endif
