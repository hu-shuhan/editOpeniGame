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
// File Name: ddb_UserXYData.h

#ifndef ddb_UserXYData_h
#define ddb_UserXYData_h

//
// SECTION: System includes
//

//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <BasicConstants.h>
#include <udd_UserXYDataShortcut.h>
#include <udd_UserXYData.h>
#include <cow_ListListFloat.h>
#include <cow_COW.h>

//  
// Class: ddb_UserXYData
// 

class ddb_UserXYData  : public mem_AllocationOperators
{
public:
    ddb_UserXYData(const udd_UserXYDataShortcut& );
   ~ddb_UserXYData();

    // Access methods
    const cow_String    &dataName()                   const;
    int                size()                       const;
    udd_XYSourceTypeEnm  sourceType()                 const;
    const cow_String    &sourceFileName()             const;
    const cow_String    &sourceDescription()          const;
    const cow_String    &contentDescription()         const;
    const cow_String    &positionDescription()        const;
    const cow_String    &xAxisLabel()                 const;
    const cow_String    &yAxisLabel()                 const;
    const cow_String    &legendLabel()                const;
    float               x(int index)               const;
    float               y(int index)               const;
    const udd_PairEntry &dataPairEntry(uint index)  const;
    const udd_PairList  &dataList()                   const;

    // Helper function
    cow_String description() const;
    const udd_UserXYData& constUserXYData() const; 
    void loadData(float *&xVals,
                  float *&yVals,
                  int &numVals,
                  int *&dupLocs,
                  int &numDups) const;

    // Set Methods
    void  setDataName(const cow_String &name);
    void  setSourceType(const udd_XYSourceTypeEnm  src );
    void  setSourceFileName(const cow_String &src);
    void  setSourceDescription(const cow_String &desc);
    void  setContentDescription(const cow_String &desc);
    void  setPositionDescription(const cow_String &desc);
    void  setXAxisLabel(const cow_String &label);
    void  setYAxisLabel(const cow_String &label);
    void  setLegendLabel(const cow_String &label);
    void  appendDataEntry(const float x, const float y);
    void  appendDataEntry( const udd_PairEntry& );
    void  setIsScalar(const bool flag);
    void  setScalar(const float number);
    void  setDataList( const udd_PairList& dataIn);
    void  setDataList(const float *xVals,
                      const float *yVals,
                      int numVals,
                      const int *dupLocs,
                      int numDups);

    void  setXYData( const udd_UserXYData& xyData );

    void  setAttr(udd_XYSourceTypeEnm src,
                  const cow_String &srcFile,
                  const cow_String &srcDesc,
                  const cow_String &cntDesc,
                  const cow_String &posDesc,
                  const cow_String &xAxisLbl,
                  const cow_String &yAxisLbl,
                  const cow_String &lgndLbl);

    udd_UserXYDataShortcut Shortcut() const { return shortcut; }

    bool operator== ( const ddb_UserXYData &rhs ) const;
    bool operator!= ( const ddb_UserXYData &rhs ) const;

    // Temperal Hack for delete for odb file ?
    bool IsDeletedFlag() const;
    void SetIsDeletedFlag ( bool in );

private:
    udd_UserXYDataShortcut shortcut;   
};

// Define class handler
COW_COW_DECL(ddb_UserXYData);

#endif
