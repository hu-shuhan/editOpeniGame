//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddr_UserDataContainerFwd_h
#define ddr_UserDataContainerFwd_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <ddr_UserDataContainer.h>

// Forward declarations
class udd_UserData;

ddr_SHORTCUT_FWDL(udd_UserData, udd_UserDataShortcut);
ddr_DICTIONARY_SHORTCUT_FWDL(ddr_UserDataContainer, udd_UserData, udd_UserDataInUserDataContainerShortcut);

#endif
