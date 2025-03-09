//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_TypeTable_h
#define cls_TypeTable_h

// Begin local includes
#include <typ_TypeTables.h>
#include <typ_Architecture.h>
#include <cls_FacilityTag.h>

extern typ_ArchCompEntry cls_ArchEntry();

#define TYP_TIX_cls_List        ((TYP_FAC_cls << 16) | 0x1)
#define TYP_TIX_cls_Map         ((TYP_FAC_cls << 16) | 0x2)
#define TYP_TIX_cls_Cow         ((TYP_FAC_cls << 16) | 0x3)
#define TYP_TIX_cls_DblCol      ((TYP_FAC_cls << 16) | 0x4)
#define TYP_TIX_cls_Blob        ((TYP_FAC_cls << 16) | 0x5)
#define TYP_TIX_cls_Ado         ((TYP_FAC_cls << 16) | 0x6)
#define TYP_TIX_cls_StdStm      ((TYP_FAC_cls << 16) | 0x7)

#endif
