//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyMultiPhysicsC_h
#define ads_CorePropertyMultiPhysicsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyMultiPhysics of the latest level of form Core */

#define ads_IDiffusionGapDiffusivityIonConcentrationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 0))

#define ads_IElectricConductivityClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 1))

#define ads_IElectricConductivityPressureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 2))

#define ads_IElectricLiquidConductivityClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 3))

#define ads_IElectricLiquidConductivityPressureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 4))

#define ads_IPoreFluidContactPermeabilityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 5))

#define ads_IPoreFluidPermeabilityFiniteLayerTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 6))

#define ads_MAcousticBulkModulusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 7))

#define ads_MAcousticCavitationPressureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 8))

#define ads_MAcousticDensityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 9))

#define ads_MAcousticPorousMediumTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 10))

#define ads_MAcousticVolumetricDragTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 11))

#define ads_MDiffusionDiffusivityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 12))

#define ads_MDiffusionDiffusivityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 13))

#define ads_MDiffusionDiffusivityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 14))

#define ads_MDiffusionPressureEffectTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 15))

#define ads_MDiffusionSolubilityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 16))

#define ads_MDiffusionSoretEffectTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 17))

#define ads_MElectricConductivityAnisotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 18))

#define ads_MElectricConductivityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 19))

#define ads_MElectricConductivityIsotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 20))

#define ads_MElectricConductivityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 21))

#define ads_MElectricConductivityOrthotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 22))

#define ads_MElectricConductivityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 23))

#define ads_MElectricMagneticNonlinearBHTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 24))

#define ads_MElectricMagneticPermeabilityAnisotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 25))

#define ads_MElectricMagneticPermeabilityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 26))

#define ads_MElectricMagneticPermeabilityIsotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 27))

#define ads_MElectricMagneticPermeabilityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 28))

#define ads_MElectricMagneticPermeabilityOrthotropicFrequencyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 29))

#define ads_MElectricMagneticPermeabilityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 30))

#define ads_MElectricPermanentMagnetizationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 31))

#define ads_MElectricResistivityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 32))

#define ads_MElectricResistivityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 33))

#define ads_MElectricResistivityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 34))

#define ads_MPiezoelectricBetaDampingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 35))

#define ads_MPiezoelectricDielectricAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 36))

#define ads_MPiezoelectricDielectricIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 37))

#define ads_MPiezoelectricDielectricOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 38))

#define ads_MPiezoelectricStrainCoefficientTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 39))

#define ads_MPiezoelectricStressCoefficientTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 40))

#define ads_MPiezoelectricStructuralDampingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 41))

#define ads_MPoreFluidBulkModulusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 42))

#define ads_MPoreFluidConductivityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 43))

#define ads_MPoreFluidConductivityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 44))

#define ads_MPoreFluidConductivityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 45))

#define ads_MPoreFluidDensityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 46))

#define ads_MPoreFluidGelTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 47))

#define ads_MPoreFluidMoistureSwellingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 48))

#define ads_MPoreFluidPermeabilityAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 49))

#define ads_MPoreFluidPermeabilityIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 50))

#define ads_MPoreFluidPermeabilityOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 51))

#define ads_MPoreFluidPermeabilityPorosityAnisoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 52))

#define ads_MPoreFluidPermeabilityPorosityInertiaAnisoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 53))

#define ads_MPoreFluidPermeabilityPorosityInertiaOrthoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 54))

#define ads_MPoreFluidPermeabilityPorosityInertiaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 55))

#define ads_MPoreFluidPermeabilityPorosityOrthoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 56))

#define ads_MPoreFluidPermeabilityPorosityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 57))

#define ads_MPoreFluidPermeabilitySaturationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 58))

#define ads_MPoreFluidPermeabilityVelocityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 59))

#define ads_MPoreFluidSorptionLogrithmicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 60))

#define ads_MPoreFluidSorptionScanningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 61))

#define ads_MPoreFluidSorptionTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 62))

#define ads_MPoreFluidSpecificHeatTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 63))

/** Gap diffusivity. */
#define ads_Prop_IDiffusion_GapDiffusivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 64))

/** Gap diffusivity, Type=Ion Concentration. */
#define ads_Prop_IDiffusion_GapDiffusivity_IonConcentration (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 65))

#define ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 66))

#define ads_Prop_IElectric_Conductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 67))

#define ads_Prop_IElectric_Conductivity_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 68))

#define ads_Prop_IElectric_Conductivity_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 69))

#define ads_Prop_IElectric_Conductivity_Pressure (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 70))

#define ads_Prop_IElectric_Conductivity_Pressure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 71))

#define ads_Prop_IElectric_Conductivity_User (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 72))

#define ads_Prop_IElectric_LiquidConductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 73))

#define ads_Prop_IElectric_LiquidConductivity_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 74))

#define ads_Prop_IElectric_LiquidConductivity_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 75))

#define ads_Prop_IElectric_LiquidConductivity_Pressure (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 76))

#define ads_Prop_IElectric_LiquidConductivity_Pressure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 77))

/** Contact Permeability. */
#define ads_Prop_IPoreFluid_ContactPermeability (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 78))

#define ads_Prop_IPoreFluid_ContactPermeability_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 79))

#define ads_Prop_IPoreFluid_Permeability (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 80))

/** This option defines leak-off coefficients for pore pressure cohesive elements. */
#define ads_Prop_IPoreFluid_Permeability_FiniteLayer (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 81))

#define ads_Prop_IPoreFluid_Permeability_FiniteLayer_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 82))

/** This option defines leak-off coefficients for pore pressure cohesive elements using subroutine UFLUIDLEAKOFF. */
#define ads_Prop_IPoreFluid_Permeability_User (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 83))

/** The bulk modulus for the acoustic medium. */
#define ads_Prop_MAcoustic_BulkModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 84))

#define ads_Prop_MAcoustic_BulkModulus_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 85))

/** Define the cavitation pressure limit for the acoustic medium. When the fluid absolute pressure drops to this limit, the acoustic medium undergoes free volume expansion or cavitation without a further decrease in the pressure. A negative cavitation limit value represents an acoustic medium that is capable of sustaining a negative absolute pressure up to the specified limit value. */
#define ads_Prop_MAcoustic_CavitationPressure (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 86))

#define ads_Prop_MAcoustic_CavitationPressure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 87))

#define ads_Prop_MAcoustic_Density (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 88))

#define ads_Prop_MAcoustic_Density_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 89))

#define ads_Prop_MAcoustic_PorousMedium (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 90))

#define ads_Prop_MAcoustic_PorousMedium_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 91))

/** define the volumetric drag coefficient for the acoustic medium. */
#define ads_Prop_MAcoustic_VolumetricDrag (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 92))

#define ads_Prop_MAcoustic_VolumetricDrag_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 93))

/** Specify mass diffusivity. */
#define ads_Prop_MDiffusion_Diffusivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 94))

/** Specify anisotropic mass diffusivity. */
#define ads_Prop_MDiffusion_Diffusivity_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 95))

#define ads_Prop_MDiffusion_Diffusivity_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 96))

/** Specify isotropic mass diffusivity. */
#define ads_Prop_MDiffusion_Diffusivity_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 97))

#define ads_Prop_MDiffusion_Diffusivity_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 98))

/** Specify orthotropic mass diffusivity. */
#define ads_Prop_MDiffusion_Diffusivity_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 99))

#define ads_Prop_MDiffusion_Diffusivity_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 100))

/** Specify the material parameters kappa_s for mass diffusion driven by gradients of equivalent pressure stress. */
#define ads_Prop_MDiffusion_PressureEffect (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 101))

#define ads_Prop_MDiffusion_PressureEffect_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 102))

/** Specify solubility. This option is used to define the solubility for a material diffusing through a base material. It must be used in conjunction with the diffusivity option. */
#define ads_Prop_MDiffusion_Solubility (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 103))

#define ads_Prop_MDiffusion_Solubility_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 104))

/** Specify the material parameters kappa_s for mass diffusion driven by gradients of temperature. */
#define ads_Prop_MDiffusion_SoretEffect (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 105))

#define ads_Prop_MDiffusion_SoretEffect_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 106))

#define ads_Prop_MElectric_Conductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 107))

/** Specify anisotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 108))

/** Specify anisotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_AnisotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 109))

#define ads_Prop_MElectric_Conductivity_AnisotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 110))

#define ads_Prop_MElectric_Conductivity_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 111))

/** Isotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 112))

/** Isotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_IsotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 113))

#define ads_Prop_MElectric_Conductivity_IsotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 114))

#define ads_Prop_MElectric_Conductivity_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 115))

/** Specify orthotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 116))

/** Specify orthotropic electrical conductivity. */
#define ads_Prop_MElectric_Conductivity_OrthotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 117))

#define ads_Prop_MElectric_Conductivity_OrthotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 118))

#define ads_Prop_MElectric_Conductivity_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 119))

#define ads_Prop_MElectric_MagneticPermeability (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 120))

#define ads_Prop_MElectric_MagneticPermeability_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 121))

#define ads_Prop_MElectric_MagneticPermeability_AnisotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 122))

#define ads_Prop_MElectric_MagneticPermeability_AnisotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 123))

#define ads_Prop_MElectric_MagneticPermeability_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 124))

#define ads_Prop_MElectric_MagneticPermeability_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 125))

#define ads_Prop_MElectric_MagneticPermeability_IsotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 126))

#define ads_Prop_MElectric_MagneticPermeability_IsotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 127))

#define ads_Prop_MElectric_MagneticPermeability_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 128))

#define ads_Prop_MElectric_MagneticPermeability_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 129))

#define ads_Prop_MElectric_MagneticPermeability_OrthotropicFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 130))

#define ads_Prop_MElectric_MagneticPermeability_OrthotropicFrequency_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 131))

#define ads_Prop_MElectric_MagneticPermeability_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 132))

#define ads_Prop_MElectric_NonlinearBH (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 133))

#define ads_Prop_MElectric_NonlinearBH_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 134))

#define ads_Prop_MElectric_PermanentMagnetization (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 135))

#define ads_Prop_MElectric_PermanentMagnetization_coercivityDirection (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 136))

#define ads_Prop_MElectric_PermanentMagnetization_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 137))

#define ads_Prop_MElectric_PorousElectrodeTheory (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 138))

#define ads_Prop_MElectric_Resistivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 139))

/** Specify anisotropic electrical resistivity. */
#define ads_Prop_MElectric_Resistivity_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 140))

#define ads_Prop_MElectric_Resistivity_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 141))

/** Isotropic electrical resistivity. */
#define ads_Prop_MElectric_Resistivity_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 142))

#define ads_Prop_MElectric_Resistivity_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 143))

/** Specify orthotropic electrical resistivity. */
#define ads_Prop_MElectric_Resistivity_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 144))

#define ads_Prop_MElectric_Resistivity_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 145))

/** Specify piezoelectric material damping. */
#define ads_Prop_MPiezoelectric_Damping (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 146))

/** Specify piezoelectric Rayleigh stiffness proportional damping. */
#define ads_Prop_MPiezoelectric_Damping_Beta (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 147))

#define ads_Prop_MPiezoelectric_Damping_Beta_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 148))

/** Specify piezoelectric stiffness proportional damping. */
#define ads_Prop_MPiezoelectric_Damping_Structural (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 149))

#define ads_Prop_MPiezoelectric_Damping_Structural_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 150))

#define ads_Prop_MPiezoelectric_Dielectric (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 151))

/** Specify dielectric material properties. */
#define ads_Prop_MPiezoelectric_Dielectric_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 152))

#define ads_Prop_MPiezoelectric_Dielectric_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 153))

/** Specify dielectric material properties. */
#define ads_Prop_MPiezoelectric_Dielectric_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 154))

#define ads_Prop_MPiezoelectric_Dielectric_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 155))

/** Specify dielectric material properties. */
#define ads_Prop_MPiezoelectric_Dielectric_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 156))

#define ads_Prop_MPiezoelectric_Dielectric_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 157))

/** specify strain material coefficients for the piezoelectric. */
#define ads_Prop_MPiezoelectric_StrainCoefficient (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 158))

#define ads_Prop_MPiezoelectric_StrainCoefficient_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 159))

/** specify stress material coefficients for the piezoelectric. */
#define ads_Prop_MPiezoelectric_StressCoefficient (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 160))

#define ads_Prop_MPiezoelectric_StressCoefficient_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 161))

/** Define bulk moduli for soils and rocks. This option is used to define the bulk moduli of solid grains and a permeating fluid such that their compressibility can be considered in the analysis of a porous medium. This option cannot be used with the porous metal plasticity material model. */
#define ads_Prop_MPoreFluid_BulkModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 162))

#define ads_Prop_MPoreFluid_BulkModulus_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 163))

#define ads_Prop_MPoreFluid_Conductivity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 164))

/** Specify anisotropic thermal conductivity. */
#define ads_Prop_MPoreFluid_Conductivity_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 165))

#define ads_Prop_MPoreFluid_Conductivity_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 166))

/** Isotropic thermal conductivity. */
#define ads_Prop_MPoreFluid_Conductivity_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 167))

#define ads_Prop_MPoreFluid_Conductivity_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 168))

/** Specify orthotropic thermal conductivity. */
#define ads_Prop_MPoreFluid_Conductivity_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 169))

#define ads_Prop_MPoreFluid_Conductivity_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 170))

/** Pore fluid density record */
#define ads_Prop_MPoreFluid_Density (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 171))

#define ads_Prop_MPoreFluid_Density_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 172))

/** Define a swelling gel. This option is used to define the growth of the gel particles that swell and trap wetting liquid in a partially saturated porous medium in the analysis of coupled wetting liquid flow and porous medium stress. */
#define ads_Prop_MPoreFluid_Gel (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 173))

#define ads_Prop_MPoreFluid_Gel_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 174))

/** Define moisture-driven swelling. TBD: do we need to derive from MaterialProperty ? */
#define ads_Prop_MPoreFluid_MoistureSwelling (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 175))

#define ads_Prop_MPoreFluid_MoistureSwelling_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 176))

/** An abstraction of permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 177))

/** Define permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 178))

#define ads_Prop_MPoreFluid_Permeability_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 179))

/** Define permeability for a fluid using the Carman-Kozeny formula. */
#define ads_Prop_MPoreFluid_Permeability_CarmanKozeny (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 180))

/** Define isotropic permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 181))

#define ads_Prop_MPoreFluid_Permeability_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 182))

/** Define orthotropic permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 183))

#define ads_Prop_MPoreFluid_Permeability_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 184))

/** Define permeability for a fluid in terms of porosity. */
#define ads_Prop_MPoreFluid_Permeability_Porosity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 185))

#define ads_Prop_MPoreFluid_Permeability_Porosity_anisoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 186))

#define ads_Prop_MPoreFluid_Permeability_Porosity_inertiaAnisoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 187))

#define ads_Prop_MPoreFluid_Permeability_Porosity_inertiaOrthoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 188))

#define ads_Prop_MPoreFluid_Permeability_Porosity_inertiaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 189))

#define ads_Prop_MPoreFluid_Permeability_Porosity_orthoTable (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 190))

#define ads_Prop_MPoreFluid_Permeability_Porosity_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 191))

/** Define permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability_Saturation (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 192))

#define ads_Prop_MPoreFluid_Permeability_Saturation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 193))

/** Define permeability for pore fluid flow. */
#define ads_Prop_MPoreFluid_Permeability_Velocity (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 194))

#define ads_Prop_MPoreFluid_Permeability_Velocity_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 195))

#define ads_Prop_MPoreFluid_Sorption (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 196))

/** Define absorption and exsorption behavior by the analytical logarithmic form. This option is used to define absorption and exsorption behaviors of a partially saturated porous medium in the analysis of coupled wetting liquid flow and porous medium stress. */
#define ads_Prop_MPoreFluid_Sorption_Logrithmic (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 197))

#define ads_Prop_MPoreFluid_Sorption_Logrithmic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 198))

/** The behavior between absorption and exsorption is defined by a scanning line of user-specified constant slope. */
#define ads_Prop_MPoreFluid_Sorption_Scanning (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 199))

#define ads_Prop_MPoreFluid_Sorption_Scanning_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 200))

/** Define absorption and exsorption behavior in tabulated form. This option is used to define absorption and exsorption behaviors of a partially saturated porous medium in the analysis of coupled wetting liquid flow and porous medium stress. */
#define ads_Prop_MPoreFluid_Sorption_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 201))

#define ads_Prop_MPoreFluid_Sorption_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 202))

#define ads_Prop_MPoreFluid_SpecificHeat (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 203))

#define ads_Prop_MPoreFluid_SpecificHeat_table (ads_CoreFragmentTypeIndex(ads_CorePropertyMultiPhysicsFragment, 204))

/** 
Enum with record members. */
enum ads_Prop_IDiffusion_GapDiffusivityMembersEnm
{
    ads_Prop_IDiffusion_GapDiffusivity_cutoffFlowAcross,
    ads_Prop_IDiffusion_GapDiffusivity_cutoffGapFill
};

/** 
Enum with record members. */
enum ads_Prop_IDiffusion_GapDiffusivity_IonConcentrationMembersEnm
{
    ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_cutoffFlowAcross,
    ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_cutoffGapFill
};

/** Enum with association roles. */
enum ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_tableRolesEnm
{
    ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_table_child,
    ads_Prop_IDiffusion_GapDiffusivity_IonConcentration_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IElectric_Conductivity_Closure_tableRolesEnm
{
    ads_Prop_IElectric_Conductivity_Closure_table_child,
    ads_Prop_IElectric_Conductivity_Closure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IElectric_Conductivity_Pressure_tableRolesEnm
{
    ads_Prop_IElectric_Conductivity_Pressure_table_child,
    ads_Prop_IElectric_Conductivity_Pressure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IElectric_LiquidConductivity_Closure_tableRolesEnm
{
    ads_Prop_IElectric_LiquidConductivity_Closure_table_child,
    ads_Prop_IElectric_LiquidConductivity_Closure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IElectric_LiquidConductivity_Pressure_tableRolesEnm
{
    ads_Prop_IElectric_LiquidConductivity_Pressure_table_child,
    ads_Prop_IElectric_LiquidConductivity_Pressure_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IPoreFluid_ContactPermeabilityMembersEnm
{
    ads_Prop_IPoreFluid_ContactPermeability_constraintMethod,
    ads_Prop_IPoreFluid_ContactPermeability_cutoffFlowAcross,
    ads_Prop_IPoreFluid_ContactPermeability_cutoffGapFill
};

enum ads_Prop_IPoreFluid_ContactPermeability_constraintMethodEnm
{
    ads_Prop_IPoreFluid_ContactPermeability_constraintMethod_DIRECT,
    ads_Prop_IPoreFluid_ContactPermeability_constraintMethod_NONE,
    ads_Prop_IPoreFluid_ContactPermeability_constraintMethod_PENALTY
};

/** Enum with association roles. */
enum ads_Prop_IPoreFluid_ContactPermeability_tableRolesEnm
{
    ads_Prop_IPoreFluid_ContactPermeability_table_child,
    ads_Prop_IPoreFluid_ContactPermeability_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IPoreFluid_Permeability_FiniteLayer_tableRolesEnm
{
    ads_Prop_IPoreFluid_Permeability_FiniteLayer_table_child,
    ads_Prop_IPoreFluid_Permeability_FiniteLayer_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MAcoustic_BulkModulus_tableRolesEnm
{
    ads_Prop_MAcoustic_BulkModulus_table_child,
    ads_Prop_MAcoustic_BulkModulus_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MAcoustic_CavitationPressure_tableRolesEnm
{
    ads_Prop_MAcoustic_CavitationPressure_table_child,
    ads_Prop_MAcoustic_CavitationPressure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MAcoustic_Density_tableRolesEnm
{
    ads_Prop_MAcoustic_Density_table_child,
    ads_Prop_MAcoustic_Density_table_parent
};

/** Enum with record members. */
enum ads_Prop_MAcoustic_PorousMediumMembersEnm
{
    ads_Prop_MAcoustic_PorousMedium_model
};

enum ads_Prop_MAcoustic_PorousMedium_modelEnm
{
    ads_Prop_MAcoustic_PorousMedium_model_DELANY_BAZLEY,
    ads_Prop_MAcoustic_PorousMedium_model_MIKI
};

/** Enum with association roles. */
enum ads_Prop_MAcoustic_PorousMedium_tableRolesEnm
{
    ads_Prop_MAcoustic_PorousMedium_table_child,
    ads_Prop_MAcoustic_PorousMedium_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MAcoustic_VolumetricDrag_tableRolesEnm
{
    ads_Prop_MAcoustic_VolumetricDrag_table_child,
    ads_Prop_MAcoustic_VolumetricDrag_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MDiffusion_DiffusivityMembersEnm
{
    ads_Prop_MDiffusion_Diffusivity_law
};

enum ads_Prop_MDiffusion_Diffusivity_lawEnm
{
    ads_Prop_MDiffusion_Diffusivity_law_FICK,
    ads_Prop_MDiffusion_Diffusivity_law_GENERAL
};

/** 
Enum with record members. */
enum ads_Prop_MDiffusion_Diffusivity_AnisotropicMembersEnm
{
    ads_Prop_MDiffusion_Diffusivity_Anisotropic_law
};

enum ads_Prop_MDiffusion_Diffusivity_Anisotropic_lawEnm
{
    ads_Prop_MDiffusion_Diffusivity_Anisotropic_law_FICK,
    ads_Prop_MDiffusion_Diffusivity_Anisotropic_law_GENERAL
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_Diffusivity_Anisotropic_tableRolesEnm
{
    ads_Prop_MDiffusion_Diffusivity_Anisotropic_table_child,
    ads_Prop_MDiffusion_Diffusivity_Anisotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MDiffusion_Diffusivity_IsotropicMembersEnm
{
    ads_Prop_MDiffusion_Diffusivity_Isotropic_law
};

enum ads_Prop_MDiffusion_Diffusivity_Isotropic_lawEnm
{
    ads_Prop_MDiffusion_Diffusivity_Isotropic_law_FICK,
    ads_Prop_MDiffusion_Diffusivity_Isotropic_law_GENERAL
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_Diffusivity_Isotropic_tableRolesEnm
{
    ads_Prop_MDiffusion_Diffusivity_Isotropic_table_child,
    ads_Prop_MDiffusion_Diffusivity_Isotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MDiffusion_Diffusivity_OrthotropicMembersEnm
{
    ads_Prop_MDiffusion_Diffusivity_Orthotropic_law
};

enum ads_Prop_MDiffusion_Diffusivity_Orthotropic_lawEnm
{
    ads_Prop_MDiffusion_Diffusivity_Orthotropic_law_FICK,
    ads_Prop_MDiffusion_Diffusivity_Orthotropic_law_GENERAL
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_Diffusivity_Orthotropic_tableRolesEnm
{
    ads_Prop_MDiffusion_Diffusivity_Orthotropic_table_child,
    ads_Prop_MDiffusion_Diffusivity_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_PressureEffect_tableRolesEnm
{
    ads_Prop_MDiffusion_PressureEffect_table_child,
    ads_Prop_MDiffusion_PressureEffect_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_Solubility_tableRolesEnm
{
    ads_Prop_MDiffusion_Solubility_table_child,
    ads_Prop_MDiffusion_Solubility_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MDiffusion_SoretEffect_tableRolesEnm
{
    ads_Prop_MDiffusion_SoretEffect_table_child,
    ads_Prop_MDiffusion_SoretEffect_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_AnisotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_AnisotropicFrequency_table_child,
    ads_Prop_MElectric_Conductivity_AnisotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_Anisotropic_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_Anisotropic_table_child,
    ads_Prop_MElectric_Conductivity_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_IsotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_IsotropicFrequency_table_child,
    ads_Prop_MElectric_Conductivity_IsotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_Isotropic_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_Isotropic_table_child,
    ads_Prop_MElectric_Conductivity_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_OrthotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_OrthotropicFrequency_table_child,
    ads_Prop_MElectric_Conductivity_OrthotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Conductivity_Orthotropic_tableRolesEnm
{
    ads_Prop_MElectric_Conductivity_Orthotropic_table_child,
    ads_Prop_MElectric_Conductivity_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_AnisotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_AnisotropicFrequency_table_child,
    ads_Prop_MElectric_MagneticPermeability_AnisotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_Anisotropic_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_Anisotropic_table_child,
    ads_Prop_MElectric_MagneticPermeability_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_IsotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_IsotropicFrequency_table_child,
    ads_Prop_MElectric_MagneticPermeability_IsotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_Isotropic_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_Isotropic_table_child,
    ads_Prop_MElectric_MagneticPermeability_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_OrthotropicFrequency_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_OrthotropicFrequency_table_child,
    ads_Prop_MElectric_MagneticPermeability_OrthotropicFrequency_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_MagneticPermeability_Orthotropic_tableRolesEnm
{
    ads_Prop_MElectric_MagneticPermeability_Orthotropic_table_child,
    ads_Prop_MElectric_MagneticPermeability_Orthotropic_table_parent
};

/** Enum with record members. */
enum ads_Prop_MElectric_NonlinearBHMembersEnm
{
    ads_Prop_MElectric_NonlinearBH_dir
};

/** Enum with association roles. */
enum ads_Prop_MElectric_NonlinearBH_tableRolesEnm
{
    ads_Prop_MElectric_NonlinearBH_table_child,
    ads_Prop_MElectric_NonlinearBH_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_PermanentMagnetization_coercivityDirectionRolesEnm
{
    ads_Prop_MElectric_PermanentMagnetization_coercivityDirection_child,
    ads_Prop_MElectric_PermanentMagnetization_coercivityDirection_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_PermanentMagnetization_tableRolesEnm
{
    ads_Prop_MElectric_PermanentMagnetization_table_child,
    ads_Prop_MElectric_PermanentMagnetization_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Resistivity_Anisotropic_tableRolesEnm
{
    ads_Prop_MElectric_Resistivity_Anisotropic_table_child,
    ads_Prop_MElectric_Resistivity_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Resistivity_Isotropic_tableRolesEnm
{
    ads_Prop_MElectric_Resistivity_Isotropic_table_child,
    ads_Prop_MElectric_Resistivity_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MElectric_Resistivity_Orthotropic_tableRolesEnm
{
    ads_Prop_MElectric_Resistivity_Orthotropic_table_child,
    ads_Prop_MElectric_Resistivity_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_Damping_Beta_tableRolesEnm
{
    ads_Prop_MPiezoelectric_Damping_Beta_table_child,
    ads_Prop_MPiezoelectric_Damping_Beta_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_Damping_Structural_tableRolesEnm
{
    ads_Prop_MPiezoelectric_Damping_Structural_table_child,
    ads_Prop_MPiezoelectric_Damping_Structural_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_Dielectric_Anisotropic_tableRolesEnm
{
    ads_Prop_MPiezoelectric_Dielectric_Anisotropic_table_child,
    ads_Prop_MPiezoelectric_Dielectric_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_Dielectric_Isotropic_tableRolesEnm
{
    ads_Prop_MPiezoelectric_Dielectric_Isotropic_table_child,
    ads_Prop_MPiezoelectric_Dielectric_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_Dielectric_Orthotropic_tableRolesEnm
{
    ads_Prop_MPiezoelectric_Dielectric_Orthotropic_table_child,
    ads_Prop_MPiezoelectric_Dielectric_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_StrainCoefficient_tableRolesEnm
{
    ads_Prop_MPiezoelectric_StrainCoefficient_table_child,
    ads_Prop_MPiezoelectric_StrainCoefficient_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPiezoelectric_StressCoefficient_tableRolesEnm
{
    ads_Prop_MPiezoelectric_StressCoefficient_table_child,
    ads_Prop_MPiezoelectric_StressCoefficient_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_BulkModulus_tableRolesEnm
{
    ads_Prop_MPoreFluid_BulkModulus_table_child,
    ads_Prop_MPoreFluid_BulkModulus_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Conductivity_Anisotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Conductivity_Anisotropic_table_child,
    ads_Prop_MPoreFluid_Conductivity_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Conductivity_Isotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Conductivity_Isotropic_table_child,
    ads_Prop_MPoreFluid_Conductivity_Isotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Conductivity_Orthotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Conductivity_Orthotropic_table_child,
    ads_Prop_MPoreFluid_Conductivity_Orthotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Density_tableRolesEnm
{
    ads_Prop_MPoreFluid_Density_table_child,
    ads_Prop_MPoreFluid_Density_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Gel_tableRolesEnm
{
    ads_Prop_MPoreFluid_Gel_table_child,
    ads_Prop_MPoreFluid_Gel_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_MoistureSwelling_tableRolesEnm
{
    ads_Prop_MPoreFluid_MoistureSwelling_table_child,
    ads_Prop_MPoreFluid_MoistureSwelling_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_PermeabilityMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_specificWeight
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_AnisotropicMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Anisotropic_specificWeight
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Anisotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Anisotropic_table_child,
    ads_Prop_MPoreFluid_Permeability_Anisotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_CarmanKozenyMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_CarmanKozeny_specificWeight,
    ads_Prop_MPoreFluid_Permeability_CarmanKozeny_averageParticleRadius,
    ads_Prop_MPoreFluid_Permeability_CarmanKozeny_carmanKozenyConstant,
    ads_Prop_MPoreFluid_Permeability_CarmanKozeny_inertialDragCoefficient
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_IsotropicMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Isotropic_specificWeight
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Isotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Isotropic_table_child,
    ads_Prop_MPoreFluid_Permeability_Isotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_OrthotropicMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Orthotropic_specificWeight
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Orthotropic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Orthotropic_table_child,
    ads_Prop_MPoreFluid_Permeability_Orthotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_PorosityMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_specificWeight,
    ads_Prop_MPoreFluid_Permeability_Porosity_inertialDragCoefficient
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_anisoTableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_anisoTable_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_anisoTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_inertiaAnisoTableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaAnisoTable_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaAnisoTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_inertiaOrthoTableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaOrthoTable_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaOrthoTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_inertiaTableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaTable_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_inertiaTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_orthoTableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_orthoTable_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_orthoTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Porosity_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Porosity_table_child,
    ads_Prop_MPoreFluid_Permeability_Porosity_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_SaturationMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Saturation_specificWeight
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Saturation_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Saturation_table_child,
    ads_Prop_MPoreFluid_Permeability_Saturation_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Permeability_VelocityMembersEnm
{
    ads_Prop_MPoreFluid_Permeability_Velocity_specificWeight
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Permeability_Velocity_tableRolesEnm
{
    ads_Prop_MPoreFluid_Permeability_Velocity_table_child,
    ads_Prop_MPoreFluid_Permeability_Velocity_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Sorption_LogrithmicMembersEnm
{
    ads_Prop_MPoreFluid_Sorption_Logrithmic_exSorption
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Sorption_Logrithmic_tableRolesEnm
{
    ads_Prop_MPoreFluid_Sorption_Logrithmic_table_child,
    ads_Prop_MPoreFluid_Sorption_Logrithmic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Sorption_Scanning_tableRolesEnm
{
    ads_Prop_MPoreFluid_Sorption_Scanning_table_child,
    ads_Prop_MPoreFluid_Sorption_Scanning_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MPoreFluid_Sorption_TabularMembersEnm
{
    ads_Prop_MPoreFluid_Sorption_Tabular_exSorption
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_Sorption_Tabular_tableRolesEnm
{
    ads_Prop_MPoreFluid_Sorption_Tabular_table_child,
    ads_Prop_MPoreFluid_Sorption_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MPoreFluid_SpecificHeat_tableRolesEnm
{
    ads_Prop_MPoreFluid_SpecificHeat_table_child,
    ads_Prop_MPoreFluid_SpecificHeat_table_parent
};

#endif
