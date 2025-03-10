//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamC_AmplitudeShortcut_h
#define kamC_AmplitudeShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kamC_AmplitudeContainer.h>

// Forward declarations
class kamC_Amplitude;

ddr_SHORTCUT_DECL(kamC_Amplitude,kamC_AmplitudeShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(kamC_AmplitudeContainer, kamC_Amplitude, kamC_AmplitudeInAmplitudeContainerShortcut);

#endif
