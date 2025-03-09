//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorPotentialShortcut_h
#define kbeC_ConnectorPotentialShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kbeC_ConnectorPotentialList.h>

// Forward declarations
class kbeC_ConnectorPotential;


ddr_SHORTCUT_DECL(kbeC_ConnectorPotential, kbeC_ConnectorPotentialShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(kbeC_ConnectorPotentialList, kbeC_ConnectorPotential, kbeC_ConnectorPotentialInConnectorPotentialListShortcut);

#endif
