//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorBehaviorOptionList_h
#define kbeC_ConnectorBehaviorOptionList_h

// Begin local includes

#include <kbeC_ConnectorBehaviorOption.h>
#include <mdl_1DArrayList.h>

CLS_xc1DARRAYLIST_DECL_ADD(kbeC_ConnectorBehaviorOptionCOW, kbeC_ConnectorBehaviorOption, kbeC_ConnectorBehaviorOptionImpl)
MDL_1DARRAYLIST_DECL_ADD(kbeC_ConnectorBehaviorOption, kbeC_ConnectorBehaviorOptionList)

#endif
