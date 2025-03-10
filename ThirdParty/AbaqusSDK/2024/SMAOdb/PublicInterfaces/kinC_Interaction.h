//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kinC_Interaction_h
#define kinC_Interaction_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kinC_Interaction.h
//
// Description:  Interaction base class definitions.  The base classes for both
//               the propagating and non-propagating portions of the
//               interaction are defined here.
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kinC_SymConst.h>
#include <kprC_PropagatingContainer.h>
#include <kprC_PropagatingState.h>
#include <kprC_utils.h>
#include <idb_Utils.h>
#include <typ_typeTag.h>
#include <rgnC_RegionList.h>

#include <cow_ArrayCOW.h>
#include <cow_ListString.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>

#include <mdl_propagationStatus.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;
class kprC_Procedure;

// Class definition of the propagating portion of a interaction common to all

class kinC_InteractionState  : public mem_AllocationOperators
{
  protected:

    kinC_InteractionState();

  public:

    virtual ~kinC_InteractionState();
    virtual kinC_InteractionState* Copy() const = 0;

    // Services

    virtual void AssertValid() const;     // Consistency checking here too
    virtual bool IsApplicable(const kprC_Procedure &) const;
    virtual void AssertApplicable(const kprC_Procedure &) const;
    virtual void Modify(const kinC_InteractionState&);
    virtual void Propagate();
    virtual void Zero();
    virtual bool IsModified() const;

    // Class data members

    bool deactivate;

    // Static class data members

    static omn_FixedString interactionStateCmd();

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    kinC_InteractionState * DeepCopy() const {
        return (kinC_InteractionState*)DDB_DeepCopyPtr(this, DynTypeId());
    }

        virtual void ChangeProcedureType(typ_typeTag procType);

    // DDB persistence
    kinC_InteractionState( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    virtual void NewUid();

private:
    cls_Uid m_ClsUid;
};

// COW and LIST declaration

COW_ARRAYCOW2_DECL(kinC_InteractionState, cow_Virtual);
KPRC_PROPAGATING_STATE_DECL(kinC_InteractionState, kinC_StateQuad);


///////////////////////////////////////////////////////////////////////////////
//
// Base class for interaction
//

typedef kprC_PropagatingState<kinC_InteractionState> kinC_PropagatingState;
typedef cow_ArrayCOW<kinC_InteractionState,
    cow_Virtual<kinC_InteractionState> > kinC_InteractionStateCOW;


class kinC_Interaction  : public mem_AllocationOperators
{
    friend class kprC_PropagatingContainer<kinC_Interaction>;

  public:

    kinC_Interaction();
    virtual ~kinC_Interaction();
    virtual kinC_Interaction* Copy() const = 0;

    // Query

    virtual kinC_CategoryEnm Category() const = 0;
    virtual void AssertValid() const;         // Consistency checking
    virtual void AssertValid(const cow_String& firstGeneralStep) const;

    virtual mdl_PropagationStatusEnm
                       GetPropStatus(const cow_String& stepName) const;
    virtual mdl_ListPropagationStatusEnm History() const;

    virtual cow_String CreateStepName() const;

    bool IsModified(const cow_String& stepName) const;

    cow_List<kinC_StateQuad>& GetStates() { return states; }
    const cow_List<kinC_StateQuad>& ConstGetStates() const { return states; }

    virtual cow_String InteractionProperty(const cow_String& stepName) const;
        // returns empty string if there is no prop

    // Map methods for states

    virtual bool Remove(const cow_String& stepName);
    virtual bool Insert(const cow_String& step, const kinC_InteractionState&);
    virtual bool IsMember(const cow_String& stepName) const;

    // Methods

    virtual void Deactivate(const cow_String& stepName);
    virtual void Move(const cow_String& fromStep, const cow_String& toStep);
    virtual void Reset(const cow_String& stepName);
    virtual void ReplaceStep(const cow_String& stepName);
    virtual void Suppress(bool fromStepSuppression=false, bool allowStepFlagReset=false);
    virtual void Resume(bool fromStepResume=false);
    virtual void CopyToStep(const cow_String& toStepName, kinC_Interaction* &intNew) const;

    virtual rgnC_RegionList Regions(rgnC_RegionType) const;
    virtual rgnC_RegionList ConstGetContactNodeSets() const;
    virtual cow_ListString Amplitudes() const;
    virtual cow_ListString ReferencedObjects() const;
    virtual bool UsesField( cow_String &fieldNameInUse ) const;
    virtual bool UsesField() const;

    bool IsSuppressed() const { return suppressionState!=kprC_RESUMED; }
    kprC_SuppressionEnm GetSuppressionState() const { return suppressionState; }

    virtual void ChangeProcedureType(typ_typeTag procType);

    // Static class data members

    static omn_FixedString interactionCmd();
    static omn_FixedString interactionRepositoryCmd();
    static omn_FixedString nameCmd();
    static omn_FixedString createStepNameCmd();
    static omn_FixedString stepNameCmd();
    static omn_FixedString categoryCmd();
    static omn_FixedString statusCmd();
    static omn_FixedString historyCmd();
    static omn_FixedString deactivateCmd();
    static omn_FixedString setParametersCmd();
    static omn_FixedString modifyParametersCmd();
    static omn_FixedString resetCmd();
    static omn_FixedString moveCmd();
    static omn_FixedString fromStepNameCmd();
    static omn_FixedString toStepNameCmd();
    static omn_FixedString copyConstructorCmd();
    static omn_FixedString suppressedCmd();
    static omn_FixedString suppressCmd();
    static omn_FixedString resumeCmd();
    static omn_FixedString upgradeInteractionMDBCmd();
    static omn_FixedString fieldNameCmd();
    static omn_FixedString distributionTypeCmd();

    // Data members common to all interactions
    kprC_SuppressionEnm suppressionState;
    cow_String fieldName;

    // Helper functions

    kinC_StateQuad& GetStateQuad(const cow_String& stepName);
    const kinC_StateQuad& ConstGetStateQuad(const cow_String& stepName) const;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
    kinC_Interaction *             DeepCopy() const {
        return (kinC_Interaction*)DDB_DeepCopyPtr(this, DynTypeId());
    }


    void UpdateStates(); // DO NOT USE, see note in .C file

    virtual bool ValidInStep( int procType,
                              bool canUseBaseState,
                              bool missingAnalysisStep,
                              int generalAnalysisProcType) const;


    virtual bool ValidInStep( const kprC_Step& aProc,
                              bool canUseBaseState=true,
                              bool missingAnalysisStep=false,
                              int generalAnalysisProcType=0) const;

    // This should really be named ValidAnalysisStep
    virtual bool ValidInStepButRelaxCheckOnInitial(int procType,
                             bool canUseBaseState=true) const;

    // This method probably repeats the logic contained
    // in kinC_utils and may need to be removed.  It was created as a result
    // of consolidating various procedure-based rules.
    virtual bool ValidInPerturbationStep(int procType,
                                     bool canUseBaseState=true) const = 0;

    virtual mdl_PropagationStatusEnm CheckForNonPropagation(
            mdl_PropagationStatusEnm& stts) const { return stts; }

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
    kinC_Interaction( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

    void CheckStep( const cow_String &stepName ) const;
    bool HasStep( const cow_String &stepName ) const;


protected:
    // Propagation related methods
    typedef enum
    {
      INVALID_STEP,ALREADY_DEACTIVATED, CANNOT_DEACTIVATE_BASE, UNKNOWN_STATUS, INVALID_STATE
    } kprC_PropErrorEnum;

    virtual cow_List<kinC_PropagatingState> ConstGetComputedStates() const;
    virtual void PropagateGenAnalysisObject(cow_List<kinC_PropagatingState>& computedStates) const;
    virtual void PropagatePerturbationObject(cow_List<kinC_PropagatingState>& computedStates) const;
    virtual void Perturb(typ_typeTag procedureType,bool& perturbation,kinC_InteractionStateCOW& eoState,
                         mdl_PropagationStatusEnm& status,kinC_InteractionStateCOW& lastGeneral,
                         mdl_PropagationStatusEnm& lastGeneralStatus, kinC_InteractionStateCOW& initState,
                         bool &lastFreqDeactivated ) const;
    virtual void Propagate(bool& perturbation,kinC_InteractionStateCOW& eoState,
                           mdl_PropagationStatusEnm& status,kinC_InteractionStateCOW& lastGeneral,
                           mdl_PropagationStatusEnm& lastGeneralStatus) const;
    virtual void Modify(const kinC_PropagatingState&,bool& perturbation, bool& created,
                        kinC_InteractionStateCOW& eoState,mdl_PropagationStatusEnm& status,
                        kinC_InteractionStateCOW& initState, bool &lastFreqDeactivated) const;
    virtual void Deactivate(typ_typeTag procedureType,kinC_InteractionStateCOW& eoState,
                            mdl_PropagationStatusEnm& status) const;
    void ErrorHandler(const kprC_PropErrorEnum) const;
private:
    void DeactivateBaseState(typ_typeTag procedureType) const;
    mdl_PropagationStatusEnm GetInvalidStatus(typ_typeTag procedureType) const;

    bool CheckFrequency( const int ) const;

    // For updating list of state quads
    virtual void Initialize(const kprC_StepNameProcedurePairList&);
    virtual void InsertStep(int pos, const kprC_StepNameProcedurePair&);
    virtual void RemoveStep(const cow_String& name);
    virtual void RenameStep(const cow_String& oName, const cow_String& nName);

    virtual void InitializeStateValues(const kprC_StepNameProcedurePairList&);

    // Synchronize states with steps

    virtual void Sync();

    // State information

    //cow_List<kinC_StateQuad> states;
    kinC_StateQuadList states;

    cls_Uid m_ClsUid;
};

// Macros

COW_ARRAYCOW2_DECL(kinC_Interaction, cow_Virtual);


#endif /* kinC_Interaction_h */
