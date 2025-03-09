//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef rgnC_SetDataList_h
#define rgnC_SetDataList_h

// Begin local includes
#include <cow_ArrayCOW.h>
#include <cls_Map2Obj.h>

// Forward Declarations
class rgnC_SetData;
COW_ARRAYCOW2_DECL(rgnC_SetData, cow_Virtual);


CLS_MAP2OBJ_ITER_DECL(int, rgnC_SetData, rgnC_SetDataList);

#define CLS_ATTR_rgnC_SetDataList(MEMBER) CLS_ATTR_MAPINT2OBJ(MEMBER,rgnC_SetDataList)
#define CLS_ATTR_rgnC_SetDataList_I(MEMBER) CLS_ATTR_MAPINT2OBJ_I(MEMBER,rgnC_SetDataList)

#endif
