//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpC_BeamProfileShortcut_h
#define kbpC_BeamProfileShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kbpC_BeamProfileContainer.h>

// Forward declarations
class kbpC_BeamProfile;

ddr_SHORTCUT_DECL(kbpC_BeamProfile, kbpC_BeamProfileShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(kbpC_BeamProfileContainer, kbpC_BeamProfile, kbpC_BeamProfileInBeamProfileContainerShortcut);

#endif
