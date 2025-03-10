//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_HistoryOutputRequestRepository_h
#define odb_HistoryOutputRequestRepository_h

// Begin local includes

#include <odb_HistoryOutputRequest.h>
#include <odb_Repository.h>
#include <odb_PropagatingComponent.h>
#include <odb_Set.h>
#include <odb_Union.h>

// Forward declarations
class ddb_Model;
COW_COW_DECL(ddb_Model);

ODB_NEWCONTAINER_DECL(odb_String, odb_HistoryOutputRequest, odb_HistoryOutputRequestRepository);

class odb_HistoryOutputRequestRepository: public odb_HistoryOutputRequestRepositoryIMPL
{
public:  // Undocumented and unsupported
    odb_HistoryOutputRequestRepository(const ddb_Model& ddb);

    odb_HistoryOutputRequest &HistoryOutputRequest(
                    const odb_String& name,
                    const odb_String& stepName,
                    const odb_Set& set = odb_Set(),
                    const odb_SequenceString variables = odb_SequenceString(), //"PRESELECT"
                    const odb_SequenceString interactions = odb_SequenceString(),
                    const odb_SequenceString connectors = odb_SequenceString(),
                    const odb_String boltLoad = odb_String(),
                    const odb_String contourIntegral = odb_String(),
                    const odb_SequenceString springs = odb_SequenceString(),
		    const odb_String frequencyType = "FREQUENCY", //"FREQUENCY", 
		                                                  //"NUMBER_INTERVALS", 
		                                                  //"TIME_INTERVAL", "MODES", 
		                                                  //"TIME_POINT"
                    const odb_Union frequency = "LAST_INCREMENT",
                    const odb_SequenceInt modes = odb_SequenceInt(), //"ALL"
                    const double timeInterval = 200.0,
                    const int numIntervals = 20,
                    const odb_SequenceInt sectionPoints = odb_SequenceInt(), // "DEFAULT"
                    const odb_String rebar = "EXCLUDE",
                    const int numContours = 0,
                    const odb_String contourType = odb_String(),
                    const odb_String kFactorDirection = odb_String());

    static void separateVariables(const odb_SequenceString& inVariables,
				  odb_SequenceString& nodeVariables,
				  odb_SequenceString& elementVariables,
				  odb_SequenceString& energyVariables,				  
				  odb_SequenceString& otherVariables, 
				  odb_SequenceString& invalidVariables);

private:
    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:  // Undocumented and unsupported
    odb_HistoryOutputRequestRepository();
    void Read();

};

COW_COW_DECL(odb_HistoryOutputRequestRepository);

#endif
