//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorPotential_h
#define kbeC_ConnectorPotential_h
///////////////////////////////////////////////////////////////////////////////
// kbeC_ConnectorPotential
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <kbeC_DerivedComponent.h>
#include <omu_PrimEnum.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbeC_ConnectorPotential : public mem_AllocationOperators
{
public:
    kbeC_ConnectorPotential();
    // Enumerations
    enum ComponentStyleEnm {
        COMPONENT_NUMBER,
        DERIVED_COMPONENT
    };
    enum SignEnm {
        POSITIVE,
        NEGATIVE
    };
    enum HFunctionEnm {
        ABS,
        MACAULEY,
        IDENTITY
    };
    // Methods
    virtual void AssertValid() const;
    bool operator==(const kbeC_ConnectorPotential& ) const;
    bool operator!=(const kbeC_ConnectorPotential& ) const;

    // Default methods
    static inline ComponentStyleEnm componentStyleDef();
    static inline int componentNumberDef();
    static inline SignEnm signDef();
    static inline double scaleFactorDef();
    static inline double positiveExponentDef();
    static inline double shiftFactorDef();
    static inline HFunctionEnm hFunctionDef();
    // command string methods
    static omn_FixedString componentStyleCmd();
    static omn_FixedString componentNumberCmd();
    static omn_FixedString signCmd();
    static omn_FixedString scaleFactorCmd();
    static omn_FixedString positiveExponentCmd();
    static omn_FixedString shiftFactorCmd();
    static omn_FixedString hFunctionCmd();
    static omn_FixedString derivedComponentCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kbeC_ConnectorPotential* Copy() const;


    // ODB database interface
    kbeC_ConnectorPotential(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    ComponentStyleEnm componentStyle;
    int componentNumber;
    SignEnm sign;
    double scaleFactor;
    double positiveExponent;
    double shiftFactor;
    HFunctionEnm hFunction;
    kbeC_DerivedComponentCOW derivedComponent;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbeC_ConnectorPotential, cow_Virtual);

typedef omu_PrimEnum<kbeC_ConnectorPotential::ComponentStyleEnm,1> kbeC_ConnectorPotentialComponentStyleConst;
typedef omu_PrimEnum<kbeC_ConnectorPotential::SignEnm,1> kbeC_ConnectorPotentialSignConst;
typedef omu_PrimEnum<kbeC_ConnectorPotential::HFunctionEnm,1> kbeC_ConnectorPotentialHFunctionConst;

// Default methods implementation
inline kbeC_ConnectorPotential::ComponentStyleEnm
kbeC_ConnectorPotential::componentStyleDef()
{
    return kbeC_ConnectorPotential::COMPONENT_NUMBER;
}

inline int
kbeC_ConnectorPotential::componentNumberDef()
{
    return 0;
}

inline kbeC_ConnectorPotential::SignEnm
kbeC_ConnectorPotential::signDef()
{
    return kbeC_ConnectorPotential::POSITIVE;
}

inline double
kbeC_ConnectorPotential::scaleFactorDef()
{
    return 1.0;
}

inline double
kbeC_ConnectorPotential::positiveExponentDef()
{
    return 2.0;
}

inline double
kbeC_ConnectorPotential::shiftFactorDef()
{
    return 0.0;
}

inline kbeC_ConnectorPotential::HFunctionEnm
kbeC_ConnectorPotential::hFunctionDef()
{
    return kbeC_ConnectorPotential::ABS;
}

#endif // kbeC_ConnectorPotential_h
