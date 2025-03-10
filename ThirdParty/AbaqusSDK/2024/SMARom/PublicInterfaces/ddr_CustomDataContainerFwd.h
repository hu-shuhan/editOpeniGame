//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddr_CustomDataContainerFwd_h
#define ddr_CustomDataContainerFwd_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <ddr_CustomDataContainer.h>

// Forward declarations
class udd_CustomData;

ddr_SHORTCUT_FWDL(udd_CustomData, udd_CustomDataShortcut);
ddr_DICTIONARY_SHORTCUT_FWDL(ddr_CustomDataContainer, udd_CustomData, udd_CustomDataInCustomDataContainerShortcut);

#endif
