//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kinC_IPShortcut_h
#define kinC_IPShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kinC_IPContainer.h>

// Forward declarations
class kinC_InteractionProperty;

ddr_SHORTCUT_DECL(kinC_InteractionProperty, kinC_InteractionPropertyShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(kinC_IPContainer, kinC_InteractionProperty, kinC_InteractionPropertyInIPContainerShortcut);

#endif
