//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorPotentialListShortcut_h
#define kbeC_ConnectorPotentialListShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kbeC_ConnectorPotentialList.h>

// Forward declarations
class kbeC_ConnectorBehaviorOption;

ddr_SHORTCUT_DECL(kbeC_ConnectorPotentialList, kbeC_ConnectorPotentialListShortcut);
ddr_MEMBER_SHORTCUT_DECL(kbeC_ConnectorBehaviorOption, kbeC_ConnectorPotentialList, kbeC_InitiationPotentialsListInConnectorBehaviorOptionShortcut);
ddr_MEMBER_SHORTCUT_DECL_ADD(kbeC_ConnectorBehaviorOption, kbeC_ConnectorPotentialList, 2, kbeC_EvolutionPotentialsListInConnectorBehaviorOptionShortcut);
ddr_MEMBER_SHORTCUT_DECL_ADD(kbeC_ConnectorBehaviorOption, kbeC_ConnectorPotentialList, 3, kbeC_ConnectorPotentialsListInConnectorBehaviorOptionShortcut);

#endif
