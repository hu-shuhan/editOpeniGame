//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaC_MaterialOptionList_h
#define kmaC_MaterialOptionList_h

// Begin local includes

#include <kmaC_MaterialOption.h>
#include <mdl_1DArrayList.h>

CLS_xc1DARRAYLIST_DECL(kmaC_MaterialOptionCOW, kmaC_MaterialOption, kmaC_MaterialOptionImpl)
MDL_1DARRAYLIST_DECL_ADD(kmaC_MaterialOption, kmaC_MaterialOptionList)

#endif
