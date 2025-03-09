//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_RebarLayersShortcut_h
#define kseC_RebarLayersShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>


// Forward declarations
class kseC_RebarLayers;
class kseC_Section;

ddr_SHORTCUT_DECL(kseC_RebarLayers, kseC_RebarLayersShortcut);
ddr_MEMBER_SHORTCUT_DECL(kseC_Section, kseC_RebarLayers, kseC_RebarLayersInSectionShortcut);
#endif
