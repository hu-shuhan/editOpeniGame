//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_AssemblyContainerFwd_h
#define odiK_AssemblyContainerFwd_h

//
// Begin local includes
//
#include <ddr_Shortcut.h>
#include <odiK_AssemblyContainer.h>

// Forward declarations
class odiK_Assembly;

ddr_SHORTCUT_FWDL(odiK_Assembly, odiK_AssemblyShortcut);
ddr_DICTIONARY_SHORTCUT_FWDL(odiK_AssemblyContainer, odiK_Assembly, odiK_AssemblyInAssemblyContainerShortcut);

#endif
