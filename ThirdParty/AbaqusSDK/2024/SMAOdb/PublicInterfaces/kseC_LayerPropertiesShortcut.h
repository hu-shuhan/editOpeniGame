//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_LayerPropertiesShortcut_h
#define kseC_LayerPropertiesShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kseC_LayerPropertiesList.h>

// Forward declarations
class kseC_LayerProperties;


ddr_SHORTCUT_DECL(kseC_LayerProperties, kseC_LayerPropertiesShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(kseC_LayerPropertiesList, kseC_LayerProperties, kseC_LayerPropertiesInLayerPropertiesListShortcut);

#endif
