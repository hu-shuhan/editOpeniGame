//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef rgnC_SubsetDataList_h
#define rgnC_SubsetDataList_h

// Begin local includes
#include <cls_Map2Obj.h>
#include <rgnC_SubsetData.h>

// Forward declarations

CLS_MAP2OBJ_ITER_DECL(int, rgnC_SubsetData, rgnC_SubsetDataList);

#define CLS_ATTR_rgnC_SubsetDataList(MEMBER) CLS_ATTR_MAPINT2OBJ(MEMBER,rgnC_SubsetDataList)
#define CLS_ATTR_rgnC_SubsetDataList_I(MEMBER) CLS_ATTR_MAPINT2OBJ_I(MEMBER,rgnC_SubsetDataList)

#endif
