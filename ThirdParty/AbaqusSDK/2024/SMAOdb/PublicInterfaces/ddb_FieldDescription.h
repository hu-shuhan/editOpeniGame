//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_FieldDescription_h
#define ddb_FieldDescription_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_FieldDescription.h
//
// Description:
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

//#include <ddb_Interaction.h>
//#include <ddb_InteractionList.h>
#include <res_FieldDescriptionShortcut.h>
#include <res_FieldDescription.h>
#include <res_Types.h>
#include <res_Invariant.h>
#include <cow_ListString.h>

#include <cow_COW.h>

// Forward declarations
class ddb_ElementClass;
class ddb_Set;
class ddb_PartInstance;
class ddb_Assembly;

// Class definition

class ddb_FieldDescription  : public mem_AllocationOperators
{
  public:
    ddb_FieldDescription(const res_FieldDescriptionShortcut& shortcut);
    ddb_FieldDescription();
    ddb_FieldDescription(const res_FieldDescription& desc);
    ddb_FieldDescription(res_FieldDescription* desc);

    void setType(res_DataTypeEnm);
    void setPosition(res_OutputPositionEnm);
    void setLocationIndex(int);
    void setVariableLabel(const cow_String&);
    void setVariableDescription(const cow_String&);
    void setInvariants(res_Invariant);
    void setNumInvariants(int);
    void setMaxClamp(float);
    void setMinClamp(float);
    void expandSet();
    void setLoadCoordSystem(res_CoordSystemEnm);
    void setFieldCompleteness(res_FieldCompletenessEnm);
    void setEngineeringTensor();
    void setAsComponents();
    void setOrientation(const ddb_FieldDescription&);
    void setAssociation(const ddb_FieldDescription&);
    void mapComponent(const cow_String&, res_ComponentEnm);
    void setInternal();
    void setPair(res_PairEnm);
    void setSurface( const ddb_Set& surf );

//    void setSurfaceOutput(const ddb_Set&, const kinC_Interaction&);
//    void appendInteraction(const kinC_Interaction&);

    // Access methods
    bool  isInternal() const;
    res_PairEnm isPair() const;
    int numInvariants() const;
    res_OutputPositionEnm position() const;
    int key() const;
    const cow_String& label() const;
    const cow_String& description() const;
    int locationIndex() const;
    ddb_FieldDescription orientation() const;
    ddb_FieldDescription association() const;
    res_DataTypeEnm dataType() const;
    const cow_ListString& componentLabels() const;
    const res_ComponentMap& componentMap() const;
    res_ComponentEnm  getComponent(const cow_String&) const;
    const res_Invariant& invariants() const;
    const res_FieldDescription& fieldDescription() const;
    ddb_PartInstance partInstance() const;
    ddb_Assembly rootAssembly() const;
    ddb_Set set() const;
    ddb_ElementClass elementClass() const;

    ddb_Assembly parentAssembly() const;

    res_ClampTypeEnm clampType() const;
    int numClamps() const;
    float maxClampValue() const;
    float minClampValue() const;

    res_CoordSystemEnm loadCoordSystem() const;
    res_FieldCompletenessEnm fieldCompleteness() const;

    bool engineeringTensor() const;
    bool surfaceVariable() const;
    bool hasOrientation() const;
    bool hasAssociation() const;
    bool asPrincipal() const;

    int length() const;
    int width() const;
    int quatWidth() const;
    ddb_Set surface() const;
//    ddb_InteractionList interaction() const;

    res_FieldDescriptionShortcut Shortcut() const { return shortcut; }
    res_FieldDescriptionCOW Object() const { return desc; }

private:
    res_FieldDescriptionShortcut shortcut;
    res_FieldDescriptionCOW desc;
    bool persistent;
};

// Define class handler
COW_COW_DECL(ddb_FieldDescription);

#endif // ddb_FieldDescription
