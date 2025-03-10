//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_SectionShortcut_h
#define kseC_SectionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class kseC_Section;

template <class A> class mdl_MapString2ObjCow;
typedef class mdl_MapString2ObjCow<kseC_Section> kseC_SectionContainer;

ddr_SHORTCUT_DECL(kseC_Section, kseC_SectionShortcut);
ddr_DICTIONARY_SHORTCUT_DECL(kseC_SectionContainer, kseC_Section, kseC_SectionInSectionContainerShortcut);

#endif
