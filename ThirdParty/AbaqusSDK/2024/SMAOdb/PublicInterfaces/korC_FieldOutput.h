//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef korC_FieldOutput_h
#define korC_FieldOutput_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: korC_FieldOutput.h
//
// Description:  FieldOutput base class definition.  Both the propagating
//               and non-propagating portions of the FieldOutput base class
//               are defined here.
//

// Includes

// Begin local includes

#include <korC_Output.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Class definition for FieldOutput state class

class korC_FieldOutputState: public korC_OutputState
{
  public:

    korC_FieldOutputState(typ_typeTag proc);
    virtual korC_OutputState* Copy() const;

    virtual bool IsField() const;

    virtual bool ValidOutputType(korC_TypeEnm outputType, const kprC_Step& step) const;

    virtual bool ValidInAnalysisProcedure( const kprC_Step&,
					  const bool canUseBaseState = true) const;

    virtual void AssertValid() const;

    // Convenience method to "clean up" output variables as needed
    virtual void ReconstructVariables(typ_typeTag procedureType);

    // Convenience method to convert symbols to variables
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




    // Propagating Data members
    mdl_PropBool timeMarks;

    static omn_FixedString fieldOutputStateCmd();

    // DDB persistence
    korC_FieldOutputState( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    virtual void NewUid();

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(korC_FieldOutputState, cow_Virtual);



// Class definition for the complete FieldOutput class

class korC_FieldOutput: public korC_Output
{
    friend class kprC_PropagatingContainer<korC_FieldOutput>;

  public:

    korC_FieldOutput();

    virtual korC_Output* Copy() const;

    const korC_FieldOutputState& ConstGetState(const cow_String& stepName) const;

    virtual bool IsField() const;

    void UpdateRecurranceType(); // DO NOT USE: database upgrade & ifr only

    virtual void CopyToStep(const cow_String& toStepName, 
        const kprC_Step& toStep, korC_FieldOutput* &outputNew) const;

    virtual bool ValidInAnalysisProcedure(const kprC_Step&,
			   const bool canUseBaseState = true) const;
			   
	virtual void AssertValid() const;


    // members
    bool directions;
	bool exteriorOnly;

    // defaults
    static inline bool directionsDef();
	static  bool exteriorOnlyDef();

    // Command Strings
    static omn_FixedString directionsCmd();
	static omn_FixedString exteriorOnlyCmd();
    static omn_FixedString timeMarksCmd();
    static omn_FixedString timeMarksStateCmd();
    static omn_FixedString constructorCmd();
    static omn_FixedString fieldOutputCmd();

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;

    static typ_typeTag      TypeId();
    korC_FieldOutput* DeepCopy() const {
        return (korC_FieldOutput*)DDB_DeepCopyPtr(this, DynTypeId());
    }




    // DDB persistence
    korC_FieldOutput( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

private:
    cls_Uid m_ClsUid;
};

inline bool
korC_FieldOutput::directionsDef() {
  return true;
}

// Macros

COW_ARRAYCOW2_DECL(korC_FieldOutput, cow_Virtual);



COW_LIST_ITER_DECL(korC_FieldOutput, korC_ListFieldOutput);

#endif /* korC_FieldOutput_h */
