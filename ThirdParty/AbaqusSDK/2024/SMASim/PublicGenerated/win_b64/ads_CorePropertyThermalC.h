//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyThermalC_h
#define ads_CorePropertyThermalC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyThermal of the latest level of form Core */

#define ads_BMecExpansionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 0))

#define ads_CThermalCapacitanceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 1))

#define ads_IThermalConductivityClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 2))

#define ads_IThermalConductivityPressureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 3))

#define ads_IThermalFilmTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 4))

#define ads_IThermalHeatGenerationInelasticFractionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 5))

#define ads_IThermalHeatGenerationSurfaceInteractionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 6))

#define ads_IThermalRadiationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 7))

#define ads_MMecExpansionAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 8))

#define ads_MMecExpansionIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 9))

#define ads_MMecExpansionOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 10))

#define ads_MMecExpansionTransIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 11))

#define ads_MPoreFluidExpansionIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 12))

#define ads_MPoreFluidExpansionOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 13))

#define ads_MThermalConductivityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 14))

#define ads_MThermalConductivityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 15))

#define ads_MThermalConductivityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 16))

#define ads_MThermalConductivityPrandtTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 17))

#define ads_MThermalConductivityTransIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 18))

#define ads_MThermalHeatGenerationElectricalDissipationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 19))

#define ads_MThermalHeatGenerationInelasticDissipationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 20))

#define ads_MThermalLatentHeatTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 21))

#define ads_MThermalLatentHeatVaporizationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 22))

#define ads_MThermalSpecificHeatPolynomialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 23))

#define ads_MThermalSpecificHeatTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 24))

#define ads_Prop_BMec_Expansion (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 25))

#define ads_Prop_BMec_Expansion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 26))

#define ads_Prop_BMec_Expansion_zero (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 27))

#define ads_Prop_CThermal_Capacitance (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 28))

#define ads_Prop_CThermal_Capacitance_mass (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 29))

#define ads_Prop_CThermal_Capacitance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 30))

/** This data type is instantiated for CFD when infinite conductivity in contact is required. For Abaqus/Standard and Explicit only its subtypes are instantiated. */
#define ads_Prop_IThermal_Conductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 31))

#define ads_Prop_IThermal_Conductivity_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 32))

#define ads_Prop_IThermal_Conductivity_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 33))

#define ads_Prop_IThermal_Conductivity_Pressure (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 34))

#define ads_Prop_IThermal_Conductivity_Pressure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 35))

#define ads_Prop_IThermal_Conductivity_User (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 36))

#define ads_Prop_IThermal_Film (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 37))

#define ads_Prop_IThermal_Film_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 38))

#define ads_Prop_IThermal_HeatGeneration (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 39))

#define ads_Prop_IThermal_HeatGeneration_InelasticFraction (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 40))

#define ads_Prop_IThermal_HeatGeneration_InelasticFraction_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 41))

#define ads_Prop_IThermal_HeatGeneration_SurfaceInteraction (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 42))

#define ads_Prop_IThermal_HeatGeneration_SurfaceInteraction_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 43))

#define ads_Prop_IThermal_Radiation (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 44))

#define ads_Prop_IThermal_Radiation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 45))

#define ads_Prop_MMec_Expansion (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 46))

/** Specify anisotropic thermal expansion. */
#define ads_Prop_MMec_Expansion_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 47))

#define ads_Prop_MMec_Expansion_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 48))

/** Thermal expansion record */
#define ads_Prop_MMec_Expansion_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 49))

#define ads_Prop_MMec_Expansion_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 50))

/** Orthotropic thermal expansion record */
#define ads_Prop_MMec_Expansion_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 51))

#define ads_Prop_MMec_Expansion_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 52))

/** Transversely Isotropic thermal expansion record */
#define ads_Prop_MMec_Expansion_TransIsotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 53))

#define ads_Prop_MMec_Expansion_TransIsotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 54))

/** User defined Thermal expansion */
#define ads_Prop_MMec_Expansion_User (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 55))

#define ads_Prop_MPoreFluid_Expansion (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 56))

#define ads_Prop_MPoreFluid_Expansion_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 57))

#define ads_Prop_MPoreFluid_Expansion_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 58))

#define ads_Prop_MPoreFluid_Expansion_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 59))

#define ads_Prop_MPoreFluid_Expansion_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 60))

#define ads_Prop_MThermal_Conductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 61))

/** Specify anisotropic thermal conductivity. */
#define ads_Prop_MThermal_Conductivity_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 62))

#define ads_Prop_MThermal_Conductivity_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 63))

/** Isotropic thermal conductivity. */
#define ads_Prop_MThermal_Conductivity_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 64))

#define ads_Prop_MThermal_Conductivity_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 65))

/** Specify orthotropic thermal conductivity. */
#define ads_Prop_MThermal_Conductivity_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 66))

#define ads_Prop_MThermal_Conductivity_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 67))

/** Specify thermal conductivity based on the Prandtl Number */
#define ads_Prop_MThermal_Conductivity_PrandtlNumber (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 68))

#define ads_Prop_MThermal_Conductivity_PrandtlNumber_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 69))

/** Sutherlands law thermal conductivity */
#define ads_Prop_MThermal_Conductivity_SutherlandsLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 70))

/** Specify Transversely Isotropic thermal conductivity. */
#define ads_Prop_MThermal_Conductivity_TransIsotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 71))

#define ads_Prop_MThermal_Conductivity_TransIsotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 72))

#define ads_Prop_MThermal_HeatGeneration (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 73))

/** Define the fraction of electric energy released as heat. This option is used to specify the fraction of dissipated electrical energy released as heat in coupled thermal-electrical problems. */
#define ads_Prop_MThermal_HeatGeneration_ElectricalDissipation (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 74))

#define ads_Prop_MThermal_HeatGeneration_ElectricalDissipation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 75))

/** Define the fraction of the rate of inelastic dissipation that appears as a heat source. */
#define ads_Prop_MThermal_HeatGeneration_InelasticDissipation (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 76))

#define ads_Prop_MThermal_HeatGeneration_InelasticDissipation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 77))

/** A data record that represents heat generation from the user subroutine */
#define ads_Prop_MThermal_HeatGeneration_User (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 78))

/** Specify latent heats. */
#define ads_Prop_MThermal_LatentHeat (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 79))

#define ads_Prop_MThermal_LatentHeatVaporization (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 80))

#define ads_Prop_MThermal_LatentHeatVaporization_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 81))

#define ads_Prop_MThermal_LatentHeat_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 82))

/** This option is used to specify a material's specific heat. */
#define ads_Prop_MThermal_SpecificHeat (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 83))

#define ads_Prop_MThermal_SpecificHeat_Polynomial (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 84))

#define ads_Prop_MThermal_SpecificHeat_Polynomial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 85))

#define ads_Prop_MThermal_SpecificHeat_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 86))

#define ads_Prop_MThermal_SpecificHeat_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 87))

#define ads_Prop_MThermal_User (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 88))

#define ads_Prop_SMec_Expansion (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 89))

#define ads_Prop_SMec_Expansion_generalizedThermalForce (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 90))

#define ads_Prop_SMec_Expansion_generalizedThermalLoad (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 91))

#define ads_Prop_SMec_Expansion_generalizedThermalMoment (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 92))

#define ads_Prop_SMec_Expansion_table (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 93))

#define ads_Prop_SMec_Expansion_zero (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 94))

#define ads_SMecExpansionTable (ads_CoreFragmentTypeIndex(ads_CorePropertyThermalFragment, 95))

/** Enum with association roles. */
enum ads_Prop_BMec_Expansion_tableRolesEnm
{
    ads_Prop_BMec_Expansion_table_child,
    ads_Prop_BMec_Expansion_table_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Expansion_zeroRolesEnm
{
    ads_Prop_BMec_Expansion_zero_child,
    ads_Prop_BMec_Expansion_zero_parent
};

/** Enum with association roles. */
enum ads_Prop_CThermal_Capacitance_massRolesEnm
{
    ads_Prop_CThermal_Capacitance_mass_child,
    ads_Prop_CThermal_Capacitance_mass_parent
};

/** Enum with association roles. */
enum ads_Prop_CThermal_Capacitance_tableRolesEnm
{
    ads_Prop_CThermal_Capacitance_table_child,
    ads_Prop_CThermal_Capacitance_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_Conductivity_Closure_tableRolesEnm
{
    ads_Prop_IThermal_Conductivity_Closure_table_child,
    ads_Prop_IThermal_Conductivity_Closure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_Conductivity_Pressure_tableRolesEnm
{
    ads_Prop_IThermal_Conductivity_Pressure_table_child,
    ads_Prop_IThermal_Conductivity_Pressure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_Film_tableRolesEnm
{
    ads_Prop_IThermal_Film_table_child,
    ads_Prop_IThermal_Film_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_HeatGeneration_InelasticFraction_tableRolesEnm
{
    ads_Prop_IThermal_HeatGeneration_InelasticFraction_table_child,
    ads_Prop_IThermal_HeatGeneration_InelasticFraction_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IThermal_HeatGeneration_SurfaceInteraction_tableRolesEnm
{
    ads_Prop_IThermal_HeatGeneration_SurfaceInteraction_table_child,
    ads_Prop_IThermal_HeatGeneration_SurfaceInteraction_table_parent
};

/** Enum with record members. */
enum ads_Prop_IThermal_RadiationMembersEnm
{
    ads_Prop_IThermal_Radiation_masterSurfaceEmissivity,
    ads_Prop_IThermal_Radiation_slaveSurfaceEmissivity
};

/** Enum with association roles. */
enum ads_Prop_IThermal_Radiation_tableRolesEnm
{
    ads_Prop_IThermal_Radiation_table_child,
    ads_Prop_IThermal_Radiation_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_ExpansionMembersEnm
{
    ads_Prop_MMec_Expansion_theta0
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Expansion_AnisotropicMembersEnm
{
    ads_Prop_MMec_Expansion_Anisotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MMec_Expansion_Anisotropic_tableRolesEnm
{
    ads_Prop_MMec_Expansion_Anisotropic_table_child,
    ads_Prop_MMec_Expansion_Anisotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Expansion_IsotropicMembersEnm
{
    ads_Prop_MMec_Expansion_Isotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MMec_Expansion_Isotropic_tableRolesEnm
{
    ads_Prop_MMec_Expansion_Isotropic_table_child,
    ads_Prop_MMec_Expansion_Isotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Expansion_OrthotropicMembersEnm
{
    ads_Prop_MMec_Expansion_Orthotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MMec_Expansion_Orthotropic_tableRolesEnm
{
    ads_Prop_MMec_Expansion_Orthotropic_table_child,
    ads_Prop_MMec_Expansion_Orthotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Expansion_TransIsotropicMembersEnm
{
    ads_Prop_MMec_Expansion_TransIsotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MMec_Expansion_TransIsotropic_tableRolesEnm
{
    ads_Prop_MMec_Expansion_TransIsotropic_table_child,
    ads_Prop_MMec_Expansion_TransIsotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Expansion_UserMembersEnm
{
    ads_Prop_MMec_Expansion_User_theta0
};

/** Enum with record members. */
enum ads_Prop_MPoreFluid_ExpansionMembersEnm
{
    ads_Prop_MPoreFluid_Expansion_theta0
};

/** Enum with record members. */
enum ads_Prop_MPoreFluid_Expansion_IsotropicMembersEnm
{
    ads_Prop_MPoreFluid_Expansion_Isotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Expansion_Isotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Expansion_Isotropic_table_child,
    ads_Prop_MPoreFluid_Expansion_Isotropic_table_parent
};

/** Enum with record members. */
enum ads_Prop_MPoreFluid_Expansion_OrthotropicMembersEnm
{
    ads_Prop_MPoreFluid_Expansion_Orthotropic_theta0
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Expansion_Orthotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Expansion_Orthotropic_table_child,
    ads_Prop_MPoreFluid_Expansion_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_Conductivity_Anisotropic_tableRolesEnm
{
    ads_Prop_MThermal_Conductivity_Anisotropic_table_child,
    ads_Prop_MThermal_Conductivity_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_Conductivity_Isotropic_tableRolesEnm
{
    ads_Prop_MThermal_Conductivity_Isotropic_table_child,
    ads_Prop_MThermal_Conductivity_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_Conductivity_Orthotropic_tableRolesEnm
{
    ads_Prop_MThermal_Conductivity_Orthotropic_table_child,
    ads_Prop_MThermal_Conductivity_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_Conductivity_PrandtlNumber_tableRolesEnm
{
    ads_Prop_MThermal_Conductivity_PrandtlNumber_table_child,
    ads_Prop_MThermal_Conductivity_PrandtlNumber_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MThermal_Conductivity_SutherlandsLawMembersEnm
{
    ads_Prop_MThermal_Conductivity_SutherlandsLaw_referenceConductivity,
    ads_Prop_MThermal_Conductivity_SutherlandsLaw_referenceTemperature,
    ads_Prop_MThermal_Conductivity_SutherlandsLaw_sutherlandTemperature
};

/** Enum with association roles. */
enum ads_Prop_MThermal_Conductivity_TransIsotropic_tableRolesEnm
{
    ads_Prop_MThermal_Conductivity_TransIsotropic_table_child,
    ads_Prop_MThermal_Conductivity_TransIsotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_HeatGeneration_ElectricalDissipation_tableRolesEnm
{
    ads_Prop_MThermal_HeatGeneration_ElectricalDissipation_table_child,
    ads_Prop_MThermal_HeatGeneration_ElectricalDissipation_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_HeatGeneration_InelasticDissipation_tableRolesEnm
{
    ads_Prop_MThermal_HeatGeneration_InelasticDissipation_table_child,
    ads_Prop_MThermal_HeatGeneration_InelasticDissipation_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MThermal_LatentHeatMembersEnm
{
    ads_Prop_MThermal_LatentHeat_poreFluid,
    ads_Prop_MThermal_LatentHeat_smooth
};

/** Enum with association roles. */
enum ads_Prop_MThermal_LatentHeatVaporization_tableRolesEnm
{
    ads_Prop_MThermal_LatentHeatVaporization_table_child,
    ads_Prop_MThermal_LatentHeatVaporization_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MThermal_LatentHeat_tableRolesEnm
{
    ads_Prop_MThermal_LatentHeat_table_child,
    ads_Prop_MThermal_LatentHeat_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MThermal_SpecificHeatMembersEnm
{
    ads_Prop_MThermal_SpecificHeat_law
};

enum ads_Prop_MThermal_SpecificHeat_lawEnm
{
    ads_Prop_MThermal_SpecificHeat_law_CONSTANT_PRESSURE,
    ads_Prop_MThermal_SpecificHeat_law_CONSTANT_VOLUME
};

/** Enum with record members. */
enum ads_Prop_MThermal_SpecificHeat_PolynomialMembersEnm
{
    ads_Prop_MThermal_SpecificHeat_Polynomial_law
};

enum ads_Prop_MThermal_SpecificHeat_Polynomial_lawEnm
{
    ads_Prop_MThermal_SpecificHeat_Polynomial_law_CONSTANT_PRESSURE,
    ads_Prop_MThermal_SpecificHeat_Polynomial_law_CONSTANT_VOLUME
};

/** Enum with association roles. */
enum ads_Prop_MThermal_SpecificHeat_Polynomial_tableRolesEnm
{
    ads_Prop_MThermal_SpecificHeat_Polynomial_table_child,
    ads_Prop_MThermal_SpecificHeat_Polynomial_table_parent
};

/** Enum with record members. */
enum ads_Prop_MThermal_SpecificHeat_TabularMembersEnm
{
    ads_Prop_MThermal_SpecificHeat_Tabular_law
};

enum ads_Prop_MThermal_SpecificHeat_Tabular_lawEnm
{
    ads_Prop_MThermal_SpecificHeat_Tabular_law_CONSTANT_PRESSURE,
    ads_Prop_MThermal_SpecificHeat_Tabular_law_CONSTANT_VOLUME
};

/** Enum with association roles. */
enum ads_Prop_MThermal_SpecificHeat_Tabular_tableRolesEnm
{
    ads_Prop_MThermal_SpecificHeat_Tabular_table_child,
    ads_Prop_MThermal_SpecificHeat_Tabular_table_parent
};

/** Enum with record members. */
enum ads_Prop_MThermal_UserMembersEnm
{
    ads_Prop_MThermal_User_hybridFormulation,
    ads_Prop_MThermal_User_unsymm
};

enum ads_Prop_MThermal_User_hybridFormulationEnm
{
    ads_Prop_MThermal_User_hybridFormulation_INCOMPRESSIBLE,
    ads_Prop_MThermal_User_hybridFormulation_INCREMENTAL,
    ads_Prop_MThermal_User_hybridFormulation_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_SMec_Expansion_generalizedThermalForceRolesEnm
{
    ads_Prop_SMec_Expansion_generalizedThermalForce_child,
    ads_Prop_SMec_Expansion_generalizedThermalForce_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Expansion_generalizedThermalLoadRolesEnm
{
    ads_Prop_SMec_Expansion_generalizedThermalLoad_child,
    ads_Prop_SMec_Expansion_generalizedThermalLoad_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Expansion_generalizedThermalMomentRolesEnm
{
    ads_Prop_SMec_Expansion_generalizedThermalMoment_child,
    ads_Prop_SMec_Expansion_generalizedThermalMoment_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Expansion_tableRolesEnm
{
    ads_Prop_SMec_Expansion_table_child,
    ads_Prop_SMec_Expansion_table_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Expansion_zeroRolesEnm
{
    ads_Prop_SMec_Expansion_zero_child,
    ads_Prop_SMec_Expansion_zero_parent
};

#endif
