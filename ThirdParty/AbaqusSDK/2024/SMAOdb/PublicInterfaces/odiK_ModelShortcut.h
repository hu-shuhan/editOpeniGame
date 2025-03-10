//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_ModelShortcut_h
#define odiK_ModelShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <ddr_ModelContainer.h>

// Forward declarations
class odiK_Model;
class odiK_Assembly;

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<odiK_Model>::~ddr_ShortcutImpl();
template <> cow_String ddr_Shortcut<odiK_Model>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<odiK_Model>::operator==(const ddr_Shortcut<odiK_Model>&) const;
#endif
ddr_SHORTCUT_DECL(odiK_Model, odiK_ModelShortcut);
#ifdef SMA_NO_TGEN
template <> odiK_Model& ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::Get(ddr_DdbContainer&) const;
template <> const odiK_Model& ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::ConstGet(const ddr_DdbContainer&) const;
template <> cow_String ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::Name() const;
template <> cow_String ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::Path() const;
template <> cow_String ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::DdbName(SMABasStringMode) const;
template <> ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::~ddr_DictionaryShortcut();
template <> omu_ShortcutImpl* ddr_DictionaryShortcut<ddr_ModelContainer,odiK_Model,1>::Copy() const;
#endif
ddr_DICTIONARY_SHORTCUT_DECL(ddr_ModelContainer, odiK_Model, odiK_ModelInModelContainerShortcut);
#ifdef SMA_NO_TGEN
template <> ddr_MemberShortcut<odiK_Assembly,odiK_Model,1>::~ddr_MemberShortcut();
template <> omu_ShortcutImpl* ddr_MemberShortcut<odiK_Assembly,odiK_Model,1>::Copy() const;
template <> cow_String ddr_MemberShortcut<odiK_Assembly,odiK_Model,1>::Path() const;
template <> cow_String ddr_MemberShortcut<odiK_Assembly,odiK_Model,1>::DdbName(SMABasStringMode) const;
#endif
ddr_MEMBER_SHORTCUT_DECL(odiK_Assembly, odiK_Model, odiK_ModelInAssemblyShortcut);

#endif
