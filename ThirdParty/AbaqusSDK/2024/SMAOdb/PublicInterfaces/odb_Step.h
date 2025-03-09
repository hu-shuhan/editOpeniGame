//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Step_h
#define odb_Step_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_Frame.h>
#include <odb_SequenceFrame.h>
#include <odb_HistoryRegionRepository.h>
#include <odb_Assembly.h>
#include <odb_LoadCaseRepository.h>
#include <odb_RestartRequest.h>
#include <odb_DictionaryStringFloat.h>
// End local includes

// Forward declarations
class ddb_Step;
COW_COW_DECL(ddb_Step);
class ddb_DdbStep;
COW_COW_DECL(ddb_DdbStep);
class ddb_Assembly;
COW_ARRAYCOW_DECL(ddb_Assembly);
class odb_SequenceNodalDofs;
class odb_Odb;
COW_COW_DECL(odb_Odb);

class odb_Step : public mem_AllocationOperators
{
public:
    // member access
    odb_String                  name() const;
    odb_String                  description() const;
    int                         number() const;
    odb_Enum::odb_DomainEnum    domain() const;
    odb_String                  procedure() const;
    double                      timePeriod() const;
    double                      totalTime() const;
    odb_String                  previousStepName() const;
    odb_SequenceInt             retainedEigenModes() const;
    odb_SequenceNodalDofs       retainedNodalDofs() const;
    odb_SequenceNodalDofs       eliminatedNodalDofs() const;
    bool                        nlgeom() const;
    bool isAnalysisStep() const;
    bool isPerturbation() const;

    double              mass() const;
    odb_SequenceDouble  massCenter() const;
    odb_SequenceDouble  inertiaAboutCenter() const;
    odb_SequenceDouble  inertiaAboutOrigin() const;
    double              acousticMass() const;
    odb_SequenceDouble  acousticMassCenter() const;

    // frame
    odb_Frame Frame(int no,
                    double xVal,
                    const odb_String& description="");
    odb_Frame Frame(const odb_LoadCase& lc,
                    const odb_String& description="");
    odb_Frame Frame(const odb_LoadCase& lc,
                    const odb_String& description,
                    double frequency);
    odb_Frame& frames(int frameNo);
    odb_SequenceFrame& frames();
    const odb_Frame& frames(int frameNo) const;
    const odb_SequenceFrame& frames() const;

    // history
    odb_HistoryRegion& HistoryRegion(const odb_String& name,
			             const odb_String& description,
                                     const odb_HistoryPoint& );
    odb_HistoryRegion& HistoryRegion(const odb_String& name,
			             const odb_String& description,
                                     const odb_Node& );
    odb_HistoryRegion& HistoryRegion(const odb_String& name,
			             const odb_String& description,
                                     const odb_HistoryPoint&,
                                     const odb_LoadCase& );
    odb_HistoryRegion& HistoryRegion(const odb_String& name,
			             const odb_String& description,
                                     const odb_Node&,
                                     const odb_LoadCase& );
    odb_HistoryRegionRepository& historyRegion(); // Deprecated after 6.2
    odb_HistoryRegionRepository& historyRegions();
    const odb_HistoryRegionRepository& historyRegions() const;

    // LoadCase
	// Creates Loadcase in a step in odb file
	// Arguments
	// name - A String specifying the name of the loadcase
	// boundaryConditions - A map of BoundaryCondition names (String) to non zero scaling factors (Float).
	// loads - A map of load names (String) to non zero scaling factors (Float). 
	// includeActiveBaseStateBC - A Boolean specifying whether to include all active 
	//         boundary conditions propagated or modified from the base state

    odb_LoadCase& LoadCase(const odb_String& name,
		                   const odb_DictionaryStringFloat& boundaryConditions = odb_DictionaryStringFloat(),
                           const odb_DictionaryStringFloat& loads = odb_DictionaryStringFloat(),
                           bool includeActiveBaseStateBC=true);

    odb_LoadCaseRepository& loadCases();
    const odb_LoadCaseRepository& loadCases() const;

    // solverInput snippet
    void setSolverInput(const odb_String& solverInput);
    odb_String solverInput() const;

    // default fields
    void setDefaultField(const odb_FieldOutput& );
    void setDefaultDeformedField(const odb_FieldOutput& );

    // Restart Requests
    const odb_RestartRequest &RestartRequest(const int &numberOfIntervals=0, const int &frequency=0,
                                const bool &overlay=false, const bool &timeMarks=false);
    const odb_RestartRequest &getRestartRequest() { return m_restartRequest.ConstGet(); }
    //
    bool update();

    // history and frame utility function
    odb_HistoryRegion& getHistoryRegion( const odb_HistoryPoint& point );
    odb_HistoryRegion& getHistoryRegion( const odb_HistoryPoint& point,
                                         const odb_LoadCase& loadCase );
    odb_Frame getFrame( double frameValue,
                        odb_Enum::odb_MatchEnum=odb_Enum::CLOSEST );
    odb_Frame getFrame( const odb_LoadCase&,
                        double frameValue = -1.0,
                        odb_Enum::odb_MatchEnum=odb_Enum::CLOSEST );

private:
    odb_String               m_name;
    odb_String               m_description;
    odb_Enum::odb_DomainEnum m_domain;
    odb_String               m_procedure;
    odb_String               m_previousStepName;

    bool IsRead;

    ddb_StepCOW                 m_step;
    ddb_AssemblyCOW             m_rootAssy;
    odb_SequenceFrame           m_frameContainer;
    odb_HistoryRegionRepository m_historyRegionCon;
    odb_LoadCaseRepository      m_loadCaseContainer;
    ddb_DdbStepCOW              m_modelStep;
    odb_RestartRequestCOW m_restartRequest;

public:   // undocumented and unsupported

    // For a RESULTS step
    odb_Step(const odb_String&         name,
                const ddb_Step&        step,
                const odb_String&      descrip,
                const ddb_Assembly&    assy,
                const odb_Odb&         parent,
                odb_Enum::odb_DomainEnum    domain=odb_Enum::TIME,
                const odb_String&      previousStep="",
                const odb_String&      procedure="");

    // For a MODEL step
    odb_Step(const ddb_DdbStep& modelStep);

    // Needed for container
    odb_Step();

    // utility functions
    void Read();
    void ReadFrameCon();
    void ReadFrame(int frameNo);
    void ReadHistories();
    void ClearFrameContainer() const;

    // find all matching frames and return their ids
    odb_SequenceInt findFrames( double frameValue,
                                odb_Enum::odb_MatchEnum=odb_Enum::CLOSEST ) const;
    odb_SequenceInt findFrames( const odb_LoadCase&,
                                double frameValue = -1.0,
                                odb_Enum::odb_MatchEnum=odb_Enum::CLOSEST ) const;

    const ddb_Step& ConstDdbStep() const
        { return m_step.ConstGet(); }
    ddb_Step& DdbStep()
        { return m_step.Get(); }
};

COW_COW_DECL(odb_Step);

#endif // odb_Step_h
