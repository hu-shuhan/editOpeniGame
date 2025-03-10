//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef rgnC_RegionList_h
#define rgnC_RegionList_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: rgnC_RegionList.h
//
// Description: cowList of regions 
//

//
// Includes
//

// Begin local includes

#include <cls_xoList.h>
#include <rgnC_Region.h>
#include <mdl_PropagatingAttribute.h>

//
// Template declarations
//

CLS_xoLIST_ITER_DECL(rgnC_Region, rgnC_RegionList);

MDL_PROP_ATTR_DECL(rgnC_RegionList, rgnC_PropRegionList);

enum rgnC_RegionType { rgnC_INTERNAL, rgnC_EXTERNAL, rgnC_ALLTYPES };


#endif  // #ifdef rgnC_RegionList_h
