//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: omu_DictIntObjInst
// 
// Description:
//
//     
//
#ifndef omu_DictIntObjInst_h
#define omu_DictIntObjInst_h

//
// Begin Local Includes
//
#include <omu_DictIntObj.h>
#include <cow_String.h>
#include <omu_PrimEnum.h>

//
// Pre instantiated DictIntObjABC's
//
typedef omu_DictIntObjABC<cow_String> omu_DictIntObjStrABC;
typedef omu_DictIntObjABC<int> omu_DictIntObjIntABC;
typedef omu_DictIntObjABC<omu_PrimEnumBase> omu_DictIntObjEnumABC;

COW_LIST_ITER_DECL(omu_PrimEnumBase, cow_ListEnum);

#endif // omu_DictIntObjInst_h
