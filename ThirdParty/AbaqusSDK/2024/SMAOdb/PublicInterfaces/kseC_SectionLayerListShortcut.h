//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_SectionLayerListShortcut_h
#define kseC_SectionLayerListShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <kseC_SectionLayerList.h>

// Forward declarations
class kseC_Section;

ddr_SHORTCUT_DECL(kseC_SectionLayerList, kseC_SectionLayerListShortcut);
ddr_MEMBER_SHORTCUT_DECL(kseC_Section, kseC_SectionLayerList, kseC_SectionLayerListInSectionShortcut);

#endif
