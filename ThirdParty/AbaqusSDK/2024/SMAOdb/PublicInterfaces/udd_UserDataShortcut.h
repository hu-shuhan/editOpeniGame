//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- Mode: c++ -*- */
#ifndef udd_UserDataShortcut_h
#define udd_UserDataShortcut_h

// Begin local includes
#include <ddr_UserDataContainerShortcut.h>

// Forward declarations
class udd_UserData;

// Shortcut

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<udd_UserData>::~ddr_ShortcutImpl();
template <> cow_String ddr_Shortcut<udd_UserData>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<udd_UserData>::operator==(const ddr_Shortcut<udd_UserData>&) const;
#endif
ddr_SHORTCUT_DECL(udd_UserData, udd_UserDataShortcut);
#ifdef SMA_NO_TGEN
template <> udd_UserData& ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::Get(ddr_DdbContainer&) const;
template <> const udd_UserData& ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::ConstGet(const ddr_DdbContainer&) const;
template <> cow_String ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::Name() const;
template <> cow_String ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::Path() const;
template <> cow_String ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::DdbName(SMABasStringMode) const;
template <> ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::~ddr_DictionaryShortcut();
template <> omu_ShortcutImpl* ddr_DictionaryShortcut<ddr_UserDataContainer,udd_UserData,1>::Copy() const;
#endif
ddr_DICTIONARY_SHORTCUT_DECL(ddr_UserDataContainer, udd_UserData, udd_UserDataInUserDataContainerShortcut);

#endif
