//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreCfdStepC_h
#define ads_CoreCfdStepC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment CfdStep of the latest level of form Core */

/** Bad Cell Indicator */
#define ads_BadCellIndicator (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 0))

/** Base class for a behavior of a PhysicsMedium */
#define ads_Behavior (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 1))

/** CFD compressible behavior */
#define ads_Behavior_Compressible (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 2))

/** Coupled behavior. */
#define ads_Behavior_Coupled (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 3))

#define ads_Behavior_Electromagnetics (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 4))

/** Electromagnetics DirectCurrent */
#define ads_Behavior_Electromagnetics_ElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 5))

/** CFD energy behavior */
#define ads_Behavior_Energy (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 6))

/** Euler behavior. */
#define ads_Behavior_Euler (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 7))

/** CFD fluid flow behavior */
#define ads_Behavior_FluidFlow (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 8))

/** Reference Buoyancy Altitude */
#define ads_Behavior_FluidFlow_referenceBuoyancyAltitude (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 9))

/** CFD fluid motion behavior */
#define ads_Behavior_Gravity (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 10))

#define ads_Behavior_Gravity_gravityVector (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 11))

/** Human comfort behavior */
#define ads_Behavior_HumanComfort (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 12))

#define ads_Behavior_HumanComfort_clusterParameters (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 13))

#define ads_Behavior_HumanComfort_globalTemperature (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 14))

/** Lagrangian behavior */
#define ads_Behavior_Lagrangian (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 15))

#define ads_Behavior_Lagrangian_massMaterial (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 16))

/** CFD fluid motion behavior */
#define ads_Behavior_Motion (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 17))

/** Multi phase behavior. */
#define ads_Behavior_MultiPhase (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 18))

/** Multi phase cavitation coefficients. */
#define ads_Behavior_MultiPhase_CavitationCoeff (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 19))

/** Multi phase sponge */
#define ads_Behavior_MultiPhase_Sponge (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 20))

/** Multi phase Vof behavior. */
#define ads_Behavior_MultiPhase_Vof (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 21))

#define ads_Behavior_MultiPhase_referenceFluid (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 22))

/** Multi species behavior. */
#define ads_Behavior_MultiSpecies (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 23))

/** Material that is used for fluid transport. */
#define ads_Behavior_MultiSpecies_backgroundFluid (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 24))

#define ads_Behavior_MultiSpecies_defogDemistModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 25))

/** Base for CFD radiation behavior */
#define ads_Behavior_Radiation (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 26))

/** Surface-surface radiation model */
#define ads_Behavior_Radiation_S2S (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 27))

/** Behavior to capture heat transfer due to solar radiation */
#define ads_Behavior_Solar (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 28))

#define ads_Behavior_Solar_solarModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 29))

/** Base for CFD turbulence behavior */
#define ads_Behavior_Turbulence (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 30))

/** DES turbulence model */
#define ads_Behavior_Turbulence_DES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 31))

/** k-epsilon renormalization group (RNG) model */
#define ads_Behavior_Turbulence_KEpsilonRNG (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 32))

/** k-epsilon realizable model */
#define ads_Behavior_Turbulence_KEpsilonRealizable (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 33))

/** k-epsilon zeta-f model */
#define ads_Behavior_Turbulence_KEpsilonZetaF (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 34))

/** k-omega SST model */
#define ads_Behavior_Turbulence_KOmegaSST (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 35))

/** Spalart-Allmaras turbulence model */
#define ads_Behavior_Turbulence_SpalartAllmaras (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 36))

/** Grid for clusters */
#define ads_ClusterClusterGrid (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 37))

/** Grid for clusters */
#define ads_ClusterGrid (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 38))

/** Nonlinear convergence controls for coupled solver. */
#define ads_CoupledNonLinearConvergenceControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 39))

/** Defog Demist Model */
#define ads_DefogDemistModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 40))

#define ads_DefogDemistModel_tagMaterial (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 41))

/** Deforming mesh controls for CFD */
#define ads_DeformingMeshFSIControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 42))

#define ads_Excitation_BC_CFD_lagrangianType (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 43))

/** Expert Numerics Controls */
#define ads_ExpertNumericsControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 44))

#define ads_ExpertNumericsControls_badCellIndicator (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 45))

/** Deforming mesh controls for explicit scheme */
#define ads_ExplicitDeformingMeshFSIControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 46))

/** Human comfort behavior- cluster parameters */
#define ads_HumanComfortClusterParameters (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 47))

/** Human comfort behavior - global temperature */
#define ads_HumanComfortGlobalTemperature (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 48))

/** Deforming mesh controls for implicit scheme */
#define ads_ImplicitDeformingMeshFSIControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 49))

#define ads_Interaction_CFD_lagrangianType (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 50))

/** Lagrangian behavior */
#define ads_LagrangianType (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 51))

/** Injection boundary */
#define ads_LagrangianType_Injection (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 52))

#define ads_LagrangianType_Injection_velocity (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 53))

/** Termination boundary */
#define ads_LagrangianType_Mixed (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 54))

/** Termination boundary */
#define ads_LagrangianType_Reflection (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 55))

/** Termination boundary */
#define ads_LagrangianType_Termination (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 56))

/** A Conjugate Projection with SSOR Preconditioner CG solver options */
#define ads_LinearEquationSolver_AConjugateProjection (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 57))

/** Algebraic Multigrid solver options */
#define ads_LinearEquationSolver_AMG (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 58))

#define ads_LinearEquationSolver_AMGBCGSL (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 59))

/** AMG preconditioner with FGMRES */
#define ads_LinearEquationSolver_AMGFGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 60))

/** AMG preconditioner with FGMRES */
#define ads_LinearEquationSolver_AMGGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 61))

/** DMP sparse-direct solver options */
#define ads_LinearEquationSolver_DMP (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 62))

/** Diagonally-scaled CG solver options */
#define ads_LinearEquationSolver_DSCG (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 63))

/** Diagonally-scaled FGMRES solver options */
#define ads_LinearEquationSolver_DSFGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 64))

/** Diagonally-scaled GMRES solver options */
#define ads_LinearEquationSolver_DSGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 65))

/** EQS AMG */
#define ads_LinearEquationSolver_EQSAMG (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 66))

/** ILU preconditioned BiCGStab(L) solver options */
#define ads_LinearEquationSolver_ILUBCGSL (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 67))

/** ILU FGMRES solver options */
#define ads_LinearEquationSolver_ILUFGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 68))

/** ILU GMRES solver options */
#define ads_LinearEquationSolver_ILUGMRES (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 69))

/** SSOR Preconditioned CG solver options */
#define ads_LinearEquationSolver_SSORCG (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 70))

/** Distribution to store element face to cluster mapping. This is a relational distribution with cluster collection as the range type. */
#define ads_Model_clusterMapping (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 71))

/** Distribution to store element face to cluster mapping. This is a relational distribution with cluster collection as the range type.This is to capture human comfort behavior. */
#define ads_Model_mrtClusterMapping (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 72))

/** Distribution to store view factors. Typically the distribution is sparse distribution with float values. This is to capture human comfort behavior. */
#define ads_Model_mrtViewFactors (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 73))

/** List of PhysicsMedium in the model */
#define ads_Model_physicsMedia (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 74))

/** Distribution to store solar arrays with the clusters as the domain. */
#define ads_Model_solarLoads (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 75))

/** Distribution to store view factors. Typically the distribution is sparse distribution with float values. */
#define ads_Model_viewFactors (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 76))

/** Non-linear solve control parameters for SIMPLE-based schemes */
#define ads_NonLinearConvergenceControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 77))

/** PPE control parameters for SIMPLE-based schemes */
#define ads_PPEControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 78))

/** A set of physics to be simulated on a set of regions */
#define ads_PhysicsMedium (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 79))

/** List of behaviors enabled for this flow */
#define ads_PhysicsMedium_behaviors (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 80))

/** List of excitations associated with this Physics Medium */
#define ads_PhysicsMedium_excitations (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 81))

/** List of behaviors enabled for this flow */
#define ads_PhysicsMedium_initialConditions (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 82))

#define ads_PhysicsMedium_sections (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 83))

/** Solar cluster parameters */
#define ads_SolarClusterParams (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 84))

/** Solar model that includes Solar model specifications and Solar parameters */
#define ads_SolarModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 85))

#define ads_SolarModel_solarClusterParams (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 86))

#define ads_SolarModel_solarParameters (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 87))

#define ads_SolarModel_solarSpecification (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 88))

/** Solar parameters */
#define ads_SolarParameters (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 89))

/** Solar model specifications */
#define ads_SolarSpecification (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 90))

/** Base CFD step */
#define ads_Step_Gen_CFD (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 91))

/** Steady-state CFD step. */
#define ads_Step_Gen_CFD_SteadyState (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 92))

#define ads_Step_Gen_CFD_SteadyState_coupledConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 93))

#define ads_Step_Gen_CFD_SteadyState_deformingMeshControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 94))

#define ads_Step_Gen_CFD_SteadyState_electricPotentialConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 95))

#define ads_Step_Gen_CFD_SteadyState_energyConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 96))

#define ads_Step_Gen_CFD_SteadyState_momentumConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 97))

#define ads_Step_Gen_CFD_SteadyState_ppeConvergenceControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 98))

#define ads_Step_Gen_CFD_SteadyState_radiationUpdateFrequencyControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 99))

#define ads_Step_Gen_CFD_SteadyState_speciesConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 100))

#define ads_Step_Gen_CFD_SteadyState_transportConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 101))

#define ads_Step_Gen_CFD_SteadyState_turbulenceConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 102))

#define ads_Step_Gen_CFD_SteadyState_vofConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 103))

/** Transient CFD step */
#define ads_Step_Gen_CFD_Transient (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 104))

#define ads_Step_Gen_CFD_Transient_coupledConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 105))

#define ads_Step_Gen_CFD_Transient_deformingMeshControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 106))

#define ads_Step_Gen_CFD_Transient_electricPotentialConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 107))

#define ads_Step_Gen_CFD_Transient_energyConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 108))

#define ads_Step_Gen_CFD_Transient_momentumConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 109))

#define ads_Step_Gen_CFD_Transient_ppeConvergenceControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 110))

#define ads_Step_Gen_CFD_Transient_radiationUpdateFrequencyControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 111))

#define ads_Step_Gen_CFD_Transient_speciesConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 112))

#define ads_Step_Gen_CFD_Transient_transportConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 113))

#define ads_Step_Gen_CFD_Transient_turbulenceConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 114))

#define ads_Step_Gen_CFD_Transient_vofConvergenceNonLinearControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 115))

#define ads_Step_Gen_CFD_coupledEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 116))

#define ads_Step_Gen_CFD_electricPotentialEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 117))

#define ads_Step_Gen_CFD_energyEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 118))

#define ads_Step_Gen_CFD_expertNumericsControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 119))

#define ads_Step_Gen_CFD_momentumEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 120))

#define ads_Step_Gen_CFD_ppeEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 121))

#define ads_Step_Gen_CFD_radiationEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 122))

#define ads_Step_Gen_CFD_radiationModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 123))

#define ads_Step_Gen_CFD_speciesEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 124))

#define ads_Step_Gen_CFD_stoppingCriteria (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 125))

#define ads_Step_Gen_CFD_transportEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 126))

#define ads_Step_Gen_CFD_turbulenceEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 127))

#define ads_Step_Gen_CFD_turbulenceModel (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 128))

#define ads_Step_Gen_CFD_vOFEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 129))

#define ads_Step_Gen_HeatTransferSteadyState_energyEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 130))

#define ads_Step_Gen_HeatTransfer_energyEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 131))

/** Solution convergence criteria: Stopping criteria */
#define ads_StoppingCriteria (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 132))

#define ads_StoppingCriteria_threshold (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 133))

/** Fixed CFL time incrementation scheme. This incremenation method is used for transient flow problems where a constant, maximum CFL condition is imposed. */
#define ads_TimeIncrementation_Fixed_CFL (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 134))

/** Fixed CFL time incrementation scheme for incompressible Navier Stokes flow. */
#define ads_TimeIncrementation_Fixed_CFL_CFD (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 135))

#define ads_UpdateFrequencyControls (ads_CoreFragmentTypeIndex(ads_CoreCfdStepFragment, 136))

/** 
Enum with record members. */
enum ads_BadCellIndicatorMembersEnm
{
    ads_BadCellIndicator_checkChevronQuality,
    ads_BadCellIndicator_checkNonOrthogonality,
    ads_BadCellIndicator_checkSkewness,
    ads_BadCellIndicator_checkSkewnessAngle,
    ads_BadCellIndicator_checkVolumeRatio,
    ads_BadCellIndicator_checkWarpAngle,
    ads_BadCellIndicator_maxNonOrthogonalityAngle,
    ads_BadCellIndicator_maxSkewnessAngle,
    ads_BadCellIndicator_maxVolumeRatio,
    ads_BadCellIndicator_maxWarpAngle,
    ads_BadCellIndicator_minSkewness
};

/** 
Enum with record members. */
enum ads_Behavior_CoupledMembersEnm
{
    ads_Behavior_Coupled_conservedVariables
};

/** 
Enum with record members. */
enum ads_Behavior_EnergyMembersEnm
{
    ads_Behavior_Energy_defaultTemperature,
    ads_Behavior_Energy_referenceBuoyancyTemperature,
    ads_Behavior_Energy_referenceHTCTemperature,
    ads_Behavior_Energy_referenceTemperature
};

/** 
Enum with record members. */
enum ads_Behavior_EulerMembersEnm
{
    ads_Behavior_Euler_operatingPressure
};

/** 
Enum with record members. */
enum ads_Behavior_FluidFlowMembersEnm
{
    ads_Behavior_FluidFlow_filmThickness,
    ads_Behavior_FluidFlow_isCompressible,
    ads_Behavior_FluidFlow_operatingPressure,
    ads_Behavior_FluidFlow_referenceBuoyancyDensity
};

/** 
Enum with association roles. */
enum ads_Behavior_FluidFlow_referenceBuoyancyAltitudeRolesEnm
{
    ads_Behavior_FluidFlow_referenceBuoyancyAltitude_child,
    ads_Behavior_FluidFlow_referenceBuoyancyAltitude_parent
};

/** 
Enum with record members. */
enum ads_Behavior_GravityMembersEnm
{
    ads_Behavior_Gravity_boussinesqApproximation
};

/** Enum with association roles. */
enum ads_Behavior_Gravity_gravityVectorRolesEnm
{
    ads_Behavior_Gravity_gravityVector_child,
    ads_Behavior_Gravity_gravityVector_parent
};

/** 
Enum with record members. */
enum ads_Behavior_HumanComfortMembersEnm
{
    ads_Behavior_HumanComfort_relativeHumidity,
    ads_Behavior_HumanComfort_useComputedRelativeHumidity
};

/** Enum with association roles. */
enum ads_Behavior_HumanComfort_clusterParametersRolesEnm
{
    ads_Behavior_HumanComfort_clusterParameters_child,
    ads_Behavior_HumanComfort_clusterParameters_parent
};

/** Enum with association roles. */
enum ads_Behavior_HumanComfort_globalTemperatureRolesEnm
{
    ads_Behavior_HumanComfort_globalTemperature_child,
    ads_Behavior_HumanComfort_globalTemperature_parent
};

/** 
Enum with record members. */
enum ads_Behavior_LagrangianMembersEnm
{
    ads_Behavior_Lagrangian_density,
    ads_Behavior_Lagrangian_massType,
    ads_Behavior_Lagrangian_particleEnergy,
    ads_Behavior_Lagrangian_poissonRatio,
    ads_Behavior_Lagrangian_specificHeat,
    ads_Behavior_Lagrangian_youngModulus
};

enum ads_Behavior_Lagrangian_massTypeEnm
{
    ads_Behavior_Lagrangian_massType_MASS,
    ads_Behavior_Lagrangian_massType_MASSLESS
};

/** Enum with association roles. */
enum ads_Behavior_Lagrangian_massMaterialRolesEnm
{
    ads_Behavior_Lagrangian_massMaterial_referent,
    ads_Behavior_Lagrangian_massMaterial_referrer
};

/** 
Enum with record members. */
enum ads_Behavior_MultiPhaseMembersEnm
{
    ads_Behavior_MultiPhase_capillarity,
    ads_Behavior_MultiPhase_cavitation,
    ads_Behavior_MultiPhase_contactAngle,
    ads_Behavior_MultiPhase_nonReflectingBoundaryCondition,
    ads_Behavior_MultiPhase_sharpening_coefficient,
    ads_Behavior_MultiPhase_surfaceTension,
    ads_Behavior_MultiPhase_wallAdhesion
};

enum ads_Behavior_MultiPhase_nonReflectingBoundaryConditionEnm
{
    ads_Behavior_MultiPhase_nonReflectingBoundaryCondition_CHARACTERISTIC,
    ads_Behavior_MultiPhase_nonReflectingBoundaryCondition_SPONGE
};

enum ads_Behavior_MultiPhase_surfaceTensionEnm
{
    ads_Behavior_MultiPhase_surfaceTension_CONTINUUMSURFACEFORCE,
    ads_Behavior_MultiPhase_surfaceTension_CONTINUUMSURFACESTRESS,
    ads_Behavior_MultiPhase_surfaceTension_NONE
};

/** 
Enum with record members. */
enum ads_Behavior_MultiPhase_CavitationCoeffMembersEnm
{
    ads_Behavior_MultiPhase_CavitationCoeff_capillarity,
    ads_Behavior_MultiPhase_CavitationCoeff_cavitation,
    ads_Behavior_MultiPhase_CavitationCoeff_contactAngle,
    ads_Behavior_MultiPhase_CavitationCoeff_nonReflectingBoundaryCondition,
    ads_Behavior_MultiPhase_CavitationCoeff_sharpening_coefficient,
    ads_Behavior_MultiPhase_CavitationCoeff_surfaceTension,
    ads_Behavior_MultiPhase_CavitationCoeff_wallAdhesion,
    ads_Behavior_MultiPhase_CavitationCoeff_bubble_density,
    ads_Behavior_MultiPhase_CavitationCoeff_condensation_coefficient,
    ads_Behavior_MultiPhase_CavitationCoeff_isotropic_reynolds_stress_constant,
    ads_Behavior_MultiPhase_CavitationCoeff_minimum_bubble_radius,
    ads_Behavior_MultiPhase_CavitationCoeff_vaporization_coefficient
};

enum ads_Behavior_MultiPhase_CavitationCoeff_nonReflectingBoundaryConditionEnm
{
    ads_Behavior_MultiPhase_CavitationCoeff_nonReflectingBoundaryCondition_CHARACTERISTIC,
    ads_Behavior_MultiPhase_CavitationCoeff_nonReflectingBoundaryCondition_SPONGE
};

enum ads_Behavior_MultiPhase_CavitationCoeff_surfaceTensionEnm
{
    ads_Behavior_MultiPhase_CavitationCoeff_surfaceTension_CONTINUUMSURFACEFORCE,
    ads_Behavior_MultiPhase_CavitationCoeff_surfaceTension_CONTINUUMSURFACESTRESS,
    ads_Behavior_MultiPhase_CavitationCoeff_surfaceTension_NONE
};

/** 
Enum with record members. */
enum ads_Behavior_MultiPhase_SpongeMembersEnm
{
    ads_Behavior_MultiPhase_Sponge_capillarity,
    ads_Behavior_MultiPhase_Sponge_cavitation,
    ads_Behavior_MultiPhase_Sponge_contactAngle,
    ads_Behavior_MultiPhase_Sponge_nonReflectingBoundaryCondition,
    ads_Behavior_MultiPhase_Sponge_sharpening_coefficient,
    ads_Behavior_MultiPhase_Sponge_surfaceTension,
    ads_Behavior_MultiPhase_Sponge_wallAdhesion,
    ads_Behavior_MultiPhase_Sponge_layerF1,
    ads_Behavior_MultiPhase_Sponge_layerF2,
    ads_Behavior_MultiPhase_Sponge_layerNd,
    ads_Behavior_MultiPhase_Sponge_layer_length
};

enum ads_Behavior_MultiPhase_Sponge_nonReflectingBoundaryConditionEnm
{
    ads_Behavior_MultiPhase_Sponge_nonReflectingBoundaryCondition_CHARACTERISTIC,
    ads_Behavior_MultiPhase_Sponge_nonReflectingBoundaryCondition_SPONGE
};

enum ads_Behavior_MultiPhase_Sponge_surfaceTensionEnm
{
    ads_Behavior_MultiPhase_Sponge_surfaceTension_CONTINUUMSURFACEFORCE,
    ads_Behavior_MultiPhase_Sponge_surfaceTension_CONTINUUMSURFACESTRESS,
    ads_Behavior_MultiPhase_Sponge_surfaceTension_NONE
};

/** 
Enum with record members. */
enum ads_Behavior_MultiPhase_VofMembersEnm
{
    ads_Behavior_MultiPhase_Vof_capillarity,
    ads_Behavior_MultiPhase_Vof_cavitation,
    ads_Behavior_MultiPhase_Vof_contactAngle,
    ads_Behavior_MultiPhase_Vof_nonReflectingBoundaryCondition,
    ads_Behavior_MultiPhase_Vof_sharpening_coefficient,
    ads_Behavior_MultiPhase_Vof_surfaceTension,
    ads_Behavior_MultiPhase_Vof_wallAdhesion,
    ads_Behavior_MultiPhase_Vof_reconstruction
};

enum ads_Behavior_MultiPhase_Vof_nonReflectingBoundaryConditionEnm
{
    ads_Behavior_MultiPhase_Vof_nonReflectingBoundaryCondition_CHARACTERISTIC,
    ads_Behavior_MultiPhase_Vof_nonReflectingBoundaryCondition_SPONGE
};

enum ads_Behavior_MultiPhase_Vof_surfaceTensionEnm
{
    ads_Behavior_MultiPhase_Vof_surfaceTension_CONTINUUMSURFACEFORCE,
    ads_Behavior_MultiPhase_Vof_surfaceTension_CONTINUUMSURFACESTRESS,
    ads_Behavior_MultiPhase_Vof_surfaceTension_NONE
};

enum ads_Behavior_MultiPhase_Vof_reconstructionEnm
{
    ads_Behavior_MultiPhase_Vof_reconstruction_CICSAM,
    ads_Behavior_MultiPhase_Vof_reconstruction_DONORACCEPTOR,
    ads_Behavior_MultiPhase_Vof_reconstruction_GEOMETRIC,
    ads_Behavior_MultiPhase_Vof_reconstruction_HRIC
};

/** Enum with association roles. */
enum ads_Behavior_MultiPhase_referenceFluidRolesEnm
{
    ads_Behavior_MultiPhase_referenceFluid_referent,
    ads_Behavior_MultiPhase_referenceFluid_referrer
};

/** 
Enum with record members. */
enum ads_Behavior_MultiSpeciesMembersEnm
{
    ads_Behavior_MultiSpecies_speciesModel
};

enum ads_Behavior_MultiSpecies_speciesModelEnm
{
    ads_Behavior_MultiSpecies_speciesModel_ACTIVE,
    ads_Behavior_MultiSpecies_speciesModel_PASSIVE
};

/** 
Enum with association roles. */
enum ads_Behavior_MultiSpecies_backgroundFluidRolesEnm
{
    ads_Behavior_MultiSpecies_backgroundFluid_referent,
    ads_Behavior_MultiSpecies_backgroundFluid_referrer
};

/** Enum with association roles. */
enum ads_Behavior_MultiSpecies_defogDemistModelRolesEnm
{
    ads_Behavior_MultiSpecies_defogDemistModel_child,
    ads_Behavior_MultiSpecies_defogDemistModel_parent
};

/** 
Enum with record members. */
enum ads_Behavior_Radiation_S2SMembersEnm
{
    ads_Behavior_Radiation_S2S_ClusterSizeLimit,
    ads_Behavior_Radiation_S2S_RaysPerCluster,
    ads_Behavior_Radiation_S2S_ambientTemperature,
    ads_Behavior_Radiation_S2S_viewFactorFacesPerCluster,
    ads_Behavior_Radiation_S2S_viewFactorSplitAngle
};

/** 
Enum with record members. */
enum ads_Behavior_SolarMembersEnm
{
    ads_Behavior_Solar_solarLoadRecompute
};

/** Enum with association roles. */
enum ads_Behavior_Solar_solarModelRolesEnm
{
    ads_Behavior_Solar_solarModel_child,
    ads_Behavior_Solar_solarModel_parent
};

/** 
Enum with record members. */
enum ads_Behavior_TurbulenceMembersEnm
{
    ads_Behavior_Turbulence_turbulentHTCYplus,
    ads_Behavior_Turbulence_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_turbulentSchmidtNumber
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_DESMembersEnm
{
    ads_Behavior_Turbulence_DES_turbulentHTCYplus,
    ads_Behavior_Turbulence_DES_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_DES_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_DES_cb1,
    ads_Behavior_Turbulence_DES_cb2,
    ads_Behavior_Turbulence_DES_ccr1,
    ads_Behavior_Turbulence_DES_ccr2,
    ads_Behavior_Turbulence_DES_cdes,
    ads_Behavior_Turbulence_DES_cfw1,
    ads_Behavior_Turbulence_DES_cfw2,
    ads_Behavior_Turbulence_DES_cr1,
    ads_Behavior_Turbulence_DES_cr2,
    ads_Behavior_Turbulence_DES_cr3,
    ads_Behavior_Turbulence_DES_curvatureCorrection,
    ads_Behavior_Turbulence_DES_cv1,
    ads_Behavior_Turbulence_DES_cv2,
    ads_Behavior_Turbulence_DES_cv3,
    ads_Behavior_Turbulence_DES_cw2,
    ads_Behavior_Turbulence_DES_cw3,
    ads_Behavior_Turbulence_DES_e,
    ads_Behavior_Turbulence_DES_kappa,
    ads_Behavior_Turbulence_DES_secondOrderClossure,
    ads_Behavior_Turbulence_DES_sigma
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_KEpsilonRNGMembersEnm
{
    ads_Behavior_Turbulence_KEpsilonRNG_turbulentHTCYplus,
    ads_Behavior_Turbulence_KEpsilonRNG_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_KEpsilonRNG_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_KEpsilonRNG_beta,
    ads_Behavior_Turbulence_KEpsilonRNG_e,
    ads_Behavior_Turbulence_KEpsilonRNG_eps1,
    ads_Behavior_Turbulence_KEpsilonRNG_eps2t,
    ads_Behavior_Turbulence_KEpsilonRNG_eps3,
    ads_Behavior_Turbulence_KEpsilonRNG_eps4,
    ads_Behavior_Turbulence_KEpsilonRNG_epsbuoyancycorrection,
    ads_Behavior_Turbulence_KEpsilonRNG_kappa,
    ads_Behavior_Turbulence_KEpsilonRNG_lambda0,
    ads_Behavior_Turbulence_KEpsilonRNG_mu,
    ads_Behavior_Turbulence_KEpsilonRNG_sigmaEps,
    ads_Behavior_Turbulence_KEpsilonRNG_sigmaK,
    ads_Behavior_Turbulence_KEpsilonRNG_sigmaRho,
    ads_Behavior_Turbulence_KEpsilonRNG_tkebuoyancycorrection
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_KEpsilonRealizableMembersEnm
{
    ads_Behavior_Turbulence_KEpsilonRealizable_turbulentHTCYplus,
    ads_Behavior_Turbulence_KEpsilonRealizable_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_KEpsilonRealizable_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_KEpsilonRealizable_a0,
    ads_Behavior_Turbulence_KEpsilonRealizable_aepsc,
    ads_Behavior_Turbulence_KEpsilonRealizable_amu,
    ads_Behavior_Turbulence_KEpsilonRealizable_curvatureCorrection,
    ads_Behavior_Turbulence_KEpsilonRealizable_delreyst,
    ads_Behavior_Turbulence_KEpsilonRealizable_e,
    ads_Behavior_Turbulence_KEpsilonRealizable_eps1,
    ads_Behavior_Turbulence_KEpsilonRealizable_eps1min,
    ads_Behavior_Turbulence_KEpsilonRealizable_eps2,
    ads_Behavior_Turbulence_KEpsilonRealizable_epsbuoyancycorrection,
    ads_Behavior_Turbulence_KEpsilonRealizable_kappa,
    ads_Behavior_Turbulence_KEpsilonRealizable_mu,
    ads_Behavior_Turbulence_KEpsilonRealizable_reyst,
    ads_Behavior_Turbulence_KEpsilonRealizable_secondOrderClossure,
    ads_Behavior_Turbulence_KEpsilonRealizable_sigmaEps,
    ads_Behavior_Turbulence_KEpsilonRealizable_sigmaK,
    ads_Behavior_Turbulence_KEpsilonRealizable_tkebuoyancycorrection
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_KEpsilonZetaFMembersEnm
{
    ads_Behavior_Turbulence_KEpsilonZetaF_turbulentHTCYplus,
    ads_Behavior_Turbulence_KEpsilonZetaF_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_KEpsilonZetaF_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_KEpsilonZetaF_c1,
    ads_Behavior_Turbulence_KEpsilonZetaF_c2p,
    ads_Behavior_Turbulence_KEpsilonZetaF_cEta,
    ads_Behavior_Turbulence_KEpsilonZetaF_cTau,
    ads_Behavior_Turbulence_KEpsilonZetaF_cl,
    ads_Behavior_Turbulence_KEpsilonZetaF_eps2,
    ads_Behavior_Turbulence_KEpsilonZetaF_eps3,
    ads_Behavior_Turbulence_KEpsilonZetaF_eps4,
    ads_Behavior_Turbulence_KEpsilonZetaF_mu,
    ads_Behavior_Turbulence_KEpsilonZetaF_sigmaEps,
    ads_Behavior_Turbulence_KEpsilonZetaF_sigmaK,
    ads_Behavior_Turbulence_KEpsilonZetaF_sigmaRho,
    ads_Behavior_Turbulence_KEpsilonZetaF_sigmaZeta
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_KOmegaSSTMembersEnm
{
    ads_Behavior_Turbulence_KOmegaSST_turbulentHTCYplus,
    ads_Behavior_Turbulence_KOmegaSST_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_KOmegaSST_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_KOmegaSST_a1,
    ads_Behavior_Turbulence_KOmegaSST_beta1,
    ads_Behavior_Turbulence_KOmegaSST_beta2,
    ads_Behavior_Turbulence_KOmegaSST_betas,
    ads_Behavior_Turbulence_KOmegaSST_curvatureCorrection,
    ads_Behavior_Turbulence_KOmegaSST_e,
    ads_Behavior_Turbulence_KOmegaSST_gamma1,
    ads_Behavior_Turbulence_KOmegaSST_gamma2,
    ads_Behavior_Turbulence_KOmegaSST_kappa,
    ads_Behavior_Turbulence_KOmegaSST_omebuoyancycorrection,
    ads_Behavior_Turbulence_KOmegaSST_secondOrderClossure,
    ads_Behavior_Turbulence_KOmegaSST_sigmaK1,
    ads_Behavior_Turbulence_KOmegaSST_sigmaK2,
    ads_Behavior_Turbulence_KOmegaSST_sigmaOmega1,
    ads_Behavior_Turbulence_KOmegaSST_sigmaOmega2,
    ads_Behavior_Turbulence_KOmegaSST_tkebuoyancycorrection
};

/** 
Enum with record members. */
enum ads_Behavior_Turbulence_SpalartAllmarasMembersEnm
{
    ads_Behavior_Turbulence_SpalartAllmaras_turbulentHTCYplus,
    ads_Behavior_Turbulence_SpalartAllmaras_turbulentPrandtlNumber,
    ads_Behavior_Turbulence_SpalartAllmaras_turbulentSchmidtNumber,
    ads_Behavior_Turbulence_SpalartAllmaras_cb1,
    ads_Behavior_Turbulence_SpalartAllmaras_cb2,
    ads_Behavior_Turbulence_SpalartAllmaras_ccr1,
    ads_Behavior_Turbulence_SpalartAllmaras_ccr2,
    ads_Behavior_Turbulence_SpalartAllmaras_cfw1,
    ads_Behavior_Turbulence_SpalartAllmaras_cfw2,
    ads_Behavior_Turbulence_SpalartAllmaras_cr1,
    ads_Behavior_Turbulence_SpalartAllmaras_cr2,
    ads_Behavior_Turbulence_SpalartAllmaras_cr3,
    ads_Behavior_Turbulence_SpalartAllmaras_curvatureCorrection,
    ads_Behavior_Turbulence_SpalartAllmaras_cv1,
    ads_Behavior_Turbulence_SpalartAllmaras_cv2,
    ads_Behavior_Turbulence_SpalartAllmaras_cv3,
    ads_Behavior_Turbulence_SpalartAllmaras_cw1,
    ads_Behavior_Turbulence_SpalartAllmaras_cw2,
    ads_Behavior_Turbulence_SpalartAllmaras_cw3,
    ads_Behavior_Turbulence_SpalartAllmaras_e,
    ads_Behavior_Turbulence_SpalartAllmaras_kappa,
    ads_Behavior_Turbulence_SpalartAllmaras_secondOrderClossure,
    ads_Behavior_Turbulence_SpalartAllmaras_sigma
};

/** 
Enum with grid dimensions. */
enum ads_ClusterClusterGridDimensionsEnm
{
    ads_ClusterClusterGrid_cluster1,
    ads_ClusterClusterGrid_cluster2
};

/** 
Enum with grid dimensions. */
enum ads_ClusterGridDimensionsEnm
{
    ads_ClusterGrid_cluster
};

/** 
Enum with record members. */
enum ads_CoupledNonLinearConvergenceControlsMembersEnm
{
    ads_CoupledNonLinearConvergenceControls_maxCFL,
    ads_CoupledNonLinearConvergenceControls_underrelaxationFactor
};

/** 
Enum with record members. */
enum ads_DefogDemistModelMembersEnm
{
    ads_DefogDemistModel_liquidDensity
};

/** Enum with association roles. */
enum ads_DefogDemistModel_tagMaterialRolesEnm
{
    ads_DefogDemistModel_tagMaterial_referent,
    ads_DefogDemistModel_tagMaterial_referrer
};

/** 
Enum with record members. */
enum ads_DeformingMeshFSIControlsMembersEnm
{
    ads_DeformingMeshFSIControls_deformingMeshStiffnessScale,
    ads_DeformingMeshFSIControls_fsiPenaltyScale,
    ads_DeformingMeshFSIControls_solidFluidDensityRatio
};

/** Enum with association roles. */
enum ads_Excitation_BC_CFD_lagrangianTypeRolesEnm
{
    ads_Excitation_BC_CFD_lagrangianType_child,
    ads_Excitation_BC_CFD_lagrangianType_parent
};

/** 
Enum with record members. */
enum ads_ExpertNumericsControlsMembersEnm
{
    ads_ExpertNumericsControls_disableSecondOrderAdvection,
    ads_ExpertNumericsControls_disableSecondaryDiffusion,
    ads_ExpertNumericsControls_enablePressureSkewnessCorrections,
    ads_ExpertNumericsControls_pressureDissipationTerm,
    ads_ExpertNumericsControls_pressureGradientFormulation,
    ads_ExpertNumericsControls_pressureSkewnessCorrectionsNumber
};

enum ads_ExpertNumericsControls_disableSecondOrderAdvectionEnm
{
    ads_ExpertNumericsControls_disableSecondOrderAdvection_ALL_CELLS,
    ads_ExpertNumericsControls_disableSecondOrderAdvection_BAD_CELLS_ONLY,
    ads_ExpertNumericsControls_disableSecondOrderAdvection_NEVER
};

enum ads_ExpertNumericsControls_disableSecondaryDiffusionEnm
{
    ads_ExpertNumericsControls_disableSecondaryDiffusion_ALL_CELLS,
    ads_ExpertNumericsControls_disableSecondaryDiffusion_BAD_CELLS_ONLY,
    ads_ExpertNumericsControls_disableSecondaryDiffusion_NEVER
};

enum ads_ExpertNumericsControls_enablePressureSkewnessCorrectionsEnm
{
    ads_ExpertNumericsControls_enablePressureSkewnessCorrections_IF_BAD_CELLS_DETECTED,
    ads_ExpertNumericsControls_enablePressureSkewnessCorrections_NO,
    ads_ExpertNumericsControls_enablePressureSkewnessCorrections_YES
};

enum ads_ExpertNumericsControls_pressureGradientFormulationEnm
{
    ads_ExpertNumericsControls_pressureGradientFormulation_CONSERVATIVE,
    ads_ExpertNumericsControls_pressureGradientFormulation_NONCONSERVATIVE
};

/** Enum with association roles. */
enum ads_ExpertNumericsControls_badCellIndicatorRolesEnm
{
    ads_ExpertNumericsControls_badCellIndicator_child,
    ads_ExpertNumericsControls_badCellIndicator_parent
};

/** 
Enum with record members. */
enum ads_ExplicitDeformingMeshFSIControlsMembersEnm
{
    ads_ExplicitDeformingMeshFSIControls_deformingMeshStiffnessScale,
    ads_ExplicitDeformingMeshFSIControls_fsiPenaltyScale,
    ads_ExplicitDeformingMeshFSIControls_solidFluidDensityRatio,
    ads_ExplicitDeformingMeshFSIControls_distortionControl,
    ads_ExplicitDeformingMeshFSIControls_remeshMaximumIncrement,
    ads_ExplicitDeformingMeshFSIControls_remeshMinimumIncrement
};

/** 
Enum with record members. */
enum ads_HumanComfortClusterParametersMembersEnm
{
    ads_HumanComfortClusterParameters_clusterAngle,
    ads_HumanComfortClusterParameters_clusterSizeLength,
    ads_HumanComfortClusterParameters_facesPerCluster,
    ads_HumanComfortClusterParameters_raysPerCluster
};

/** 
Enum with record members. */
enum ads_HumanComfortGlobalTemperatureMembersEnm
{
    ads_HumanComfortGlobalTemperature_clothingInsulation,
    ads_HumanComfortGlobalTemperature_diameter,
    ads_HumanComfortGlobalTemperature_emissivity,
    ads_HumanComfortGlobalTemperature_mets,
    ads_HumanComfortGlobalTemperature_temperature
};

/** 
Enum with record members. */
enum ads_ImplicitDeformingMeshFSIControlsMembersEnm
{
    ads_ImplicitDeformingMeshFSIControls_deformingMeshStiffnessScale,
    ads_ImplicitDeformingMeshFSIControls_fsiPenaltyScale,
    ads_ImplicitDeformingMeshFSIControls_solidFluidDensityRatio,
    ads_ImplicitDeformingMeshFSIControls_StiffnessVariationRangeType,
    ads_ImplicitDeformingMeshFSIControls_checkConvergenceInterval,
    ads_ImplicitDeformingMeshFSIControls_distance,
    ads_ImplicitDeformingMeshFSIControls_iterationLimit,
    ads_ImplicitDeformingMeshFSIControls_linearConvergenceLimit,
    ads_ImplicitDeformingMeshFSIControls_powerCoeff,
    ads_ImplicitDeformingMeshFSIControls_ratio,
    ads_ImplicitDeformingMeshFSIControls_solveFromUndeformed,
    ads_ImplicitDeformingMeshFSIControls_stiffnessType,
    ads_ImplicitDeformingMeshFSIControls_volume
};

enum ads_ImplicitDeformingMeshFSIControls_StiffnessVariationRangeTypeEnm
{
    ads_ImplicitDeformingMeshFSIControls_StiffnessVariationRangeType_DISTANCE,
    ads_ImplicitDeformingMeshFSIControls_StiffnessVariationRangeType_RATIO,
    ads_ImplicitDeformingMeshFSIControls_StiffnessVariationRangeType_VOLUME
};

enum ads_ImplicitDeformingMeshFSIControls_stiffnessTypeEnm
{
    ads_ImplicitDeformingMeshFSIControls_stiffnessType_INVERSE_DISTANCE,
    ads_ImplicitDeformingMeshFSIControls_stiffnessType_INVERSE_VOLUME,
    ads_ImplicitDeformingMeshFSIControls_stiffnessType_UNIFORM
};

/** Enum with association roles. */
enum ads_Interaction_CFD_lagrangianTypeRolesEnm
{
    ads_Interaction_CFD_lagrangianType_child,
    ads_Interaction_CFD_lagrangianType_parent
};

/** 
Enum with record members. */
enum ads_LagrangianType_InjectionMembersEnm
{
    ads_LagrangianType_Injection_deviation,
    ads_LagrangianType_Injection_diameter1,
    ads_LagrangianType_Injection_diameter2,
    ads_LagrangianType_Injection_distributionType,
    ads_LagrangianType_Injection_exponent,
    ads_LagrangianType_Injection_injectionProfile,
    ads_LagrangianType_Injection_massFlowRate,
    ads_LagrangianType_Injection_particleFlowRate,
    ads_LagrangianType_Injection_temperature,
    ads_LagrangianType_Injection_velocityType
};

enum ads_LagrangianType_Injection_distributionTypeEnm
{
    ads_LagrangianType_Injection_distributionType_LINER,
    ads_LagrangianType_Injection_distributionType_LOG_NORMAL,
    ads_LagrangianType_Injection_distributionType_NORMAL,
    ads_LagrangianType_Injection_distributionType_ROSIN_RAMMLER,
    ads_LagrangianType_Injection_distributionType_UNIFORM
};

enum ads_LagrangianType_Injection_injectionProfileEnm
{
    ads_LagrangianType_Injection_injectionProfile_MASS_FLOW_RATE,
    ads_LagrangianType_Injection_injectionProfile_PARTICLE_FLOW_RATE
};

enum ads_LagrangianType_Injection_velocityTypeEnm
{
    ads_LagrangianType_Injection_velocityType_ABSOLUTE,
    ads_LagrangianType_Injection_velocityType_RELATIVE
};

/** Enum with association roles. */
enum ads_LagrangianType_Injection_velocityRolesEnm
{
    ads_LagrangianType_Injection_velocity_child,
    ads_LagrangianType_Injection_velocity_parent
};

/** 
Enum with record members. */
enum ads_LagrangianType_MixedMembersEnm
{
    ads_LagrangianType_Mixed_alterPorosity,
    ads_LagrangianType_Mixed_constantFraction,
    ads_LagrangianType_Mixed_fractionDeposited,
    ads_LagrangianType_Mixed_normalCoefficientRestitution,
    ads_LagrangianType_Mixed_poissonRatio,
    ads_LagrangianType_Mixed_tangentialCoefficientRestitution,
    ads_LagrangianType_Mixed_youngModulus
};

enum ads_LagrangianType_Mixed_fractionDepositedEnm
{
    ads_LagrangianType_Mixed_fractionDeposited_CONSTANT,
    ads_LagrangianType_Mixed_fractionDeposited_DEPOSITION_MODEL
};

/** 
Enum with record members. */
enum ads_LagrangianType_ReflectionMembersEnm
{
    ads_LagrangianType_Reflection_normalCoefficientRestitution,
    ads_LagrangianType_Reflection_tangentialCoefficientRestitution
};

/** 
Enum with record members. */
enum ads_LagrangianType_TerminationMembersEnm
{
    ads_LagrangianType_Termination_alterPorosity
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_AConjugateProjectionMembersEnm
{
    ads_LinearEquationSolver_AConjugateProjection_aNormMinimum,
    ads_LinearEquationSolver_AConjugateProjection_checkConvergenceInterval,
    ads_LinearEquationSolver_AConjugateProjection_iterationLimit,
    ads_LinearEquationSolver_AConjugateProjection_linearConvergenceLimit,
    ads_LinearEquationSolver_AConjugateProjection_maximumPhiVectors,
    ads_LinearEquationSolver_AConjugateProjection_phiVectorSeeding,
    ads_LinearEquationSolver_AConjugateProjection_writeConvergence,
    ads_LinearEquationSolver_AConjugateProjection_writeDiagnostics
};

enum ads_LinearEquationSolver_AConjugateProjection_phiVectorSeedingEnm
{
    ads_LinearEquationSolver_AConjugateProjection_phiVectorSeeding_LINEAR,
    ads_LinearEquationSolver_AConjugateProjection_phiVectorSeeding_NONE,
    ads_LinearEquationSolver_AConjugateProjection_phiVectorSeeding_SCALAR
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_AMGMembersEnm
{
    ads_LinearEquationSolver_AMG_checkConvergenceInterval,
    ads_LinearEquationSolver_AMG_cycle,
    ads_LinearEquationSolver_AMG_iterationLimit,
    ads_LinearEquationSolver_AMG_krylovSolver,
    ads_LinearEquationSolver_AMG_linearConvergenceLimit,
    ads_LinearEquationSolver_AMG_maximumCoarseSize,
    ads_LinearEquationSolver_AMG_maximumLevels,
    ads_LinearEquationSolver_AMG_minimumIterations,
    ads_LinearEquationSolver_AMG_numberSmoothDownSteps,
    ads_LinearEquationSolver_AMG_numberSmoothUpSteps,
    ads_LinearEquationSolver_AMG_smoother,
    ads_LinearEquationSolver_AMG_writeConvergence,
    ads_LinearEquationSolver_AMG_writeDiagnostics
};

enum ads_LinearEquationSolver_AMG_cycleEnm
{
    ads_LinearEquationSolver_AMG_cycle_V,
    ads_LinearEquationSolver_AMG_cycle_W
};

enum ads_LinearEquationSolver_AMG_krylovSolverEnm
{
    ads_LinearEquationSolver_AMG_krylovSolver_BCGS,
    ads_LinearEquationSolver_AMG_krylovSolver_CG,
    ads_LinearEquationSolver_AMG_krylovSolver_FGMRES
};

enum ads_LinearEquationSolver_AMG_smootherEnm
{
    ads_LinearEquationSolver_AMG_smoother_CHEBYCHEV,
    ads_LinearEquationSolver_AMG_smoother_ICC,
    ads_LinearEquationSolver_AMG_smoother_ILU,
    ads_LinearEquationSolver_AMG_smoother_SSOR
};

/** Enum with record members. */
enum ads_LinearEquationSolver_AMGBCGSLMembersEnm
{
    ads_LinearEquationSolver_AMGBCGSL_checkConvergenceInterval,
    ads_LinearEquationSolver_AMGBCGSL_cycle,
    ads_LinearEquationSolver_AMGBCGSL_iterationLimit,
    ads_LinearEquationSolver_AMGBCGSL_linearConvergenceLimit,
    ads_LinearEquationSolver_AMGBCGSL_maximumCoarseSize,
    ads_LinearEquationSolver_AMGBCGSL_maximumLevels,
    ads_LinearEquationSolver_AMGBCGSL_minimumIterations,
    ads_LinearEquationSolver_AMGBCGSL_numberSmoothDownSteps,
    ads_LinearEquationSolver_AMGBCGSL_numberSmoothUpSteps,
    ads_LinearEquationSolver_AMGBCGSL_smoother,
    ads_LinearEquationSolver_AMGBCGSL_writeConvergence,
    ads_LinearEquationSolver_AMGBCGSL_writeDiagnostics
};

enum ads_LinearEquationSolver_AMGBCGSL_cycleEnm
{
    ads_LinearEquationSolver_AMGBCGSL_cycle_V,
    ads_LinearEquationSolver_AMGBCGSL_cycle_W
};

enum ads_LinearEquationSolver_AMGBCGSL_smootherEnm
{
    ads_LinearEquationSolver_AMGBCGSL_smoother_CHEBYCHEV,
    ads_LinearEquationSolver_AMGBCGSL_smoother_ICC,
    ads_LinearEquationSolver_AMGBCGSL_smoother_ILU,
    ads_LinearEquationSolver_AMGBCGSL_smoother_SSOR
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_AMGFGMRESMembersEnm
{
    ads_LinearEquationSolver_AMGFGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_AMGFGMRES_cycle,
    ads_LinearEquationSolver_AMGFGMRES_iterationLimit,
    ads_LinearEquationSolver_AMGFGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_AMGFGMRES_maximumCoarseSize,
    ads_LinearEquationSolver_AMGFGMRES_maximumLevels,
    ads_LinearEquationSolver_AMGFGMRES_minimumIterations,
    ads_LinearEquationSolver_AMGFGMRES_numberRestartVectors,
    ads_LinearEquationSolver_AMGFGMRES_numberSmoothDownSteps,
    ads_LinearEquationSolver_AMGFGMRES_numberSmoothUpSteps,
    ads_LinearEquationSolver_AMGFGMRES_smoother,
    ads_LinearEquationSolver_AMGFGMRES_writeConvergence,
    ads_LinearEquationSolver_AMGFGMRES_writeDiagnostics
};

enum ads_LinearEquationSolver_AMGFGMRES_cycleEnm
{
    ads_LinearEquationSolver_AMGFGMRES_cycle_V,
    ads_LinearEquationSolver_AMGFGMRES_cycle_W
};

enum ads_LinearEquationSolver_AMGFGMRES_smootherEnm
{
    ads_LinearEquationSolver_AMGFGMRES_smoother_CHEBYCHEV,
    ads_LinearEquationSolver_AMGFGMRES_smoother_ILU
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_AMGGMRESMembersEnm
{
    ads_LinearEquationSolver_AMGGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_AMGGMRES_cycle,
    ads_LinearEquationSolver_AMGGMRES_iterationLimit,
    ads_LinearEquationSolver_AMGGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_AMGGMRES_maximumCoarseSize,
    ads_LinearEquationSolver_AMGGMRES_maximumLevels,
    ads_LinearEquationSolver_AMGGMRES_minimumIterations,
    ads_LinearEquationSolver_AMGGMRES_numberRestartVectors,
    ads_LinearEquationSolver_AMGGMRES_numberSmoothDownSteps,
    ads_LinearEquationSolver_AMGGMRES_numberSmoothUpSteps,
    ads_LinearEquationSolver_AMGGMRES_smoother,
    ads_LinearEquationSolver_AMGGMRES_writeConvergence,
    ads_LinearEquationSolver_AMGGMRES_writeDiagnostics
};

enum ads_LinearEquationSolver_AMGGMRES_cycleEnm
{
    ads_LinearEquationSolver_AMGGMRES_cycle_V,
    ads_LinearEquationSolver_AMGGMRES_cycle_W
};

enum ads_LinearEquationSolver_AMGGMRES_smootherEnm
{
    ads_LinearEquationSolver_AMGGMRES_smoother_CHEBYCHEV,
    ads_LinearEquationSolver_AMGGMRES_smoother_ILU
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_DMPMembersEnm
{
    ads_LinearEquationSolver_DMP_dropTolerance
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_DSCGMembersEnm
{
    ads_LinearEquationSolver_DSCG_checkConvergenceInterval,
    ads_LinearEquationSolver_DSCG_iterationLimit,
    ads_LinearEquationSolver_DSCG_linearConvergenceLimit,
    ads_LinearEquationSolver_DSCG_minimumIterations,
    ads_LinearEquationSolver_DSCG_writeConvergence,
    ads_LinearEquationSolver_DSCG_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_DSFGMRESMembersEnm
{
    ads_LinearEquationSolver_DSFGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_DSFGMRES_iterationLimit,
    ads_LinearEquationSolver_DSFGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_DSFGMRES_minimumIterations,
    ads_LinearEquationSolver_DSFGMRES_numberRestartVectors,
    ads_LinearEquationSolver_DSFGMRES_writeConvergence,
    ads_LinearEquationSolver_DSFGMRES_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_DSGMRESMembersEnm
{
    ads_LinearEquationSolver_DSGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_DSGMRES_iterationLimit,
    ads_LinearEquationSolver_DSGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_DSGMRES_minimumIterations,
    ads_LinearEquationSolver_DSGMRES_numberRestartVectors,
    ads_LinearEquationSolver_DSGMRES_writeConvergence,
    ads_LinearEquationSolver_DSGMRES_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_EQSAMGMembersEnm
{
    ads_LinearEquationSolver_EQSAMG_agglomerate,
    ads_LinearEquationSolver_EQSAMG_checkConvergenceInterval,
    ads_LinearEquationSolver_EQSAMG_cycle,
    ads_LinearEquationSolver_EQSAMG_groupSize,
    ads_LinearEquationSolver_EQSAMG_iterationLimit,
    ads_LinearEquationSolver_EQSAMG_linearConvergenceLimit,
    ads_LinearEquationSolver_EQSAMG_maximumCoarseSize,
    ads_LinearEquationSolver_EQSAMG_maximumLevels,
    ads_LinearEquationSolver_EQSAMG_minimumIterations,
    ads_LinearEquationSolver_EQSAMG_numberSmoothDownSteps,
    ads_LinearEquationSolver_EQSAMG_numberSmoothUpSteps,
    ads_LinearEquationSolver_EQSAMG_omegaIlu,
    ads_LinearEquationSolver_EQSAMG_overlap,
    ads_LinearEquationSolver_EQSAMG_smoother,
    ads_LinearEquationSolver_EQSAMG_writeConvergence,
    ads_LinearEquationSolver_EQSAMG_writeDiagnostics
};

enum ads_LinearEquationSolver_EQSAMG_agglomerateEnm
{
    ads_LinearEquationSolver_EQSAMG_agglomerate_BFS,
    ads_LinearEquationSolver_EQSAMG_agglomerate_DFS
};

enum ads_LinearEquationSolver_EQSAMG_cycleEnm
{
    ads_LinearEquationSolver_EQSAMG_cycle_F,
    ads_LinearEquationSolver_EQSAMG_cycle_FLEX,
    ads_LinearEquationSolver_EQSAMG_cycle_V,
    ads_LinearEquationSolver_EQSAMG_cycle_W
};

enum ads_LinearEquationSolver_EQSAMG_smootherEnm
{
    ads_LinearEquationSolver_EQSAMG_smoother_GS,
    ads_LinearEquationSolver_EQSAMG_smoother_ILU
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_ILUBCGSLMembersEnm
{
    ads_LinearEquationSolver_ILUBCGSL_checkConvergenceInterval,
    ads_LinearEquationSolver_ILUBCGSL_iterationLimit,
    ads_LinearEquationSolver_ILUBCGSL_linearConvergenceLimit,
    ads_LinearEquationSolver_ILUBCGSL_minimumIterations,
    ads_LinearEquationSolver_ILUBCGSL_writeConvergence,
    ads_LinearEquationSolver_ILUBCGSL_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_ILUFGMRESMembersEnm
{
    ads_LinearEquationSolver_ILUFGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_ILUFGMRES_iterationLimit,
    ads_LinearEquationSolver_ILUFGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_ILUFGMRES_minimumIterations,
    ads_LinearEquationSolver_ILUFGMRES_numberRestartVectors,
    ads_LinearEquationSolver_ILUFGMRES_writeConvergence,
    ads_LinearEquationSolver_ILUFGMRES_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_ILUGMRESMembersEnm
{
    ads_LinearEquationSolver_ILUGMRES_checkConvergenceInterval,
    ads_LinearEquationSolver_ILUGMRES_iterationLimit,
    ads_LinearEquationSolver_ILUGMRES_linearConvergenceLimit,
    ads_LinearEquationSolver_ILUGMRES_minimumIterations,
    ads_LinearEquationSolver_ILUGMRES_numberRestartVectors,
    ads_LinearEquationSolver_ILUGMRES_writeConvergence,
    ads_LinearEquationSolver_ILUGMRES_writeDiagnostics
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_SSORCGMembersEnm
{
    ads_LinearEquationSolver_SSORCG_checkConvergenceInterval,
    ads_LinearEquationSolver_SSORCG_iterationLimit,
    ads_LinearEquationSolver_SSORCG_linearConvergenceLimit,
    ads_LinearEquationSolver_SSORCG_minimumIterations,
    ads_LinearEquationSolver_SSORCG_writeConvergence,
    ads_LinearEquationSolver_SSORCG_writeDiagnostics
};

/** 
Enum with association roles. */
enum ads_Model_clusterMappingRolesEnm
{
    ads_Model_clusterMapping_child,
    ads_Model_clusterMapping_parent
};

/** 
Enum with association roles. */
enum ads_Model_mrtClusterMappingRolesEnm
{
    ads_Model_mrtClusterMapping_child,
    ads_Model_mrtClusterMapping_parent
};

/** 
Enum with association roles. */
enum ads_Model_mrtViewFactorsRolesEnm
{
    ads_Model_mrtViewFactors_child,
    ads_Model_mrtViewFactors_parent
};

/** 
Enum with association roles. */
enum ads_Model_physicsMediaRolesEnm
{
    ads_Model_physicsMedia_child,
    ads_Model_physicsMedia_parent
};

/** 
Enum with association roles. */
enum ads_Model_solarLoadsRolesEnm
{
    ads_Model_solarLoads_child,
    ads_Model_solarLoads_parent
};

/** 
Enum with association roles. */
enum ads_Model_viewFactorsRolesEnm
{
    ads_Model_viewFactors_child,
    ads_Model_viewFactors_parent
};

/** 
Enum with record members. */
enum ads_NonLinearConvergenceControlsMembersEnm
{
    ads_NonLinearConvergenceControls_firstOrderUpwind,
    ads_NonLinearConvergenceControls_iterationBetweenSolves,
    ads_NonLinearConvergenceControls_nonlinearConvergenceTolerance,
    ads_NonLinearConvergenceControls_solutionChangeTolerance,
    ads_NonLinearConvergenceControls_solvePeriod,
    ads_NonLinearConvergenceControls_subCycling,
    ads_NonLinearConvergenceControls_subCyclingCfl,
    ads_NonLinearConvergenceControls_underrelaxationFactor,
    ads_NonLinearConvergenceControls_underrelaxationFactorSolid,
    ads_NonLinearConvergenceControls_zeroTheGradients
};

enum ads_NonLinearConvergenceControls_subCyclingEnm
{
    ads_NonLinearConvergenceControls_subCycling_AUTOMATIC,
    ads_NonLinearConvergenceControls_subCycling_MANUAL,
    ads_NonLinearConvergenceControls_subCycling_NONE
};

/** 
Enum with record members. */
enum ads_PPEControlsMembersEnm
{
    ads_PPEControls_underrelaxationFactor
};

/** 
Enum with association roles. */
enum ads_PhysicsMedium_behaviorsRolesEnm
{
    ads_PhysicsMedium_behaviors_child,
    ads_PhysicsMedium_behaviors_parent
};

/** 
Enum with association roles. */
enum ads_PhysicsMedium_excitationsRolesEnm
{
    ads_PhysicsMedium_excitations_referent,
    ads_PhysicsMedium_excitations_referrer
};

/** 
Enum with association roles. */
enum ads_PhysicsMedium_initialConditionsRolesEnm
{
    ads_PhysicsMedium_initialConditions_referent,
    ads_PhysicsMedium_initialConditions_referrer
};

/** Enum with association roles. */
enum ads_PhysicsMedium_sectionsRolesEnm
{
    ads_PhysicsMedium_sections_referent,
    ads_PhysicsMedium_sections_referrer
};

/** 
Enum with record members. */
enum ads_SolarClusterParamsMembersEnm
{
    ads_SolarClusterParams_solarClusterAngle,
    ads_SolarClusterParams_solarClusterSizeLength,
    ads_SolarClusterParams_solarLoadFactorFacesPerCluster,
    ads_SolarClusterParams_solarRaysPerCluster
};

/** Enum with association roles. */
enum ads_SolarModel_solarClusterParamsRolesEnm
{
    ads_SolarModel_solarClusterParams_child,
    ads_SolarModel_solarClusterParams_parent
};

/** Enum with association roles. */
enum ads_SolarModel_solarParametersRolesEnm
{
    ads_SolarModel_solarParameters_child,
    ads_SolarModel_solarParameters_parent
};

/** Enum with association roles. */
enum ads_SolarModel_solarSpecificationRolesEnm
{
    ads_SolarModel_solarSpecification_child,
    ads_SolarModel_solarSpecification_parent
};

/** 
Enum with record members. */
enum ads_SolarParametersMembersEnm
{
    ads_SolarParameters_groundAxis,
    ads_SolarParameters_groundAxisHeight,
    ads_SolarParameters_groundReflectivity,
    ads_SolarParameters_spectralFraction
};

/** 
Enum with record members. */
enum ads_SolarSpecificationMembersEnm
{
    ads_SolarSpecification_dateAndTime,
    ads_SolarSpecification_directIrradiance,
    ads_SolarSpecification_eastDirection,
    ads_SolarSpecification_indirectFraction,
    ads_SolarSpecification_indirectIrradiance,
    ads_SolarSpecification_latitude,
    ads_SolarSpecification_longitude,
    ads_SolarSpecification_northDirection,
    ads_SolarSpecification_solarDatabaseType,
    ads_SolarSpecification_sunDirection,
    ads_SolarSpecification_timeZone
};

/** 
Enum with record members. */
enum ads_Step_Gen_CFDMembersEnm
{
    ads_Step_Gen_CFD_designSensitivity,
    ads_Step_Gen_CFD_dsa,
    ads_Step_Gen_CFD_beginningTime,
    ads_Step_Gen_CFD_autoUnderRelaxation,
    ads_Step_Gen_CFD_energyEquation,
    ads_Step_Gen_CFD_errorThreshold,
    ads_Step_Gen_CFD_freezeEnergy,
    ads_Step_Gen_CFD_freezeFlow,
    ads_Step_Gen_CFD_gradientCalculation
};

enum ads_Step_Gen_CFD_designSensitivityEnm
{
    ads_Step_Gen_CFD_designSensitivity_ADJOINT,
    ads_Step_Gen_CFD_designSensitivity_NONE
};

enum ads_Step_Gen_CFD_energyEquationEnm
{
    ads_Step_Gen_CFD_energyEquation_ENTHALPY,
    ads_Step_Gen_CFD_energyEquation_NOENERGY,
    ads_Step_Gen_CFD_energyEquation_TEMPERATURE
};

enum ads_Step_Gen_CFD_gradientCalculationEnm
{
    ads_Step_Gen_CFD_gradientCalculation_GREEN_GAUSS,
    ads_Step_Gen_CFD_gradientCalculation_HYBRID_LEAST_SQUARES,
    ads_Step_Gen_CFD_gradientCalculation_LEAST_SQUARES
};

/** 
Enum with record members. */
enum ads_Step_Gen_CFD_SteadyStateMembersEnm
{
    ads_Step_Gen_CFD_SteadyState_designSensitivity,
    ads_Step_Gen_CFD_SteadyState_dsa,
    ads_Step_Gen_CFD_SteadyState_beginningTime,
    ads_Step_Gen_CFD_SteadyState_autoUnderRelaxation,
    ads_Step_Gen_CFD_SteadyState_energyEquation,
    ads_Step_Gen_CFD_SteadyState_errorThreshold,
    ads_Step_Gen_CFD_SteadyState_freezeEnergy,
    ads_Step_Gen_CFD_SteadyState_freezeFlow,
    ads_Step_Gen_CFD_SteadyState_gradientCalculation,
    ads_Step_Gen_CFD_SteadyState_innerIterations,
    ads_Step_Gen_CFD_SteadyState_maximumOuterIterations,
    ads_Step_Gen_CFD_SteadyState_momentumCoupling,
    ads_Step_Gen_CFD_SteadyState_particleEnergyCoupling,
    ads_Step_Gen_CFD_SteadyState_postSimulationTime
};

enum ads_Step_Gen_CFD_SteadyState_designSensitivityEnm
{
    ads_Step_Gen_CFD_SteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_CFD_SteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_CFD_SteadyState_energyEquationEnm
{
    ads_Step_Gen_CFD_SteadyState_energyEquation_ENTHALPY,
    ads_Step_Gen_CFD_SteadyState_energyEquation_NOENERGY,
    ads_Step_Gen_CFD_SteadyState_energyEquation_TEMPERATURE
};

enum ads_Step_Gen_CFD_SteadyState_gradientCalculationEnm
{
    ads_Step_Gen_CFD_SteadyState_gradientCalculation_GREEN_GAUSS,
    ads_Step_Gen_CFD_SteadyState_gradientCalculation_HYBRID_LEAST_SQUARES,
    ads_Step_Gen_CFD_SteadyState_gradientCalculation_LEAST_SQUARES
};

enum ads_Step_Gen_CFD_SteadyState_momentumCouplingEnm
{
    ads_Step_Gen_CFD_SteadyState_momentumCoupling_ONE_WAY,
    ads_Step_Gen_CFD_SteadyState_momentumCoupling_TWO_WAY
};

enum ads_Step_Gen_CFD_SteadyState_particleEnergyCouplingEnm
{
    ads_Step_Gen_CFD_SteadyState_particleEnergyCoupling_ONE_WAY,
    ads_Step_Gen_CFD_SteadyState_particleEnergyCoupling_TWO_WAY
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_coupledConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_coupledConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_coupledConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_deformingMeshControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_deformingMeshControls_child,
    ads_Step_Gen_CFD_SteadyState_deformingMeshControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_electricPotentialConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_electricPotentialConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_electricPotentialConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_energyConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_energyConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_energyConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_momentumConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_momentumConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_momentumConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_ppeConvergenceControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_ppeConvergenceControls_child,
    ads_Step_Gen_CFD_SteadyState_ppeConvergenceControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_radiationUpdateFrequencyControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_radiationUpdateFrequencyControls_child,
    ads_Step_Gen_CFD_SteadyState_radiationUpdateFrequencyControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_speciesConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_speciesConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_speciesConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_transportConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_transportConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_transportConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_turbulenceConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_turbulenceConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_turbulenceConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_SteadyState_vofConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_SteadyState_vofConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_SteadyState_vofConvergenceNonLinearControls_parent
};

/** 
Enum with record members. */
enum ads_Step_Gen_CFD_TransientMembersEnm
{
    ads_Step_Gen_CFD_Transient_designSensitivity,
    ads_Step_Gen_CFD_Transient_dsa,
    ads_Step_Gen_CFD_Transient_beginningTime,
    ads_Step_Gen_CFD_Transient_autoUnderRelaxation,
    ads_Step_Gen_CFD_Transient_energyEquation,
    ads_Step_Gen_CFD_Transient_errorThreshold,
    ads_Step_Gen_CFD_Transient_freezeEnergy,
    ads_Step_Gen_CFD_Transient_freezeFlow,
    ads_Step_Gen_CFD_Transient_gradientCalculation,
    ads_Step_Gen_CFD_Transient_advectionTimeWeight,
    ads_Step_Gen_CFD_Transient_bcTimeWeight,
    ads_Step_Gen_CFD_Transient_btdTimeWeight,
    ads_Step_Gen_CFD_Transient_diffusionTimeWeight,
    ads_Step_Gen_CFD_Transient_divergenceTolerance,
    ads_Step_Gen_CFD_Transient_frictionCoefficientRestitution,
    ads_Step_Gen_CFD_Transient_innerIterations,
    ads_Step_Gen_CFD_Transient_momentumCoupling,
    ads_Step_Gen_CFD_Transient_normalCoefficientRestitution,
    ads_Step_Gen_CFD_Transient_particleEnergyCoupling,
    ads_Step_Gen_CFD_Transient_particleParticleCollision,
    ads_Step_Gen_CFD_Transient_solarLoadCalcTime,
    ads_Step_Gen_CFD_Transient_timeIntegrator,
    ads_Step_Gen_CFD_Transient_totalTime
};

enum ads_Step_Gen_CFD_Transient_designSensitivityEnm
{
    ads_Step_Gen_CFD_Transient_designSensitivity_ADJOINT,
    ads_Step_Gen_CFD_Transient_designSensitivity_NONE
};

enum ads_Step_Gen_CFD_Transient_energyEquationEnm
{
    ads_Step_Gen_CFD_Transient_energyEquation_ENTHALPY,
    ads_Step_Gen_CFD_Transient_energyEquation_NOENERGY,
    ads_Step_Gen_CFD_Transient_energyEquation_TEMPERATURE
};

enum ads_Step_Gen_CFD_Transient_gradientCalculationEnm
{
    ads_Step_Gen_CFD_Transient_gradientCalculation_GREEN_GAUSS,
    ads_Step_Gen_CFD_Transient_gradientCalculation_HYBRID_LEAST_SQUARES,
    ads_Step_Gen_CFD_Transient_gradientCalculation_LEAST_SQUARES
};

enum ads_Step_Gen_CFD_Transient_momentumCouplingEnm
{
    ads_Step_Gen_CFD_Transient_momentumCoupling_ONE_WAY,
    ads_Step_Gen_CFD_Transient_momentumCoupling_TWO_WAY
};

enum ads_Step_Gen_CFD_Transient_particleEnergyCouplingEnm
{
    ads_Step_Gen_CFD_Transient_particleEnergyCoupling_ONE_WAY,
    ads_Step_Gen_CFD_Transient_particleEnergyCoupling_TWO_WAY
};

enum ads_Step_Gen_CFD_Transient_timeIntegratorEnm
{
    ads_Step_Gen_CFD_Transient_timeIntegrator_BDF2,
    ads_Step_Gen_CFD_Transient_timeIntegrator_FORWARD_EULER,
    ads_Step_Gen_CFD_Transient_timeIntegrator_THETA
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_coupledConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_coupledConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_coupledConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_deformingMeshControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_deformingMeshControls_child,
    ads_Step_Gen_CFD_Transient_deformingMeshControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_electricPotentialConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_electricPotentialConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_electricPotentialConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_energyConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_energyConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_energyConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_momentumConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_momentumConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_momentumConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_ppeConvergenceControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_ppeConvergenceControls_child,
    ads_Step_Gen_CFD_Transient_ppeConvergenceControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_radiationUpdateFrequencyControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_radiationUpdateFrequencyControls_child,
    ads_Step_Gen_CFD_Transient_radiationUpdateFrequencyControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_speciesConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_speciesConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_speciesConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_transportConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_transportConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_transportConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_turbulenceConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_turbulenceConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_turbulenceConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_Transient_vofConvergenceNonLinearControlsRolesEnm
{
    ads_Step_Gen_CFD_Transient_vofConvergenceNonLinearControls_child,
    ads_Step_Gen_CFD_Transient_vofConvergenceNonLinearControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_coupledEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_coupledEquationSolver_child,
    ads_Step_Gen_CFD_coupledEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_electricPotentialEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_electricPotentialEquationSolver_child,
    ads_Step_Gen_CFD_electricPotentialEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_energyEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_energyEquationSolver_child,
    ads_Step_Gen_CFD_energyEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_expertNumericsControlsRolesEnm
{
    ads_Step_Gen_CFD_expertNumericsControls_child,
    ads_Step_Gen_CFD_expertNumericsControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_momentumEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_momentumEquationSolver_child,
    ads_Step_Gen_CFD_momentumEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_ppeEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_ppeEquationSolver_child,
    ads_Step_Gen_CFD_ppeEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_radiationEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_radiationEquationSolver_child,
    ads_Step_Gen_CFD_radiationEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_radiationModelRolesEnm
{
    ads_Step_Gen_CFD_radiationModel_child,
    ads_Step_Gen_CFD_radiationModel_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_speciesEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_speciesEquationSolver_child,
    ads_Step_Gen_CFD_speciesEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_stoppingCriteriaRolesEnm
{
    ads_Step_Gen_CFD_stoppingCriteria_child,
    ads_Step_Gen_CFD_stoppingCriteria_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_transportEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_transportEquationSolver_child,
    ads_Step_Gen_CFD_transportEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_turbulenceEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_turbulenceEquationSolver_child,
    ads_Step_Gen_CFD_turbulenceEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_turbulenceModelRolesEnm
{
    ads_Step_Gen_CFD_turbulenceModel_child,
    ads_Step_Gen_CFD_turbulenceModel_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_CFD_vOFEquationSolverRolesEnm
{
    ads_Step_Gen_CFD_vOFEquationSolver_child,
    ads_Step_Gen_CFD_vOFEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_HeatTransferSteadyState_energyEquationSolverRolesEnm
{
    ads_Step_Gen_HeatTransferSteadyState_energyEquationSolver_child,
    ads_Step_Gen_HeatTransferSteadyState_energyEquationSolver_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_HeatTransfer_energyEquationSolverRolesEnm
{
    ads_Step_Gen_HeatTransfer_energyEquationSolver_child,
    ads_Step_Gen_HeatTransfer_energyEquationSolver_parent
};

/** 
Enum with record members. */
enum ads_StoppingCriteriaMembersEnm
{
    ads_StoppingCriteria_deviation,
    ads_StoppingCriteria_sampledIterations,
    ads_StoppingCriteria_strategyType
};

enum ads_StoppingCriteria_strategyTypeEnm
{
    ads_StoppingCriteria_strategyType_ERROR,
    ads_StoppingCriteria_strategyType_RESIDUAL
};

/** Enum with association roles. */
enum ads_StoppingCriteria_thresholdRolesEnm
{
    ads_StoppingCriteria_threshold_referent,
    ads_StoppingCriteria_threshold_referrer
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_CFLMembersEnm
{
    ads_TimeIncrementation_Fixed_CFL_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_CFL_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_CFL_checkStableTimeStepSizeInterval,
    ads_TimeIncrementation_Fixed_CFL_initialCFL,
    ads_TimeIncrementation_Fixed_CFL_maximumCFL,
    ads_TimeIncrementation_Fixed_CFL_scaleFactor
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_CFL_CFDMembersEnm
{
    ads_TimeIncrementation_Fixed_CFL_CFD_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_CFL_CFD_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_CFL_CFD_checkStableTimeStepSizeInterval,
    ads_TimeIncrementation_Fixed_CFL_CFD_initialCFL,
    ads_TimeIncrementation_Fixed_CFL_CFD_maximumCFL,
    ads_TimeIncrementation_Fixed_CFL_CFD_scaleFactor,
    ads_TimeIncrementation_Fixed_CFL_CFD_maximumTimeIncrement,
    ads_TimeIncrementation_Fixed_CFL_CFD_pressureSubcyclingInterval
};

/** Enum with record members. */
enum ads_UpdateFrequencyControlsMembersEnm
{
    ads_UpdateFrequencyControls_updateFrequency
};

#endif
