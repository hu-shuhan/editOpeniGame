//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseC_LayerProperties_h
#define kseC_LayerProperties_h
///////////////////////////////////////////////////////////////////////////////
// kseC_LayerProperties
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <cow_String.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kseC_LayerProperties : public mem_AllocationOperators
{
public:
    kseC_LayerProperties();
    // Methods
    virtual void AssertValid() const;

    // Default methods
    static inline double barSpacingDef();
    static inline double layerPositionDef();
    static inline double spacingAngleDef();
    static inline double extensionRatioDef();
    static inline double radiusDef();
    // command string methods
    static omn_FixedString barAreaCmd();
    static omn_FixedString orientationAngleCmd();
    static omn_FixedString barSpacingCmd();
    static omn_FixedString layerPositionCmd();
    static omn_FixedString spacingAngleCmd();
    static omn_FixedString extensionRatioCmd();
    static omn_FixedString radiusCmd();
    static omn_FixedString layerNameCmd();
    static omn_FixedString materialCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kseC_LayerProperties* Copy() const;


    // ODB database interface
    kseC_LayerProperties(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    double barArea;
    cow_String orientationAngleString;
    double orientationAngleDouble;
    double barSpacing;
    double layerPosition;
    double spacingAngle;
    double extensionRatio;
    double radius;
    cow_String layerName;
    cow_String material;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kseC_LayerProperties, cow_Virtual);

// Default methods implementation
inline double
kseC_LayerProperties::barSpacingDef()
{
    return 0;
}

inline double
kseC_LayerProperties::layerPositionDef()
{
    return 0;
}

inline double
kseC_LayerProperties::spacingAngleDef()
{
    return 0;
}

inline double
kseC_LayerProperties::extensionRatioDef()
{
    return 0;
}

inline double
kseC_LayerProperties::radiusDef()
{
    return 0;
}

#endif // kseC_LayerProperties_h
