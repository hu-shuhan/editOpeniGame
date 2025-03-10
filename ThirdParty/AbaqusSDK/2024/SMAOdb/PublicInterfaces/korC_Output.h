//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef korC_Output_h
#define korC_Output_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: korC_Output.h
//
// Description:  Output base class definitions.  The base classes for both the
//               propagating and non-propagating portions are
//               defined here.
//

// Includes
#ifdef Status
#undef Status
#endif

// Begin local includes
#include <mem_AllocationOperators.h>

#include <kprC_typeUtils.h>
#include <kprC_PropagatingContainer.h>
#include <kprC_PropagatingState.h>
#include <kprC_StepNameProcedurePair.h>
#include <mdl_propagationStatus.h>
#include <kprC_utils.h>
#include <rgnC_Region.h>
#include <rgnC_RegionList.h>
#include <korC_DomainTypes.h>

#include <cow_ArrayCOW.h>
#include <cow_List.h>
#include <cow_ListString.h>
#include <omu_PrimEnum.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <cls_xp1DArrayInt.h>
#include <cls_xp1DArrayListString.h>

#include <typ_typeTag.h>
#include <nex_assert.h>

// Forward declarations

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Enum
enum korC_VariablesTypeEnm {korC_NO_OUTPUT_VARIABLES, korC_PRESELECT, korC_ALL, korC_CONTACT_PRESELECT, korC_ELEMENT_PRESELECT, korC_ENERGY_PRESELECT, korC_INCREMENTATION_PRESELECT, korC_NODE_PRESELECT, korC_INTEGRATED_PRESELECT, korC_CONTACT_ALL, korC_ELEMENT_ALL, korC_ENERGY_ALL, korC_INCREMENTATION_ALL, korC_MODAL_ALL, korC_NODE_ALL, korC_RADIATION_ALL, korC_INTEGRATED_ALL};
typedef omu_PrimEnum<korC_VariablesTypeEnm,1> korC_VariablesTypeConst;

enum korC_TypeEnm {korC_FREQUENCY, korC_NUMBER_INTERVALS, korC_TIME_INTERVAL, korC_MODES, korC_INVALID, korC_TIME_POINT};
typedef omu_PrimEnum<korC_TypeEnm,1> korC_TypeConst;

// Macros
MDL_PROP_ATTR_DECL(korC_VariablesTypeEnm, korC_VariablesType);
MDL_PROP_ATTR_DECL(korC_TypeEnm, korC_Type);

// Symbolic constants

// Class definition of the propagating portion common to all

class korC_OutputState  : public mem_AllocationOperators
{
protected:
    korC_OutputState();

public:
    virtual ~korC_OutputState();
    virtual korC_OutputState* Copy() const = 0;

    // Services

    virtual bool ValidInAnalysisProcedure(const kprC_Step& step,
					  const bool canUseBaseState = true) const;

    virtual bool ValidOutputType(korC_TypeEnm outputType, const kprC_Step& step) const = 0;

    virtual bool IsField() const = 0;
    bool IsHistory() const { return !IsField(); }

    virtual cow_ListString GetInvalidVariables(
        const cow_ListString&, typ_typeTag procedureType) const = 0;
    virtual cow_ListString GetInvalidVariables(typ_typeTag procedureType) const = 0;

    // Used by step replacement mechanism to retain attributes
    virtual void ChangeFrequencyType(const kprC_Step& step) = 0;
    void ChangeProcedureType(const kprC_Step& step, bool isCreateState);

    // Convenience method to "clean up" output variables as needed
    virtual void ReconstructVariables(typ_typeTag procedureType)=0;

    void SortVariables();

    // Convenience method to convert symbols to variables
    virtual void ConvertVariablesToVariablesSymbol(typ_typeTag procedureType, bool hasUpstreamSimStep) = 0;
    // This method intended as a convenience for the input file reader.
    virtual void ConvertVariablesSymbolToVariables(typ_typeTag procedureType, bool hasUpstreamSimStep) = 0;

protected:
    void ConvertSymbolToVariables(typ_typeTag procedureType, bool isField, bool hasUpstreamSimStep);
    void ConvertVariablesToSymbol(typ_typeTag procedureType, bool isField, bool hasUpstreamSimStep);

public:
     // Consistency checking here too
    virtual void Modify(const korC_OutputState&);
    virtual void Propagate();
    virtual bool IsModified() const; // Does not include deactivate flag
    virtual void AssertValid() const;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();


    // Class data members

    bool deactivate;

    // "Frequency" data members
    mdl_PropInt frequency;
    mdl_PropInt numIntervals;
    mdl_PropDouble timeInterval;
    mdl_Prop1DArrayInt modes;

    mdl_PropListString variables;
    korC_VariablesType variablesType;
    korC_Type type;

    mdl_PropString timePoint;

    // DDB persistence
    korC_OutputState( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    virtual void NewUid();

    static inline bool timeMarksDef( typ_typeTag procType );

private:
    cls_Uid m_ClsUid;
};

// COW and LIST declaration

COW_ARRAYCOW2_DECL(korC_OutputState, cow_Virtual);
KPRC_PROPAGATING_STATE_DECL(korC_OutputState, korC_StateQuad);



// Base class

class korC_Output  : public mem_AllocationOperators
{
    friend class kprC_PropagatingContainer<korC_Output>;

public:
    korC_Output();
    virtual ~korC_Output();
    virtual korC_Output* Copy() const = 0;

    // Enum
    enum ModelEnm {MODEL};
    enum AllModesEnm {ALL};
    enum EveryTimeIncrementEnm {EVERY_TIME_INCREMENT};
    enum DefaultSectionPointsEnm {DEFAULT};
    enum LastIncrementEnm {LAST_INCREMENT};
    enum RebarEnm { EXCLUDE, INCLUDE, ONLY };
    enum LocationMethodEnm { ALL_LOCATIONS, SPECIFIED, TYPED_IN };
    enum PositionEnm {AVERAGED_AT_NODES, CENTROIDAL, INTEGRATION_POINTS, NODES};


    // Query

    virtual korC_DomainTypeEnm DomainType() const;

    virtual void AssertValid() const;
    void AssertValid(const korC_StateQuad&) const;
    void AssertValidVariables(const korC_StateQuad&) const;

    // This method exists in 2 versions.  Avoid using the one that takes a type tag
    // since it's a more relaxed check.
    virtual bool ValidInStep( int procType,
			      bool canUseBaseState=true,
			      bool ignoredParameter=false,
			      int ignoredAlso=0) const;
    virtual bool ValidInStep( const kprC_Step& aProc,
			      bool canUseBaseState=true,
			      bool ignoredParameter=false,
			      int ignoredAlso=0) const;

    virtual bool ValidInAnalysisProcedure(const kprC_Step&,
			   const bool canUseBaseState = true) const;

    cow_String CreateStepName() const;
    mdl_PropagationStatusEnm Status(const cow_String& stepName) const;
    mdl_ListPropagationStatusEnm History() const;
    cow_List<korC_StateQuad>& GetStates() { return states; }
    const cow_List<korC_StateQuad>& ConstGetStates() const { return states; }

    void ChangeProcedureType(const kprC_Step&, const cow_String&);

    // Map methods for states
    virtual void SetStateStatusChecks();

    bool Remove(const cow_String& stepName);
    bool Insert(const cow_String& stepName, const korC_OutputState&);
    bool IsMember(const cow_String& stepName) const;

    // Methods
    virtual void Deactivate(const cow_String& stepName);
    virtual void Move(const cow_String& fromStepName,
		      const cow_String& toStepName);
    void Reset(const cow_String& stepName);
    void ReplaceStep(const cow_String& stepName);
    virtual void Suppress(bool fromStepSuppression=false, bool allowStepFlagReset=false);
    virtual void Resume(bool fromStepResume=false);

    virtual void ClearNonFlowParameters();

    bool IsSuppressed() const { return suppressionState!=kprC_RESUMED; }
    kprC_SuppressionEnm GetSuppressionState() const { return suppressionState; }

    void ClearBaseDomains();

    virtual rgnC_RegionList Regions(rgnC_RegionType) const;


    // Class data members

    static omn_FixedString setParametersCmd();
    static omn_FixedString modifyParametersCmd();
    static omn_FixedString nameCmd();
    static omn_FixedString regionCmd();
    static omn_FixedString historyCmd();
    static omn_FixedString createStepNameCmd();
    static omn_FixedString deactivateCmd();
    static omn_FixedString moveCmd();
    static omn_FixedString resetCmd();
    static omn_FixedString suppressCmd();
    static omn_FixedString suppressedCmd();
    static omn_FixedString resumeCmd();
    static omn_FixedString statusCmd();
    static omn_FixedString stepNameCmd();
    static omn_FixedString fromStepNameCmd();
    static omn_FixedString toStepNameCmd();
    static omn_FixedString boltLoadCmd();
    static omn_FixedString asmbdFastenerCmd();
    static omn_FixedString asmbdFastenerSetCmd();
    static omn_FixedString connectorsCmd();
    static omn_FixedString interactionsCmd();
    static omn_FixedString substructuresCmd();
    static omn_FixedString fastenersCmd();
    static omn_FixedString sectionPointsCmd();
    static omn_FixedString typeCmd();
    static omn_FixedString typeStateCmd();
    static omn_FixedString procedureTypeCmd();
    static omn_FixedString variablesCmd();
    static omn_FixedString variablesStateCmd();
    static omn_FixedString frequencyCmd();
    static omn_FixedString frequencyStateCmd();
    static omn_FixedString numIntervalsCmd();
    static omn_FixedString numIntervalsStateCmd();
    static omn_FixedString timeIntervalCmd();
    static omn_FixedString timeIntervalStateCmd();
    static omn_FixedString modesCmd();
    static omn_FixedString modesStateCmd();
    static omn_FixedString upgradeOutputMDBCmd();
    static omn_FixedString upgradeOperationCmd();
    static omn_FixedString rebarCmd();
    static omn_FixedString filterCmd();
    static omn_FixedString timePointCmd();
    static omn_FixedString timePointStateCmd();
    static omn_FixedString frequencyTypeCmd();
    static omn_FixedString positionCmd();

    // Layups
    static omn_FixedString layupNamesCmd();
    static omn_FixedString layupLocationMethodCmd();
    static omn_FixedString outputAtPlyTopCmd();
    static omn_FixedString outputAtPlyMidCmd();
    static omn_FixedString outputAtPlyBottomCmd();

    // Defaults
    static inline RebarEnm rebarDef();
    static inline PositionEnm positionDef();

    // Common data members

    kprC_SuppressionEnm suppressionState;

    // Members that determine the domain
    rgnC_Region region;
    cow_String boltLoad;
    cow_String asmbdFastener;
    cow_String asmbdFastenerSet;
    cls_xp1DArrayListString connectors, substructures;
    cls_xp1DArrayListString interactions;
    cow_String fasteners;

    // Layups
    cls_xp1DArrayListString layupNames;
    LocationMethodEnm layupLocationMethod;
    bool outputAtPlyTop, outputAtPlyMid, outputAtPlyBottom;

    cls_xp1DArrayInt sectionPoints;
    RebarEnm rebar;
    cow_String filter;
    PositionEnm position;
    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // Helper functions

    korC_StateQuad& GetStateQuad(const cow_String& stepName);
    const korC_StateQuad& ConstGetStateQuad(const cow_String& stepName) const;
    void CheckStep( const cow_String &stepName ) const;
    bool HasStep( const cow_String &stepName ) const;

    virtual bool IsField() const = 0;
    bool IsHistory() const { return !IsField(); }

    void UpdateStates(); // DO NOT USE, see note in .C file

    // Propagation Utilitiy Functions
    static bool ProcIsModal( const int &procType );
    static bool IsActive(mdl_PropagationStatusEnm);
    static bool IsValueComputed(mdl_PropagationStatusEnm);
    static bool IsModified(mdl_PropagationStatusEnm);
    static bool IsBuiltIn(mdl_PropagationStatusEnm);
    static bool IsPropagated(mdl_PropagationStatusEnm);
    static bool IsIllegalMove(mdl_PropagationStatusEnm);
    virtual bool IsEditable( const cow_String &stepName ) const;
    virtual const cow_String GetEditableStepName( const cow_String &stepName ) const;

    // DDB persistence
    korC_Output( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

protected:

    // For updating list of state quads

    void Initialize(const kprC_StepNameProcedurePairList&);
    void InsertStep(int pos, const kprC_StepNameProcedurePair&);
    void RemoveStep(const cow_String& name);
    void RenameStep(const cow_String& oldName, const cow_String& newName);

    void InitializeStateValues(const kprC_StepNameProcedurePairList& ii) {
      Initialize(ii); }

    // Synchronize states with steps

    void Sync();

    // State information

    //cow_List<korC_StateQuad> states;
    korC_StateQuadList states;

private:
    cls_Uid m_ClsUid;

};

// Type symbolic constant

typedef omu_PrimEnum<korC_Output::RebarEnm,1> korC_RebarConst;
typedef omu_PrimEnum<korC_Output::ModelEnm,1> korC_ModelConst;
typedef omu_PrimEnum<korC_Output::AllModesEnm,1> korC_AllModesConst;
typedef omu_PrimEnum<korC_Output::EveryTimeIncrementEnm,1> korC_EveryTimeIncrementConst;
typedef omu_PrimEnum<korC_Output::DefaultSectionPointsEnm,1> korC_DefaultSectionPointsConst;
typedef omu_PrimEnum<korC_Output::LastIncrementEnm,1> korC_LastIncrementConst;
typedef omu_PrimEnum<korC_Output::LocationMethodEnm,1> korC_LocationMethodConst;
typedef omu_PrimEnum<korC_Output::PositionEnm, 1> korC_PositionConst;
// Macros
COW_ARRAYCOW2_DECL(korC_Output, cow_Virtual);


// Function to determine the output type for a given step type.
korC_TypeEnm korC_DefaultOutputType(int procType, bool field);

inline korC_Output::RebarEnm korC_Output::rebarDef() {
    return korC_Output::EXCLUDE;
}
inline bool korC_OutputState::timeMarksDef( typ_typeTag procedureTypeValue ) {
    return !kprC_IsExplicit( procedureTypeValue );
}

inline korC_Output::PositionEnm korC_Output::positionDef() {
    return korC_Output::INTEGRATION_POINTS;
}
// ///////////////////////////////////////////////////////////////////////////////
// //
// // Output container class
// //

#endif /* korC_Output_h */
