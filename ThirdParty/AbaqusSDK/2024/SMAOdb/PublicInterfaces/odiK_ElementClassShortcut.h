//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_ElementClassShortcut_h
#define odiK_ElementClassShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <bme_ElementClassList.h>

// Forward delcarations

class odiK_ElementClass;

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<odiK_ElementClass>::~ddr_ShortcutImpl();
template <> cow_String ddr_Shortcut<odiK_ElementClass>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<odiK_ElementClass>::operator==(const ddr_Shortcut<odiK_ElementClass>&) const;
#endif
ddr_SHORTCUT_DECL(odiK_ElementClass, odiK_ElementClassShortcut);
#ifdef SMA_NO_TGEN
template <> odiK_ElementClass& ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::Get(ddr_DdbContainer&) const;
template <> const odiK_ElementClass& ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::ConstGet(const ddr_DdbContainer&) const;
template <> cow_String ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::Name() const;
template <> cow_String ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::Path() const;
template <> cow_String ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::DdbName(SMABasStringMode) const;
template <> ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::~ddr_SequenceShortcut();
template <> omu_ShortcutImpl* ddr_SequenceShortcut<bme_ElementClassList,odiK_ElementClass,1>::Copy() const;
#endif
ddr_SEQUENCE_SHORTCUT_DECL(bme_ElementClassList, odiK_ElementClass, odiK_ElementClassInElementClassListShortcut);

#endif
