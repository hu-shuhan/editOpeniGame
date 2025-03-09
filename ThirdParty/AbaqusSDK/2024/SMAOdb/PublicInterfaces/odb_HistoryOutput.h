//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_HistoryOutput_h
#define odb_HistoryOutput_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <odb_Enum.h>
#include <cow_COW.h>
// End local includes

// Forward Declarations
class ddb_HistoryClock;
COW_COW_DECL(ddb_HistoryClock)
class ddb_HistoryFloat;
COW_COW_DECL(ddb_HistoryFloat)

class odb_HistoryOutput  : public mem_AllocationOperators
{
public:
    // member access
    odb_String name() const {return m_name;}
    odb_String description() const {return m_description;}
    odb_Enum::odb_DataTypeEnum type() const {return m_dataType;}

    // methods
    virtual odb_SequenceSequenceFloat data() const;
    virtual odb_SequenceSequenceFloat conjugateData() const;
    virtual void addData(const odb_SequenceSequenceFloat& data);
    virtual void addData(double frame,
                         double value);
    virtual void addData(const odb_SequenceFloat& frame,
                         const odb_SequenceFloat& value);

protected:
    odb_String           m_name;
    odb_String           m_description;
    odb_Enum::odb_DataTypeEnum     m_dataType;

private:     
    ddb_HistoryClockCOW       m_clock;
    ddb_HistoryFloatCOW       m_hist;
    bool                     m_persistent;
    odb_SequenceSequenceFloat m_data;
    odb_SequenceSequenceFloat m_conjugateData;

public:     // undocumented and unsupported
    odb_HistoryOutput();
    odb_HistoryOutput(const odb_String& name,
                      const odb_String& descript,
                      odb_Enum::odb_DataTypeEnum type,
                      const ddb_HistoryClock& c,
                      const ddb_HistoryFloat& hist);
    odb_HistoryOutput(const odb_String& name,
                      const odb_String& description,
                      odb_Enum::odb_DataTypeEnum type,
                      const odb_SequenceSequenceFloat& data);

    odb_HistoryOutput getSubset(double start) const;
    odb_HistoryOutput getSubset(double start,
                                double end) const;

    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};
    
    const ddb_HistoryClock & clock() const { return m_clock.ConstGet(); }
    const ddb_HistoryFloat & ddbHist() const { return m_hist.ConstGet(); }

    virtual void addData(const odb_SequenceSequenceFloat& data,
                         const odb_SequenceSequenceFloat& conjugate,
                         bool expandClock);
    virtual void addData(const odb_SequenceSequenceFloat& data, bool expandClock);
};

#endif // odb_HistoryOutput_h
