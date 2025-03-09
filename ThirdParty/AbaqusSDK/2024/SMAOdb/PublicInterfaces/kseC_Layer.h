//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_Layer_h
#define kseC_Layer_h
///////////////////////////////////////////////////////////////////////////////
// kseC_Layer
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <dtmC_DatumRef.h>
#include <cow_String.h>
#include <omu_PrimEnum.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kseC_Layer : public mem_AllocationOperators
{
public:
    kseC_Layer();
    // Enumerations
    enum AxisEnm {
        AXIS_1,
        AXIS_2,
        AXIS_3
    };
    enum AdditionalRotationTypeEnm {
        ROTATION_NONE,
        ROTATION_ANGLE,
        ROTATION_FIELD
    };
    enum ThicknessTypeEnum { THICKNESS_MAGNITUDE, THICKNESS_DISCRETE_FIELD };
    // Methods
    virtual void AssertValid() const;

    // Default methods
    static inline double orientAngleDef();
    static inline int numIntPtsDef();
    static inline AxisEnm axisDef();
    static inline double angleDef();
    static inline AdditionalRotationTypeEnm additionalRotationTypeDef();
    static inline cow_String plyNameDef();
    static inline dtmC_DatumRef orientationDef();
    static inline cow_String additionalRotationFieldDef();
    // command string methods
    static omn_FixedString thicknessCmd();
    static omn_FixedString orientAngleCmd();
    static omn_FixedString numIntPtsCmd();
    static omn_FixedString axisCmd();
    static omn_FixedString angleCmd();
    static omn_FixedString additionalRotationTypeCmd();
    static omn_FixedString materialCmd();
    static omn_FixedString plyNameCmd();
    static omn_FixedString orientationCmd();
    static omn_FixedString additionalRotationFieldCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();
    static omn_FixedString thicknessTypeCmd();
    static omn_FixedString thicknessFieldCmd();

    virtual kseC_Layer* Copy() const;


    // ODB database interface
    kseC_Layer(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    double thickness;
    cow_String orientAngleString;
    double orientAngleDouble;
    int numIntPts;
    AxisEnm axis;
    double angle;
    AdditionalRotationTypeEnm additionalRotationType;
    ThicknessTypeEnum thicknessType;
    cow_String material;
    cow_String plyName;
    dtmC_DatumRef orientation;
    cow_String additionalRotationField, thicknessField;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kseC_Layer, cow_Virtual);

typedef omu_PrimEnum<kseC_Layer::AxisEnm,1> kseC_LayerAxisConst;
typedef omu_PrimEnum<kseC_Layer::AdditionalRotationTypeEnm,1> kseC_LayerAdditionalRotationTypeConst;
typedef omu_PrimEnum<kseC_Layer::ThicknessTypeEnum,1> kseC_LayerThicknessTypeConst;

// Default methods implementation
inline double
kseC_Layer::orientAngleDef()
{
    return 0.0;
}

inline int
kseC_Layer::numIntPtsDef()
{
    return 3;
}

inline kseC_Layer::AxisEnm
kseC_Layer::axisDef()
{
    return kseC_Layer::AXIS_3;
}

inline double
kseC_Layer::angleDef()
{
    return 0.0;
}

inline kseC_Layer::AdditionalRotationTypeEnm
kseC_Layer::additionalRotationTypeDef()
{
    return kseC_Layer::ROTATION_NONE;
}

inline cow_String
kseC_Layer::plyNameDef()
{
    return """";
}

inline dtmC_DatumRef
kseC_Layer::orientationDef()
{
    return dtmC_DatumRef();
}

inline cow_String
kseC_Layer::additionalRotationFieldDef()
{
    return """";
}

#endif // kseC_Layer_h
