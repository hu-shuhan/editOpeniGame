//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyConcreteC_h
#define ads_CorePropertyConcreteC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyConcrete of the latest level of form Core */

#define ads_MMecConcreteCrackingDisplacementTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 0))

#define ads_MMecConcreteCrackingGFITable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 1))

#define ads_MMecConcreteCrackingStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 2))

#define ads_MMecConcreteDamagedPlasticityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 3))

#define ads_MMecConcreteOptionConcreteFailureTensileDisplacementTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 4))

#define ads_MMecConcreteOptionConcreteFailureTensileStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 5))

#define ads_MMecConcreteOptionCrackingDisplacementFailureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 6))

#define ads_MMecConcreteOptionCrackingPowerLawShearRetentionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 7))

#define ads_MMecConcreteOptionCrackingStrainFailureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 8))

#define ads_MMecConcreteOptionCrackingTabularShearRetentionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 9))

#define ads_MMecConcreteOptionDamagedPlasticityCompressionDamageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 10))

#define ads_MMecConcreteOptionDamagedPlasticityCompressionHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 11))

#define ads_MMecConcreteOptionDamagedPlasticityDisplacementTensionDamageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 12))

#define ads_MMecConcreteOptionDamagedPlasticityDisplacementTensionStiffeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 13))

#define ads_MMecConcreteOptionDamagedPlasticityGFITensionStiffeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 14))

#define ads_MMecConcreteOptionDamagedPlasticityStrainTensionDamageTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 15))

#define ads_MMecConcreteOptionDamagedPlasticityStrainTensionStiffeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 16))

#define ads_MMecConcreteOptionSmearedDisplacementTensionStiffeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 17))

#define ads_MMecConcreteOptionSmearedFailureRatiosTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 18))

#define ads_MMecConcreteOptionSmearedShearRetentionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 19))

#define ads_MMecConcreteOptionSmearedStrainTensionStiffeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 20))

#define ads_MMecConcreteSmearedTable (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 21))

#define ads_Prop_MMec_Concrete (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 22))

#define ads_Prop_MMec_ConcreteOption (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 23))

#define ads_Prop_MMec_ConcreteOption_Cracking (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 24))

/** Specify brittle failure criterion. */
#define ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 25))

#define ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 26))

/** Define the postcracking shear behavior of a material used with the brittle cracking model by entering the material parameters for the power law shear retention model. */
#define ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 27))

#define ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 28))

/** Specify brittle failure criterion. */
#define ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 29))

#define ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 30))

/** Define the postcracking shear behavior of a material used with the brittle cracking model by entering the shear retention factor-crack opening strain relationship. */
#define ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 31))

#define ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 32))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 33))

/** Define compression damage properties for the concrete damaged plasticity model. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 34))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 35))

/** Define hardening in compression for the concrete damaged plasticity model. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 36))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 37))

/** Define postcracking damage properties for the concrete damaged plasticity model as a function of cracking displacement. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 38))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 39))

/** Define postcracking properties for the concrete damaged plasticity model by entering the postfailure stress/cracking-displacement relationship. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionStiffening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 40))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionStiffening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 41))

/** Define postcracking properties for the concrete damaged plasticity model by entering the failure stress and the fracture energy G_f. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_GFITensionStiffening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 42))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_GFITensionStiffening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 43))

/** Define postcracking damage properties for the concrete damaged plasticity model as a function of cracking strain. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 44))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 45))

/** Define postcracking properties for the concrete damaged plasticity model by entering the postfailure stress/cracking-strain relationship. */
#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionStiffening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 46))

#define ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionStiffening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 47))

/** Specify failure criteria for the concrete damaged plasticity material model. */
#define ads_Prop_MMec_ConcreteOption_Failure (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 48))

/** Specify tensile failure based on a tensile cracking displacement criterion. */
#define ads_Prop_MMec_ConcreteOption_Failure_TensileDisplacement (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 49))

#define ads_Prop_MMec_ConcreteOption_Failure_TensileDisplacement_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 50))

/** Specify tensile failure based on a tensile cracking strain criterion. */
#define ads_Prop_MMec_ConcreteOption_Failure_TensileStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 51))

#define ads_Prop_MMec_ConcreteOption_Failure_TensileStrain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 52))

#define ads_Prop_MMec_ConcreteOption_Smeared (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 53))

/** Define the retained tensile stress normal to a crack in a CONCRETE model. */
#define ads_Prop_MMec_ConcreteOption_Smeared_DisplacementTensionStiffening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 54))

#define ads_Prop_MMec_ConcreteOption_Smeared_DisplacementTensionStiffening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 55))

/** Define the shape of the failure surface for a CONCRETE model. */
#define ads_Prop_MMec_ConcreteOption_Smeared_FailureRatios (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 56))

#define ads_Prop_MMec_ConcreteOption_Smeared_FailureRatios_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 57))

/** Define the reduction of the shear modulus associated with crack surfaces in a CONCRETE model as a function of the tensile strain across the crack. */
#define ads_Prop_MMec_ConcreteOption_Smeared_ShearRetention (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 58))

#define ads_Prop_MMec_ConcreteOption_Smeared_ShearRetention_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 59))

/** Define the retained tensile stress normal to a crack in a CONCRETE model. */
#define ads_Prop_MMec_ConcreteOption_Smeared_StrainTensionStiffening (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 60))

#define ads_Prop_MMec_ConcreteOption_Smeared_StrainTensionStiffening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 61))

#define ads_Prop_MMec_Concrete_Cracking (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 62))

/** Define brittle cracking properties by entering the postfailure stress/displacement relationship directly. */
#define ads_Prop_MMec_Concrete_Cracking_Displacement (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 63))

#define ads_Prop_MMec_Concrete_Cracking_Displacement_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 64))

/** Define brittle cracking properties by entering the failure stress and the Mode I fracture energy. */
#define ads_Prop_MMec_Concrete_Cracking_GFI (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 65))

#define ads_Prop_MMec_Concrete_Cracking_GFI_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 66))

/** Define brittle cracking properties by entering the postfailure stress-strain relationship directly. */
#define ads_Prop_MMec_Concrete_Cracking_Strain (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 67))

#define ads_Prop_MMec_Concrete_Cracking_Strain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 68))

/** Define flow potential, yield surface, and viscosity parameters for the concrete damaged plasticity model. */
#define ads_Prop_MMec_Concrete_DamagedPlasticity (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 69))

#define ads_Prop_MMec_Concrete_DamagedPlasticity_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 70))

/** Define concrete properties beyond the elastic range. */
#define ads_Prop_MMec_Concrete_Smeared (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 71))

#define ads_Prop_MMec_Concrete_Smeared_table (ads_CoreFragmentTypeIndex(ads_CorePropertyConcreteFragment, 72))

/** Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_CrackingMembersEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_cracks
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailureMembersEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure_cracks
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure_table_child,
    ads_Prop_MMec_ConcreteOption_Cracking_DisplacementFailure_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetentionMembersEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention_cracks
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention_table_child,
    ads_Prop_MMec_ConcreteOption_Cracking_PowerLawShearRetention_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_Cracking_StrainFailureMembersEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure_cracks
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure_table_child,
    ads_Prop_MMec_ConcreteOption_Cracking_StrainFailure_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetentionMembersEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention_cracks
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention_table_child,
    ads_Prop_MMec_ConcreteOption_Cracking_TabularShearRetention_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamageMembersEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage_tensionRecovery
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionDamage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionHardening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionHardening_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_CompressionHardening_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamageMembersEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage_compressionRecovery
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionDamage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionStiffening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionStiffening_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_DisplacementTensionStiffening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_GFITensionStiffening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_GFITensionStiffening_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_GFITensionStiffening_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamageMembersEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage_compressionRecovery
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionDamage_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionStiffening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionStiffening_table_child,
    ads_Prop_MMec_ConcreteOption_DamagedPlasticity_StrainTensionStiffening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Failure_TensileDisplacement_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Failure_TensileDisplacement_table_child,
    ads_Prop_MMec_ConcreteOption_Failure_TensileDisplacement_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Failure_TensileStrain_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Failure_TensileStrain_table_child,
    ads_Prop_MMec_ConcreteOption_Failure_TensileStrain_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Smeared_DisplacementTensionStiffening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Smeared_DisplacementTensionStiffening_table_child,
    ads_Prop_MMec_ConcreteOption_Smeared_DisplacementTensionStiffening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Smeared_FailureRatios_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Smeared_FailureRatios_table_child,
    ads_Prop_MMec_ConcreteOption_Smeared_FailureRatios_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Smeared_ShearRetention_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Smeared_ShearRetention_table_child,
    ads_Prop_MMec_ConcreteOption_Smeared_ShearRetention_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ConcreteOption_Smeared_StrainTensionStiffening_tableRolesEnm
{
    ads_Prop_MMec_ConcreteOption_Smeared_StrainTensionStiffening_table_child,
    ads_Prop_MMec_ConcreteOption_Smeared_StrainTensionStiffening_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Concrete_Cracking_Displacement_tableRolesEnm
{
    ads_Prop_MMec_Concrete_Cracking_Displacement_table_child,
    ads_Prop_MMec_Concrete_Cracking_Displacement_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Concrete_Cracking_GFI_tableRolesEnm
{
    ads_Prop_MMec_Concrete_Cracking_GFI_table_child,
    ads_Prop_MMec_Concrete_Cracking_GFI_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Concrete_Cracking_Strain_tableRolesEnm
{
    ads_Prop_MMec_Concrete_Cracking_Strain_table_child,
    ads_Prop_MMec_Concrete_Cracking_Strain_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Concrete_DamagedPlasticity_tableRolesEnm
{
    ads_Prop_MMec_Concrete_DamagedPlasticity_table_child,
    ads_Prop_MMec_Concrete_DamagedPlasticity_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Concrete_Smeared_tableRolesEnm
{
    ads_Prop_MMec_Concrete_Smeared_table_child,
    ads_Prop_MMec_Concrete_Smeared_table_parent
};

#endif
