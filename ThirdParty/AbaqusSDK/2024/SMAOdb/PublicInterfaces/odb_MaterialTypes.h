//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_MaterialTypes_h
#define odb_MaterialTypes_h

// Begin local includes
#include <kmaO_ApiExtension.h>
#include <kmaO_MaterialContainer.h>
#include <kmaO_AcousticMedium.h>
#include <kmaO_AnnealTemperature.h>
#include <kmaO_BiaxialTestData.h>
#include <kmaO_BrittleCracking.h>
#include <kmaO_BrittleShear.h>
#include <kmaO_BrittleFailure.h>
#include <kmaO_CapCreepCohesion.h>
#include <kmaO_CapCreepConsolidation.h>
#include <kmaO_CapHardening.h>
#include <kmaO_CapPlasticity.h>
#include <kmaO_CastIronCompHardening.h>
#include <kmaO_CastIronPlasticity.h>
#include <kmaO_CastIronTensionHardening.h>
#include <kmaO_ClayHardening.h>
#include <kmaO_ClayPlasticity.h>
#include <kmaO_CombinedTestData.h>
#include <kmaO_Concrete.h>
#include <kmaO_ConcreteCompDamage.h>
#include <kmaO_ConcreteCompHardening.h>
#include <kmaO_ConcreteDamagedPlasticity.h>
#include <kmaO_ConcreteTensionDamage.h>
#include <kmaO_ConcreteTensionStiffening.h>
#include <kmaO_Conductivity.h>
#include <kmaO_ContactArea.h>
#include <kmaO_Creep.h>
#include <kmaO_CycledPlastic.h>
#include <kmaO_CyclicHardening.h>
#include <kmaO_DamageInitiation.h>
#include <kmaO_DamageEvolution.h>
#include <kmaO_DamageStabilization.h>
#include <kmaO_DamageStabilizationCo.h>
#include <kmaO_Damping.h>
#include <kmaO_DeformationPlasticity.h>
#include <kmaO_Density.h>
#include <kmaO_Depvar.h>
#include <kmaO_DetonationPoint.h>
#include <kmaO_Dielectric.h>
#include <kmaO_Diffusivity.h>
#include <kmaO_DruckerPrager.h>
#include <kmaO_DruckerPragerCreep.h>
#include <kmaO_DruckerPragerHardening.h>
#include <kmaO_Elastic.h>
#include <kmaO_ElectricalConductivity.h>
#include <kmaO_Eos.h>
//#include <kmaO_EosShear.h>
#include <kmaO_EosCompaction.h>
#include <kmaO_Expansion.h>
#include <kmaO_FailStrain.h>
#include <kmaO_FailStress.h>
#include <kmaO_FailureRatios.h>
#include <kmaO_FluidLeakoff.h>
#include <kmaO_CrushableFoam.h>
#include <kmaO_CrushableFoamHardening.h>
#include <kmaO_GapFlow.h>
#include <kmaO_Gel.h>
#include <kmaO_HeatGeneration.h>
#include <kmaO_Hyperelastic.h>
#include <kmaO_Hyperfoam.h>
#include <kmaO_Hypoelastic.h>
#include <kmaO_Hysteresis.h>
#include <kmaO_InelasticHeatFraction.h>
#include <kmaO_JouleHeatFraction.h>
#include <kmaO_LatentHeat.h>
#include <kmaO_Material.h>
#include <kmaO_GasketMembraneElastic.h>
#include <kmaO_MohrCoulombPlasticity.h>
#include <kmaO_MohrCoulombHardening.h>
#include <kmaO_TensionCutOff.h>
#include <kmaO_MoistureSwelling.h>
#include <kmaO_Ornl.h>
#include <kmaO_Permeability.h>
#include <kmaO_Piezoelectric.h>
#include <kmaO_PlanarTestData.h>
#include <kmaO_Plastic.h>
#include <kmaO_PoreFluidExpansion.h>
#include <kmaO_PorousBulkModuli.h>
#include <kmaO_PorousElastic.h>
#include <kmaO_PorousFailureCriteria.h>
#include <kmaO_PorousMetalPlasticity.h>
#include <kmaO_Potential.h>
#include <kmaO_PressureEffect.h>
#include <kmaO_RateDependent.h>
#include <kmaO_Ratios.h>
#include <kmaO_Regularization.h>
#include <kmaO_SaturationDependence.h>
#include <kmaO_ShearRetention.h>
#include <kmaO_ShearTestData.h>
#include <kmaO_SimpleShearTestData.h>
#include <kmaO_Solubility.h>
#include <kmaO_SoretEffect.h>
#include <kmaO_Sorption.h>
#include <kmaO_SpecificHeat.h>
#include <kmaO_Swelling.h>
#include <kmaO_TensionStiffening.h>
#include <kmaO_GasketThicknessBehavior.h>
#include <kmaO_GasketTransShearElastic.h>
#include <kmaO_TriaxialTestData.h>
#include <kmaO_Trs.h>
#include <kmaO_UniaxialTestData.h>
#include <kmaO_UserDefinedField.h>
#include <kmaO_UserMaterial.h>
#include <kmaO_UserOutputVariables.h>
#include <kmaO_VelocityDependence.h>
#include <kmaO_Viscoelastic.h>
#include <kmaO_Viscous.h>
#include <kmaO_VoidNucleation.h>
#include <kmaO_VolumetricTestData.h>
#include <kmaO_Shear.h>
#include <kmaO_Viscosity.h>
#include <kmaO_MullinsEffect.h>
#include <kmaO_LowDensityFoam.h>

typedef kmaO_ApiExtension odb_MaterialApi;
typedef kmaO_MaterialContainer odb_MaterialContainer;
typedef kmaO_MaterialContainerIT odb_MaterialContainerIT;
typedef kmaO_AcousticMedium odb_AcousticMedium;
typedef kmaO_AnnealTemperature odb_AnnealTemperature;
typedef kmaO_BiaxialTestData odb_BiaxialTestData;
typedef kmaO_BrittleCracking odb_BrittleCracking;
typedef kmaO_BrittleShear odb_BrittleShear;
typedef kmaO_BrittleFailure odb_BrittleFailure;
typedef kmaO_CapCreepCohesion odb_CapCreepCohesion;
typedef kmaO_CapCreepConsolidation odb_CapCreepConsolidation;
typedef kmaO_CapHardening odb_CapHardening;
typedef kmaO_CapPlasticity odb_CapPlasticity;
typedef kmaO_CastIronCompressionHardening odb_CastIronCompressionHardening;
typedef kmaO_CastIronPlasticity odb_CastIronPlasticity;
typedef kmaO_CastIronTensionHardening odb_CastIronTensionHardening;
typedef kmaO_ClayHardening odb_ClayHardening;
typedef kmaO_ClayPlasticity odb_ClayPlasticity;
typedef kmaO_CombinedTestData odb_CombinedTestData;
typedef kmaO_Concrete odb_Concrete;
typedef kmaO_ConcreteCompressionDamage odb_ConcreteCompressionDamage;
typedef kmaO_ConcreteCompressionHardening odb_ConcreteCompressionHardening;
typedef kmaO_ConcreteDamagedPlasticity odb_ConcreteDamagedPlasticity;
typedef kmaO_ConcreteTensionDamage odb_ConcreteTensionDamage;
typedef kmaO_ConcreteTensionStiffening odb_ConcreteTensionStiffening;
typedef kmaO_Conductivity odb_Conductivity;
typedef kmaO_ContactArea odb_ContactArea;
typedef kmaO_Creep odb_Creep;
typedef kmaO_CycledPlastic odb_CycledPlastic;
typedef kmaO_CyclicHardening odb_CyclicHardening;
typedef kmaO_DamageInitiation odb_DamageInitiation;
typedef kmaO_DamageEvolution odb_DamageEvolution;
typedef kmaO_DamageStabilization odb_DamageStabilization;
typedef kmaO_DamageStabilizationCohesive odb_DamageStabilizationCohesive;
typedef kmaO_Damping odb_Damping;
typedef kmaO_DeformationPlasticity odb_DeformationPlasticity;
typedef kmaO_Density odb_Density;
typedef kmaO_Depvar odb_Depvar;
typedef kmaO_DetonationPoint odb_DetonationPoint;
typedef kmaO_Dielectric odb_Dielectric;
typedef kmaO_Diffusivity odb_Diffusivity;
typedef kmaO_DruckerPrager odb_DruckerPrager;
typedef kmaO_DruckerPragerCreep odb_DruckerPragerCreep;
typedef kmaO_DruckerPragerHardening odb_DruckerPragerHardening;
typedef kmaO_Elastic odb_Elastic;
typedef kmaO_ElectricalConductivity odb_ElectricalConductivity;
typedef kmaO_Eos odb_Eos;
typedef kmaO_EosCompaction odb_EosCompaction;
typedef kmaO_Expansion odb_Expansion;
typedef kmaO_FailStrain odb_FailStrain;
typedef kmaO_FailStress odb_FailStress;
typedef kmaO_FailureRatios odb_FailureRatios;
typedef kmaO_FluidLeakoff odb_FluidLeakoff;
typedef kmaO_CrushableFoam odb_CrushableFoam;
typedef kmaO_CrushableFoamHardening odb_CrushableFoamHardening;
typedef kmaO_GapFlow odb_GapFlow;
typedef kmaO_Gel odb_Gel;
typedef kmaO_HeatGeneration odb_HeatGeneration;
typedef kmaO_Hyperelastic odb_Hyperelastic;
typedef kmaO_Hyperfoam odb_Hyperfoam;
typedef kmaO_Hypoelastic odb_Hypoelastic;
typedef kmaO_Hysteresis odb_Hysteresis;
typedef kmaO_InelasticHeatFraction odb_InelasticHeatFraction;
typedef kmaO_JouleHeatFraction odb_JouleHeatFraction;
typedef kmaO_LatentHeat odb_LatentHeat;
typedef kmaO_Material odb_Material;
typedef kmaO_GasketMembraneElastic odb_GasketMembraneElastic;
typedef kmaO_MohrCoulombPlasticity odb_MohrCoulombPlasticity;
typedef kmaO_MohrCoulombHardening odb_MohrCoulombHardening;
typedef kmaO_TensionCutOff odb_TensionCutOff;
typedef kmaO_MoistureSwelling odb_MoistureSwelling;
typedef kmaO_Ornl odb_Ornl;
typedef kmaO_Permeability odb_Permeability;
typedef kmaO_Piezoelectric odb_Piezoelectric;
typedef kmaO_PlanarTestData odb_PlanarTestData;
typedef kmaO_Plastic odb_Plastic;
typedef kmaO_PoreFluidExpansion odb_PoreFluidExpansion;
typedef kmaO_PorousBulkModuli odb_PorousBulkModuli;
typedef kmaO_PorousElastic odb_PorousElastic;
typedef kmaO_PorousFailureCriteria odb_PorousFailureCriteria;
typedef kmaO_PorousMetalPlasticity odb_PorousMetalPlasticity;
typedef kmaO_Potential odb_Potential;
typedef kmaO_PressureEffect odb_PressureEffect;
typedef kmaO_RateDependent odb_RateDependent;
typedef kmaO_Ratios odb_Ratios;
typedef kmaO_Regularization odb_Regularization;
typedef kmaO_SaturationDependence odb_SaturationDependence;
typedef kmaO_ShearRetention odb_ShearRetention;
typedef kmaO_ShearTestData odb_ShearTestData;
typedef kmaO_SimpleShearTestData odb_SimpleShearTestData;
typedef kmaO_Solubility odb_Solubility;
typedef kmaO_SoretEffect odb_SoretEffect;
typedef kmaO_Sorption odb_Sorption;
typedef kmaO_SpecificHeat odb_SpecificHeat;
typedef kmaO_Swelling odb_Swelling;
typedef kmaO_TensionStiffening odb_TensionStiffening;
typedef kmaO_GasketThicknessBehavior odb_GasketThicknessBehavior;
typedef kmaO_GasketTransverseShearElastic odb_GasketTransverseShearElastic;
typedef kmaO_TriaxialTestData odb_TriaxialTestData;
typedef kmaO_Trs odb_Trs;
typedef kmaO_UniaxialTestData odb_UniaxialTestData;
typedef kmaO_UserDefinedField odb_UserDefinedField;;
typedef kmaO_UserMaterial odb_UserMaterial;
typedef kmaO_UserOutputVariables odb_UserOutputVariables;
typedef kmaO_VelocityDependence odb_VelocityDependence;
typedef kmaO_Viscoelastic odb_Viscoelastic;
typedef kmaO_Viscous odb_Viscous;
typedef kmaO_VoidNucleation odb_VoidNucleation;
typedef kmaO_VolumetricTestData odb_VolumetricTestData;
typedef kmaO_Shear odb_Shear;
typedef kmaO_Viscosity odb_Viscosity;
typedef kmaO_MullinsEffect odb_MullinsEffect;
typedef kmaO_LowDensityFoam odb_LowDensityFoam;

#endif
