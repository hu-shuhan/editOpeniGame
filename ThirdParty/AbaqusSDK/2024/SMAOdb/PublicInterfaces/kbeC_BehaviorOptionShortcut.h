//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorBehaviorOptionShortcut_h
#define kbeC_ConnectorBehaviorOptionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kbeC_BehaviorOptionList.h>

// Forward declarations


ddr_SHORTCUT_DECL(kbeC_ConnectorBehaviorOption, kbeC_ConnectorBehaviorOptionShortcut);
ddr_SHORTCUT_DECL(kbeC_ConnectorBehaviorOptionList, kbeC_ConnectorBehaviorOptionListShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(kbeC_ConnectorBehaviorOptionList, kbeC_ConnectorBehaviorOption, kbeC_ConnectorBehaviorOptionInConnectorBehaviorListShortcut);

#endif
