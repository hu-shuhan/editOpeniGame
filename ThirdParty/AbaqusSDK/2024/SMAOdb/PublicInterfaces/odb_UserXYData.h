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
// File Name: odb_UserXYData.h

#ifndef odb_UserXYData_h
#define odb_UserXYData_h

//
// SECTION: System includes
//

//
// Begin local includes
//
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_String.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <ddb_UserXYData.h>

//  
// Class: odb_UserXYData
// 

class odb_UserXYData : public mem_AllocationOperators
{
public:

    // Access methods
    const odb_String &name()                const { return m_name; }
    odb_Enum::odb_XYSourceTypeEnm sourceType()        const { return m_sourceType; }
    const odb_String &sourceFileName()      const { return m_sourceFileName; }
    const odb_String &sourceDescription()   const { return m_sourceDescription; }
    const odb_String &contentDescription()  const { return m_contentDescription; }
    const odb_String &positionDescription() const { return m_positionDescription; }
    const odb_String &xAxisLabel()          const { return m_xAxisLabel; }
    const odb_String &yAxisLabel()          const { return m_yAxisLabel; }
    const odb_String &legendLabel()         const { return m_legendLabel; }

    void getData(odb_SequenceSequenceFloat &) const;
    void getDuplicateLoc(odb_SequenceInt &) const;
    const odb_String description() const ;

    // Set Methods
    void  setName(const odb_String &);
    void  setSourceType(odb_Enum::odb_XYSourceTypeEnm ) ;
    void  setSourceFileName(const odb_String &)  ;
    void  setSourceDescription(const odb_String &)     ;
    void  setContentDescription(const odb_String &)    ;
    void  setPositionDescription(const odb_String &)   ;
    void  setXAxisLabel(const odb_String &)    ;
    void  setYAxisLabel(const odb_String &)    ;
    void  setLegendLabel(const odb_String &)   ;

    void  addData( const odb_SequenceSequenceFloat & );
    void  addData( const odb_SequenceSequenceFloat &,
                   const odb_SequenceInt & ) ;

    //
    bool update();

private:
    odb_String m_name;
    odb_Enum::odb_XYSourceTypeEnm m_sourceType;
    odb_String m_sourceFileName;
    odb_String m_sourceDescription;
    odb_String m_contentDescription;
    odb_String m_positionDescription;
    odb_String m_xAxisLabel;
    odb_String m_yAxisLabel;
    odb_String m_legendLabel;

    ddb_UserXYDataCOW m_userXYData;

    // cached data
    float *m_xVals, *m_yVals;
    int m_numVals;
    int *m_dupLocs;
    int m_numDups;
    bool m_isRead;

    void clearData();

    // hide
    bool operator==(const odb_UserXYData &) const; 
    bool operator!=(const odb_UserXYData &) const;

public:   // undocumented and unsupported
    odb_UserXYData( const odb_String& name,
		    const odb_Enum::odb_XYSourceTypeEnm& sourceType,
		    const odb_String& sourceFileName,
		    const odb_String& sourceDescription,
		    const odb_String& contentDescription,
		    const odb_String& positionDescription,
		    const odb_String& xAxisLabel,
		    const odb_String& yAxisLabel,
		    const odb_String& legendLabel,
		    ddb_UserXYData    xyData );
    ~odb_UserXYData();

    const ddb_UserXYData& constUserXYData() const { return m_userXYData.ConstGet(); }
    ddb_UserXYData& userXYData() { return m_userXYData.Get(); }
  
    // Don't use, just here to make cow_Btree happy
    odb_UserXYData();

    // operators
    odb_UserXYData(const odb_UserXYData &);
    odb_UserXYData &operator=(const odb_UserXYData &);

    // utility functions
    void Read();

    void addData(float *xVals, float *yVals, int numVals,
                 int *dupLocs = 0, int numDups = 0);
};

#endif
