//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kamC_BaselineCorrectionShortcut_h
#define kamC_BaselineCorrectionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class kamC_Amplitude;
class kamC_BaselineCorrection;

ddr_SHORTCUT_DECL(kamC_BaselineCorrection, kamC_BaselineCorrectionShortcut);
ddr_MEMBER_SHORTCUT_DECL(kamC_Amplitude, kamC_BaselineCorrection, kamC_BaselineCorrectionInEquallySpacedShortcut);
ddr_MEMBER_SHORTCUT_DECL_ADD(kamC_Amplitude, kamC_BaselineCorrection, 2, kamC_BaselineCorrectionInTabularShortcut);
ddr_MEMBER_SHORTCUT_DECL_ADD(kamC_Amplitude, kamC_BaselineCorrection, 3, kamC_BaselineCorrectionInAmplitudeShortcut);

#endif
