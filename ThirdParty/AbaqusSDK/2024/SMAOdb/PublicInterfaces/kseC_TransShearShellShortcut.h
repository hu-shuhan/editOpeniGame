//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_TransverseShearShellShortcut_h
#define kseC_TransverseShearShellShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class kseC_TransverseShearShell;
class kseC_Section;

ddr_SHORTCUT_DECL(kseC_TransverseShearShell, kseC_TransverseShearShellShortcut);
ddr_MEMBER_SHORTCUT_DECL(kseC_Section, kseC_TransverseShearShell, kseC_TransverseShearShellInSectionShortcut);

#endif
