//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_AssemblyShortcut_h
#define odiK_AssemblyShortcut_h

// Begin local includes
#include <odiK_AssemblyContainerShortcut.h>

// Forward declarations
class odiK_Assembly;

ddr_SHORTCUT_DECL(odiK_Assembly, odiK_AssemblyShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(odiK_AssemblyContainer, odiK_Assembly, odiK_AssemblyInAssemblyContainerShortcut);

#endif
