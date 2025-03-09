//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_HistoryRegionPers_h
#define odb_HistoryRegionPers_h

// Begin local includes
#include <odb_HistoryRegion.h>
#include <cow_COW.h>
// End local includes

//
// SECTION: Forward declarations
//
class ddb_History;
class odb_HistoryPoint;
class ddb_HistoryStep;
COW_COW_DECL(ddb_HistoryStep);
class odb_HistoryOutput;

class odb_HistoryRegionPers : public odb_HistoryRegion
{
public:

private:
    ddb_HistoryStepCOW m_histStep;

public:   // undocumented and unsupported
    odb_HistoryRegionPers(const odb_String& name,
                          const odb_String& description,
	                  const odb_HistoryPoint& hP,
                          ddb_HistoryStep hStep);
    odb_HistoryRegionPers();
    // History Output
    odb_HistoryOutput& HistoryOutput(const odb_String& name,
                                     const odb_String& description,
                                     odb_Enum::odb_DataTypeEnum type=odb_Enum::SCALAR);

    // methods
    odb_HistoryRegion getSubset(const odb_String& variableName) const;
    odb_HistoryRegion getSubset(double start) const;
    odb_HistoryRegion getSubset(double start,
                                double end) const;

    bool isHistoryNameTaken(const odb_String& name);

    void summaryValue1( odb_String & ) const {};
    void summaryValue2( odb_String & ) const {};

    void read_history (const ddb_History& h, const odb_String& stepName, int historyCount);
    void addHistoryOutput (const odb_HistoryOutput& hVar);

};

#endif // odb_HistoryRegionPers_h
