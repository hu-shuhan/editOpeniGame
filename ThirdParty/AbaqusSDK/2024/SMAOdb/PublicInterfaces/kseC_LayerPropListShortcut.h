//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_LayerPropListShortcut_h
#define kseC_LayerPropListShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kseC_LayerPropertiesList.h>

// Forward declarations
class kseC_RebarLayers;

ddr_SHORTCUT_DECL(kseC_LayerPropertiesList, kseC_LayerPropertiesListShortcut);
ddr_MEMBER_SHORTCUT_DECL(kseC_RebarLayers, kseC_LayerPropertiesList, kseC_LayerPropertiesListInRebarLayersShortcut);
#endif
