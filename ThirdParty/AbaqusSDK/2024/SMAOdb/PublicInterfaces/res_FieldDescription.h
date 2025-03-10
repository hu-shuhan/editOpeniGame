//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_FieldDescription_h
#define res_FieldDescription_h

//
// Begin local includes
#include <mem_AllocationOperators.h>
#include <cls_IntCOW.h>
//
#include <cow_String.h>
#include <cow_ListString.h>
#include <res_Invariant.h>
#include <utf_ResEnum.h>
#include <odiK_BaseInstanceShortcut.h>
#include <odiK_ElementClassShortcut.h>
#include <rgnC_SetDataShortcut.h>
#include <odiK_AssemblyShortcut.h>
#include <cow_ArrayCOW.h>
#include <res_Types.h>
#include <res_ComponentMap.h>

#include <typ_typeTag.h>
#include <res_FieldDescriptionShortcut.h>

#include <cls_Uid.h>
#include <cls_xp1DArrayListString.h>

//
// Forward declarations
//
class res_FieldDescription;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

COW_ARRAYCOW2_DECL(res_FieldDescription,cow_Virtual);

//
//
//

class res_FieldDescription  : public mem_AllocationOperators
{
public:
    res_FieldDescription(
        int                            key,
	res_DataTypeEnm                type,
	res_OutputPositionEnm          position,
	int                            locationIndex,
	const odiK_BaseInstanceShortcut partInstance,
	const odiK_ElementClassShortcut elemClass,
	const rgnC_SetDataShortcut     set,
	const cow_String&              varLabel,
	const cow_String&              varDescription,
	const cow_ListString&   compLabels,
	const res_ComponentMap&        compMap, 
	const res_FieldDescriptionShortcut orient,
	int                            numInvariants,
	res_Invariant                  invariant,
	int                            partDim,
        int                            length = -1);

    res_FieldDescription(
        int                            key,
	res_DataTypeEnm                type,
	res_OutputPositionEnm          position,
	int                            locationIndex,
	const odiK_AssemblyShortcut    assembly,
	const odiK_ElementClassShortcut elemClass,
	const rgnC_SetDataShortcut     set,
	const cow_String&              varLabel,
	const cow_String&              varDescription,
	const cow_ListString&   compLabels,
	const res_ComponentMap&        compMap, 
	const res_FieldDescriptionShortcut orient,
	int                            numInvariants,
	res_Invariant                  invariant,
	int                            partDim,
        int                            length = -1);
    
    res_FieldDescription( const res_FieldDescription& rhs,
			  const res_FieldDescription& lhs);

    res_FieldDescription();

    ~res_FieldDescription();

    // These will call the ComputeLengthAndWidth method.
    void SetType(res_DataTypeEnm dataType);
    void SetPosition(res_OutputPositionEnm outputPosition);

    void SetLocationIndex( int lIndex ) { locationIndex = lIndex; }

    void SetVariableLabel( const cow_String &label ) { varLabel = label; }
    void SetVariableDescription( const cow_String &description ) { varDescription = description; }
    void SetComponentLabels( const cow_ListString& clabels ) { compLabels = clabels; }

    // Component access
    void MapComponent(const cow_String& compLabel, res_ComponentEnm comp);
    void SetVariableMap(const res_ComponentMap& map) { compMap = map; }
    void SetDefaultComponentMap();

    res_ComponentEnm   GetComponent(const cow_String& compLabel) const { return compMap.ConstGet(compLabel); }
    const cow_String&  GetComponent(res_ComponentEnm comp) const;

    bool IsCompValid(const cow_String& compLabel) const { return compMap.IsMember(compLabel); }

    inline const res_ComponentMap&  VariableMap() const { return  compMap; }
    inline const cow_ListString& ComponentLabels() const { return compLabels; };

    // Invariant access
    void SetInvariants( res_Invariant invt ) { invariant = invt; }

    void SetNumInvariants( int numInv ) { numInvariants = numInv; }

    inline int NumInvariants() const { return numInvariants; }
    inline const res_Invariant& Invariant() const { return invariant; }

    // Misc.
    void SetPair(res_PairEnm p) { isPair = p; }

    inline int Key() const { return key; }
    inline const cow_String& Variable() const { return varLabel; }
    inline int LocationIndex() const { return locationIndex; }
    inline const cow_String& Description() const { return varDescription; }
    inline const res_FieldDescriptionShortcut& Orientation() const { return orientation; }
    inline const res_FieldDescriptionCOW& OrientationObject() const { return orientationCOW; }
    // above only for orphaned orientations (tranformations of fields)
    inline const res_FieldDescriptionShortcut& Association() const { return association; }
    void SetOrientation( const res_FieldDescriptionShortcut orient);
    void SetOrientation( res_FieldDescription* orient);
    void SetAssociation( const res_FieldDescriptionShortcut assoc);
    inline res_DataTypeEnm DataType() const { return type; }

    inline res_OutputPositionEnm Position() const { return position; }

    inline const odiK_BaseInstanceShortcut& PartInstance() const { return partInstance; }
    inline const rgnC_SetDataShortcut& Set() const { return set; }
    const rgnC_SetData& setObject() const;
    inline const odiK_AssemblyShortcut& Assembly() const { return assembly; }

    inline const odiK_ElementClassShortcut& ElementClass() const { return elemClass; };
    const odiK_ElementClass& elementClass() const;

    void SetMaxClamp( float maxClampValue );
    void SetMinClamp( float minClampValue );
    void UnsetClamps();

    inline res_ClampTypeEnm ClampType() const { return clampType; }

    int NumClamps() const { return numClamps; }

    float MaxClampValue() const { return maxClamp; }
    float MinClampValue() const { return minClamp; }

    bool  ExpandSet() const { return  expandSet; }

    res_CoordSystemEnm LoadCoordSystem() const { return loadCoordSystem; }

    void SetLoadCoordSystem( res_CoordSystemEnm loadCoord );

    void Allocate( res_FieldDataFloat& fieldData) const;
    void Allocate( res_FieldDataFloat& fieldData, res_FieldDataFloat& quatData) const;
    void Allocate( res_FieldDataDouble& fieldData ) const;
    void Allocate( res_FieldDataDouble& fieldData, res_FieldDataDouble& quatData) const;
    void Allocate( res_FieldDataInt& fieldData ) const;
    void Allocate( res_FieldDataBool& fieldData ) const;
    void Allocate( res_FieldDataEnm& fieldData ) const;
    // "internal" - allocate only principal values, ignore quartenions if any
    void AllocatePrin( res_FieldDataFloat& fieldData) const;
    void AllocatePrin( res_FieldDataDouble& fieldData ) const;
    void AllocatePrin( res_FieldDataInt& fieldData ) const;

    res_FieldCompletenessEnm FieldCompleteness() const { return fieldCompleteness; }

    void SetFieldCompleteness( res_FieldCompletenessEnm fieldComplete ) { fieldCompleteness = fieldComplete; }

    void SetExpandSet() { expandSet = true; }

    void SetHasOrientation() { hasOrientation = true; }
    void SetHasOrientation(bool orient) { hasOrientation = orient; }

    void SetEngineeringTensor() { engineeringTensor = true; }
    void SetEngineeringTensor(bool et) { engineeringTensor = et; }
    void SetAsComponents();
    void SetAsPrincipalValues();
    inline bool IsEngineeringTensor() const { return engineeringTensor; }

    inline bool HasOrientation() const { return hasOrientation; }

    inline bool HasAssociation() const { return hasAssociation; }

    inline res_PairEnm IsPair() const { return isPair; }

    inline bool  IsInternal() const { return  isInternal; }
    void  SetInternal() { isInternal = true; }

    const rgnC_SetDataShortcut Surface() const { return surface; }
    void SetSurface( const rgnC_SetDataShortcut& surf ) { surface = surf; }

    int Size() const;

    int QuaternionSize() const;

    virtual res_FieldDescription* Copy() const;
    void SetWidth(int w) { width = w; }
    inline int Width() const { return width; }
    inline int QuatWidth() const { return quatwidth; }
    inline int Length() const { return length; }
    inline int DataLength() const { return isPair?2*length:length; }
    inline bool AsPrincipal() const { return asPrincipal; }
    inline int PartDim() const {return partDim;}


    // Note: Now only COPEN can be partial variable. Consult with Downs or Woods on this
    bool CanBePartialVariable() const { return varLabel.Find("COPEN") != varLabel.Length(); }

    static bool VerifyOperands(const res_FieldDescription& lhs,
			       const res_FieldDescription& rhs );

    static res_OutputPositionEnm ChoosePosition(const res_FieldDescription& lhs,
                                                const res_FieldDescription& rhs );

    // for dynamic casting only
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

    bool  operator==( const res_FieldDescription  &rhs ) const;

    res_FieldDescription&  operator=(const res_FieldDescription  &rhs );
    res_FieldDescription( const res_FieldDescription  &copy );

public: // Database interface
    res_FieldDescription(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void Dtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;


private:
    int key;
    res_DataTypeEnm type;
    res_OutputPositionEnm position;
    cow_String varLabel;
    cow_String varDescription;
    int locationIndex;

    res_CoordSystemEnm loadCoordSystem;

    odiK_BaseInstanceShortcut partInstance;
    rgnC_SetDataShortcut set;
    odiK_ElementClassShortcut elemClass;
    odiK_AssemblyShortcut assembly; // Used for root assembly level elements and nodes (root assembly not instanced)
     
    res_FieldDescriptionShortcut orientation;
    res_FieldDescriptionCOW orientationCOW;
    res_FieldDescriptionShortcut association;
    res_Invariant invariant;
    int numInvariants;

    cls_xp1DArrayListString compLabels;

    int numClamps;
    float maxClamp;
    float minClamp;
    res_ClampTypeEnm clampType;
    int length;
    int width;
    int quatwidth;
    bool asPrincipal;
     
    bool expandSet;
    int partDim;

    res_FieldCompletenessEnm fieldCompleteness;

    bool hasOrientation, engineeringTensor, hasAssociation, isInternal;
    rgnC_SetDataShortcut surface;

    res_ComponentMap compMap;

    void ComputeLengthAndWidth();
    res_PairEnm isPair;
    cls_Uid m_ClsUid;
};

#endif // #ifndef res_FieldDescription_h
