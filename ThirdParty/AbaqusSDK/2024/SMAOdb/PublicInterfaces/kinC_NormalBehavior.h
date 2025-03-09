//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kinC_NormalBehavior_h
#define kinC_NormalBehavior_h
///////////////////////////////////////////////////////////////////////////////
//
// File Name: kinC_NormalBehavior
//
// Description: Engineering object class for kinC_NormalBehavior
//

// Includes
#include <float.h>

// Begin local includes

#include <kinC_IPOption.h>
#include <typ_typeTag.h>
#include <cow_COW.h>
#include <omu_PrimEnum.h>

#include <cow_ListDouble.h>

#include <cls_xp2DArrayDouble.h>

// Forward declarations
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

// Class definition

class kinC_NormalBehavior: public kinC_IPOption
{
  public:

    kinC_NormalBehavior();
    virtual ~kinC_NormalBehavior();
    virtual kinC_IPOption* Copy() const;

    // Enums

    enum PressureOverclosureEnm {HARD, EXPONENTIAL, LINEAR, TABULAR, SCALE_FACTOR};
    enum DefaultEnm {DEFAULT};
    enum EnforcementMethodEnm {ANALYSIS_PRODUCT_DEFAULT, AUGMENTED_LAGRANGE,
	PENALTY, DIRECT};
    enum StiffnessBehaviorEnm {LINEAR_STIFFNESS, NONLINEAR_STIFFNESS};

    // Services

    virtual void AssertValid() const;
    bool operator == (const kinC_NormalBehavior&) const;

    // Data members

    PressureOverclosureEnm pressureOverclosure;
    bool allowSeparation;
    StiffnessBehaviorEnm stiffnessBehavior;
    double maxStiffness;
    double contactStiffness;
    cls_xp2DArrayDouble table;
    EnforcementMethodEnm constraintEnforcementMethod;
    double overclosureFactor;
    double overclosureMeasure;
    double contactStiffnessScaleFactor;
    double initialStiffnessScaleFactor;
    double clearanceAtZeroContactPressure;
    double stiffnessRatio;
    double upperQuadraticFactor;
    double lowerQuadraticRatio;

    // Static data members

    static omn_FixedString constructorCmd();
    static omn_FixedString setParametersCmd();

    static omn_FixedString pressureOverclosureCmd();
    static omn_FixedString allowSeparationCmd();
    static omn_FixedString stiffnessBehaviorCmd();
    static omn_FixedString maxStiffnessCmd();
    static omn_FixedString contactStiffnessCmd();
    static omn_FixedString tableCmd();
    static omn_FixedString constraintEnforcementMethodCmd();
    static omn_FixedString overclosureFactorCmd();
    static omn_FixedString overclosureMeasureCmd();
    static omn_FixedString contactStiffnessScaleFactorCmd();
    static omn_FixedString initialStiffnessScaleFactorCmd();
    static omn_FixedString clearanceAtZeroContactPressureCmd();
    static omn_FixedString stiffnessRatioCmd();
    static omn_FixedString upperQuadraticFactorCmd();
    static omn_FixedString lowerQuadraticRatioCmd();

    static inline EnforcementMethodEnm constraintEnforcementMethodDef();
    static inline double noStiffness();
    static inline double maxStiffnessDef();
    static inline StiffnessBehaviorEnm stiffnessBehaviorDef();
    static inline double overclosureFactorDef();
    static inline double overclosureMeasureDef();
    static inline double contactStiffnessScaleFactorDef();
    static inline double initialStiffnessScaleFactorDef();
    static inline double clearanceAtZeroContactPressureDef();
    static inline double stiffnessRatioDef();
    static inline double upperQuadraticFactorDef();
    static inline double lowerQuadraticRatioDef();

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;

    static typ_typeTag      TypeId();



    kinC_NormalBehavior *             DeepCopy() const
    { return (kinC_NormalBehavior*)(this->kinC_IPOption::DeepCopy()); }


    // DDB persistence
    kinC_NormalBehavior( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

private:
    cls_Uid m_ClsUid;
};

COW_COW_DECL(kinC_NormalBehavior);

// Map to symbolic constant

typedef omu_PrimEnum<kinC_NormalBehavior::PressureOverclosureEnm,1> kinC_NormalBehaviorPressureOverclosureConst;
typedef omu_PrimEnum<kinC_NormalBehavior::DefaultEnm,1> kinC_NormalBehaviorDefaultConst;
typedef omu_PrimEnum<kinC_NormalBehavior::EnforcementMethodEnm,1> kinC_NormalBehaviorMethodConst;
typedef omu_PrimEnum<kinC_NormalBehavior::StiffnessBehaviorEnm,1> kinC_NormalBehaviorStiffnessConst;


inline kinC_NormalBehavior::StiffnessBehaviorEnm
kinC_NormalBehavior::stiffnessBehaviorDef()
{
    return kinC_NormalBehavior::LINEAR_STIFFNESS;
}

inline double kinC_NormalBehavior::maxStiffnessDef()
{
    return DBL_MAX;
}
inline double kinC_NormalBehavior::noStiffness()
{
    return 0.;
}
inline kinC_NormalBehavior::EnforcementMethodEnm kinC_NormalBehavior::constraintEnforcementMethodDef()
{
  return kinC_NormalBehavior::ANALYSIS_PRODUCT_DEFAULT;
}
inline double kinC_NormalBehavior::overclosureFactorDef()
{
  return 0.;
}
inline double kinC_NormalBehavior::overclosureMeasureDef()
{
  return 0.;
}
inline double kinC_NormalBehavior::contactStiffnessScaleFactorDef()
{
  // For augmented Lagrange, 
  // this has a default of 1, and the value must be > 0
  //
  // When pressureOverclosure=SCALE_FACTOR,
  // this has no default, and the value must be > 1.
  return 1.;
}
inline double kinC_NormalBehavior::initialStiffnessScaleFactorDef()
{
  return 1.;
}
inline double kinC_NormalBehavior::clearanceAtZeroContactPressureDef()
{
  return 0.;
}
inline double kinC_NormalBehavior::stiffnessRatioDef()
{
    return 0.01;
}
inline double kinC_NormalBehavior::upperQuadraticFactorDef()
{
    return 0.03;
}
inline double kinC_NormalBehavior::lowerQuadraticRatioDef()
{
    return 0.33333;
}
#endif
