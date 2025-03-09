//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kseC_LayerPropertiesList_h
#define kseC_LayerPropertiesList_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kseC_LayerTable.h
//
// Description:  Definition of an array of section layers.
//

// Includes

// Begin local includes

#include <cls_xo1DArrayList.h>
#include <kseC_LayerProperties.h>

///////////////////////////////////////////////////////////////////////////////
//
// Array of layers
//

//CIO_1DARRAYLIST_DECL(kseC_LayerProperties, kseC_LayerTable);
CLS_xo1DARRAYLIST_DECL(kseC_LayerProperties, kseC_LayerPropertiesList);

#endif // kseC_LayerPropertiesList_h 
