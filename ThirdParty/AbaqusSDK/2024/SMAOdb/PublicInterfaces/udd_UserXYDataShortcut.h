//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef udd_UserXYDataShortcut_h
#define udd_UserXYDataShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class udd_UserXYData;
class udd_UserXYDataContainer;

ddr_SHORTCUT_DECL(udd_UserXYData, udd_UserXYDataShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(udd_UserXYDataContainer, udd_UserXYData, udd_UserXYDataInCShortcut);

#endif
