//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyFatigueC_h
#define ads_CorePropertyFatigueC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyFatigue of the latest level of form Core */

#define ads_Focus_surfaceFinishProperties (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 0))

/** Axial strain life parameters */
#define ads_MMecFatigueAxialStrainLifeTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 1))

/** Data to define Brown Miller Morrow material properties */
#define ads_MMecFatigueBrownMillerMorrowTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 2))

/** Data to define the constant amplitude endurance limit Reversals (2Nf) */
#define ads_MMecFatigueCAELTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 3))

/** Data to define Cast Iron SWT material properties */
#define ads_MMecFatigueCastIronSWTTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 4))

/** Data to define Cast Iron-specific material properties */
#define ads_MMecFatigueCastIronTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 5))

/** Data to define crack propagation material properties */
#define ads_MMecFatigueCrackPropagationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 6))

/** Cyclic Ramberg-Osgood plasticity parameters, K' and n' */
#define ads_MMecFatigueCyclicRambergOsgoodTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 7))

/** Data to define Principal Strain Morrow material properties */
#define ads_MMecFatiguePrincipalStrainMorrowTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 8))

/** S-N data from uniaxial stress fatigue tests. */
#define ads_MMecFatigueSNTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 9))

/** Shear-N data from pure shear stress fatigue tests. */
#define ads_MMecFatigueShearNTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 10))

/** Axial strain life parameters */
#define ads_MMecFatigueShearStrainLifeTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 11))

/** Walker Mean Stress Correction parameters */
#define ads_MMecFatigueWalkerMSCTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 12))

/** Tr-N data: Longtudinal shear stress vs N data. Note: Shear stress RANGE, not amplitude */
#define ads_MMecFatigueWeldFatigueLongShearNTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 13))

/** Sr-N data. Note: Stress RANGE, not amplitude */
#define ads_MMecFatigueWeldFatigueSNTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 14))

#define ads_Prop_MMec_Fatigue (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 15))

/** Captures the fatigue algorithm choice along with mean stress correction options */
#define ads_Prop_MMec_Fatigue_Algorithm (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 16))

/** Reference to the mean stress correction curve data when frequencyMscFrf is set to USER_DEFINED. */
#define ads_Prop_MMec_Fatigue_Algorithm_frequencyUserDefMSC (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 17))

/** Reference to the mean stress correction curve data when msc_frf is set to USER_DEFINED. */
#define ads_Prop_MMec_Fatigue_Algorithm_userDefMSC (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 18))

#define ads_Prop_MMec_Fatigue_BrownMillerMorrow (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 19))

#define ads_Prop_MMec_Fatigue_BrownMillerMorrow_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 20))

#define ads_Prop_MMec_Fatigue_CAEL (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 21))

#define ads_Prop_MMec_Fatigue_CAEL_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 22))

#define ads_Prop_MMec_Fatigue_CastIron (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 23))

#define ads_Prop_MMec_Fatigue_CastIronSWT (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 24))

#define ads_Prop_MMec_Fatigue_CastIronSWT_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 25))

#define ads_Prop_MMec_Fatigue_CastIron_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 26))

#define ads_Prop_MMec_Fatigue_CrackPropagation (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 27))

#define ads_Prop_MMec_Fatigue_CrackPropagation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 28))

/** Crack length parameters. */
#define ads_Prop_MMec_Fatigue_Crack_Length (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 29))

/** Cyclic Ramberg-Osgood plasticity parameters needed for plasticity adjustments made during fatigue solves (not during FEA) */
#define ads_Prop_MMec_Fatigue_CyclicRambergOsgood (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 30))

#define ads_Prop_MMec_Fatigue_CyclicRambergOsgood_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 31))

/** DTMF Crack growth rate parameters. */
#define ads_Prop_MMec_Fatigue_DTMF_CGR (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 32))

/** DTMF Creep-Fatigue interaction parameters. */
#define ads_Prop_MMec_Fatigue_DTMF_Creep (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 33))

/** DTMF Oxidation-fatigue interaction parameters. */
#define ads_Prop_MMec_Fatigue_DTMF_Oxidation (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 34))

/** Only to be used for 7th Edition of the FKM material type designation. */
#define ads_Prop_MMec_Fatigue_FKMMaterialType (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 35))

#define ads_Prop_MMec_Fatigue_PrincipalStrainMorrow (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 36))

#define ads_Prop_MMec_Fatigue_PrincipalStrainMorrow_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 37))

#define ads_Prop_MMec_Fatigue_ProofStress (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 38))

/** Shear Strain Life (Basquin and Coffin-Manson parameters) */
#define ads_Prop_MMec_Fatigue_ShearStrainLife (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 39))

#define ads_Prop_MMec_Fatigue_ShearStrainLife_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 40))

/** Strain Life (Basquin and Coffin-Manson parameters) */
#define ads_Prop_MMec_Fatigue_StrainLife (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 41))

/** Fatemi-Socie Strain-Life properties. */
#define ads_Prop_MMec_Fatigue_StrainLifeOptionFatemiSocie (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 42))

#define ads_Prop_MMec_Fatigue_StrainLife_axialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 43))

/** Unused composition. The shear strain life table should be a child of the ShearStrainLife record. */
#define ads_Prop_MMec_Fatigue_StrainLife_shearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 44))

/** Captures S-N and optionally Shear-N properties. (Stress amplitudes vs N.) */
#define ads_Prop_MMec_Fatigue_StressLife (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 45))

#define ads_Prop_MMec_Fatigue_StressLife_shearNTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 46))

#define ads_Prop_MMec_Fatigue_StressLife_snTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 47))

/** Walker Mean Stress Correction parameters */
#define ads_Prop_MMec_Fatigue_WalkerMSC (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 48))

#define ads_Prop_MMec_Fatigue_WalkerMSC_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 49))

/** Captures S-N and, optionally, longitudinal Shear-N properties. (Stress RANGES vs N.) */
#define ads_Prop_MMec_Fatigue_WeldFatigue (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 50))

#define ads_Prop_MMec_Fatigue_WeldFatigue_srnTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 51))

#define ads_Prop_MMec_Fatigue_WeldFatigue_trnTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 52))

/** Base data type to capture the surface finish table data. */
#define ads_SurfaceFinishProp (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 53))

/** Data to define the surface finish Kf table data using Ra roughness values. */
#define ads_SurfaceFinishPropKfacRaaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 54))

/** Data to define the surface finish Kf table data using roughness names. */
#define ads_SurfaceFinishPropKfacRnamesTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 55))

/** Data to define the surface finish Kf table data using Rz roughness values. */
#define ads_SurfaceFinishPropKfacRzzTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 56))

/** Data to define the surface finish Kt table data using Ra roughness values. */
#define ads_SurfaceFinishPropKteeRaaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 57))

/** Data to define the surface finish Kt table data using roughness names. */
#define ads_SurfaceFinishPropKteeRnamesTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 58))

/** Data to define the surface finish Kt table data using Rz roughness values. */
#define ads_SurfaceFinishPropKteeRzzTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 59))

/** Data type to capture the surface finish Kf table data using Ra roughness values. */
#define ads_SurfaceFinishProp_KfacRa (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 60))

#define ads_SurfaceFinishProp_KfacRa_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 61))

/** Data type to capture the surface finish Kf table data using roughness names. */
#define ads_SurfaceFinishProp_KfacRnames (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 62))

#define ads_SurfaceFinishProp_KfacRnames_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 63))

/** Data type to capture the surface finish Kf table data using Rz roughness values. */
#define ads_SurfaceFinishProp_KfacRz (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 64))

#define ads_SurfaceFinishProp_KfacRz_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 65))

/** Data type to capture the surface finish Kt table data using Ra roughness values. */
#define ads_SurfaceFinishProp_KteeRa (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 66))

#define ads_SurfaceFinishProp_KteeRa_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 67))

/** Data type to capture the surface finish Kt table data using roughness names. */
#define ads_SurfaceFinishProp_KteeRnames (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 68))

#define ads_SurfaceFinishProp_KteeRnames_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 69))

/** Data type to capture the surface finish Kt table data using Rz roughness values. */
#define ads_SurfaceFinishProp_KteeRz (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 70))

#define ads_SurfaceFinishProp_KteeRz_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 71))

/** Provides the roughness name column headings for SurfaceFinishProp tables. Uses Annotation to capture the roughness name string. */
#define ads_SurfaceFinishProp_roughnessNames (ads_CoreFragmentTypeIndex(ads_CorePropertyFatigueFragment, 72))

/** Enum with association roles. */
enum ads_Focus_surfaceFinishPropertiesRolesEnm
{
    ads_Focus_surfaceFinishProperties_child,
    ads_Focus_surfaceFinishProperties_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_AlgorithmMembersEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_algorithm,
    ads_Prop_MMec_Fatigue_Algorithm_combinedShearNormalKFactor,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithm,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrf,
    ads_Prop_MMec_Fatigue_Algorithm_lifeKind,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf,
    ads_Prop_MMec_Fatigue_Algorithm_usrDefMSCAuxId
};

enum ads_Prop_MMec_Fatigue_Algorithm_algorithmEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_BROWN_MILLER,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_BS5400_WELD,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_BS7608_WELD,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_CAST_IRON,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_DANG_VAN,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_DTMF,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_FATEMI_SOCIE,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_FKM_GUIDELINE,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_LIU_I,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_LIU_II,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_MANSON_MCKNIGHT_OCTAHEDRAL,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_MATAKE,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_MAX_SHEAR_STRAIN,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_NORMAL_STRAIN,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_NORMAL_STRESS,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_PRISMATIC_HULL,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_STRESS_BASED_BROWN_MILLER,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_STRUC_STRESS_MOD_WANG_BROWN,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_STRUC_STRESS_NORMAL,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_STRUC_STRESS_SHEAR,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_SUSMEL_LAZZARIN,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_SUSMEL_LAZZARIN_MODIFIED,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_UNIAXIAL_STRAIN_LIFE,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_UNIAXIAL_STRESS_LIFE,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_VON_MISES,
    ads_Prop_MMec_Fatigue_Algorithm_algorithm_WANG_BROWN
};

enum ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithmEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithm_COMBINED_NORMAL_SHEAR,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithm_NORMAL,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithm_SHEAR,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyAlgorithm_VON_MISES
};

enum ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrfEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrf_GOODMAN,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrf_MORROW,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrf_NONE,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyMscFrf_USER_DEFINED
};

enum ads_Prop_MMec_Fatigue_Algorithm_lifeKindEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_lifeKind_FINITE_LIFE,
    ads_Prop_MMec_Fatigue_Algorithm_lifeKind_INFINITE_LIFE
};

enum ads_Prop_MMec_Fatigue_Algorithm_msc_frfEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_DEFAULT_MSC,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_F1_OVERLOADING,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_F2_OVERLOADING,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_F3_OVERLOADING,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_F4_OVERLOADING,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_GERBER,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_GOODMAN,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_MORROW,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_MORROW_B,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_NONE,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_R_RATIO_SN_CURVES,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_SMITH_WATSON_TOPPER,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_SWT_WITH_DOWNING_LIFE_CURVE,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_UDEF_WITH_STD_STRAIN_LIFE_CURVE,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_USER_DEFINED,
    ads_Prop_MMec_Fatigue_Algorithm_msc_frf_WALKER
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Fatigue_Algorithm_frequencyUserDefMSCRolesEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_frequencyUserDefMSC_referent,
    ads_Prop_MMec_Fatigue_Algorithm_frequencyUserDefMSC_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Fatigue_Algorithm_userDefMSCRolesEnm
{
    ads_Prop_MMec_Fatigue_Algorithm_userDefMSC_referent,
    ads_Prop_MMec_Fatigue_Algorithm_userDefMSC_referrer
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_BrownMillerMorrow_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_BrownMillerMorrow_table_child,
    ads_Prop_MMec_Fatigue_BrownMillerMorrow_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_CAEL_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_CAEL_table_child,
    ads_Prop_MMec_Fatigue_CAEL_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_CastIronSWT_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_CastIronSWT_table_child,
    ads_Prop_MMec_Fatigue_CastIronSWT_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_CastIron_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_CastIron_table_child,
    ads_Prop_MMec_Fatigue_CastIron_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_CrackPropagation_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_CrackPropagation_table_child,
    ads_Prop_MMec_Fatigue_CrackPropagation_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_Crack_LengthMembersEnm
{
    ads_Prop_MMec_Fatigue_Crack_Length_a_0,
    ads_Prop_MMec_Fatigue_Crack_Length_a_f
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_CyclicRambergOsgood_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_CyclicRambergOsgood_table_child,
    ads_Prop_MMec_Fatigue_CyclicRambergOsgood_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_DTMF_CGRMembersEnm
{
    ads_Prop_MMec_Fatigue_DTMF_CGR_B,
    ads_Prop_MMec_Fatigue_DTMF_CGR_beta_prime
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_DTMF_CreepMembersEnm
{
    ads_Prop_MMec_Fatigue_DTMF_Creep_Q_cr_over_R,
    ads_Prop_MMec_Fatigue_DTMF_Creep_Sref,
    ads_Prop_MMec_Fatigue_DTMF_Creep_alpha_prime,
    ads_Prop_MMec_Fatigue_DTMF_Creep_n_Norton
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_DTMF_OxidationMembersEnm
{
    ads_Prop_MMec_Fatigue_DTMF_Oxidation_D_0,
    ads_Prop_MMec_Fatigue_DTMF_Oxidation_Q_ox_over_R
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_FKMMaterialTypeMembersEnm
{
    ads_Prop_MMec_Fatigue_FKMMaterialType_effectiveDiameter_NA,
    ads_Prop_MMec_Fatigue_FKMMaterialType_effectiveDiameter_Nm,
    ads_Prop_MMec_Fatigue_FKMMaterialType_effectiveDiameter_Np,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType,
    ads_Prop_MMec_Fatigue_FKMMaterialType_sizeFactorConstant_adA,
    ads_Prop_MMec_Fatigue_FKMMaterialType_sizeFactorConstant_adm,
    ads_Prop_MMec_Fatigue_FKMMaterialType_sizeFactorConstant_adp,
    ads_Prop_MMec_Fatigue_FKMMaterialType_weldFactor_Compression,
    ads_Prop_MMec_Fatigue_FKMMaterialType_weldFactor_TensionShear,
    ads_Prop_MMec_Fatigue_FKMMaterialType_weldSofteningFactor
};

enum ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatTypeEnm
{
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_ADI_AUSFERRITIC_NODULAR_CASTIRON,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_AUSTENITIC_STAINLESS_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_CASE_HARDENING_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_CAST_ALUMINUM_ALLOY,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_FINE_GRAIN_STRUCT_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_GJL_LAMELLAR_GRAPHITE_CASTIRON,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_GJM_MALLEABLE_CASTIRON,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_GJS_NODULAR_CASTIRON,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_GS_CAST_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_GS_HEAT_TREATABLE_CAST_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_HEAT_TREATABLE_STEEL_N,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_HEAT_TREATABLE_STEEL_QT,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_LARGER_FORGINGS_STEEL_N,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_LARGER_FORGINGS_STEEL_QT,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_NITRIDING_STEEL_QT,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_NONE,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_NON_ALLOYED_STRUCT_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_STAINLESS_STEEL,
    ads_Prop_MMec_Fatigue_FKMMaterialType_fkmMatType_WROUGHT_ALUMINUM_ALLOY
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_PrincipalStrainMorrow_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_PrincipalStrainMorrow_table_child,
    ads_Prop_MMec_Fatigue_PrincipalStrainMorrow_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_ShearStrainLife_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_ShearStrainLife_table_child,
    ads_Prop_MMec_Fatigue_ShearStrainLife_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Fatigue_StrainLifeOptionFatemiSocieMembersEnm
{
    ads_Prop_MMec_Fatigue_StrainLifeOptionFatemiSocie_k
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_StrainLife_axialTableRolesEnm
{
    ads_Prop_MMec_Fatigue_StrainLife_axialTable_child,
    ads_Prop_MMec_Fatigue_StrainLife_axialTable_parent
};

/** 
Enum with association roles. */
enum ads_Prop_MMec_Fatigue_StrainLife_shearTableRolesEnm
{
    ads_Prop_MMec_Fatigue_StrainLife_shearTable_child,
    ads_Prop_MMec_Fatigue_StrainLife_shearTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_StressLife_shearNTableRolesEnm
{
    ads_Prop_MMec_Fatigue_StressLife_shearNTable_child,
    ads_Prop_MMec_Fatigue_StressLife_shearNTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_StressLife_snTableRolesEnm
{
    ads_Prop_MMec_Fatigue_StressLife_snTable_child,
    ads_Prop_MMec_Fatigue_StressLife_snTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_WalkerMSC_tableRolesEnm
{
    ads_Prop_MMec_Fatigue_WalkerMSC_table_child,
    ads_Prop_MMec_Fatigue_WalkerMSC_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_WeldFatigue_srnTableRolesEnm
{
    ads_Prop_MMec_Fatigue_WeldFatigue_srnTable_child,
    ads_Prop_MMec_Fatigue_WeldFatigue_srnTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Fatigue_WeldFatigue_trnTableRolesEnm
{
    ads_Prop_MMec_Fatigue_WeldFatigue_trnTable_child,
    ads_Prop_MMec_Fatigue_WeldFatigue_trnTable_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KfacRa_tableRolesEnm
{
    ads_SurfaceFinishProp_KfacRa_table_child,
    ads_SurfaceFinishProp_KfacRa_table_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KfacRnames_tableRolesEnm
{
    ads_SurfaceFinishProp_KfacRnames_table_child,
    ads_SurfaceFinishProp_KfacRnames_table_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KfacRz_tableRolesEnm
{
    ads_SurfaceFinishProp_KfacRz_table_child,
    ads_SurfaceFinishProp_KfacRz_table_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KteeRa_tableRolesEnm
{
    ads_SurfaceFinishProp_KteeRa_table_child,
    ads_SurfaceFinishProp_KteeRa_table_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KteeRnames_tableRolesEnm
{
    ads_SurfaceFinishProp_KteeRnames_table_child,
    ads_SurfaceFinishProp_KteeRnames_table_parent
};

/** Enum with association roles. */
enum ads_SurfaceFinishProp_KteeRz_tableRolesEnm
{
    ads_SurfaceFinishProp_KteeRz_table_child,
    ads_SurfaceFinishProp_KteeRz_table_parent
};

/** 
Enum with association roles. */
enum ads_SurfaceFinishProp_roughnessNamesRolesEnm
{
    ads_SurfaceFinishProp_roughnessNames_child,
    ads_SurfaceFinishProp_roughnessNames_parent
};

#endif
