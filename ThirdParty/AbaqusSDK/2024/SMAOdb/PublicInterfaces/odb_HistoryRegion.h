//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_HistoryRegion_h
#define odb_HistoryRegion_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_HistoryPoint.h>
#include <odb_HistoryOutput.h>
#include <odb_HistoryOutputRepository.h>
#include <odb_LoadCase.h>

// Forward declarations
class ddb_Set;
COW_COW_DECL(ddb_Set);

class odb_HistoryRegion : public mem_AllocationOperators
{
public:
    // member access
    odb_String name() const;
    odb_String description() const;
    odb_HistoryPoint point() const;
    odb_Enum::odb_ResultPositionEnum position() const;
    const odb_LoadCase& loadCase() const;

    // History Output
    virtual odb_HistoryOutput& HistoryOutput(const odb_String& name,
                                             const odb_String& description,
                                             odb_Enum::odb_DataTypeEnum type=    odb_Enum::SCALAR);

    odb_HistoryOutput& historyOutputs(const odb_String& name);
    odb_HistoryOutputRepository& historyOutputs();
    const odb_HistoryOutput& historyOutputs(const odb_String& name) const;
    const odb_HistoryOutputRepository& historyOutputs() const;

    // methods
    virtual odb_HistoryRegion getSubset (const odb_String& variableName) const;
    virtual odb_HistoryRegion getSubset (double start) const;
    virtual odb_HistoryRegion getSubset (double start, double end) const;

protected:
    odb_String                  m_name;
    odb_String                  m_description;
    odb_HistoryPoint            m_historyPoint;
    odb_HistoryOutputRepository m_histVarCon;
    odb_LoadCase                m_loadCase;
    ddb_SetCOW                  m_set; // Used for writing history to the database

private:

public:   // undocumented and unsupported
    odb_HistoryRegion ();
    odb_HistoryPoint historyPoint() const;
    odb_HistoryRegion (const odb_String& name,
	 	       const odb_String& description,
                       const odb_HistoryPoint& hP,
		       const odb_HistoryOutputRepository& varCon,
                       const odb_LoadCase& lc = odb_LoadCase());

    // mutator
    void setLoadCase(const odb_LoadCase& lc);
};

#endif // odb_HistoryRegion_h
