//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_DerivedComponentShortcut_h
#define kbeC_DerivedComponentShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class kbeC_DerivedComponent;
class kbeC_ConnectorPotential;
class kbeC_ConnectorBehaviorOption;
ddr_SHORTCUT_DECL(kbeC_DerivedComponent, kbeC_DerivedComponentShortcut);

ddr_MEMBER_SHORTCUT_DECL(kbeC_ConnectorPotential, kbeC_DerivedComponent, kbeC_DerivedComponentInConnectorPotentialShortcut);
ddr_MEMBER_SHORTCUT_DECL(kbeC_ConnectorBehaviorOption, kbeC_DerivedComponent, kbeC_DerivedComponentInConnectorBehaviorOptionShortcut);


#endif
