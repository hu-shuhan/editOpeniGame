//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyOtherC_h
#define ads_CorePropertyOtherC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyOther of the latest level of form Core */

#define ads_CMecDampingStructuralCoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 0))

#define ads_CMecDampingStructuralRotationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 1))

#define ads_CMecDampingStructuralTranslationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 2))

#define ads_CMecDashpotLinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 3))

/** DashpotForce may actually be a force or moment; */
#define ads_CMecDashpotNonlinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 4))

#define ads_CMecFrictionForceExpDecayTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 5))

#define ads_CMecFrictionForceTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 6))

#define ads_CMecFrictionMomentExpDecayTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 7))

#define ads_CMecFrictionMomentTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 8))

#define ads_CMecFrictionPredefinedTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 9))

#define ads_CMecFrictionUserDefinedTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 10))

#define ads_CMecSpringLinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 11))

/** SpringForce may actually be a force or moment; RelDisp is the difference between the two nodal DOFs, and hence may be a relative displacement, relative rotation, or even a difference between a displacement and rotation (yes, this is allowed by SPRING2). */
#define ads_CMecSpringNonlinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 12))

#define ads_IMecAcousticAdmittanceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 13))

#define ads_IMecAcousticImpedanceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 14))

#define ads_IMecCohesiveBehaviorCoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 15))

#define ads_IMecCohesiveBehaviorUnCoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 16))

#define ads_IMecDampingBetaClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 17))

#define ads_IMecExchangeBulkViscosityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 18))

#define ads_IMecExchangeEnergyFluxTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 19))

#define ads_IMecExchangeEnergyRateLeakageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 20))

#define ads_IMecExchangeFabricLeakageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 21))

#define ads_IMecExchangeMassFluxTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 22))

#define ads_IMecExchangeMassRateLeakageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 23))

#define ads_IMecExchangeOrificeTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 24))

#define ads_IMecExchangeVolumeFluxTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 25))

#define ads_IMecExchangeVoumeRateLeakageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 26))

#define ads_IMecFrictionAnisotropicTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 27))

#define ads_IMecFrictionExpDecayTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 28))

#define ads_IMecFrictionExponentialDecayTestDataTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 29))

#define ads_IMecFrictionIsotropicTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 30))

#define ads_IMecNormalbehaviorTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 31))

#define ads_IThermalCureHeatGenerationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 32))

#define ads_MMecCureMaxConversionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 33))

#define ads_MMecCureShrinkageAnisoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 34))

#define ads_MMecCureShrinkageIsoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 35))

#define ads_MMecCureShrinkageOrthoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 36))

#define ads_MMecCureShrinkageVolTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 37))

#define ads_MMecDampingFactorsAlphaBetaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 38))

#define ads_MMecKamalCureKineticsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 39))

#define ads_MMecTabularCureKineticsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 40))

#define ads_Prop_CMec_Damping (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 41))

#define ads_Prop_CMec_Damping_Structural (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 42))

#define ads_Prop_CMec_Damping_Structural_Coupled (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 43))

#define ads_Prop_CMec_Damping_Structural_Coupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 44))

#define ads_Prop_CMec_Damping_Structural_RotationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 45))

#define ads_Prop_CMec_Damping_Structural_RotationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 46))

#define ads_Prop_CMec_Damping_Structural_TranslationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 47))

#define ads_Prop_CMec_Damping_Structural_TranslationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 48))

/** Data type to capture the degrees of freedom and the regularization tolerance. */
#define ads_Prop_CMec_Dashpot (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 49))

/** Data type to capture linear behavior of dashpots. */
#define ads_Prop_CMec_Dashpot_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 50))

#define ads_Prop_CMec_Dashpot_Linear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 51))

/** Data type to capture nonlinear behavior of dashpots. */
#define ads_Prop_CMec_Dashpot_Nonlinear (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 52))

#define ads_Prop_CMec_Dashpot_Nonlinear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 53))

/** 0..1 Reference to a permuted set of 1 or 2 DofTypes (no ref for DASHPOTA; 1 DofType for DASHPOT1 and 2 for DASHPOT2) */
#define ads_Prop_CMec_Dashpot_dofs (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 54))

#define ads_Prop_CMec_Friction (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 55))

#define ads_Prop_CMec_Friction_ForceExpDecay (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 56))

#define ads_Prop_CMec_Friction_ForceExpDecay_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 57))

#define ads_Prop_CMec_Friction_ForceTabular (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 58))

#define ads_Prop_CMec_Friction_ForceTabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 59))

#define ads_Prop_CMec_Friction_MomentExpDecay (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 60))

#define ads_Prop_CMec_Friction_MomentExpDecay_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 61))

#define ads_Prop_CMec_Friction_MomentTabular (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 62))

#define ads_Prop_CMec_Friction_MomentTabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 63))

/** specify predefined friction interaction in a connector. */
#define ads_Prop_CMec_Friction_Predefined (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 64))

#define ads_Prop_CMec_Friction_Predefined_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 65))

#define ads_Prop_CMec_Friction_UserDefined (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 66))

#define ads_Prop_CMec_Friction_UserDefined_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 67))

/** Connector plasticity is used to model plastic/irreversible deformation of parts forming a connecting device. */
#define ads_Prop_CMec_Plasticity (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 68))

/** Connector potentials are user-defined mathematical functions that represent yield surfaces, limiting surfaces, or magnitude measures in the space spanned by the components of relative motion in the connector. The functions can be quadratic, general elliptical, or maximum norms. The connector potential does not define a connector behavior by itself; instead, it is used to define select coupled connector behaviors. */
#define ads_Prop_CMec_Potential (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 69))

/** Data type to capture the degrees of freedom and the regularization tolerance. */
#define ads_Prop_CMec_Spring (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 70))

/** Data type to capture linear behavior of springs. */
#define ads_Prop_CMec_Spring_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 71))

#define ads_Prop_CMec_Spring_Linear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 72))

/** Data type to capture nonlinear behavior of springs. */
#define ads_Prop_CMec_Spring_Nonlinear (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 73))

#define ads_Prop_CMec_Spring_Nonlinear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 74))

/** 0..1 Reference to a permuted set of 1 or 2 DofTypes (no ref for SPRINGA; 1 DofType for SPRING1 and 2 for SPRING2) */
#define ads_Prop_CMec_Spring_dofs (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 75))

/** Specify connector stop for connector elements. */
#define ads_Prop_CMec_Stop (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 76))

#define ads_Prop_IMec_Acoustic (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 77))

#define ads_Prop_IMec_Acoustic_Admittance (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 78))

#define ads_Prop_IMec_Acoustic_Admittance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 79))

#define ads_Prop_IMec_Acoustic_Impedance (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 80))

#define ads_Prop_IMec_Acoustic_Impedance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 81))

#define ads_Prop_IMec_CohesiveBehavior (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 82))

#define ads_Prop_IMec_CohesiveBehaviorCoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 83))

#define ads_Prop_IMec_CohesiveBehaviorCoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 84))

#define ads_Prop_IMec_CohesiveBehaviorUnCoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 85))

#define ads_Prop_IMec_CohesiveBehaviorUnCoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 86))

#define ads_Prop_IMec_Damping (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 87))

#define ads_Prop_IMec_Damping_Beta (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 88))

#define ads_Prop_IMec_Damping_Beta_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 89))

#define ads_Prop_IMec_Damping_Beta_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 90))

#define ads_Prop_IMec_Exchange (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 91))

#define ads_Prop_IMec_Exchange_BulkViscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 92))

#define ads_Prop_IMec_Exchange_BulkViscosity_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 93))

#define ads_Prop_IMec_Exchange_EnergyFlux (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 94))

#define ads_Prop_IMec_Exchange_EnergyFlux_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 95))

#define ads_Prop_IMec_Exchange_EnergyRateLeakage (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 96))

#define ads_Prop_IMec_Exchange_EnergyRateLeakage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 97))

#define ads_Prop_IMec_Exchange_FabricLeakage (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 98))

#define ads_Prop_IMec_Exchange_FabricLeakage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 99))

#define ads_Prop_IMec_Exchange_MassFlux (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 100))

#define ads_Prop_IMec_Exchange_MassFlux_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 101))

#define ads_Prop_IMec_Exchange_MassRateLeakage (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 102))

#define ads_Prop_IMec_Exchange_MassRateLeakage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 103))

#define ads_Prop_IMec_Exchange_Orifice (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 104))

#define ads_Prop_IMec_Exchange_Orifice_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 105))

#define ads_Prop_IMec_Exchange_User (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 106))

#define ads_Prop_IMec_Exchange_VolumeFlux (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 107))

#define ads_Prop_IMec_Exchange_VolumeFlux_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 108))

#define ads_Prop_IMec_Exchange_VoumeRateLeakage (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 109))

#define ads_Prop_IMec_Exchange_VoumeRateLeakage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 110))

#define ads_Prop_IMec_Friction (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 111))

#define ads_Prop_IMec_Friction_AnisotropicTabular (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 112))

#define ads_Prop_IMec_Friction_AnisotropicTabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 113))

#define ads_Prop_IMec_Friction_ExpDecay (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 114))

#define ads_Prop_IMec_Friction_ExpDecay_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 115))

#define ads_Prop_IMec_Friction_ExponentialDecayTestData (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 116))

#define ads_Prop_IMec_Friction_ExponentialDecayTestData_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 117))

#define ads_Prop_IMec_Friction_IsotropicTabular (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 118))

#define ads_Prop_IMec_Friction_IsotropicTabular_Special (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 119))

/** Velocity vector is implemented as a special "quick fix" and is not officially documented. It is used to allow for prescribing steady-state relative motion of a planar rigid surface in an otherwise Lagrangian analysis. This is similar to *Motion, type=VELOCITY. *Motion offers more generality than a velocity vector on *Friction, and should be the preferred approach. */
#define ads_Prop_IMec_Friction_IsotropicTabular_Special_rigidSurfaceVelocity (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 120))

#define ads_Prop_IMec_Friction_IsotropicTabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 121))

#define ads_Prop_IMec_Friction_Rough (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 122))

#define ads_Prop_IMec_Friction_User (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 123))

#define ads_Prop_IMec_Normalbehavior (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 124))

#define ads_Prop_IMec_Normalbehavior_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 125))

/** Cure volumetric heat generation. */
#define ads_Prop_IThermal_CureHeatGeneration (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 126))

#define ads_Prop_IThermal_CureHeatGeneration_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 127))

#define ads_Prop_MMec_CureKinetics (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 128))

#define ads_Prop_MMec_CureKinetics_Kamal (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 129))

#define ads_Prop_MMec_CureKinetics_Kamal_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 130))

#define ads_Prop_MMec_CureKinetics_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 131))

#define ads_Prop_MMec_CureKinetics_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 132))

#define ads_Prop_MMec_CureMaxConversion (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 133))

#define ads_Prop_MMec_CureMaxConversion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 134))

/** Cure volumetric heat generation. */
#define ads_Prop_MMec_CureShrinkage (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 135))

/** Cure Shrinkage, aniso. */
#define ads_Prop_MMec_CureShrinkage_Aniso (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 136))

#define ads_Prop_MMec_CureShrinkage_Aniso_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 137))

/** Cure Shrinkage, iso. */
#define ads_Prop_MMec_CureShrinkage_Iso (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 138))

#define ads_Prop_MMec_CureShrinkage_Iso_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 139))

/** Cure Shrinkage, ortho. */
#define ads_Prop_MMec_CureShrinkage_Ortho (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 140))

#define ads_Prop_MMec_CureShrinkage_Ortho_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 141))

/** Cure Shrinkage, vol. */
#define ads_Prop_MMec_CureShrinkage_Vol (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 142))

#define ads_Prop_MMec_CureShrinkage_Vol_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 143))

#define ads_Prop_MMec_Damping (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 144))

/** Specify material damping. */
#define ads_Prop_MMec_Damping_Factors (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 145))

/** Applicable to some modal events. */
#define ads_Prop_MMec_Damping_Factors_dampingControl (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 146))

#define ads_Prop_MMec_Damping_Factors_table (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 147))

#define ads_Prop_MMec_Fabric (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 148))

#define ads_Prop_MMec_Fabric_User (ads_CoreFragmentTypeIndex(ads_CorePropertyOtherFragment, 149))

/** Enum with association roles. */
enum ads_Prop_CMec_Damping_Structural_Coupled_tableRolesEnm
{
    ads_Prop_CMec_Damping_Structural_Coupled_table_child,
    ads_Prop_CMec_Damping_Structural_Coupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Damping_Structural_RotationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Damping_Structural_RotationalUncoupled_table_child,
    ads_Prop_CMec_Damping_Structural_RotationalUncoupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Damping_Structural_TranslationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Damping_Structural_TranslationalUncoupled_table_child,
    ads_Prop_CMec_Damping_Structural_TranslationalUncoupled_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_DashpotMembersEnm
{
    ads_Prop_CMec_Dashpot_rtol
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Dashpot_LinearMembersEnm
{
    ads_Prop_CMec_Dashpot_Linear_rtol
};

/** Enum with association roles. */
enum ads_Prop_CMec_Dashpot_Linear_tableRolesEnm
{
    ads_Prop_CMec_Dashpot_Linear_table_child,
    ads_Prop_CMec_Dashpot_Linear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Dashpot_NonlinearMembersEnm
{
    ads_Prop_CMec_Dashpot_Nonlinear_rtol
};

/** Enum with association roles. */
enum ads_Prop_CMec_Dashpot_Nonlinear_tableRolesEnm
{
    ads_Prop_CMec_Dashpot_Nonlinear_table_child,
    ads_Prop_CMec_Dashpot_Nonlinear_table_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Dashpot_dofsRolesEnm
{
    ads_Prop_CMec_Dashpot_dofs_referent,
    ads_Prop_CMec_Dashpot_dofs_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_ForceExpDecay_tableRolesEnm
{
    ads_Prop_CMec_Friction_ForceExpDecay_table_child,
    ads_Prop_CMec_Friction_ForceExpDecay_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_ForceTabular_tableRolesEnm
{
    ads_Prop_CMec_Friction_ForceTabular_table_child,
    ads_Prop_CMec_Friction_ForceTabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_MomentExpDecay_tableRolesEnm
{
    ads_Prop_CMec_Friction_MomentExpDecay_table_child,
    ads_Prop_CMec_Friction_MomentExpDecay_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_MomentTabular_tableRolesEnm
{
    ads_Prop_CMec_Friction_MomentTabular_table_child,
    ads_Prop_CMec_Friction_MomentTabular_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Friction_PredefinedMembersEnm
{
    ads_Prop_CMec_Friction_Predefined_stickStiffness,
    ads_Prop_CMec_Friction_Predefined_stickStiffnessEnm
};

enum ads_Prop_CMec_Friction_Predefined_stickStiffnessEnmEnm
{
    ads_Prop_CMec_Friction_Predefined_stickStiffnessEnm_CALCULATED,
    ads_Prop_CMec_Friction_Predefined_stickStiffnessEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_Predefined_tableRolesEnm
{
    ads_Prop_CMec_Friction_Predefined_table_child,
    ads_Prop_CMec_Friction_Predefined_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Friction_UserDefinedMembersEnm
{
    ads_Prop_CMec_Friction_UserDefined_indepCompEnm,
    ads_Prop_CMec_Friction_UserDefined_stickStiffness,
    ads_Prop_CMec_Friction_UserDefined_stickStiffnessEnm
};

enum ads_Prop_CMec_Friction_UserDefined_indepCompEnmEnm
{
    ads_Prop_CMec_Friction_UserDefined_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Friction_UserDefined_indepCompEnm_NONE,
    ads_Prop_CMec_Friction_UserDefined_indepCompEnm_POSITION
};

enum ads_Prop_CMec_Friction_UserDefined_stickStiffnessEnmEnm
{
    ads_Prop_CMec_Friction_UserDefined_stickStiffnessEnm_CALCULATED,
    ads_Prop_CMec_Friction_UserDefined_stickStiffnessEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_UserDefined_tableRolesEnm
{
    ads_Prop_CMec_Friction_UserDefined_table_child,
    ads_Prop_CMec_Friction_UserDefined_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_PotentialMembersEnm
{
    ads_Prop_CMec_Potential_beta,
    ads_Prop_CMec_Potential_operator
};

enum ads_Prop_CMec_Potential_operatorEnm
{
    ads_Prop_CMec_Potential_operator_MAX,
    ads_Prop_CMec_Potential_operator_SUM
};

/** 
Enum with record members. */
enum ads_Prop_CMec_SpringMembersEnm
{
    ads_Prop_CMec_Spring_rtol
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Spring_LinearMembersEnm
{
    ads_Prop_CMec_Spring_Linear_rtol
};

/** Enum with association roles. */
enum ads_Prop_CMec_Spring_Linear_tableRolesEnm
{
    ads_Prop_CMec_Spring_Linear_table_child,
    ads_Prop_CMec_Spring_Linear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Spring_NonlinearMembersEnm
{
    ads_Prop_CMec_Spring_Nonlinear_rtol
};

/** Enum with association roles. */
enum ads_Prop_CMec_Spring_Nonlinear_tableRolesEnm
{
    ads_Prop_CMec_Spring_Nonlinear_table_child,
    ads_Prop_CMec_Spring_Nonlinear_table_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Spring_dofsRolesEnm
{
    ads_Prop_CMec_Spring_dofs_referent,
    ads_Prop_CMec_Spring_dofs_referrer
};

/** Enum with association roles. */
enum ads_Prop_IMec_Acoustic_Admittance_tableRolesEnm
{
    ads_Prop_IMec_Acoustic_Admittance_table_child,
    ads_Prop_IMec_Acoustic_Admittance_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Acoustic_Impedance_tableRolesEnm
{
    ads_Prop_IMec_Acoustic_Impedance_table_child,
    ads_Prop_IMec_Acoustic_Impedance_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_CohesiveBehaviorMembersEnm
{
    ads_Prop_IMec_CohesiveBehavior_cohere,
    ads_Prop_IMec_CohesiveBehavior_eligibility,
    ads_Prop_IMec_CohesiveBehavior_repeatedContacts
};

enum ads_Prop_IMec_CohesiveBehavior_cohereEnm
{
    ads_Prop_IMec_CohesiveBehavior_cohere_FIRSTCONTACTS,
    ads_Prop_IMec_CohesiveBehavior_cohere_NONE,
    ads_Prop_IMec_CohesiveBehavior_cohere_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehavior_cohere_REPEATEDCONTACTS
};

enum ads_Prop_IMec_CohesiveBehavior_eligibilityEnm
{
    ads_Prop_IMec_CohesiveBehavior_eligibility_CURRENTCONTACTS,
    ads_Prop_IMec_CohesiveBehavior_eligibility_NONE,
    ads_Prop_IMec_CohesiveBehavior_eligibility_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehavior_eligibility_SPECIFIEDCONTACTS
};

/** Enum with record members. */
enum ads_Prop_IMec_CohesiveBehaviorCoupledMembersEnm
{
    ads_Prop_IMec_CohesiveBehaviorCoupled_cohere,
    ads_Prop_IMec_CohesiveBehaviorCoupled_eligibility,
    ads_Prop_IMec_CohesiveBehaviorCoupled_repeatedContacts
};

enum ads_Prop_IMec_CohesiveBehaviorCoupled_cohereEnm
{
    ads_Prop_IMec_CohesiveBehaviorCoupled_cohere_FIRSTCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorCoupled_cohere_NONE,
    ads_Prop_IMec_CohesiveBehaviorCoupled_cohere_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorCoupled_cohere_REPEATEDCONTACTS
};

enum ads_Prop_IMec_CohesiveBehaviorCoupled_eligibilityEnm
{
    ads_Prop_IMec_CohesiveBehaviorCoupled_eligibility_CURRENTCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorCoupled_eligibility_NONE,
    ads_Prop_IMec_CohesiveBehaviorCoupled_eligibility_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorCoupled_eligibility_SPECIFIEDCONTACTS
};

/** Enum with association roles. */
enum ads_Prop_IMec_CohesiveBehaviorCoupled_tableRolesEnm
{
    ads_Prop_IMec_CohesiveBehaviorCoupled_table_child,
    ads_Prop_IMec_CohesiveBehaviorCoupled_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_CohesiveBehaviorUnCoupledMembersEnm
{
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohere,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibility,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_repeatedContacts
};

enum ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohereEnm
{
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohere_FIRSTCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohere_NONE,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohere_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_cohere_REPEATEDCONTACTS
};

enum ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibilityEnm
{
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibility_CURRENTCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibility_NONE,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibility_ORIGINALCONTACTS,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_eligibility_SPECIFIEDCONTACTS
};

/** Enum with association roles. */
enum ads_Prop_IMec_CohesiveBehaviorUnCoupled_tableRolesEnm
{
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_table_child,
    ads_Prop_IMec_CohesiveBehaviorUnCoupled_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Damping_Beta_ClosureMembersEnm
{
    ads_Prop_IMec_Damping_Beta_Closure_tangentFraction
};

/** Enum with association roles. */
enum ads_Prop_IMec_Damping_Beta_Closure_tableRolesEnm
{
    ads_Prop_IMec_Damping_Beta_Closure_table_child,
    ads_Prop_IMec_Damping_Beta_Closure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_BulkViscosity_tableRolesEnm
{
    ads_Prop_IMec_Exchange_BulkViscosity_table_child,
    ads_Prop_IMec_Exchange_BulkViscosity_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_EnergyFlux_tableRolesEnm
{
    ads_Prop_IMec_Exchange_EnergyFlux_table_child,
    ads_Prop_IMec_Exchange_EnergyFlux_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_EnergyRateLeakage_tableRolesEnm
{
    ads_Prop_IMec_Exchange_EnergyRateLeakage_table_child,
    ads_Prop_IMec_Exchange_EnergyRateLeakage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_FabricLeakage_tableRolesEnm
{
    ads_Prop_IMec_Exchange_FabricLeakage_table_child,
    ads_Prop_IMec_Exchange_FabricLeakage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_MassFlux_tableRolesEnm
{
    ads_Prop_IMec_Exchange_MassFlux_table_child,
    ads_Prop_IMec_Exchange_MassFlux_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_MassRateLeakage_tableRolesEnm
{
    ads_Prop_IMec_Exchange_MassRateLeakage_table_child,
    ads_Prop_IMec_Exchange_MassRateLeakage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_Orifice_tableRolesEnm
{
    ads_Prop_IMec_Exchange_Orifice_table_child,
    ads_Prop_IMec_Exchange_Orifice_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Exchange_UserMembersEnm
{
    ads_Prop_IMec_Exchange_User_depvar,
    ads_Prop_IMec_Exchange_User_numConstants
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_VolumeFlux_tableRolesEnm
{
    ads_Prop_IMec_Exchange_VolumeFlux_table_child,
    ads_Prop_IMec_Exchange_VolumeFlux_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Exchange_VoumeRateLeakage_tableRolesEnm
{
    ads_Prop_IMec_Exchange_VoumeRateLeakage_table_child,
    ads_Prop_IMec_Exchange_VoumeRateLeakage_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_FrictionMembersEnm
{
    ads_Prop_IMec_Friction_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_elasticSlip,
    ads_Prop_IMec_Friction_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_elasticSlipStiffness,
    ads_Prop_IMec_Friction_elasticSlipVelocity,
    ads_Prop_IMec_Friction_taumax
};

enum ads_Prop_IMec_Friction_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_constraintEnforcementMethod_PENALTY
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_AnisotropicTabularMembersEnm
{
    ads_Prop_IMec_Friction_AnisotropicTabular_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_AnisotropicTabular_elasticSlip,
    ads_Prop_IMec_Friction_AnisotropicTabular_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_AnisotropicTabular_elasticSlipStiffness,
    ads_Prop_IMec_Friction_AnisotropicTabular_elasticSlipVelocity,
    ads_Prop_IMec_Friction_AnisotropicTabular_taumax
};

enum ads_Prop_IMec_Friction_AnisotropicTabular_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_AnisotropicTabular_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_AnisotropicTabular_constraintEnforcementMethod_PENALTY
};

/** Enum with association roles. */
enum ads_Prop_IMec_Friction_AnisotropicTabular_tableRolesEnm
{
    ads_Prop_IMec_Friction_AnisotropicTabular_table_child,
    ads_Prop_IMec_Friction_AnisotropicTabular_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_ExpDecayMembersEnm
{
    ads_Prop_IMec_Friction_ExpDecay_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_ExpDecay_elasticSlip,
    ads_Prop_IMec_Friction_ExpDecay_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_ExpDecay_elasticSlipStiffness,
    ads_Prop_IMec_Friction_ExpDecay_elasticSlipVelocity,
    ads_Prop_IMec_Friction_ExpDecay_taumax
};

enum ads_Prop_IMec_Friction_ExpDecay_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_ExpDecay_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_ExpDecay_constraintEnforcementMethod_PENALTY
};

/** Enum with association roles. */
enum ads_Prop_IMec_Friction_ExpDecay_tableRolesEnm
{
    ads_Prop_IMec_Friction_ExpDecay_table_child,
    ads_Prop_IMec_Friction_ExpDecay_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_ExponentialDecayTestDataMembersEnm
{
    ads_Prop_IMec_Friction_ExponentialDecayTestData_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_elasticSlip,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_elasticSlipStiffness,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_elasticSlipVelocity,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_taumax
};

enum ads_Prop_IMec_Friction_ExponentialDecayTestData_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_ExponentialDecayTestData_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_constraintEnforcementMethod_PENALTY
};

/** Enum with association roles. */
enum ads_Prop_IMec_Friction_ExponentialDecayTestData_tableRolesEnm
{
    ads_Prop_IMec_Friction_ExponentialDecayTestData_table_child,
    ads_Prop_IMec_Friction_ExponentialDecayTestData_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_IsotropicTabularMembersEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_IsotropicTabular_elasticSlip,
    ads_Prop_IMec_Friction_IsotropicTabular_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_IsotropicTabular_elasticSlipStiffness,
    ads_Prop_IMec_Friction_IsotropicTabular_elasticSlipVelocity,
    ads_Prop_IMec_Friction_IsotropicTabular_taumax
};

enum ads_Prop_IMec_Friction_IsotropicTabular_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_IsotropicTabular_constraintEnforcementMethod_PENALTY
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_IsotropicTabular_SpecialMembersEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_Special_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_elasticSlip,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_elasticSlipStiffness,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_elasticSlipVelocity,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_taumax
};

enum ads_Prop_IMec_Friction_IsotropicTabular_Special_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_Special_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_constraintEnforcementMethod_PENALTY
};

/** 
Enum with association roles. */
enum ads_Prop_IMec_Friction_IsotropicTabular_Special_rigidSurfaceVelocityRolesEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_Special_rigidSurfaceVelocity_child,
    ads_Prop_IMec_Friction_IsotropicTabular_Special_rigidSurfaceVelocity_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Friction_IsotropicTabular_tableRolesEnm
{
    ads_Prop_IMec_Friction_IsotropicTabular_table_child,
    ads_Prop_IMec_Friction_IsotropicTabular_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_RoughMembersEnm
{
    ads_Prop_IMec_Friction_Rough_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_Rough_elasticSlip,
    ads_Prop_IMec_Friction_Rough_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_Rough_elasticSlipStiffness,
    ads_Prop_IMec_Friction_Rough_elasticSlipVelocity,
    ads_Prop_IMec_Friction_Rough_taumax
};

enum ads_Prop_IMec_Friction_Rough_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_Rough_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_Rough_constraintEnforcementMethod_PENALTY
};

/** Enum with record members. */
enum ads_Prop_IMec_Friction_UserMembersEnm
{
    ads_Prop_IMec_Friction_User_constraintEnforcementMethod,
    ads_Prop_IMec_Friction_User_elasticSlip,
    ads_Prop_IMec_Friction_User_elasticSlipAbsolute,
    ads_Prop_IMec_Friction_User_elasticSlipStiffness,
    ads_Prop_IMec_Friction_User_elasticSlipVelocity,
    ads_Prop_IMec_Friction_User_taumax
};

enum ads_Prop_IMec_Friction_User_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Friction_User_constraintEnforcementMethod_LAGRANGE_MULTIPLIER,
    ads_Prop_IMec_Friction_User_constraintEnforcementMethod_PENALTY
};

/** Enum with record members. */
enum ads_Prop_IMec_NormalbehaviorMembersEnm
{
    ads_Prop_IMec_Normalbehavior_allowSeparation,
    ads_Prop_IMec_Normalbehavior_clearanceAtZeroContactPressure,
    ads_Prop_IMec_Normalbehavior_constraintEnforcementMethod,
    ads_Prop_IMec_Normalbehavior_contactStiffness,
    ads_Prop_IMec_Normalbehavior_contactStiffnessScaleFactor,
    ads_Prop_IMec_Normalbehavior_initialStiffnessScaleFactor,
    ads_Prop_IMec_Normalbehavior_lowerQuadraticRatio,
    ads_Prop_IMec_Normalbehavior_maxStiffness,
    ads_Prop_IMec_Normalbehavior_overclosureFactor,
    ads_Prop_IMec_Normalbehavior_overclosureMeasure,
    ads_Prop_IMec_Normalbehavior_pressureAtZeroClearance,
    ads_Prop_IMec_Normalbehavior_pressureOverclosure,
    ads_Prop_IMec_Normalbehavior_stiffnessBehavior,
    ads_Prop_IMec_Normalbehavior_stiffnessRatio,
    ads_Prop_IMec_Normalbehavior_upperQuadraticFactor
};

enum ads_Prop_IMec_Normalbehavior_constraintEnforcementMethodEnm
{
    ads_Prop_IMec_Normalbehavior_constraintEnforcementMethod_AUGMENTED_LAGRANGE,
    ads_Prop_IMec_Normalbehavior_constraintEnforcementMethod_DEFAULT,
    ads_Prop_IMec_Normalbehavior_constraintEnforcementMethod_DIRECT,
    ads_Prop_IMec_Normalbehavior_constraintEnforcementMethod_PENALTY
};

enum ads_Prop_IMec_Normalbehavior_pressureOverclosureEnm
{
    ads_Prop_IMec_Normalbehavior_pressureOverclosure_EXPONENTIAL,
    ads_Prop_IMec_Normalbehavior_pressureOverclosure_HARD,
    ads_Prop_IMec_Normalbehavior_pressureOverclosure_LINEAR,
    ads_Prop_IMec_Normalbehavior_pressureOverclosure_SCALE_FACTOR,
    ads_Prop_IMec_Normalbehavior_pressureOverclosure_TABULAR
};

enum ads_Prop_IMec_Normalbehavior_stiffnessBehaviorEnm
{
    ads_Prop_IMec_Normalbehavior_stiffnessBehavior_LINEAR,
    ads_Prop_IMec_Normalbehavior_stiffnessBehavior_NONLINEAR
};

/** Enum with association roles. */
enum ads_Prop_IMec_Normalbehavior_tableRolesEnm
{
    ads_Prop_IMec_Normalbehavior_table_child,
    ads_Prop_IMec_Normalbehavior_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_CureHeatGeneration_tableRolesEnm
{
    ads_Prop_IThermal_CureHeatGeneration_table_child,
    ads_Prop_IThermal_CureHeatGeneration_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureKinetics_Kamal_tableRolesEnm
{
    ads_Prop_MMec_CureKinetics_Kamal_table_child,
    ads_Prop_MMec_CureKinetics_Kamal_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureKinetics_Tabular_tableRolesEnm
{
    ads_Prop_MMec_CureKinetics_Tabular_table_child,
    ads_Prop_MMec_CureKinetics_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureMaxConversion_tableRolesEnm
{
    ads_Prop_MMec_CureMaxConversion_table_child,
    ads_Prop_MMec_CureMaxConversion_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureShrinkage_Aniso_tableRolesEnm
{
    ads_Prop_MMec_CureShrinkage_Aniso_table_child,
    ads_Prop_MMec_CureShrinkage_Aniso_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureShrinkage_Iso_tableRolesEnm
{
    ads_Prop_MMec_CureShrinkage_Iso_table_child,
    ads_Prop_MMec_CureShrinkage_Iso_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureShrinkage_Ortho_tableRolesEnm
{
    ads_Prop_MMec_CureShrinkage_Ortho_table_child,
    ads_Prop_MMec_CureShrinkage_Ortho_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CureShrinkage_Vol_tableRolesEnm
{
    ads_Prop_MMec_CureShrinkage_Vol_table_child,
    ads_Prop_MMec_CureShrinkage_Vol_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Damping_FactorsMembersEnm
{
    ads_Prop_MMec_Damping_Factors_alpha,
    ads_Prop_MMec_Damping_Factors_beta,
    ads_Prop_MMec_Damping_Factors_composite,
    ads_Prop_MMec_Damping_Factors_structural
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Damping_Factors_dampingControlRolesEnm
{
    ads_Prop_MMec_Damping_Factors_dampingControl_child,
    ads_Prop_MMec_Damping_Factors_dampingControl_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Damping_Factors_tableRolesEnm
{
    ads_Prop_MMec_Damping_Factors_table_child,
    ads_Prop_MMec_Damping_Factors_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_FabricMembersEnm
{
    ads_Prop_MMec_Fabric_stressFreeInitialSlack
};

/** Enum with record members. */
enum ads_Prop_MMec_Fabric_UserMembersEnm
{
    ads_Prop_MMec_Fabric_User_stressFreeInitialSlack
};

#endif
