//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyElasticOtherC_h
#define ads_CorePropertyElasticOtherC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyElasticOther of the latest level of form Core */

#define ads_MMecEOSGasSpecificHeatTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 0))

#define ads_MMecEOSIdealGasGasConstantTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 1))

#define ads_MMecEOSIdealGasMolecularWeightTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 2))

#define ads_MMecEOSIgnitionAndGrowthTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 3))

#define ads_MMecEOSJWLTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 4))

#define ads_MMecEOSOptionCompactionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 5))

#define ads_MMecEOSOptionDetonationPointTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 6))

#define ads_MMecEOSPneumaticTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 7))

#define ads_MMecEOSReactionRateTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 8))

#define ads_MMecEOSTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 9))

#define ads_MMecEOSUSUPTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 10))

#define ads_MMecElasticDiscreteTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 11))

/** Data to define Hypoelastic */
#define ads_MMecElasticHypoInvariantsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 12))

#define ads_MMecElasticHypoPorousPoissonTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 13))

#define ads_MMecElasticHypoPorousShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 14))

#define ads_MMecElasticHypoRambergOsgoodTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 15))

/** Data for the superelastic material model. */
#define ads_MMecElasticSuperelasticHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 16))

/** Data for the superelastic material model. */
#define ads_MMecElasticSuperelasticModifiedHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 17))

/** Data for the superelastic material model. */
#define ads_MMecElasticSuperelasticTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 18))

#define ads_MMecEosRealGasTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 19))

#define ads_MMecTensileFailureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 20))

#define ads_Prop_MMec_EOS (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 21))

#define ads_Prop_MMec_EOSOption (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 22))

/** Specify plastic compaction behavior for an equation of state model of type UsUp. */
#define ads_Prop_MMec_EOSOption_Compaction (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 23))

#define ads_Prop_MMec_EOSOption_Compaction_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 24))

/** Defines the detonation point for the explosive equation of state. */
#define ads_Prop_MMec_EOSOption_DetonationPoint (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 25))

#define ads_Prop_MMec_EOSOption_DetonationPoint_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 26))

/** Defines the specific heat of reacted gas products for an ignition and growth equation of state. */
#define ads_Prop_MMec_EOSOption_GasSpecificHeat (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 27))

#define ads_Prop_MMec_EOSOption_GasSpecificHeat_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 28))

/** Defines the reaction rate for an ignition and growth equation of state. */
#define ads_Prop_MMec_EOSOption_ReactionRate (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 29))

#define ads_Prop_MMec_EOSOption_ReactionRate_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 30))

#define ads_Prop_MMec_EOS_IdealGas (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 31))

#define ads_Prop_MMec_EOS_IdealGas_GasConstant (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 32))

#define ads_Prop_MMec_EOS_IdealGas_GasConstant_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 33))

#define ads_Prop_MMec_EOS_IdealGas_MolecularWeight (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 34))

#define ads_Prop_MMec_EOS_IdealGas_MolecularWeight_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 35))

#define ads_Prop_MMec_EOS_IgnitionAndGrowth (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 36))

#define ads_Prop_MMec_EOS_IgnitionAndGrowth_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 37))

/** Specify an equation of state model for explosive. */
#define ads_Prop_MMec_EOS_JWL (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 38))

#define ads_Prop_MMec_EOS_JWL_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 39))

#define ads_Prop_MMec_EOS_Pneumatic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 40))

#define ads_Prop_MMec_EOS_Pneumatic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 41))

#define ads_Prop_MMec_EOS_RealGas (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 42))

#define ads_Prop_MMec_EOS_RealGas_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 43))

#define ads_Prop_MMec_EOS_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 44))

#define ads_Prop_MMec_EOS_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 45))

#define ads_Prop_MMec_EOS_Tait (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 46))

/** Specify an equation of state model for explosive. */
#define ads_Prop_MMec_EOS_USUP (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 47))

#define ads_Prop_MMec_EOS_USUP_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 48))

/** Specify user-defined equation of state that is defined in user subroutine VUEOS. */
#define ads_Prop_MMec_EOS_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 49))

/** To define effective elastic moduli for PD3D elements */
#define ads_Prop_MMec_Elastic_Discrete (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 50))

#define ads_Prop_MMec_Elastic_Discrete_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 51))

#define ads_Prop_MMec_Elastic_Hypo (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 52))

/** Specify hypoelastic material properties. */
#define ads_Prop_MMec_Elastic_Hypo_Invariants (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 53))

#define ads_Prop_MMec_Elastic_Hypo_Invariants_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 54))

/** Specify elastic material properties for porous materials by giving the poisson ratio. */
#define ads_Prop_MMec_Elastic_Hypo_PorousPoisson (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 55))

#define ads_Prop_MMec_Elastic_Hypo_PorousPoisson_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 56))

/** Specify elastic material properties for porous materials by giving the shear modulus G. */
#define ads_Prop_MMec_Elastic_Hypo_PorousShear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 57))

#define ads_Prop_MMec_Elastic_Hypo_PorousShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 58))

/** Specify the deformation plasticity model. */
#define ads_Prop_MMec_Elastic_Hypo_RambergOsgood (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 59))

#define ads_Prop_MMec_Elastic_Hypo_RambergOsgood_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 60))

/** Specify user hypoelastic material properties. */
#define ads_Prop_MMec_Elastic_Hypo_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 61))

#define ads_Prop_MMec_Elastic_Super (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 62))

#define ads_Prop_MMec_Elastic_Super_Superelastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 63))

#define ads_Prop_MMec_Elastic_Super_SuperelasticHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 64))

#define ads_Prop_MMec_Elastic_Super_SuperelasticHardening_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 65))

#define ads_Prop_MMec_Elastic_Super_SuperelasticHardening_modificationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 66))

#define ads_Prop_MMec_Elastic_Super_SuperelasticHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 67))

#define ads_Prop_MMec_Elastic_Super_Superelastic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 68))

/** This option is used with the Mises or the Johnson-Cook plasticity models or the equation of state model to specify a tensile failure model and criterion. */
#define ads_Prop_MMec_TensileFailure (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 69))

#define ads_Prop_MMec_TensileFailure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticOtherFragment, 70))

/** Enum with association roles. */
enum ads_Prop_MMec_EOSOption_Compaction_tableRolesEnm
{
    ads_Prop_MMec_EOSOption_Compaction_table_child,
    ads_Prop_MMec_EOSOption_Compaction_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOSOption_DetonationPoint_tableRolesEnm
{
    ads_Prop_MMec_EOSOption_DetonationPoint_table_child,
    ads_Prop_MMec_EOSOption_DetonationPoint_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOSOption_GasSpecificHeat_tableRolesEnm
{
    ads_Prop_MMec_EOSOption_GasSpecificHeat_table_child,
    ads_Prop_MMec_EOSOption_GasSpecificHeat_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOSOption_ReactionRate_tableRolesEnm
{
    ads_Prop_MMec_EOSOption_ReactionRate_table_child,
    ads_Prop_MMec_EOSOption_ReactionRate_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_IdealGas_GasConstant_tableRolesEnm
{
    ads_Prop_MMec_EOS_IdealGas_GasConstant_table_child,
    ads_Prop_MMec_EOS_IdealGas_GasConstant_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_IdealGas_MolecularWeight_tableRolesEnm
{
    ads_Prop_MMec_EOS_IdealGas_MolecularWeight_table_child,
    ads_Prop_MMec_EOS_IdealGas_MolecularWeight_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_EOS_IgnitionAndGrowthMembersEnm
{
    ads_Prop_MMec_EOS_IgnitionAndGrowth_detonationEnergy
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_IgnitionAndGrowth_tableRolesEnm
{
    ads_Prop_MMec_EOS_IgnitionAndGrowth_table_child,
    ads_Prop_MMec_EOS_IgnitionAndGrowth_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_JWL_tableRolesEnm
{
    ads_Prop_MMec_EOS_JWL_table_child,
    ads_Prop_MMec_EOS_JWL_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_EOS_PneumaticMembersEnm
{
    ads_Prop_MMec_EOS_Pneumatic_pressure,
    ads_Prop_MMec_EOS_Pneumatic_temperature
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_Pneumatic_tableRolesEnm
{
    ads_Prop_MMec_EOS_Pneumatic_table_child,
    ads_Prop_MMec_EOS_Pneumatic_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_EOS_RealGasMembersEnm
{
    ads_Prop_MMec_EOS_RealGas_modelType
};

enum ads_Prop_MMec_EOS_RealGas_modelTypeEnm
{
    ads_Prop_MMec_EOS_RealGas_modelType_AUNGIER_REDLICH_KWONG,
    ads_Prop_MMec_EOS_RealGas_modelType_PENG_ROBINSON,
    ads_Prop_MMec_EOS_RealGas_modelType_REDLICH_KWONG,
    ads_Prop_MMec_EOS_RealGas_modelType_SOAVE_REDLICH_KWONG,
    ads_Prop_MMec_EOS_RealGas_modelType_VANDERWAALS
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_RealGas_tableRolesEnm
{
    ads_Prop_MMec_EOS_RealGas_table_child,
    ads_Prop_MMec_EOS_RealGas_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_Tabular_tableRolesEnm
{
    ads_Prop_MMec_EOS_Tabular_table_child,
    ads_Prop_MMec_EOS_Tabular_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_EOS_TaitMembersEnm
{
    ads_Prop_MMec_EOS_Tait_coeffB,
    ads_Prop_MMec_EOS_Tait_coeffC,
    ads_Prop_MMec_EOS_Tait_referenceDensity,
    ads_Prop_MMec_EOS_Tait_referencePressure
};

/** Enum with association roles. */
enum ads_Prop_MMec_EOS_USUP_tableRolesEnm
{
    ads_Prop_MMec_EOS_USUP_table_child,
    ads_Prop_MMec_EOS_USUP_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_DiscreteMembersEnm
{
    ads_Prop_MMec_Elastic_Discrete_moduli
};

enum ads_Prop_MMec_Elastic_Discrete_moduliEnm
{
    ads_Prop_MMec_Elastic_Discrete_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Discrete_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Discrete_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Discrete_table_child,
    ads_Prop_MMec_Elastic_Discrete_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_HypoMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_moduli_LONG_TERM
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hypo_InvariantsMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_Invariants_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_Invariants_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_Invariants_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_Invariants_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hypo_Invariants_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hypo_Invariants_table_child,
    ads_Prop_MMec_Elastic_Hypo_Invariants_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hypo_PorousPoissonMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousPoisson_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_PorousPoisson_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousPoisson_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_PorousPoisson_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hypo_PorousPoisson_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousPoisson_table_child,
    ads_Prop_MMec_Elastic_Hypo_PorousPoisson_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hypo_PorousShearMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousShear_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_PorousShear_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousShear_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_PorousShear_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hypo_PorousShear_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hypo_PorousShear_table_child,
    ads_Prop_MMec_Elastic_Hypo_PorousShear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hypo_RambergOsgoodMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_RambergOsgood_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_RambergOsgood_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_RambergOsgood_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_RambergOsgood_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hypo_RambergOsgood_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hypo_RambergOsgood_table_child,
    ads_Prop_MMec_Elastic_Hypo_RambergOsgood_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hypo_UserMembersEnm
{
    ads_Prop_MMec_Elastic_Hypo_User_moduli
};

enum ads_Prop_MMec_Elastic_Hypo_User_moduliEnm
{
    ads_Prop_MMec_Elastic_Hypo_User_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hypo_User_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_SuperMembersEnm
{
    ads_Prop_MMec_Elastic_Super_moduli
};

enum ads_Prop_MMec_Elastic_Super_moduliEnm
{
    ads_Prop_MMec_Elastic_Super_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Super_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Super_SuperelasticMembersEnm
{
    ads_Prop_MMec_Elastic_Super_Superelastic_moduli,
    ads_Prop_MMec_Elastic_Super_Superelastic_volumetricStrain
};

enum ads_Prop_MMec_Elastic_Super_Superelastic_moduliEnm
{
    ads_Prop_MMec_Elastic_Super_Superelastic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Super_Superelastic_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Super_SuperelasticHardeningMembersEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_moduli
};

enum ads_Prop_MMec_Elastic_Super_SuperelasticHardening_moduliEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Super_SuperelasticHardening_UserMembersEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_User_moduli
};

enum ads_Prop_MMec_Elastic_Super_SuperelasticHardening_User_moduliEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_User_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_User_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Super_SuperelasticHardening_modificationTableRolesEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_modificationTable_child,
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_modificationTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Super_SuperelasticHardening_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_table_child,
    ads_Prop_MMec_Elastic_Super_SuperelasticHardening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Super_Superelastic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Super_Superelastic_table_child,
    ads_Prop_MMec_Elastic_Super_Superelastic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_TensileFailureMembersEnm
{
    ads_Prop_MMec_TensileFailure_elementDeletion,
    ads_Prop_MMec_TensileFailure_pressure,
    ads_Prop_MMec_TensileFailure_shear
};

enum ads_Prop_MMec_TensileFailure_pressureEnm
{
    ads_Prop_MMec_TensileFailure_pressure_ABSENT,
    ads_Prop_MMec_TensileFailure_pressure_BRITTLE,
    ads_Prop_MMec_TensileFailure_pressure_DUCTILE
};

enum ads_Prop_MMec_TensileFailure_shearEnm
{
    ads_Prop_MMec_TensileFailure_shear_ABSENT,
    ads_Prop_MMec_TensileFailure_shear_BRITTLE,
    ads_Prop_MMec_TensileFailure_shear_DUCTILE
};

/** Enum with association roles. */
enum ads_Prop_MMec_TensileFailure_tableRolesEnm
{
    ads_Prop_MMec_TensileFailure_table_child,
    ads_Prop_MMec_TensileFailure_table_parent
};

#endif
