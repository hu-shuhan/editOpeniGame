//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_CDCTermShortcut_h
#define kbeC_CDCTermShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kbeC_CDCTermList.h>

// Forward declarations
class kbeC_CDCTerm;


ddr_SHORTCUT_DECL(kbeC_CDCTerm, kbeC_CDCTermShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(kbeC_CDCTermList, kbeC_CDCTerm, kbeC_CDCTermInCDCTermListShortcut);

#endif
