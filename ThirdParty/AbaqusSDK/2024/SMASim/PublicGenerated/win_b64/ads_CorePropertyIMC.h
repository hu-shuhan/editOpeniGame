//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyIMC_h
#define ads_CorePropertyIMC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyIM of the latest level of form Core */

#define ads_MMecIMCuringKamalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 0))

#define ads_MMecIMEOSModifiedTaitTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 1))

#define ads_MMecIMEOSSpencerGilmoreTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 2))

#define ads_MMecIMFiberReinforcementTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 3))

#define ads_MMecIMMetalParticleTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 4))

#define ads_MMecIMRecommendedValuesTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 5))

#define ads_MMecIMShearRelaxationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 6))

#define ads_MMecIMThermalExpansionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 7))

#define ads_MMecIMTransIsoElasTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 8))

#define ads_MMecIMViscosityCampusCarreauWLFTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 9))

#define ads_MMecIMViscosityCrossArrTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 10))

#define ads_MMecIMViscosityCrossWLFTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 11))

#define ads_MMecIMViscosityMacoskoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 12))

#define ads_MMecIMViscosityPowerLawExpTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 13))

#define ads_MThermalIMCharacteristicTemperaturesTable (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 14))

/** Injection Molding material properties */
#define ads_Prop_MMec_IM (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 15))

/** Injection Molding Curing material properties */
#define ads_Prop_MMec_IM_Curing (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 16))

/** Kamal Curing model */
#define ads_Prop_MMec_IM_Curing_Kamal (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 17))

#define ads_Prop_MMec_IM_Curing_Kamal_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 18))

/** Injection Molding Equation of State behavior record */
#define ads_Prop_MMec_IM_EOS (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 19))

/** Modified Tait Equation of State behavior record */
#define ads_Prop_MMec_IM_EOS_ModifiedTait (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 20))

#define ads_Prop_MMec_IM_EOS_ModifiedTait_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 21))

/** Spencer Gilmore Equation of State behavior record */
#define ads_Prop_MMec_IM_EOS_SpencerGilmore (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 22))

#define ads_Prop_MMec_IM_EOS_SpencerGilmore_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 23))

/** Thermoplastic fiber reinforcement properties */
#define ads_Prop_MMec_IM_FiberReinforcement (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 24))

#define ads_Prop_MMec_IM_FiberReinforcement_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 25))

/** Metal particle values */
#define ads_Prop_MMec_IM_MetalParticle (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 26))

#define ads_Prop_MMec_IM_MetalParticle_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 27))

/** Injection molding material recommended values */
#define ads_Prop_MMec_IM_RecommendedValues (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 28))

#define ads_Prop_MMec_IM_RecommendedValues_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 29))

/** shear relaxation modulus for injection molding materials */
#define ads_Prop_MMec_IM_ShearRelaxation (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 30))

#define ads_Prop_MMec_IM_ShearRelaxation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 31))

/** thermal expansion properties for injection molding materials */
#define ads_Prop_MMec_IM_ThermalExpansion (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 32))

#define ads_Prop_MMec_IM_ThermalExpansion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 33))

/** Transversely elastic isotropic properties for injection molding materials */
#define ads_Prop_MMec_IM_TransverselyIsotropicElastic (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 34))

#define ads_Prop_MMec_IM_TransverselyIsotropicElastic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 35))

/** Injection Molding material properties */
#define ads_Prop_MMec_IM_Viscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 36))

/** Campus Carreau viscosity with WLF thermal behavior record */
#define ads_Prop_MMec_IM_Viscosity_CampusCarreauWLF (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 37))

#define ads_Prop_MMec_IM_Viscosity_CampusCarreauWLF_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 38))

/** Cross viscosity with Arrhenius thermal behavior record */
#define ads_Prop_MMec_IM_Viscosity_CrossArr (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 39))

#define ads_Prop_MMec_IM_Viscosity_CrossArr_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 40))

/** Cross viscosity with WLF thermal behavior record */
#define ads_Prop_MMec_IM_Viscosity_CrossWLF (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 41))

#define ads_Prop_MMec_IM_Viscosity_CrossWLF_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 42))

/** Plastic viscosity with Macosko record */
#define ads_Prop_MMec_IM_Viscosity_Macosko (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 43))

#define ads_Prop_MMec_IM_Viscosity_Macosko_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 44))

/** Power Law viscosity with Exponential thermal behavior record */
#define ads_Prop_MMec_IM_Viscosity_PowerLawExp (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 45))

#define ads_Prop_MMec_IM_Viscosity_PowerLawExp_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 46))

/** Thermoplastic characteristic temperature values */
#define ads_Prop_MThermal_IM_CharacteristicTemperatures (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 47))

#define ads_Prop_MThermal_IM_CharacteristicTemperatures_table (ads_CoreFragmentTypeIndex(ads_CorePropertyIMFragment, 48))

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Curing_Kamal_tableRolesEnm
{
    ads_Prop_MMec_IM_Curing_Kamal_table_child,
    ads_Prop_MMec_IM_Curing_Kamal_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_EOS_ModifiedTait_tableRolesEnm
{
    ads_Prop_MMec_IM_EOS_ModifiedTait_table_child,
    ads_Prop_MMec_IM_EOS_ModifiedTait_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_EOS_SpencerGilmore_tableRolesEnm
{
    ads_Prop_MMec_IM_EOS_SpencerGilmore_table_child,
    ads_Prop_MMec_IM_EOS_SpencerGilmore_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_FiberReinforcement_tableRolesEnm
{
    ads_Prop_MMec_IM_FiberReinforcement_table_child,
    ads_Prop_MMec_IM_FiberReinforcement_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_MetalParticle_tableRolesEnm
{
    ads_Prop_MMec_IM_MetalParticle_table_child,
    ads_Prop_MMec_IM_MetalParticle_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_RecommendedValues_tableRolesEnm
{
    ads_Prop_MMec_IM_RecommendedValues_table_child,
    ads_Prop_MMec_IM_RecommendedValues_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_ShearRelaxation_tableRolesEnm
{
    ads_Prop_MMec_IM_ShearRelaxation_table_child,
    ads_Prop_MMec_IM_ShearRelaxation_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_ThermalExpansion_tableRolesEnm
{
    ads_Prop_MMec_IM_ThermalExpansion_table_child,
    ads_Prop_MMec_IM_ThermalExpansion_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_TransverselyIsotropicElastic_tableRolesEnm
{
    ads_Prop_MMec_IM_TransverselyIsotropicElastic_table_child,
    ads_Prop_MMec_IM_TransverselyIsotropicElastic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Viscosity_CampusCarreauWLF_tableRolesEnm
{
    ads_Prop_MMec_IM_Viscosity_CampusCarreauWLF_table_child,
    ads_Prop_MMec_IM_Viscosity_CampusCarreauWLF_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Viscosity_CrossArr_tableRolesEnm
{
    ads_Prop_MMec_IM_Viscosity_CrossArr_table_child,
    ads_Prop_MMec_IM_Viscosity_CrossArr_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Viscosity_CrossWLF_tableRolesEnm
{
    ads_Prop_MMec_IM_Viscosity_CrossWLF_table_child,
    ads_Prop_MMec_IM_Viscosity_CrossWLF_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Viscosity_Macosko_tableRolesEnm
{
    ads_Prop_MMec_IM_Viscosity_Macosko_table_child,
    ads_Prop_MMec_IM_Viscosity_Macosko_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_IM_Viscosity_PowerLawExp_tableRolesEnm
{
    ads_Prop_MMec_IM_Viscosity_PowerLawExp_table_child,
    ads_Prop_MMec_IM_Viscosity_PowerLawExp_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_IM_CharacteristicTemperatures_tableRolesEnm
{
    ads_Prop_MThermal_IM_CharacteristicTemperatures_table_child,
    ads_Prop_MThermal_IM_CharacteristicTemperatures_table_parent
};

#endif
