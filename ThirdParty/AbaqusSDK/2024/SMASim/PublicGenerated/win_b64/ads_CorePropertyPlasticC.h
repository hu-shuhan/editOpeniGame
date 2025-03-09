//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyPlasticC_h
#define ads_CorePropertyPlasticC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyPlastic of the latest level of form Core */

#define ads_CMecPlasticOptionTransRotIsotropicExpHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 0))

#define ads_CMecPlasticOptionTransRotIsotropicTabHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 1))

#define ads_CMecPlasticOptionTransRotKinematicHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 2))

#define ads_MMecPlasticCamClayExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 3))

#define ads_MMecPlasticCamClayTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 4))

#define ads_MMecPlasticCapTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 5))

#define ads_MMecPlasticCastIronTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 6))

#define ads_MMecPlasticCrushableFoamIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 7))

#define ads_MMecPlasticCrushableFoamVolumetricTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 8))

#define ads_MMecPlasticDruckerPragerExponentTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 9))

#define ads_MMecPlasticDruckerPragerHyperbolicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 10))

#define ads_MMecPlasticDruckerPragerLinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 11))

#define ads_MMecPlasticHillTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 12))

#define ads_MMecPlasticJointedRockTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 13))

#define ads_MMecPlasticLowDensityFoamNoLateralStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 14))

#define ads_MMecPlasticLowDensityFoamWithLateralStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 15))

#define ads_MMecPlasticMohrCoulombTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 16))

#define ads_MMecPlasticOptionAnnealTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 17))

#define ads_MMecPlasticOptionCycledKinematicHardeingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 18))

#define ads_MMecPlasticOptionIsotropicExpHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 19))

#define ads_MMecPlasticOptionIsotropicJCookHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 20))

#define ads_MMecPlasticOptionIsotropicTabHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 21))

#define ads_MMecPlasticOptionKinematicHardeningLinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 22))

#define ads_MMecPlasticOptionKinematicHardeningNonlinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 23))

#define ads_MMecPlasticOptionKinematicHardeningStaticRecoveryTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 24))

#define ads_MMecPlasticOptionMultiLinearKinematicHardeingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 25))

#define ads_MMecPlasticOptionPorousFailureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 26))

#define ads_MMecPlasticOptionPorousVoidNucleationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 27))

#define ads_MMecPlasticOptionRankineTensionCutoffTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 28))

#define ads_MMecPlasticOptionRateChabocheTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 29))

#define ads_MMecPlasticOptionRateJohnsonCookTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 30))

#define ads_MMecPlasticOptionRatePowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 31))

#define ads_MMecPlasticOptionRateYieldRatioTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 32))

#define ads_MMecPlasticOptionTensionCutoffTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 33))

#define ads_MMecPlasticPorousTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 34))

#define ads_MMecPlasticSoftRockHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 35))

#define ads_MMecPlasticSoftRockTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 36))

#define ads_MMecPlasticSofteningRegularizationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 37))

#define ads_MMecPlasticityCorrectionRambergTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 38))

#define ads_MMecPlasticityCorrectionTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 39))

#define ads_Prop_CMec_Plastic (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 40))

#define ads_Prop_CMec_PlasticOption (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 41))

#define ads_Prop_CMec_PlasticOption_Rotational (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 42))

#define ads_Prop_CMec_PlasticOption_TransRot (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 43))

#define ads_Prop_CMec_PlasticOption_TransRot_IsotropicExpHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 44))

#define ads_Prop_CMec_PlasticOption_TransRot_IsotropicExpHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 45))

#define ads_Prop_CMec_PlasticOption_TransRot_IsotropicTabHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 46))

#define ads_Prop_CMec_PlasticOption_TransRot_IsotropicTabHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 47))

#define ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 48))

#define ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 49))

#define ads_Prop_CMec_PlasticOption_Translational (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 50))

#define ads_Prop_MMec_Plastic (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 51))

#define ads_Prop_MMec_PlasticOption (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 52))

#define ads_Prop_MMec_PlasticOption_Anneal (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 53))

#define ads_Prop_MMec_PlasticOption_Anneal_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 54))

#define ads_Prop_MMec_PlasticOption_Cycled (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 55))

/** Specify cycled yield stress data for the ORNL model. */
#define ads_Prop_MMec_PlasticOption_Cycled_KinematicHardeing (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 56))

#define ads_Prop_MMec_PlasticOption_Cycled_KinematicHardeing_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 57))

/** Specify the size of the elastic range for the combined hardening model using material parameters directly. */
#define ads_Prop_MMec_PlasticOption_IsotropicExpHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 58))

#define ads_Prop_MMec_PlasticOption_IsotropicExpHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 59))

/** ParametersCombined hardening plasticity record */
#define ads_Prop_MMec_PlasticOption_IsotropicJCookHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 60))

#define ads_Prop_MMec_PlasticOption_IsotropicJCookHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 61))

#define ads_Prop_MMec_PlasticOption_IsotropicTabHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 62))

#define ads_Prop_MMec_PlasticOption_IsotropicTabHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 63))

/** Kinematic hardening plasticity record */
#define ads_Prop_MMec_PlasticOption_KinematicHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 64))

/** Linear Kinematic hardening plasticity record */
#define ads_Prop_MMec_PlasticOption_KinematicHardening_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 65))

#define ads_Prop_MMec_PlasticOption_KinematicHardening_Linear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 66))

/** Non Linear Kinematic hardening plasticity record */
#define ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 67))

#define ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 68))

/** Non Linear Kinematic hardening plasticity record with static recovery */
#define ads_Prop_MMec_PlasticOption_KinematicHardening_StaticRecovery (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 69))

#define ads_Prop_MMec_PlasticOption_KinematicHardening_StaticRecovery_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 70))

/** Specify multi linear kinematic hardening model. */
#define ads_Prop_MMec_PlasticOption_MultiLinear_KinematicHardeing (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 71))

#define ads_Prop_MMec_PlasticOption_MultiLinear_KinematicHardeing_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 72))

#define ads_Prop_MMec_PlasticOption_Porous (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 73))

/** Define porous material failure criteria for a POROUS METAL PLASTICITY model. */
#define ads_Prop_MMec_PlasticOption_Porous_Failure (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 74))

#define ads_Prop_MMec_PlasticOption_Porous_Failure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 75))

/** Define the nucleation of voids in a porous material. */
#define ads_Prop_MMec_PlasticOption_Porous_VoidNucleation (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 76))

#define ads_Prop_MMec_PlasticOption_Porous_VoidNucleation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 77))

#define ads_Prop_MMec_PlasticOption_RankineTensionCutoff (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 78))

#define ads_Prop_MMec_PlasticOption_RankineTensionCutoff_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 79))

#define ads_Prop_MMec_PlasticOption_Rate (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 80))

/** Define a rate-dependent viscoplastic model based on Chaboche */
#define ads_Prop_MMec_PlasticOption_Rate_Chaboche (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 81))

#define ads_Prop_MMec_PlasticOption_Rate_Chaboche_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 82))

/** Define a rate-dependent viscoplastic model based on Johnson-Cook rate dependence. */
#define ads_Prop_MMec_PlasticOption_Rate_JohnsonCook (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 83))

#define ads_Prop_MMec_PlasticOption_Rate_JohnsonCook_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 84))

/** specify the Cowper-Symonds overstress power law. */
#define ads_Prop_MMec_PlasticOption_Rate_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 85))

#define ads_Prop_MMec_PlasticOption_Rate_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 86))

/** Define a rate-dependent viscoplastic model based on yield stress ratios. */
#define ads_Prop_MMec_PlasticOption_Rate_YieldRatio (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 87))

#define ads_Prop_MMec_PlasticOption_Rate_YieldRatio_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 88))

#define ads_Prop_MMec_PlasticOption_SoftRockHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 89))

#define ads_Prop_MMec_PlasticOption_SoftRockHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 90))

#define ads_Prop_MMec_PlasticOption_SofteningRegularization (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 91))

#define ads_Prop_MMec_PlasticOption_SofteningRegularization_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 92))

/** Define tension cutoff data to limit the load carrying capacity of the Mohr-Coulomb plasticity. */
#define ads_Prop_MMec_PlasticOption_TensionCutoff (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 93))

#define ads_Prop_MMec_PlasticOption_TensionCutoff_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 94))

/** user defined isotropic hardening. */
#define ads_Prop_MMec_PlasticOption_UserHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 95))

#define ads_Prop_MMec_Plastic_CamClay (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 96))

/** Specify the extended Cam-clay plasticity model using a exponential hardening/softening relationship. */
#define ads_Prop_MMec_Plastic_CamClay_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 97))

#define ads_Prop_MMec_Plastic_CamClay_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 98))

/** Specify the extended Cam-clay plasticity model using a piecewise linear hardening/softening relationship. */
#define ads_Prop_MMec_Plastic_CamClay_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 99))

#define ads_Prop_MMec_Plastic_CamClay_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 100))

/** Yield surface parameters for elastic-plastic materials that use the modified Drucker-Prager/Cap plasticity model. */
#define ads_Prop_MMec_Plastic_Cap (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 101))

#define ads_Prop_MMec_Plastic_Cap_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 102))

/** Specify the compression hardening data for gray cast iron. */
#define ads_Prop_MMec_Plastic_CastIron (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 103))

#define ads_Prop_MMec_Plastic_CastIron_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 104))

#define ads_Prop_MMec_Plastic_CrushableFoam (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 105))

/** The crushable foam plasticity model based on the isotropic hardening. */
#define ads_Prop_MMec_Plastic_CrushableFoam_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 106))

#define ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 107))

/** The crushable foam plasticity model based on the volumetric hardening. */
#define ads_Prop_MMec_Plastic_CrushableFoam_Volumetric (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 108))

#define ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 109))

/** Plasticity calculations for type 304 and type 316 stainless steel according to the specification in Nuclear Standard NEF 9-5T. */
#define ads_Prop_MMec_Plastic_Cycled (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 110))

/** Abstraction of the extended Drucker-Prager plasticity model. */
#define ads_Prop_MMec_Plastic_DruckerPrager (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 111))

/** Specify the extended Drucker-Prager plasticity model. Define the exponent form as a yield criterion. */
#define ads_Prop_MMec_Plastic_DruckerPrager_Exponent (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 112))

#define ads_Prop_MMec_Plastic_DruckerPrager_Exponent_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 113))

/** Specify the extended Drucker-Prager plasticity model. */
#define ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 114))

#define ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 115))

/** define the linear yield criterion for the extended Drucker-Prager plasticity model. */
#define ads_Prop_MMec_Plastic_DruckerPrager_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 116))

#define ads_Prop_MMec_Plastic_DruckerPrager_Linear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 117))

/** Define an anisotropic yield/creep model. This option is used to define stress ratios for anisotropic yield and creep behavior. */
#define ads_Prop_MMec_Plastic_Hill (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 118))

#define ads_Prop_MMec_Plastic_Hill_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 119))

#define ads_Prop_MMec_Plastic_JointedRock (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 120))

#define ads_Prop_MMec_Plastic_JointedRock_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 121))

/** Defines low density foam materials. */
#define ads_Prop_MMec_Plastic_LowDensityFoam (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 122))

/** Defines low density foam materials. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 123))

/** *UNIAXIAL TEST DATA, DIRECTION=COMPRESSION with *LOW DENSITY FOAM, LATERAL STRAIN DATA=YES. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_compressionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 124))

/** *UNIAXIAL TEST DATA, DIRECTION=TENSION with *LOW DENSITY FOAM, LATERAL STRAIN DATA=YES. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_tensionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 125))

/** Defines low density foam materials. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 126))

/** *UNIAXIAL TEST DATA, DIRECTION=COMPRESSION with *LOW DENSITY FOAM, LATERAL STRAIN DATA=NO. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_compressionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 127))

/** *UNIAXIAL TEST DATA, DIRECTION=TENSION with *LOW DENSITY FOAM, LATERAL STRAIN DATA=NO. */
#define ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_tensionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 128))

#define ads_Prop_MMec_Plastic_Mises (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 129))

/** Specify the Mohr-Coulomb plasticity model. */
#define ads_Prop_MMec_Plastic_MohrCoulomb (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 130))

#define ads_Prop_MMec_Plastic_MohrCoulomb_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 131))

/** Specify a porous metal plasticity model. */
#define ads_Prop_MMec_Plastic_Porous (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 132))

#define ads_Prop_MMec_Plastic_Porous_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 133))

/** Specify the plastic part of the material behavior for elastic-plastic materials that use the soft rock plasticity model. */
#define ads_Prop_MMec_Plastic_SoftRock (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 134))

#define ads_Prop_MMec_Plastic_SoftRock_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 135))

#define ads_Prop_MMec_PlasticityCorrection (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 136))

#define ads_Prop_MMec_PlasticityCorrection_RambergOsgood (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 137))

#define ads_Prop_MMec_PlasticityCorrection_RambergOsgood_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 138))

#define ads_Prop_MMec_PlasticityCorrection_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 139))

#define ads_Prop_MMec_PlasticityCorrection_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyPlasticFragment, 140))

/** Enum with association roles. */
enum ads_Prop_CMec_PlasticOption_TransRot_IsotropicExpHardening_tableRolesEnm
{
    ads_Prop_CMec_PlasticOption_TransRot_IsotropicExpHardening_table_child,
    ads_Prop_CMec_PlasticOption_TransRot_IsotropicExpHardening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_PlasticOption_TransRot_IsotropicTabHardening_tableRolesEnm
{
    ads_Prop_CMec_PlasticOption_TransRot_IsotropicTabHardening_table_child,
    ads_Prop_CMec_PlasticOption_TransRot_IsotropicTabHardening_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_PlasticOption_TransRot_KinematicHardeningMembersEnm
{
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_defEnm
};

enum ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_defEnmEnm
{
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_defEnm_HALF_CYCLE,
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_defEnm_PARAMETERS,
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_defEnm_STABILIZED
};

/** Enum with association roles. */
enum ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_tableRolesEnm
{
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_table_child,
    ads_Prop_CMec_PlasticOption_TransRot_KinematicHardening_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_PlasticMembersEnm
{
    ads_Prop_MMec_Plastic_extrapolation
};

enum ads_Prop_MMec_Plastic_extrapolationEnm
{
    ads_Prop_MMec_Plastic_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Anneal_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Anneal_table_child,
    ads_Prop_MMec_PlasticOption_Anneal_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Cycled_KinematicHardeing_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Cycled_KinematicHardeing_table_child,
    ads_Prop_MMec_PlasticOption_Cycled_KinematicHardeing_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_IsotropicExpHardening_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicExpHardening_table_child,
    ads_Prop_MMec_PlasticOption_IsotropicExpHardening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_IsotropicJCookHardening_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicJCookHardening_table_child,
    ads_Prop_MMec_PlasticOption_IsotropicJCookHardening_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_PlasticOption_IsotropicTabHardeningMembersEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_extrapolationCyclic,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningType,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_scaleStress,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_softeningRegularization
};

enum ads_Prop_MMec_PlasticOption_IsotropicTabHardening_extrapolationCyclicEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_extrapolationCyclic_CONSTANT,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_extrapolationCyclic_LINEAR
};

enum ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningTypeEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningType_ABSENT,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningType_COMPRESSION,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningType_SHEAR,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_hardeningType_TENSION
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_IsotropicTabHardening_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_table_child,
    ads_Prop_MMec_PlasticOption_IsotropicTabHardening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_KinematicHardening_Linear_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_KinematicHardening_Linear_table_child,
    ads_Prop_MMec_PlasticOption_KinematicHardening_Linear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_PlasticOption_KinematicHardening_NonlinearMembersEnm
{
    ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear_staticRecovery
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear_table_child,
    ads_Prop_MMec_PlasticOption_KinematicHardening_Nonlinear_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_KinematicHardening_StaticRecovery_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_KinematicHardening_StaticRecovery_table_child,
    ads_Prop_MMec_PlasticOption_KinematicHardening_StaticRecovery_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_MultiLinear_KinematicHardeing_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_MultiLinear_KinematicHardeing_table_child,
    ads_Prop_MMec_PlasticOption_MultiLinear_KinematicHardeing_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Porous_Failure_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Porous_Failure_table_child,
    ads_Prop_MMec_PlasticOption_Porous_Failure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Porous_VoidNucleation_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Porous_VoidNucleation_table_child,
    ads_Prop_MMec_PlasticOption_Porous_VoidNucleation_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_RankineTensionCutoff_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_RankineTensionCutoff_table_child,
    ads_Prop_MMec_PlasticOption_RankineTensionCutoff_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Rate_Chaboche_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Rate_Chaboche_table_child,
    ads_Prop_MMec_PlasticOption_Rate_Chaboche_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Rate_JohnsonCook_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Rate_JohnsonCook_table_child,
    ads_Prop_MMec_PlasticOption_Rate_JohnsonCook_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Rate_PowerLaw_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Rate_PowerLaw_table_child,
    ads_Prop_MMec_PlasticOption_Rate_PowerLaw_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_Rate_YieldRatio_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_Rate_YieldRatio_table_child,
    ads_Prop_MMec_PlasticOption_Rate_YieldRatio_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_PlasticOption_SoftRockHardeningMembersEnm
{
    ads_Prop_MMec_PlasticOption_SoftRockHardening_softeningRegularization,
    ads_Prop_MMec_PlasticOption_SoftRockHardening_type
};

enum ads_Prop_MMec_PlasticOption_SoftRockHardening_typeEnm
{
    ads_Prop_MMec_PlasticOption_SoftRockHardening_type_COMPRESSION,
    ads_Prop_MMec_PlasticOption_SoftRockHardening_type_TENSION
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_SoftRockHardening_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_SoftRockHardening_table_child,
    ads_Prop_MMec_PlasticOption_SoftRockHardening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_SofteningRegularization_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_SofteningRegularization_table_child,
    ads_Prop_MMec_PlasticOption_SofteningRegularization_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticOption_TensionCutoff_tableRolesEnm
{
    ads_Prop_MMec_PlasticOption_TensionCutoff_table_child,
    ads_Prop_MMec_PlasticOption_TensionCutoff_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Plastic_CamClayMembersEnm
{
    ads_Prop_MMec_Plastic_CamClay_extrapolation
};

enum ads_Prop_MMec_Plastic_CamClay_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CamClay_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CamClay_extrapolation_LINEAR
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CamClay_ExponentialMembersEnm
{
    ads_Prop_MMec_Plastic_CamClay_Exponential_extrapolation,
    ads_Prop_MMec_Plastic_CamClay_Exponential_intercept,
    ads_Prop_MMec_Plastic_CamClay_Exponential_interceptUnionType
};

enum ads_Prop_MMec_Plastic_CamClay_Exponential_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CamClay_Exponential_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CamClay_Exponential_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_CamClay_Exponential_interceptUnionTypeEnm
{
    ads_Prop_MMec_Plastic_CamClay_Exponential_interceptUnionType_ABSENT,
    ads_Prop_MMec_Plastic_CamClay_Exponential_interceptUnionType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_CamClay_Exponential_tableRolesEnm
{
    ads_Prop_MMec_Plastic_CamClay_Exponential_table_child,
    ads_Prop_MMec_Plastic_CamClay_Exponential_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CamClay_TabularMembersEnm
{
    ads_Prop_MMec_Plastic_CamClay_Tabular_extrapolation
};

enum ads_Prop_MMec_Plastic_CamClay_Tabular_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CamClay_Tabular_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CamClay_Tabular_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_CamClay_Tabular_tableRolesEnm
{
    ads_Prop_MMec_Plastic_CamClay_Tabular_table_child,
    ads_Prop_MMec_Plastic_CamClay_Tabular_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CapMembersEnm
{
    ads_Prop_MMec_Plastic_Cap_extrapolation
};

enum ads_Prop_MMec_Plastic_Cap_extrapolationEnm
{
    ads_Prop_MMec_Plastic_Cap_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_Cap_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_Cap_tableRolesEnm
{
    ads_Prop_MMec_Plastic_Cap_table_child,
    ads_Prop_MMec_Plastic_Cap_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CastIronMembersEnm
{
    ads_Prop_MMec_Plastic_CastIron_extrapolation
};

enum ads_Prop_MMec_Plastic_CastIron_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CastIron_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CastIron_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_CastIron_tableRolesEnm
{
    ads_Prop_MMec_Plastic_CastIron_table_child,
    ads_Prop_MMec_Plastic_CastIron_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Plastic_CrushableFoamMembersEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_extrapolation
};

enum ads_Prop_MMec_Plastic_CrushableFoam_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CrushableFoam_extrapolation_LINEAR
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CrushableFoam_IsotropicMembersEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_extrapolation
};

enum ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_tableRolesEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_table_child,
    ads_Prop_MMec_Plastic_CrushableFoam_Isotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CrushableFoam_VolumetricMembersEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_extrapolation
};

enum ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_extrapolationEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_tableRolesEnm
{
    ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_table_child,
    ads_Prop_MMec_Plastic_CrushableFoam_Volumetric_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_CycledMembersEnm
{
    ads_Prop_MMec_Plastic_Cycled_extrapolation,
    ads_Prop_MMec_Plastic_Cycled_a,
    ads_Prop_MMec_Plastic_Cycled_h,
    ads_Prop_MMec_Plastic_Cycled_hPresence,
    ads_Prop_MMec_Plastic_Cycled_reset
};

enum ads_Prop_MMec_Plastic_Cycled_extrapolationEnm
{
    ads_Prop_MMec_Plastic_Cycled_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_Cycled_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_Cycled_hPresenceEnm
{
    ads_Prop_MMec_Plastic_Cycled_hPresence_ABSENT,
    ads_Prop_MMec_Plastic_Cycled_hPresence_PRESENT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_DruckerPragerMembersEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_extrapolation,
    ads_Prop_MMec_Plastic_DruckerPrager_eccentricity,
    ads_Prop_MMec_Plastic_DruckerPrager_eccentricityEnm
};

enum ads_Prop_MMec_Plastic_DruckerPrager_extrapolationEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_DruckerPrager_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_DruckerPrager_eccentricityEnmEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_eccentricityEnm_ABSENT,
    ads_Prop_MMec_Plastic_DruckerPrager_eccentricityEnm_USER_INPUT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_DruckerPrager_ExponentMembersEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_extrapolation,
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_eccentricity,
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_eccentricityEnm
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Exponent_extrapolationEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Exponent_eccentricityEnmEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_eccentricityEnm_ABSENT,
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_eccentricityEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_DruckerPrager_Exponent_tableRolesEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_table_child,
    ads_Prop_MMec_Plastic_DruckerPrager_Exponent_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_DruckerPrager_HyperbolicMembersEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_extrapolation,
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_eccentricity,
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_eccentricityEnm
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_extrapolationEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_eccentricityEnmEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_eccentricityEnm_ABSENT,
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_eccentricityEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_tableRolesEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_table_child,
    ads_Prop_MMec_Plastic_DruckerPrager_Hyperbolic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_DruckerPrager_LinearMembersEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_extrapolation,
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_eccentricity,
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_eccentricityEnm
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Linear_extrapolationEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_DruckerPrager_Linear_eccentricityEnmEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_eccentricityEnm_ABSENT,
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_eccentricityEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_DruckerPrager_Linear_tableRolesEnm
{
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_table_child,
    ads_Prop_MMec_Plastic_DruckerPrager_Linear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_HillMembersEnm
{
    ads_Prop_MMec_Plastic_Hill_extrapolation
};

enum ads_Prop_MMec_Plastic_Hill_extrapolationEnm
{
    ads_Prop_MMec_Plastic_Hill_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_Hill_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_Hill_tableRolesEnm
{
    ads_Prop_MMec_Plastic_Hill_table_child,
    ads_Prop_MMec_Plastic_Hill_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Plastic_JointedRockMembersEnm
{
    ads_Prop_MMec_Plastic_JointedRock_extrapolation
};

enum ads_Prop_MMec_Plastic_JointedRock_extrapolationEnm
{
    ads_Prop_MMec_Plastic_JointedRock_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_JointedRock_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_JointedRock_tableRolesEnm
{
    ads_Prop_MMec_Plastic_JointedRock_table_child,
    ads_Prop_MMec_Plastic_JointedRock_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_LowDensityFoamMembersEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_extrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_alpha,
    ads_Prop_MMec_Plastic_LowDensityFoam_fail,
    ads_Prop_MMec_Plastic_LowDensityFoam_mu0,
    ads_Prop_MMec_Plastic_LowDensityFoam_mu1,
    ads_Prop_MMec_Plastic_LowDensityFoam_rateExtrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_strainRate,
    ads_Prop_MMec_Plastic_LowDensityFoam_tensionCutoff
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_extrapolationEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_LowDensityFoam_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_strainRateEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_strainRate_MAX_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_strainRate_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_strainRate_VOLUMETRIC
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrainMembersEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_extrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_alpha,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_fail,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_mu0,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_mu1,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_rateExtrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_strainRate,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_tensionCutoff
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_extrapolationEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_strainRateEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_strainRate_MAX_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_strainRate_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_strainRate_VOLUMETRIC
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_compressionTableRolesEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_compressionTable_child,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_compressionTable_parent
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_tensionTableRolesEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_tensionTable_child,
    ads_Prop_MMec_Plastic_LowDensityFoam_LateralStrain_tensionTable_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrainMembersEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_extrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_alpha,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_fail,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_mu0,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_mu1,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_rateExtrapolation,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_strainRate,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_tensionCutoff
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_extrapolationEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_strainRateEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_strainRate_MAX_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_strainRate_PRINCIPAL,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_strainRate_VOLUMETRIC
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_compressionTableRolesEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_compressionTable_child,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_compressionTable_parent
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_tensionTableRolesEnm
{
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_tensionTable_child,
    ads_Prop_MMec_Plastic_LowDensityFoam_NoLateralStrain_tensionTable_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Plastic_MisesMembersEnm
{
    ads_Prop_MMec_Plastic_Mises_extrapolation
};

enum ads_Prop_MMec_Plastic_Mises_extrapolationEnm
{
    ads_Prop_MMec_Plastic_Mises_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_Mises_extrapolation_LINEAR
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_MohrCoulombMembersEnm
{
    ads_Prop_MMec_Plastic_MohrCoulomb_extrapolation,
    ads_Prop_MMec_Plastic_MohrCoulomb_deviatoricEccentricity,
    ads_Prop_MMec_Plastic_MohrCoulomb_deviatoricEccentricityEnm,
    ads_Prop_MMec_Plastic_MohrCoulomb_eccentricity
};

enum ads_Prop_MMec_Plastic_MohrCoulomb_extrapolationEnm
{
    ads_Prop_MMec_Plastic_MohrCoulomb_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_MohrCoulomb_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_MohrCoulomb_deviatoricEccentricityEnmEnm
{
    ads_Prop_MMec_Plastic_MohrCoulomb_deviatoricEccentricityEnm_COMPUTE,
    ads_Prop_MMec_Plastic_MohrCoulomb_deviatoricEccentricityEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_MohrCoulomb_tableRolesEnm
{
    ads_Prop_MMec_Plastic_MohrCoulomb_table_child,
    ads_Prop_MMec_Plastic_MohrCoulomb_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_PorousMembersEnm
{
    ads_Prop_MMec_Plastic_Porous_extrapolation,
    ads_Prop_MMec_Plastic_Porous_relativeDensity,
    ads_Prop_MMec_Plastic_Porous_relativeDensityEnm
};

enum ads_Prop_MMec_Plastic_Porous_extrapolationEnm
{
    ads_Prop_MMec_Plastic_Porous_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_Porous_extrapolation_LINEAR
};

enum ads_Prop_MMec_Plastic_Porous_relativeDensityEnmEnm
{
    ads_Prop_MMec_Plastic_Porous_relativeDensityEnm_INTERPOLATE_FROM_INITIAL_CONDITIONS,
    ads_Prop_MMec_Plastic_Porous_relativeDensityEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_Porous_tableRolesEnm
{
    ads_Prop_MMec_Plastic_Porous_table_child,
    ads_Prop_MMec_Plastic_Porous_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Plastic_SoftRockMembersEnm
{
    ads_Prop_MMec_Plastic_SoftRock_extrapolation,
    ads_Prop_MMec_Plastic_SoftRock_eccentricity
};

enum ads_Prop_MMec_Plastic_SoftRock_extrapolationEnm
{
    ads_Prop_MMec_Plastic_SoftRock_extrapolation_CONSTANT,
    ads_Prop_MMec_Plastic_SoftRock_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_Plastic_SoftRock_tableRolesEnm
{
    ads_Prop_MMec_Plastic_SoftRock_table_child,
    ads_Prop_MMec_Plastic_SoftRock_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticityCorrection_RambergOsgood_tableRolesEnm
{
    ads_Prop_MMec_PlasticityCorrection_RambergOsgood_table_child,
    ads_Prop_MMec_PlasticityCorrection_RambergOsgood_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlasticityCorrection_Tabular_tableRolesEnm
{
    ads_Prop_MMec_PlasticityCorrection_Tabular_table_child,
    ads_Prop_MMec_PlasticityCorrection_Tabular_table_parent
};

#endif
