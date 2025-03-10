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
// File Name: udd_UserXYData.h
// 
// Description: 
//


#ifndef udd_UserXYData_h
#define udd_UserXYData_h

//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <BasicConstants.h>
#include <omi_types.h>
//#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <cow_ArrayCOW.h>
#include <atr_String.h>
#include <cow_String.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <cls_xp1DArrayListInt.h>
#include <udd_Enums.h>
#include <cow_ListListFloat.h>
#include <omn_FixedString.h>

//
// Section: Forward declarations
//
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;
class omu_Primitive;
class omu_PrimTuple;
class pnt_PrettyPrint;
class omi_BlkBitVect;

struct udd_PairEntry : public mem_AllocationOperators
{
    float  x;
    float  y;

    udd_PairEntry();
    udd_PairEntry( const float, const float);
    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;
    static void* Ctor(cls_ReadVisitor& rv);
    udd_PairEntry(const cls_ReadVisitor& rv);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
    static void InitializeMetadata(cls_ClassRegistrar& reg);

private:
    cls_Uid m_ClsUid;
};

//COW_LIST_DECL(udd_PairEntry, udd_PairList);
CLS_xo1DARRAYLIST_DECL(udd_PairEntry, udd_PairList);

//  
// Class: udd_UserXYData
// 

class udd_UserXYData  : public mem_AllocationOperators
{
public:
    udd_UserXYData(uint size = 0);
    udd_UserXYData(const cow_String& name, uint size = 0);
    udd_UserXYData(const udd_UserXYData &copy);

    ~udd_UserXYData();

    // Assignment operator
    const udd_UserXYData& operator=(const udd_UserXYData&);

    // Access methods
    const cow_String&   DataName()     const {return dataName;}
    int                 Size()         const; 
    udd_XYSourceTypeEnm SourceType()   const {return sourceType;}
    const cow_String &SourceFileName() const {return sourceFileName;}
    const cow_String &SourceDesc()     const {return sourceDescription;}
    const cow_String &ContentDesc()    const {return contentDescription;}
    const cow_String &PositionDesc()   const {return positionDescription;}
    const cow_String &XAxisLabel()     const {return xAxisLabel;}
    const cow_String &YAxisLabel()     const {return yAxisLabel;}
    const cow_String &LegendLabel()    const {return legendLabel;}
    float             X(int index)     const;
    float             Y(int index)     const;
    const udd_PairEntry&  DataPairEntry(uint index)   const;
    const udd_PairList&   DataList() const;
    bool HasDataPair(float x, float y) const;
    bool IsScalar() const { return isScalar; }
    float Scalar() const { return scalar; }
    const cow_ListInt &duplicateLocations() const { return duplicateLoc; }

    static omn_FixedString setValuesCmd();
    static omn_FixedString addDataCmd();
    static omn_FixedString nameCmd();
    static omn_FixedString dataCmd();
    static omn_FixedString fileNameCmd();
    static omn_FixedString sourceTypeCmd();
    static omn_FixedString sourceDescriptionCmd();
    static omn_FixedString contentDescriptionCmd();
    static omn_FixedString positionDescriptionCmd();
    static omn_FixedString descriptionCmd();
    static omn_FixedString xValuesLabelCmd();
    static omn_FixedString yValuesLabelCmd();
    static omn_FixedString legendLabelCmd();

     // Helper function
    cow_String Description( bool summary = false ) const;
    bool IsDeletedFlag() const { return isDeletedFlag; }
    cow_ListListFloat ListListData() const;

    // Set Methods
    void  SetDataName(const cow_String &name)       {dataName = name;}
    void  SetSourceType(udd_XYSourceTypeEnm src )   {sourceType=src;}
    void  SetSourceFileName(const cow_String &src)  {sourceFileName=src;}
    void  SetSourceDesc(const cow_String &desc)     {sourceDescription=desc;}
    void  SetContentDesc(const cow_String &desc)    {contentDescription=desc;}
    void  SetPositionDesc(const cow_String &desc)   {positionDescription=desc;}
    void  SetXAxisLabel(const cow_String &label)    {xAxisLabel = label;}
    void  SetYAxisLabel(const cow_String &label)    {yAxisLabel = label;}
    void  SetLegendLabel(const cow_String &label)   {legendLabel = label;}
    void  AppendDataEntry(float x, float y);
    void  AppendDataEntry(const udd_PairEntry& e );
    void  SetIsScalar(bool flag)                    {isScalar = flag;}
    void  SetScalar(float number)                   {scalar = number;}
    void  SetDataList( const udd_PairList& dataIn); 
    void  SetDataList(const float *xVals,
                      const float *yVals,
                      int numVals,
                      const int *dupLocs,
                      int numDups);
    void SetDataList(omu_PrimTuple &tuple);

    void  Clear(); 
    void  SetIsDeletedFlag ( bool in )              {isDeletedFlag = in;}

    // equality methods used before interpolation
    bool IsXEqual(const udd_UserXYData &) const; 
    
    // monotonic test methods used before interpolation
    bool IsXMonotonic() const; 

    // number methods
    udd_UserXYData Add(const udd_UserXYData &) const;
    udd_UserXYData Sub(const udd_UserXYData &) const;
    udd_UserXYData Mul(const udd_UserXYData &) const;
    udd_UserXYData Div(const udd_UserXYData &, bool &) const;

    udd_UserXYData Add(float) const;
    udd_UserXYData Div(float) const;

    udd_UserXYData Absolute() const;
    udd_UserXYData UnaryNegative() const;

    // sequence methods
    omu_Primitive *Index(int) const;
    udd_UserXYData Slice(int, int) const;

    // xyData methods
    udd_UserXYData cosXY() const;
    udd_UserXYData sinXY() const;
    udd_UserXYData tanXY() const;
    udd_UserXYData acosXY() const;
    udd_UserXYData asinXY() const;
    udd_UserXYData atanXY() const;
    udd_UserXYData coshXY() const;
    udd_UserXYData sinhXY() const;
    udd_UserXYData tanhXY() const;

    udd_UserXYData logXY() const;
    udd_UserXYData log10XY() const;
    udd_UserXYData expXY() const;
    udd_UserXYData powXY(const udd_UserXYData &) const;
    udd_UserXYData powXY(float) const;
    udd_UserXYData rootXY(float) const;

    udd_UserXYData currentMax() const;
    udd_UserXYData currentMin() const;
    udd_UserXYData currentRng() const;
    udd_UserXYData currentAvg() const;

    udd_UserXYData maxXY(const udd_UserXYData &) const;
    udd_UserXYData minXY(const udd_UserXYData &) const;
    udd_UserXYData maxXY(float) const;
    udd_UserXYData minXY(float) const;

    udd_UserXYData norXY() const;
    udd_UserXYData sqrtXY() const;

    udd_UserXYData degreeToRadian() const;
    udd_UserXYData radianToDegree() const;

    udd_UserXYData diff() const;
    udd_UserXYData intg() const;

    udd_UserXYData srss(const udd_UserXYData &) const;
    udd_UserXYData srssInterpolate(const udd_UserXYData &) const;

    udd_UserXYData combine(const udd_UserXYData &) const;
    udd_UserXYData append(const udd_UserXYData &) const;
    udd_UserXYData swap() const;

    udd_UserXYData smooth(int) const;

    int lineIntervals() const;

    // Database interface
    virtual udd_UserXYData* Copy() const;
    udd_UserXYData(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    // needed for dynamic casting and repository
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

    // Messaging interface
    static pnt_PrettyPrint& PrintData(pnt_PrettyPrint&, const void*);

    // Helper functions to get data other than the name and datapairList
    void SetAllStrings( const udd_UserXYData & );

    void ConvertToArray(float *&x, float *&y, omi_BlkBitVect &dupLoc) const;
    void ConvertToArray(float *&x, float *&y, int *&dups, int &numDups) const;

protected:
    // dummy functions
    bool operator==(const udd_UserXYData &) const;
    bool operator!=(const udd_UserXYData &) const;

    // Data Members:
    cow_String              dataName;
    udd_XYSourceTypeEnm     sourceType;
    cow_String              sourceFileName;
    cow_String              sourceDescription;
    cow_String              contentDescription;
    cow_String              positionDescription;
    cow_String              xAxisLabel;
    cow_String              yAxisLabel;
    cow_String              legendLabel;
    bool                    isScalar;
    float                   scalar;

    udd_PairList            dataList;
    cls_xp1DArrayListInt    duplicateLoc;

    // deleted flag
    bool isDeletedFlag; 

private:
    cls_Uid m_ClsUid;
};

pnt_PrettyPrint& operator<<( pnt_PrettyPrint &pp,
                             const udd_PairEntry &obj);
pnt_PrettyPrint& operator<<( pnt_PrettyPrint &pp,
                             const udd_UserXYData *ptr);
pnt_PrettyPrint& operator<<( pnt_PrettyPrint &pp, 
                             const udd_UserXYData &obj);


COW_ARRAYCOW2_DECL(udd_UserXYData, cow_Virtual);
COW_LIST_DECL(udd_UserXYData, udd_UserXYDataList);

#endif
