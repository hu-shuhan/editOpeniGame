//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef rgnC_SetDataShortcut_h
#define rgnC_SetDataShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <rgnC_SetDataList.h>

// Forward declarations
class rgnC_SetData;

#ifdef SMA_NO_TGEN
template <> ddr_ShortcutImpl<rgnC_SetData>::~ddr_ShortcutImpl();
template <> void ddr_ShortcutImpl<rgnC_SetData>::DBWrite(const cls_WriteVisitor&) const;
template <> void ddr_ShortcutImpl<rgnC_SetData>::CowDtor(cls_IntCOW*);
template <> ddr_ShortcutImpl<rgnC_SetData>::ddr_ShortcutImpl(const cls_ReadVisitor&);
template <> void ddr_ShortcutImpl<rgnC_SetData>::InitializeMetadata(cls_ClassRegistrar&);
template <> cow_String ddr_Shortcut<rgnC_SetData>::DdbName(SMABasStringMode) const;
template <> bool ddr_Shortcut<rgnC_SetData>::operator==(const ddr_Shortcut<rgnC_SetData>&) const;
template <> ddr_Shortcut<rgnC_SetData>::ddr_Shortcut(const cls_ReadVisitor&);
template <> void* ddr_Shortcut<rgnC_SetData>::Ctor(cls_ReadVisitor&);
template <> void ddr_Shortcut<rgnC_SetData>::DBWrite(const cls_WriteVisitor&) const;
template <> void ddr_Shortcut<rgnC_SetData>::InitializeMetadata(cls_ClassRegistrar&);
#endif
ddr_SHORTCUT_DECL(rgnC_SetData, rgnC_SetDataShortcut);
#ifdef SMA_NO_TGEN
template <> rgnC_SetData& ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::Get(ddr_DdbContainer&) const;
template <> const rgnC_SetData& ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::ConstGet(const ddr_DdbContainer&) const;
template <> cow_String ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::Name() const;
template <> cow_String ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::Path() const;
template <> cow_String ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::DdbName(SMABasStringMode) const;
template <> ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::~ddr_SequenceShortcut();
template <> omu_ShortcutImpl* ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::Copy() const;
template <> ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::ddr_SequenceShortcut(const cls_ReadVisitor&);
template <> void* ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::Ctor(cls_ReadVisitor&);
template <> void ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::DBWrite(const cls_WriteVisitor&) const;
template <> void ddr_SequenceShortcut<rgnC_SetDataList,rgnC_SetData,1>::InitializeMetadata(cls_ClassRegistrar&);
#endif
ddr_SEQUENCE_SHORTCUT_DECL(rgnC_SetDataList, rgnC_SetData, rgnC_SetDataInSetDataListShortcut);

#endif
