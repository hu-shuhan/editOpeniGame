//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyDamageC_h
#define ads_CorePropertyDamageC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyDamage of the latest level of form Core */

#define ads_CMecDamageEvolutionEnergyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 0))

#define ads_CMecDamageEvolutionTransRotExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 1))

#define ads_CMecDamageEvolutionTransRotLinearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 2))

#define ads_CMecDamageEvolutionTransRotTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 3))

#define ads_CMecDamageInitiationLoadTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 4))

#define ads_CMecDamageInitiationMotionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 5))

#define ads_CMecDamageInitiationPlasticMotionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 6))

#define ads_CMecPlyFabricFailureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 7))

#define ads_CMecPlyFabricHardeningJCookTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 8))

#define ads_CMecPlyFabricHardeningTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 9))

#define ads_IMecDamageEvolutionEnergyExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 10))

#define ads_IMecDamageEvolutionEnergyMixedModeExpressionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 11))

#define ads_IMecDamageEvolutionSeparationExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 12))

#define ads_IMecDamageEvolutionSeparationTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 13))

#define ads_IMecDamageInitiationSeparationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 14))

#define ads_IMecDamageInitiationTractionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 15))

#define ads_MMecDamageEvolutionDisplacementExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 16))

#define ads_MMecDamageEvolutionDisplacementTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 17))

#define ads_MMecDamageEvolutionEnergyExponentialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 18))

#define ads_MMecDamageEvolutionEnergyLinearHashinTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 19))

#define ads_MMecDamageEvolutionEnergyMixedModeExpressionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 20))

#define ads_MMecDamageEvolutionHysteresisTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 21))

#define ads_MMecDamageInitiationContactSeparationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 22))

#define ads_MMecDamageInitiationDuctileLodeTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 23))

#define ads_MMecDamageInitiationDuctileTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 24))

#define ads_MMecDamageInitiationFLDTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 25))

#define ads_MMecDamageInitiationFLSDTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 26))

/** Data to define FailStrain */
#define ads_MMecDamageInitiationFailStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 27))

/** Data to define FailStress */
#define ads_MMecDamageInitiationFailStressTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 28))

#define ads_MMecDamageInitiationHCTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 29))

#define ads_MMecDamageInitiationHashinTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 30))

#define ads_MMecDamageInitiationHysteresisEnergyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 31))

#define ads_MMecDamageInitiationJCookTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 32))

#define ads_MMecDamageInitiationLaminaStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 33))

#define ads_MMecDamageInitiationLaminaStressTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 34))

#define ads_MMecDamageInitiationLarc05Table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 35))

#define ads_MMecDamageInitiationMKTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 36))

#define ads_MMecDamageInitiationMSFLDMajorMinorTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 37))

#define ads_MMecDamageInitiationMSFLDStrainRatioTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 38))

#define ads_MMecDamageInitiationPlyFabricTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 39))

#define ads_MMecDamageInitiationShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 40))

#define ads_MMecDamageInitiationTsaiWuETable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 41))

#define ads_MMecDamageInitiationTsaiWuTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 42))

#define ads_MMecDamageInitiationUserTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 43))

#define ads_MMecDamageStabilizationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 44))

#define ads_Prop_CMec_DamageEvolution (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 45))

#define ads_Prop_CMec_DamageEvolution_Displacement (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 46))

#define ads_Prop_CMec_DamageEvolution_Energy (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 47))

#define ads_Prop_CMec_DamageEvolution_Energy_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 48))

#define ads_Prop_CMec_DamageEvolution_Rotation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 49))

#define ads_Prop_CMec_DamageEvolution_TransRot (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 50))

#define ads_Prop_CMec_DamageEvolution_TransRot_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 51))

#define ads_Prop_CMec_DamageEvolution_TransRot_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 52))

#define ads_Prop_CMec_DamageEvolution_TransRot_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 53))

#define ads_Prop_CMec_DamageEvolution_TransRot_Linear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 54))

#define ads_Prop_CMec_DamageEvolution_TransRot_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 55))

#define ads_Prop_CMec_DamageEvolution_TransRot_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 56))

#define ads_Prop_CMec_DamageInitiation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 57))

#define ads_Prop_CMec_DamageInitiation_Load (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 58))

#define ads_Prop_CMec_DamageInitiation_Load_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 59))

#define ads_Prop_CMec_DamageInitiation_Motion (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 60))

#define ads_Prop_CMec_DamageInitiation_Motion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 61))

#define ads_Prop_CMec_DamageInitiation_PlasticMotion (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 62))

#define ads_Prop_CMec_DamageInitiation_PlasticMotion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 63))

#define ads_Prop_IMec_DamageEvolution (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 64))

#define ads_Prop_IMec_DamageEvolution_Energy (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 65))

#define ads_Prop_IMec_DamageEvolution_Energy_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 66))

#define ads_Prop_IMec_DamageEvolution_Energy_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 67))

#define ads_Prop_IMec_DamageEvolution_Energy_MixedModeExpression (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 68))

#define ads_Prop_IMec_DamageEvolution_Energy_MixedModeExpression_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 69))

#define ads_Prop_IMec_DamageEvolution_Separation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 70))

#define ads_Prop_IMec_DamageEvolution_Separation_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 71))

#define ads_Prop_IMec_DamageEvolution_Separation_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 72))

#define ads_Prop_IMec_DamageEvolution_Separation_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 73))

#define ads_Prop_IMec_DamageEvolution_Separation_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 74))

#define ads_Prop_IMec_DamageInitiation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 75))

#define ads_Prop_IMec_DamageInitiation_Separation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 76))

#define ads_Prop_IMec_DamageInitiation_Separation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 77))

#define ads_Prop_IMec_DamageInitiation_Traction (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 78))

#define ads_Prop_IMec_DamageInitiation_Traction_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 79))

#define ads_Prop_MMec_DamageEvolution (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 80))

/** define the evolution of damage as a function of the total (for elastic materials in cohesive elements) or the plastic (for bulk elastic-plastic materials) displacement after the initiation of damage. */
#define ads_Prop_MMec_DamageEvolution_Displacement (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 81))

#define ads_Prop_MMec_DamageEvolution_Displacement_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 82))

#define ads_Prop_MMec_DamageEvolution_Displacement_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 83))

#define ads_Prop_MMec_DamageEvolution_Displacement_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 84))

#define ads_Prop_MMec_DamageEvolution_Displacement_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 85))

/** define the evolution of damage in terms of the energy required for failure (fracture energy) after the initiation of damage. */
#define ads_Prop_MMec_DamageEvolution_Energy (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 86))

#define ads_Prop_MMec_DamageEvolution_Energy_Exponential (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 87))

#define ads_Prop_MMec_DamageEvolution_Energy_Exponential_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 88))

#define ads_Prop_MMec_DamageEvolution_Energy_LinearHashin (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 89))

#define ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 90))

#define ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 91))

#define ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 92))

#define ads_Prop_MMec_DamageEvolution_Hysteresis (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 93))

#define ads_Prop_MMec_DamageEvolution_Hysteresis_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 94))

#define ads_Prop_MMec_DamageInitiation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 95))

/** specify a damage initiation criterion based on the maximum separation criterion for cohesive elements. */
#define ads_Prop_MMec_DamageInitiation_ContactSeparation (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 96))

#define ads_Prop_MMec_DamageInitiation_ContactSeparation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 97))

/** specify a damage initiation criterion based on the ductile failure strain. */
#define ads_Prop_MMec_DamageInitiation_Ductile (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 98))

#define ads_Prop_MMec_DamageInitiation_Ductile_lodeTable (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 99))

#define ads_Prop_MMec_DamageInitiation_Ductile_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 100))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_FLD (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 101))

#define ads_Prop_MMec_DamageInitiation_FLD_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 102))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_FLSD (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 103))

#define ads_Prop_MMec_DamageInitiation_FLSD_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 104))

/** Define parameters for strain-based failure measures. */
#define ads_Prop_MMec_DamageInitiation_FailStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 105))

#define ads_Prop_MMec_DamageInitiation_FailStrain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 106))

/** Define parameters for stress-based failure measures. */
#define ads_Prop_MMec_DamageInitiation_FailStress (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 107))

#define ads_Prop_MMec_DamageInitiation_FailStress_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 108))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_HC (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 109))

#define ads_Prop_MMec_DamageInitiation_HC_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 110))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_Hashin (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 111))

#define ads_Prop_MMec_DamageInitiation_Hashin_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 112))

#define ads_Prop_MMec_DamageInitiation_HysteresisEnergy (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 113))

#define ads_Prop_MMec_DamageInitiation_HysteresisEnergy_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 114))

/** specify a damage initiation criterion based on the Johnson-Cook failure strain. */
#define ads_Prop_MMec_DamageInitiation_JCook (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 115))

#define ads_Prop_MMec_DamageInitiation_JCook_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 116))

/** specify a damage initiation criterion based on the maximum nominal strain for cohesive elements. */
#define ads_Prop_MMec_DamageInitiation_LaminaStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 117))

#define ads_Prop_MMec_DamageInitiation_LaminaStrain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 118))

/** specify a damage initiation criterion based on the maximum nominal stress criterion for cohesive elements. */
#define ads_Prop_MMec_DamageInitiation_LaminaStress (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 119))

#define ads_Prop_MMec_DamageInitiation_LaminaStress_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 120))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_Larc05 (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 121))

#define ads_Prop_MMec_DamageInitiation_Larc05_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 122))

/** specify a damage initiation criterion based on a Marciniak-Kuczynski analysis. */
#define ads_Prop_MMec_DamageInitiation_MK (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 123))

#define ads_Prop_MMec_DamageInitiation_MK_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 124))

#define ads_Prop_MMec_DamageInitiation_MSFLD (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 125))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 126))

#define ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 127))

/** Specify a damage initiation criterion based on the Muschenborn and Sonne forming limit diagram. specify the MSFLD damage initiation criterion by providing the limit equivalent plastic strain as a tabular function of alpha. */
#define ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 128))

#define ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 129))

/** Specify material properties to define the initiation of damage. */
#define ads_Prop_MMec_DamageInitiation_PlyFabric (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 130))

#define ads_Prop_MMec_DamageInitiation_PlyFabric_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 131))

/** specify a damage initiation criterion based on the shear failure strain. */
#define ads_Prop_MMec_DamageInitiation_Shear (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 132))

#define ads_Prop_MMec_DamageInitiation_Shear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 133))

/** Specify a damage initiation based on the TsaiWu criterion. */
#define ads_Prop_MMec_DamageInitiation_TsaiWu (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 134))

/** Specify a damage initiation based on the TsaiWuE criterion. */
#define ads_Prop_MMec_DamageInitiation_TsaiWuE (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 135))

#define ads_Prop_MMec_DamageInitiation_TsaiWuE_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 136))

#define ads_Prop_MMec_DamageInitiation_TsaiWu_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 137))

/** Specify a user-defined damage initiation criterion for enriched elements. */
#define ads_Prop_MMec_DamageInitiation_User (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 138))

#define ads_Prop_MMec_DamageInitiation_User_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 139))

/** Specify viscosity coefficients for the damage model for fiber reinforced materials. */
#define ads_Prop_MMec_DamageStabilization (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 140))

#define ads_Prop_MMec_DamageStabilization_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 141))

/** Specify material properties to define ply fabric. */
#define ads_Prop_MMec_PlyFabric (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 142))

/** Specify material properties to define ply fabric failure. */
#define ads_Prop_MMec_PlyFabric_Failure (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 143))

#define ads_Prop_MMec_PlyFabric_Failure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 144))

/** Specify material properties to define ply fabric hardening. */
#define ads_Prop_MMec_PlyFabric_Hardening (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 145))

/** ply fabric hardening with Johnson-Cook type */
#define ads_Prop_MMec_PlyFabric_Hardening_JCook (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 146))

#define ads_Prop_MMec_PlyFabric_Hardening_JCook_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 147))

/** ply fabric hardening with tabular type */
#define ads_Prop_MMec_PlyFabric_Hardening_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 148))

#define ads_Prop_MMec_PlyFabric_Hardening_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyDamageFragment, 149))

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolutionMembersEnm
{
    ads_Prop_CMec_DamageEvolution_degradation
};

enum ads_Prop_CMec_DamageEvolution_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_degradation_MULTIPLICATIVE
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_DisplacementMembersEnm
{
    ads_Prop_CMec_DamageEvolution_Displacement_degradation
};

enum ads_Prop_CMec_DamageEvolution_Displacement_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_Displacement_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_Displacement_degradation_MULTIPLICATIVE
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_EnergyMembersEnm
{
    ads_Prop_CMec_DamageEvolution_Energy_degradation
};

enum ads_Prop_CMec_DamageEvolution_Energy_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_Energy_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_Energy_degradation_MULTIPLICATIVE
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_Energy_tableRolesEnm
{
    ads_Prop_CMec_DamageEvolution_Energy_table_child,
    ads_Prop_CMec_DamageEvolution_Energy_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_RotationMembersEnm
{
    ads_Prop_CMec_DamageEvolution_Rotation_degradation
};

enum ads_Prop_CMec_DamageEvolution_Rotation_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_Rotation_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_Rotation_degradation_MULTIPLICATIVE
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_TransRotMembersEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_degradation
};

enum ads_Prop_CMec_DamageEvolution_TransRot_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_TransRot_degradation_MULTIPLICATIVE
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_TransRot_ExponentialMembersEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_degradation
};

enum ads_Prop_CMec_DamageEvolution_TransRot_Exponential_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_degradation_MULTIPLICATIVE
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Exponential_tableRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_table_child,
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_TransRot_LinearMembersEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_degradation
};

enum ads_Prop_CMec_DamageEvolution_TransRot_Linear_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_degradation_MULTIPLICATIVE
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Linear_tableRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_table_child,
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageEvolution_TransRot_TabularMembersEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_degradation
};

enum ads_Prop_CMec_DamageEvolution_TransRot_Tabular_degradationEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_degradation_MAXIMUM,
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_degradation_MULTIPLICATIVE
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Tabular_tableRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_table_child,
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageInitiation_Load_tableRolesEnm
{
    ads_Prop_CMec_DamageInitiation_Load_table_child,
    ads_Prop_CMec_DamageInitiation_Load_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageInitiation_Motion_tableRolesEnm
{
    ads_Prop_CMec_DamageInitiation_Motion_table_child,
    ads_Prop_CMec_DamageInitiation_Motion_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_DamageInitiation_PlasticMotionMembersEnm
{
    ads_Prop_CMec_DamageInitiation_PlasticMotion_rateFilterFactor,
    ads_Prop_CMec_DamageInitiation_PlasticMotion_rateInterpolation
};

enum ads_Prop_CMec_DamageInitiation_PlasticMotion_rateInterpolationEnm
{
    ads_Prop_CMec_DamageInitiation_PlasticMotion_rateInterpolation_LINEAR,
    ads_Prop_CMec_DamageInitiation_PlasticMotion_rateInterpolation_LOGARITHMIC
};

/** Enum with association roles. */
enum ads_Prop_CMec_DamageInitiation_PlasticMotion_tableRolesEnm
{
    ads_Prop_CMec_DamageInitiation_PlasticMotion_table_child,
    ads_Prop_CMec_DamageInitiation_PlasticMotion_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageEvolution_Energy_Exponential_tableRolesEnm
{
    ads_Prop_IMec_DamageEvolution_Energy_Exponential_table_child,
    ads_Prop_IMec_DamageEvolution_Energy_Exponential_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageEvolution_Energy_MixedModeExpression_tableRolesEnm
{
    ads_Prop_IMec_DamageEvolution_Energy_MixedModeExpression_table_child,
    ads_Prop_IMec_DamageEvolution_Energy_MixedModeExpression_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageEvolution_Separation_Exponential_tableRolesEnm
{
    ads_Prop_IMec_DamageEvolution_Separation_Exponential_table_child,
    ads_Prop_IMec_DamageEvolution_Separation_Exponential_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageEvolution_Separation_Tabular_tableRolesEnm
{
    ads_Prop_IMec_DamageEvolution_Separation_Tabular_table_child,
    ads_Prop_IMec_DamageEvolution_Separation_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageInitiation_Separation_tableRolesEnm
{
    ads_Prop_IMec_DamageInitiation_Separation_table_child,
    ads_Prop_IMec_DamageInitiation_Separation_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_DamageInitiation_Traction_tableRolesEnm
{
    ads_Prop_IMec_DamageInitiation_Traction_table_child,
    ads_Prop_IMec_DamageInitiation_Traction_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolutionMembersEnm
{
    ads_Prop_MMec_DamageEvolution_degradation,
    ads_Prop_MMec_DamageEvolution_failureIndex,
    ads_Prop_MMec_DamageEvolution_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_power,
    ads_Prop_MMec_DamageEvolution_rateDependent,
    ads_Prop_MMec_DamageEvolution_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_refEnergy,
    ads_Prop_MMec_DamageEvolution_softening
};

enum ads_Prop_MMec_DamageEvolution_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_softening_TABULAR
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_DisplacementMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_degradation,
    ads_Prop_MMec_DamageEvolution_Displacement_failureIndex,
    ads_Prop_MMec_DamageEvolution_Displacement_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Displacement_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Displacement_power,
    ads_Prop_MMec_DamageEvolution_Displacement_rateDependent,
    ads_Prop_MMec_DamageEvolution_Displacement_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Displacement_refEnergy,
    ads_Prop_MMec_DamageEvolution_Displacement_softening
};

enum ads_Prop_MMec_DamageEvolution_Displacement_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Displacement_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Displacement_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Displacement_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Displacement_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Displacement_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Displacement_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Displacement_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Displacement_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Displacement_softening_TABULAR
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_Displacement_ExponentialMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_degradation,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_failureIndex,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_power,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_rateDependent,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_refEnergy,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_softening
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Exponential_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Exponential_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Exponential_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Exponential_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Displacement_Exponential_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_table_child,
    ads_Prop_MMec_DamageEvolution_Displacement_Exponential_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_Displacement_TabularMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_degradation,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_failureIndex,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_power,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_rateDependent,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_refEnergy,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_softening
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Tabular_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Tabular_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Tabular_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Displacement_Tabular_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Displacement_Tabular_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_table_child,
    ads_Prop_MMec_DamageEvolution_Displacement_Tabular_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_EnergyMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_degradation,
    ads_Prop_MMec_DamageEvolution_Energy_failureIndex,
    ads_Prop_MMec_DamageEvolution_Energy_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Energy_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Energy_power,
    ads_Prop_MMec_DamageEvolution_Energy_rateDependent,
    ads_Prop_MMec_DamageEvolution_Energy_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Energy_refEnergy,
    ads_Prop_MMec_DamageEvolution_Energy_softening
};

enum ads_Prop_MMec_DamageEvolution_Energy_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Energy_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Energy_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Energy_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Energy_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Energy_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Energy_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Energy_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Energy_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Energy_softening_TABULAR
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_Energy_ExponentialMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_degradation,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_failureIndex,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_power,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_rateDependent,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_refEnergy,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_softening
};

enum ads_Prop_MMec_DamageEvolution_Energy_Exponential_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Energy_Exponential_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Energy_Exponential_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Energy_Exponential_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Energy_Exponential_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_table_child,
    ads_Prop_MMec_DamageEvolution_Energy_Exponential_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashinMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_degradation,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_failureIndex,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_power,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_rateDependent,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_refEnergy,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_softening
};

enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_table_child,
    ads_Prop_MMec_DamageEvolution_Energy_LinearHashin_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpressionMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_degradation,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_failureIndex,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_power,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_rateDependent,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_refEnergy,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_softening
};

enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_table_child,
    ads_Prop_MMec_DamageEvolution_Energy_MixedModeExpression_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageEvolution_HysteresisMembersEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_degradation,
    ads_Prop_MMec_DamageEvolution_Hysteresis_failureIndex,
    ads_Prop_MMec_DamageEvolution_Hysteresis_mixedModeBehavior,
    ads_Prop_MMec_DamageEvolution_Hysteresis_modeMixRatio,
    ads_Prop_MMec_DamageEvolution_Hysteresis_power,
    ads_Prop_MMec_DamageEvolution_Hysteresis_rateDependent,
    ads_Prop_MMec_DamageEvolution_Hysteresis_rateOfSeparation,
    ads_Prop_MMec_DamageEvolution_Hysteresis_refEnergy,
    ads_Prop_MMec_DamageEvolution_Hysteresis_softening
};

enum ads_Prop_MMec_DamageEvolution_Hysteresis_degradationEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_degradation_MAXIMUM,
    ads_Prop_MMec_DamageEvolution_Hysteresis_degradation_MULTIPLICATIVE
};

enum ads_Prop_MMec_DamageEvolution_Hysteresis_mixedModeBehaviorEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_mixedModeBehavior_BK,
    ads_Prop_MMec_DamageEvolution_Hysteresis_mixedModeBehavior_POWER_LAW,
    ads_Prop_MMec_DamageEvolution_Hysteresis_mixedModeBehavior_TABULAR
};

enum ads_Prop_MMec_DamageEvolution_Hysteresis_modeMixRatioEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_modeMixRatio_ENERGY,
    ads_Prop_MMec_DamageEvolution_Hysteresis_modeMixRatio_TRACTION
};

enum ads_Prop_MMec_DamageEvolution_Hysteresis_softeningEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_softening_EXPONENTIAL,
    ads_Prop_MMec_DamageEvolution_Hysteresis_softening_LINEAR,
    ads_Prop_MMec_DamageEvolution_Hysteresis_softening_TABULAR
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageEvolution_Hysteresis_tableRolesEnm
{
    ads_Prop_MMec_DamageEvolution_Hysteresis_table_child,
    ads_Prop_MMec_DamageEvolution_Hysteresis_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageInitiationMembersEnm
{
    ads_Prop_MMec_DamageInitiation_accumulationPower,
    ads_Prop_MMec_DamageInitiation_rateDependent,
    ads_Prop_MMec_DamageInitiation_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_refEnergy
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_ContactSeparationMembersEnm
{
    ads_Prop_MMec_DamageInitiation_ContactSeparation_accumulationPower,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_rateDependent,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_refEnergy,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_criterion
};

enum ads_Prop_MMec_DamageInitiation_ContactSeparation_criterionEnm
{
    ads_Prop_MMec_DamageInitiation_ContactSeparation_criterion_MAXU,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_criterion_QUADU
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_ContactSeparation_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_ContactSeparation_table_child,
    ads_Prop_MMec_DamageInitiation_ContactSeparation_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_DuctileMembersEnm
{
    ads_Prop_MMec_DamageInitiation_Ductile_accumulationPower,
    ads_Prop_MMec_DamageInitiation_Ductile_rateDependent,
    ads_Prop_MMec_DamageInitiation_Ductile_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_Ductile_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_Ductile_lodeTableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_Ductile_lodeTable_child,
    ads_Prop_MMec_DamageInitiation_Ductile_lodeTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_Ductile_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_Ductile_table_child,
    ads_Prop_MMec_DamageInitiation_Ductile_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_FLDMembersEnm
{
    ads_Prop_MMec_DamageInitiation_FLD_accumulationPower,
    ads_Prop_MMec_DamageInitiation_FLD_rateDependent,
    ads_Prop_MMec_DamageInitiation_FLD_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_FLD_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_FLD_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_FLD_table_child,
    ads_Prop_MMec_DamageInitiation_FLD_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_FLSDMembersEnm
{
    ads_Prop_MMec_DamageInitiation_FLSD_accumulationPower,
    ads_Prop_MMec_DamageInitiation_FLSD_rateDependent,
    ads_Prop_MMec_DamageInitiation_FLSD_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_FLSD_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_FLSD_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_FLSD_table_child,
    ads_Prop_MMec_DamageInitiation_FLSD_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_FailStrainMembersEnm
{
    ads_Prop_MMec_DamageInitiation_FailStrain_accumulationPower,
    ads_Prop_MMec_DamageInitiation_FailStrain_rateDependent,
    ads_Prop_MMec_DamageInitiation_FailStrain_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_FailStrain_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_FailStrain_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_FailStrain_table_child,
    ads_Prop_MMec_DamageInitiation_FailStrain_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_FailStressMembersEnm
{
    ads_Prop_MMec_DamageInitiation_FailStress_accumulationPower,
    ads_Prop_MMec_DamageInitiation_FailStress_rateDependent,
    ads_Prop_MMec_DamageInitiation_FailStress_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_FailStress_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_FailStress_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_FailStress_table_child,
    ads_Prop_MMec_DamageInitiation_FailStress_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_HCMembersEnm
{
    ads_Prop_MMec_DamageInitiation_HC_accumulationPower,
    ads_Prop_MMec_DamageInitiation_HC_rateDependent,
    ads_Prop_MMec_DamageInitiation_HC_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_HC_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_HC_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_HC_table_child,
    ads_Prop_MMec_DamageInitiation_HC_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_HashinMembersEnm
{
    ads_Prop_MMec_DamageInitiation_Hashin_accumulationPower,
    ads_Prop_MMec_DamageInitiation_Hashin_rateDependent,
    ads_Prop_MMec_DamageInitiation_Hashin_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_Hashin_refEnergy,
    ads_Prop_MMec_DamageInitiation_Hashin_alpha
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_Hashin_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_Hashin_table_child,
    ads_Prop_MMec_DamageInitiation_Hashin_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_HysteresisEnergyMembersEnm
{
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_accumulationPower,
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_rateDependent,
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_HysteresisEnergy_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_table_child,
    ads_Prop_MMec_DamageInitiation_HysteresisEnergy_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_JCookMembersEnm
{
    ads_Prop_MMec_DamageInitiation_JCook_accumulationPower,
    ads_Prop_MMec_DamageInitiation_JCook_rateDependent,
    ads_Prop_MMec_DamageInitiation_JCook_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_JCook_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_JCook_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_JCook_table_child,
    ads_Prop_MMec_DamageInitiation_JCook_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_LaminaStrainMembersEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStrain_accumulationPower,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_rateDependent,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_refEnergy,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_criterion
};

enum ads_Prop_MMec_DamageInitiation_LaminaStrain_criterionEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStrain_criterion_MAXE,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_criterion_QUADE
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_LaminaStrain_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStrain_table_child,
    ads_Prop_MMec_DamageInitiation_LaminaStrain_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_LaminaStressMembersEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStress_accumulationPower,
    ads_Prop_MMec_DamageInitiation_LaminaStress_rateDependent,
    ads_Prop_MMec_DamageInitiation_LaminaStress_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_LaminaStress_refEnergy,
    ads_Prop_MMec_DamageInitiation_LaminaStress_criterion
};

enum ads_Prop_MMec_DamageInitiation_LaminaStress_criterionEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStress_criterion_MAXS,
    ads_Prop_MMec_DamageInitiation_LaminaStress_criterion_QUADS
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_LaminaStress_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_LaminaStress_table_child,
    ads_Prop_MMec_DamageInitiation_LaminaStress_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_Larc05MembersEnm
{
    ads_Prop_MMec_DamageInitiation_Larc05_accumulationPower,
    ads_Prop_MMec_DamageInitiation_Larc05_rateDependent,
    ads_Prop_MMec_DamageInitiation_Larc05_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_Larc05_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_Larc05_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_Larc05_table_child,
    ads_Prop_MMec_DamageInitiation_Larc05_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_MKMembersEnm
{
    ads_Prop_MMec_DamageInitiation_MK_accumulationPower,
    ads_Prop_MMec_DamageInitiation_MK_rateDependent,
    ads_Prop_MMec_DamageInitiation_MK_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_MK_refEnergy,
    ads_Prop_MMec_DamageInitiation_MK_feq,
    ads_Prop_MMec_DamageInitiation_MK_fnn,
    ads_Prop_MMec_DamageInitiation_MK_fnt,
    ads_Prop_MMec_DamageInitiation_MK_frequency,
    ads_Prop_MMec_DamageInitiation_MK_numberImperfections
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_MK_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_MK_table_child,
    ads_Prop_MMec_DamageInitiation_MK_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_MSFLDMembersEnm
{
    ads_Prop_MMec_DamageInitiation_MSFLD_accumulationPower,
    ads_Prop_MMec_DamageInitiation_MSFLD_rateDependent,
    ads_Prop_MMec_DamageInitiation_MSFLD_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_MSFLD_refEnergy,
    ads_Prop_MMec_DamageInitiation_MSFLD_omega
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinorMembersEnm
{
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_accumulationPower,
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_rateDependent,
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_refEnergy,
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_omega
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_table_child,
    ads_Prop_MMec_DamageInitiation_MSFLD_MajorMinor_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatioMembersEnm
{
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_accumulationPower,
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_rateDependent,
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_refEnergy,
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_omega
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_table_child,
    ads_Prop_MMec_DamageInitiation_MSFLD_StrainRatio_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_PlyFabricMembersEnm
{
    ads_Prop_MMec_DamageInitiation_PlyFabric_accumulationPower,
    ads_Prop_MMec_DamageInitiation_PlyFabric_rateDependent,
    ads_Prop_MMec_DamageInitiation_PlyFabric_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_PlyFabric_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_PlyFabric_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_PlyFabric_table_child,
    ads_Prop_MMec_DamageInitiation_PlyFabric_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_ShearMembersEnm
{
    ads_Prop_MMec_DamageInitiation_Shear_accumulationPower,
    ads_Prop_MMec_DamageInitiation_Shear_rateDependent,
    ads_Prop_MMec_DamageInitiation_Shear_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_Shear_refEnergy,
    ads_Prop_MMec_DamageInitiation_Shear_ks
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_Shear_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_Shear_table_child,
    ads_Prop_MMec_DamageInitiation_Shear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_TsaiWuMembersEnm
{
    ads_Prop_MMec_DamageInitiation_TsaiWu_accumulationPower,
    ads_Prop_MMec_DamageInitiation_TsaiWu_rateDependent,
    ads_Prop_MMec_DamageInitiation_TsaiWu_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_TsaiWu_refEnergy
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_TsaiWuEMembersEnm
{
    ads_Prop_MMec_DamageInitiation_TsaiWuE_accumulationPower,
    ads_Prop_MMec_DamageInitiation_TsaiWuE_rateDependent,
    ads_Prop_MMec_DamageInitiation_TsaiWuE_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_TsaiWuE_refEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_TsaiWuE_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_TsaiWuE_table_child,
    ads_Prop_MMec_DamageInitiation_TsaiWuE_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_TsaiWu_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_TsaiWu_table_child,
    ads_Prop_MMec_DamageInitiation_TsaiWu_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_DamageInitiation_UserMembersEnm
{
    ads_Prop_MMec_DamageInitiation_User_accumulationPower,
    ads_Prop_MMec_DamageInitiation_User_rateDependent,
    ads_Prop_MMec_DamageInitiation_User_rateOfSeparation,
    ads_Prop_MMec_DamageInitiation_User_refEnergy,
    ads_Prop_MMec_DamageInitiation_User_failmech,
    ads_Prop_MMec_DamageInitiation_User_properties
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageInitiation_User_tableRolesEnm
{
    ads_Prop_MMec_DamageInitiation_User_table_child,
    ads_Prop_MMec_DamageInitiation_User_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_DamageStabilization_tableRolesEnm
{
    ads_Prop_MMec_DamageStabilization_table_child,
    ads_Prop_MMec_DamageStabilization_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_PlyFabric_FailureMembersEnm
{
    ads_Prop_MMec_PlyFabric_Failure_numFiber
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlyFabric_Failure_tableRolesEnm
{
    ads_Prop_MMec_PlyFabric_Failure_table_child,
    ads_Prop_MMec_PlyFabric_Failure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlyFabric_Hardening_JCook_tableRolesEnm
{
    ads_Prop_MMec_PlyFabric_Hardening_JCook_table_child,
    ads_Prop_MMec_PlyFabric_Hardening_JCook_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_PlyFabric_Hardening_Tabular_tableRolesEnm
{
    ads_Prop_MMec_PlyFabric_Hardening_Tabular_table_child,
    ads_Prop_MMec_PlyFabric_Hardening_Tabular_table_parent
};

#endif
