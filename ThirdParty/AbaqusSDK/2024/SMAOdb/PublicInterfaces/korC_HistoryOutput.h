//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef korC_HistoryOutput_h
#define korC_HistoryOutput_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: korC_HistoryOutput.h
//
// Description:  HistoryOutput base class definition.  Both the propagating
//               and non-propagating portions of the HistoryOutput base class
//               are defined here.
//

// Includes

// Begin local includes

#include <korC_Output.h>

// Forward declarations
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

// Class definition for HistoryOutput state class

class korC_HistoryOutputState: public korC_OutputState
{
  public:

    korC_HistoryOutputState(typ_typeTag proc);
    virtual korC_OutputState* Copy() const;

    virtual bool IsField() const;
    virtual void AssertValid() const;

    virtual bool ValidOutputType(korC_TypeEnm outputType, const kprC_Step& step) const;

    virtual bool ValidInAnalysisProcedure( const kprC_Step&,
					  const bool canUseBaseState = true) const;

    // Convenience method to "clean up" output variables as needed
    virtual void ReconstructVariables(typ_typeTag procedureType);

    // Convenience method to convert preselect symbols to variables
    virtual void ConvertVariablesToVariablesSymbol(typ_typeTag procedureType, bool hasUpstreamSimStep);
    // This method intended as a convenience for the input file reader.
    virtual void ConvertVariablesSymbolToVariables(typ_typeTag procedureType, bool hasUpstreamSimStep);

    virtual cow_ListString GetInvalidVariables(
        const cow_ListString&, typ_typeTag procedureType) const;
    virtual cow_ListString GetInvalidVariables(typ_typeTag proc) const;

    // Used by step replacement mechanism to retain attributes
    virtual void ChangeFrequencyType(const kprC_Step& step);

    // Services
    virtual void Modify(const korC_OutputState&);
    virtual void Propagate();
    virtual bool IsModified() const;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;

    static typ_typeTag      TypeId();

    // Members
    mdl_PropBool timeMarks;

    static omn_FixedString historyOutputStateCmd();

    // DDB persistence
    korC_HistoryOutputState( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    virtual void NewUid();

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(korC_HistoryOutputState, cow_Virtual);



// Class definition for the complete HistoryOutput class

class korC_HistoryOutput: public korC_Output
{
    friend class kprC_PropagatingContainer<korC_HistoryOutput>;

  public:

    korC_HistoryOutput();

    virtual korC_Output* Copy() const;

    virtual korC_DomainTypeEnm DomainType() const;

    virtual void AssertValid() const;
    void AssertValidVariables(const korC_StateQuad&) const;

    virtual bool ValidInAnalysisProcedure(const kprC_Step&,
			   const bool canUseBaseState = true) const;

    virtual void ClearNonFlowParameters();
    virtual void CopyToStep(const cow_String& toStepName, const kprC_Step& toStep, 
        korC_HistoryOutput* &outputNew) const;

    // Convenience method for unusual database upgrade
    void ReplaceParentVariables();

    // Enum
    enum ContourTypeEnm { J_INTEGRAL, C_INTEGRAL, T_STRESS, K_FACTORS };
    enum KFactorDirectionEnm { MTS, MERR, KII0 };

    virtual bool IsField() const;
    void ClearHistoryDomains(); // clear domain members which only exist in history output

    const korC_HistoryOutputState& ConstGetState(const cow_String& stepName) const;
    //Check for Multiple Load Cases Only.
    bool IsOnlyHistoryNodalOutput(const korC_HistoryOutputState& state,typ_typeTag procedureType,bool hasMultipleLoadCase=true) const;
    // Command Strings
    static omn_FixedString constructorCmd();
    static omn_FixedString historyOutputCmd();
    static omn_FixedString contourIntegralCmd();
    static omn_FixedString numberOfContoursCmd();
    static omn_FixedString contourTypeCmd();
    static omn_FixedString kFactorDirectionCmd();
    static omn_FixedString integratedOutputSectionCmd();
    static omn_FixedString springsCmd();
    static omn_FixedString sensorCmd();
    static omn_FixedString timeMarksCmd();
    static omn_FixedString timeMarksStateCmd();
	static omn_FixedString stressInitializationStepCmd();
    static omn_FixedString useGlobalCmd();
    static omn_FixedString useXFEMCmd();

    // defaults
    static inline ContourTypeEnm contourTypeDef();
    static inline KFactorDirectionEnm kFactorDirectionDef();
    static inline bool sensorDef();
    static inline bool useGlobalDef();
    static inline bool useXFEMDef();

    // data members for domain
    cow_String integratedOutputSection;
    cls_xp1DArrayListString springs; // May be of the form PartInst.Spring
    bool sensor;
    bool useGlobal;

    // data members for contour integral
    bool useXFEM;
    cow_String contourIntegral;
    int numberOfContours;
    ContourTypeEnm contourType;
    KFactorDirectionEnm kFactorDirection;

	cow_String stressInitializationStep;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;

    static typ_typeTag      TypeId();
    korC_HistoryOutput* DeepCopy() const {
        return (korC_HistoryOutput*)DDB_DeepCopyPtr(this, DynTypeId());
    }




    // DDB persistence
    korC_HistoryOutput( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

private:
    cls_Uid m_ClsUid;
};

typedef omu_PrimEnum<korC_HistoryOutput::ContourTypeEnm,1> korC_ContourTypeConst;
typedef omu_PrimEnum<korC_HistoryOutput::KFactorDirectionEnm,1> korC_KFactorDirectionConst;

inline korC_HistoryOutput::ContourTypeEnm
korC_HistoryOutput::contourTypeDef() {
  return korC_HistoryOutput::J_INTEGRAL;
}
inline korC_HistoryOutput::KFactorDirectionEnm
korC_HistoryOutput::kFactorDirectionDef() {
  return korC_HistoryOutput::MTS;
}
inline bool
korC_HistoryOutput::sensorDef() {
  return false;
}
inline bool
korC_HistoryOutput::useGlobalDef() {
  return true;
}
inline bool
korC_HistoryOutput::useXFEMDef() {
  return false;
}

// Macros

COW_ARRAYCOW2_DECL(korC_HistoryOutput, cow_Virtual);


COW_LIST_ITER_DECL(korC_HistoryOutput, korC_ListHistoryOutput);

#endif /* korC_HistoryOutput_h */
