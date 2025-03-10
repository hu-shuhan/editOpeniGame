//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaC_MaterialOptionShortcut_h
#define kmaC_MaterialOptionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>

// Forward declarations
class kmaC_Material;
class kmaC_MaterialOption;

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<kmaC_MaterialOption>::~ddr_ShortcutImpl();
template <> cow_String ddr_Shortcut<kmaC_MaterialOption>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<kmaC_MaterialOption>::operator==(const ddr_Shortcut<kmaC_MaterialOption>&) const;
#endif
ddr_SHORTCUT_DECL(kmaC_MaterialOption, kmaC_MaterialOptionShortcut);
#ifdef SMA_NO_TGEN
template <> ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::~ddr_DictionaryShortcut();
template <> omu_ShortcutImpl* ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::Copy() const;
template <> const kmaC_MaterialOption& ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::ConstGet(const ddr_DdbContainer&) const;
template <> kmaC_MaterialOption& ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::Get(ddr_DdbContainer&) const;
template <> cow_String ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::Name() const;
template <> cow_String ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::Path() const;
template <> cow_String ddr_DictionaryShortcut<kmaC_Material,kmaC_MaterialOption,1>::DdbName(SMABasStringMode) const;
#endif
ddr_DICTIONARY_SHORTCUT_DECL(kmaC_Material, kmaC_MaterialOption, kmaC_MaterialOptionInMaterialShortcut);
#ifdef SMA_NO_TGEN
template <> ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::~ddr_DictionaryShortcut();
template <> omu_ShortcutImpl* ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::Copy() const;
template <> const kmaC_MaterialOption& ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::ConstGet(const ddr_DdbContainer&) const;
template <> kmaC_MaterialOption& ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::Get(ddr_DdbContainer&) const;
template <> cow_String ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::Name() const;
template <> cow_String ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::Path() const;
template <> cow_String ddr_DictionaryShortcut<kmaC_MaterialOption,kmaC_MaterialOption,1>::DdbName(SMABasStringMode) const;
#endif
ddr_DICTIONARY_SHORTCUT_DECL(kmaC_MaterialOption, kmaC_MaterialOption, kmaC_MaterialOptionInMaterialOptionShortcut);

#endif
