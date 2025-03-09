//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- Mode: c++ -*- */
#ifndef udd_CustomDataShortcut_h
#define udd_CustomDataShortcut_h

// Begin local includes
#include <ddr_CustomDataContainerShortcut.h>

// Forward declarations
class udd_CustomData;

// Shortcut

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<udd_CustomData>::~ddr_ShortcutImpl();
template <> cow_String ddr_Shortcut<udd_CustomData>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<udd_CustomData>::operator==(const ddr_Shortcut<udd_CustomData>&) const;
#endif
ddr_SHORTCUT_DECL(udd_CustomData, udd_CustomDataShortcut);
#ifdef SMA_NO_TGEN
template <> udd_CustomData& ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::Get(ddr_DdbContainer&) const;
template <> const udd_CustomData& ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::ConstGet(const ddr_DdbContainer&) const;
template <> cow_String ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::Name() const;
template <> cow_String ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::Path() const;
template <> cow_String ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::DdbName(SMABasStringMode) const;
template <> ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::~ddr_DictionaryShortcut();
template <> omu_ShortcutImpl* ddr_DictionaryShortcut<ddr_CustomDataContainer,udd_CustomData,1>::Copy() const;
#endif
ddr_DICTIONARY_SHORTCUT_DECL(ddr_CustomDataContainer, udd_CustomData, udd_CustomDataInCustomDataContainerShortcut);

#endif
