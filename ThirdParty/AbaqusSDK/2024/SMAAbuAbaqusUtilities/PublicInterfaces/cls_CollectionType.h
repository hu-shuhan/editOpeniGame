//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_CollectionType_h
#define cls_CollectionType_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//   
//
// Modification history:
//      Rob deFriesse 3/2002 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes

// End local includes

class cow_String;

enum cls_CollectionType
  {
    cls_ColNotACollectionT = 0,
    cls_ColListObjT,
    cls_ColMapString2ObjT,
    cls_ColMapInt2ObjT,
    cls_ColMapString2ListObjT,
    cls_ColMapKey2ObjT,
    cls_ColBlobT,

    cls_ColListPrimT = 50,
    cls_ColListBoolT,
    cls_ColListDoubleT,
    cls_ColListFloatT,
    cls_ColListIntT,
    cls_ColListStringT,
    cls_ColListTRStringT,
    cls_ColListUintT,

    cls_ColMapString2PrimT = 60,
    cls_ColMapString2BoolT,
    cls_ColMapString2DoubleT,
    cls_ColMapString2FloatT,
    cls_ColMapString2IntT,
    cls_ColMapString2StringT,
    cls_ColMapString2UintT,

    cls_ColMapInt2PrimT = 70,
    cls_ColMapInt2BoolT,
    cls_ColMapInt2DoubleT,
    cls_ColMapInt2FloatT,
    cls_ColMapInt2IntT,
    cls_ColMapInt2StringT,
    cls_ColMapInt2UintT,

    cls_ColList2DPrimT = 80,
    cls_ColList2DBoolT,
    cls_ColList2DDoubleT,
    cls_ColList2DFloatT,
    cls_ColList2DIntT,
    cls_ColList2DStringT,
    cls_ColList2DTRStringT,
    cls_ColList2DUintT,

    cls_ColMapString2ListPrimT = 90,
    cls_ColMapString2ListBoolT,
    cls_ColMapString2ListDoubleT,
    cls_ColMapString2ListFloatT,
    cls_ColMapString2ListIntT,
    cls_ColMapString2ListStringT,
    cls_ColMapString2ListUintT,
    
    cls_ColMapInt2ListPrimT = 100,
    cls_ColMapInt2ListBoolT,
    cls_ColMapInt2ListDoubleT,
    cls_ColMapInt2ListFloatT,
    cls_ColMapInt2ListIntT,
    cls_ColMapInt2ListStringT,
    cls_ColMapInt2ListUintT,

    cls_ColMapString2ArrayPrimT = 110,
    cls_ColMapString2ArrayBoolT,
    cls_ColMapString2ArrayDoubleT,
    cls_ColMapString2ArrayFloatT,
    cls_ColMapString2ArrayIntT,
    cls_ColMapString2ArrayStringT,
    cls_ColMapString2ArrayUintT
  };

cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, bool* flag);
cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, double* flag);
cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, float* flag);
cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, int* flag);
cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, cow_String* flag);
cls_CollectionType cls_CollectionTypeFix(cls_CollectionType colType, unsigned int* flag);
bool cls_CollectionsMatch(cls_CollectionType colType1, cls_CollectionType colType2);

#endif
