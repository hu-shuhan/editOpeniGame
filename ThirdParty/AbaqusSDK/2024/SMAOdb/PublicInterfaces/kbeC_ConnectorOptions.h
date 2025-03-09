//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorOptions_h
#define kbeC_ConnectorOptions_h
///////////////////////////////////////////////////////////////////////////////
// kbeC_ConnectorOptions
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <omu_PrimEnum.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbeC_ConnectorOptions : public mem_AllocationOperators
{
public:
    kbeC_ConnectorOptions();
    // Enumerations
    enum InterpolationEnm {
        LINEAR,
        LOGARITHMIC
    };
    enum ExtrapolationEnm {
        CONSTANT_1,
        LINEAR_1
    };
    // Methods
    virtual void AssertValid() const;
    bool operator==(const kbeC_ConnectorOptions& ) const;
    bool operator!=(const kbeC_ConnectorOptions& ) const;

    // Default methods
    static inline bool useBehRegSettingsDef();
    static inline bool regularizeDef();
    static inline bool defaultToleranceDef();
    static inline double regularizationDef();
    static inline bool defaultRateFactorDef();
    static inline double rateFactorDef();
    static inline InterpolationEnm interpolationDef();
    static inline bool useBehExtSettingsDef();
    static inline ExtrapolationEnm extrapolationDef();
    // command string methods
    static omn_FixedString useBehRegSettingsCmd();
    static omn_FixedString regularizeCmd();
    static omn_FixedString defaultToleranceCmd();
    static omn_FixedString regularizationCmd();
    static omn_FixedString defaultRateFactorCmd();
    static omn_FixedString rateFactorCmd();
    static omn_FixedString interpolationCmd();
    static omn_FixedString useBehExtSettingsCmd();
    static omn_FixedString extrapolationCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kbeC_ConnectorOptions* Copy() const;


    // ODB database interface
    kbeC_ConnectorOptions(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    bool useBehRegSettings;
    bool regularize;
    bool defaultTolerance;
    double regularization;
    bool defaultRateFactor;
    double rateFactor;
    InterpolationEnm interpolation;
    bool useBehExtSettings;
    ExtrapolationEnm extrapolation;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbeC_ConnectorOptions, cow_Virtual);

typedef omu_PrimEnum<kbeC_ConnectorOptions::InterpolationEnm,1> kbeC_ConnectorOptionsInterpolationConst;
typedef omu_PrimEnum<kbeC_ConnectorOptions::ExtrapolationEnm,1> kbeC_ConnectorOptionsExtrapolationConst;

// Default methods implementation
inline bool
kbeC_ConnectorOptions::useBehRegSettingsDef()
{
    return true;
}

inline bool
kbeC_ConnectorOptions::regularizeDef()
{
    return true;
}

inline bool
kbeC_ConnectorOptions::defaultToleranceDef()
{
    return true;
}

inline double
kbeC_ConnectorOptions::regularizationDef()
{
    return 0.03;
}

inline bool
kbeC_ConnectorOptions::defaultRateFactorDef()
{
    return true;
}

inline double
kbeC_ConnectorOptions::rateFactorDef()
{
    return 0.9;
}

inline kbeC_ConnectorOptions::InterpolationEnm
kbeC_ConnectorOptions::interpolationDef()
{
    return kbeC_ConnectorOptions::LINEAR;
}

inline bool
kbeC_ConnectorOptions::useBehExtSettingsDef()
{
    return true;
}

inline kbeC_ConnectorOptions::ExtrapolationEnm
kbeC_ConnectorOptions::extrapolationDef()
{
    return kbeC_ConnectorOptions::CONSTANT_1;
}

#endif // kbeC_ConnectorOptions_h
