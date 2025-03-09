//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreStepsC_h
#define ads_CoreStepsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Steps of the latest level of form Core */

/** Automatic stabilization control. */
#define ads_AutomaticStepStabilization (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 0))

/** *CONTOUR INTEGRAL without NORMAL, CRACK TIP NODES, XFEM options. */
#define ads_ContourIntegral (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 1))

/** This option offers the evaluation of the J-integral, the Ct-integral, the stress intensity factors, and the T-stress for fracture mechanics studies based on either the conventional FEM or the XFEM. The option also computes the crack propagation direction at initiation when the stress intensity factors are evaluated. */
#define ads_ContourIntegralBase (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 2))

/** Element set containing all elements inside the contour integral domain. */
#define ads_ContourIntegralBase_Elset (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 3))

#define ads_ContourIntegral_CosineData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 4))

#define ads_ContourIntegral_CosineNodeData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 5))

#define ads_ContourIntegral_CosineNodeData_crackFrontNodeSet (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 6))

#define ads_ContourIntegral_CosineNodeData_crackTipNodeSet (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 7))

/** Use this to indicate that the crack tip nodes are specified to form the crack front line. */
#define ads_ContourIntegral_CrackTipNodes (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 8))

#define ads_ContourIntegral_NodeData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 9))

#define ads_ContourIntegral_NodeData_crackFrontNodeSet (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 10))

#define ads_ContourIntegral_NodeData_crackTipNodeSet (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 11))

/** Use this to indicate that the direction normal to the plane of the crack n is specified. */
#define ads_ContourIntegral_Normal (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 12))

/** Use this to indicate that the direction normal to the plane of the crack n is specified with the crack tip nodes to form the crack front line. */
#define ads_ContourIntegral_NormalCrackTipNodes (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 13))

#define ads_ContourIntegral_Normal_cosineData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 14))

#define ads_ContourIntegral_Normal_nodeData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 15))

/** Use this to indicate the type of integration method to use. This setting is applicable only to cracks modeled as an enriched feature (XFEM). */
#define ads_ContourIntegral_XFEM (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 16))

#define ads_ContourIntegral_cosineNodeData (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 17))

#define ads_Controls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 18))

#define ads_ControlsRelay (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 19))

#define ads_ControlsRelay_controls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 20))

#define ads_ControlsRelay_relay (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 21))

#define ads_Controls_ContactPair (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 22))

#define ads_Controls_ContactPair_cpSets (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 23))

#define ads_Controls_ContactPair_dampingCoefficient (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 24))

#define ads_Controls_ContactPair_stepEndDampingFactor (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 25))

#define ads_Controls_ContactPair_tangentFraction (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 26))

#define ads_Controls_ContactPair_zeroDampingClearance (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 27))

#define ads_Controls_ExplicitDynamicsSolver (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 28))

#define ads_Controls_GeneralContact (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 29))

#define ads_Controls_GeneralContact_incrementEndDampingFactor (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 30))

#define ads_Controls_GeneralContact_zeroDampingClearance (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 31))

#define ads_Controls_MassScaling (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 32))

#define ads_Controls_MassScaling_Fixed (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 33))

#define ads_Controls_MassScaling_Variable (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 34))

#define ads_Controls_MassScaling_Variable_massScalingRollingOption (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 35))

#define ads_Controls_MassScaling_dt (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 36))

#define ads_Controls_MassScaling_elements (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 37))

#define ads_Controls_SolutionTechnique (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 38))

#define ads_Controls_Task (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 39))

#define ads_Controls_Task_Termination (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 40))

#define ads_Controls_Task_Termination_criteria (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 41))

#define ads_Controls_TimeIncrement (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 42))

/** For a direct cyclic fatigue analysis, this data type represents a particluar cycle under which the time values are grouped */
#define ads_Cycle (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 43))

/** The collection of cycles */
#define ads_CycleCollection (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 44))

/** Represents a grid of cycles over which a distribution of cycle numbers can be written */
#define ads_CycleGrid (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 45))

/** Represents a grid for a dset that groups a cycle with all of its accompanying time increments */
#define ads_CycleTimeGrid (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 46))

#define ads_Interaction_ContactInterference (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 47))

/** Contact elements for which contact interference is defined. */
#define ads_Interaction_ContactInterference_contactElements (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 48))

/** Contact pair for which contact interference is defined. */
#define ads_Interaction_ContactInterference_contactPair (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 49))

#define ads_Interaction_ContactInterference_cosineOfShiftDirectionVector (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 50))

/** Reference allowable interference. */
#define ads_Interaction_ContactInterference_refInterference (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 51))

#define ads_MassScalingRollingOption (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 52))

#define ads_MassScalingRollingOption_extrudedLength (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 53))

#define ads_MassScalingRollingOption_feedRate (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 54))

#define ads_Model_contactInterferences (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 55))

#define ads_Model_taskControls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 56))

#define ads_SensorCriterion (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 57))

#define ads_SensorCriterion_sensor (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 58))

/** This step is used to anneal a structure by setting the velocites and all appropriate state variables to zero. */
#define ads_Step_Gen_Anneal (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 59))

#define ads_Step_Gen_Anneal_temperature (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 60))

/** This is a Step that consists of a single operation: reaching equilibrium, which is the minimum definition of a Step. The Step_Gen_BaseStateResolution is used with simulations that have an event but do not have a general step before. The Step_Gen_BaseStateResolution may be associated with a state based on initial conditions, or with a zero base state. In general the association StepEvent links (indirectly) an event to its base state. For the cases where there is no general step before the event, the same association simply links to the Step_Gen_BaseStateResolution instance. The Step_Gen_BaseStateResolution does not correspond to any step type in the ABAQUS input file. Base state output is simply the state of the analysis prior to the start of a linear perturbation task. On the old odb the base state was treated as an additional frame on the linear perturbation task itself. However this didn't necessarily make sense if one then tried to interpret all frames for a task as a solution history. */
#define ads_Step_Gen_BaseStateResolution (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 61))

/** This step is used to provide a direct cyclic procedure for nonlinear, non-isothermal quasi-static analysis in Abaqus/Standard. */
#define ads_Step_Gen_DirectCyclic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 62))

/** This step is used to provide a direct cyclic procedure for nonlinear, non-isothermal quasi-static analysis in Abaqus/Standard. */
#define ads_Step_Gen_DirectCyclic_Fatigue (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 63))

#define ads_Step_Gen_DirectCyclic_cycleTimes (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 64))

#define ads_Step_Gen_DirectCyclic_cycleValues (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 65))

#define ads_Step_Gen_DirectCyclic_timePoints (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 66))

/** A dynamic step. */
#define ads_Step_Gen_Dynamic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 67))

/** A dynamic step. */
#define ads_Step_Gen_Dynamic_Explicit (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 68))

/** This option is used to indicate that a dynamic coupled thermal-stress analysis is to be performed using explicit integration. */
#define ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 69))

#define ads_Step_Gen_Dynamic_Explicit_massScalings (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 70))

/** A dynamic step. */
#define ads_Step_Gen_Dynamic_Implicit (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 71))

/** This procedure uses the subspace projection method: explicit integration of the model projected onto the eigenvectors obtained in the last *FREQUENCY step preceding this step. */
#define ads_Step_Gen_Dynamic_SubspaceProjection (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 72))

#define ads_Step_Gen_Electromagnetic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 73))

/** This step is used to predict delamination/debonding growth at the brittle material interfaces in laminated composites and to predict crack growth in bulk brittle materials under cyclic fatigue loading. */
#define ads_Step_Gen_FatigueCrackGrowth (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 74))

#define ads_Step_Gen_FatigueCrackGrowth_cycleTimes (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 75))

#define ads_Step_Gen_FatigueCrackGrowth_cycleValues (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 76))

#define ads_Step_Gen_FatigueCrackGrowth_timePoints (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 77))

/** This step is used to verify that the geostatic stress field is in equilibrium with the applied loads and boundary conditions on the model and to iterate, if needed, to obtain equilibrium. */
#define ads_Step_Gen_Geostatic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 78))

/** A heat transfer step. */
#define ads_Step_Gen_HeatTransfer (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 79))

/** This option is used to control uncoupled heat transfer for steady-state response. */
#define ads_Step_Gen_HeatTransferSteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 80))

#define ads_Step_Gen_Magnetostatic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 81))

/** This step is used to specify an uncoupled transient mass diffusion analysis. */
#define ads_Step_Gen_MassDiffusion (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 82))

/** This step is used to specify an uncoupled steady-state mass diffusion analysis. */
#define ads_Step_Gen_MassDiffusionSteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 83))

/** Step definition for the Unfold procedure. */
#define ads_Step_Gen_OneStepInverse (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 84))

#define ads_Step_Gen_OneStepInverse_nodes (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 85))

/** A Tosca Optimization step. */
#define ads_Step_Gen_Optimization (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 86))

/** This step is used to specify transient (consolidation) analysis of partially or fully saturated fluid-filled porous media. */
#define ads_Step_Gen_Soils (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 87))

/** This step is used to specify steady state response analysis of partially or fully saturated fluid-filled porous media. */
#define ads_Step_Gen_SoilsSteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 88))

/** Potentially non-linear static stress/displacement analysis step. */
#define ads_Step_Gen_Static (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 89))

/** A static stress/displacement analysis that uses the modified RIKS method for proportional loading cases. */
#define ads_Step_Gen_StaticRiks (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 90))

#define ads_Step_Gen_StaticRiks_arcLengths (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 91))

/** Composition to a MeshedField to store the maximum displacement value for nodeXdof pair . Its grid is NodeXComponent to the value. */
#define ads_Step_Gen_StaticRiks_maximumDisplacement (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 92))

#define ads_Step_Gen_Static_fullyPlastic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 93))

/** This option is used to indicate that the step should be analyzed as a steady-state transport analysis. */
#define ads_Step_Gen_SteadyStateTransport (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 94))

#define ads_Step_Gen_SteadyStateTransport_eulerianElements (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 95))

/** This base class step is used to analyze steady state problems where the simulataneous solution of the temperature and stress/displacement fields is necessary. */
#define ads_Step_Gen_TemperatureDisplacement (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 96))

/** This step is used to analyze steady state problems where the simulataneous solution of the temperature and stress/displacement fields is necessary. */
#define ads_Step_Gen_TemperatureDisplacement_SteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 97))

/** This step is used to analyze transient problems where the simulataneous solution of the temperature and stress/displacement fields is necessary. */
#define ads_Step_Gen_TemperatureDisplacement_Transient (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 98))

/** This option is used to analyze problems where the electrical potential and temperaure fields must be solved simultaneously. */
#define ads_Step_Gen_TemperatureElectrical (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 99))

/** This option is used to analyze steady state problems where the electrical potential and temperaure fields must be solved simultaneously. */
#define ads_Step_Gen_TemperatureElectricalSteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 100))

/** This option is used to analyze problems where the electrochemical and temperaure fields must be solved simultaneously. */
#define ads_Step_Gen_TemperatureElectroChemical (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 101))

/** This option is used to analyze steady state problems where the electrochemical and temperaure fields must be solved simultaneously. */
#define ads_Step_Gen_TemperatureElectroChemicalSteadyState (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 102))

/** This step is used to obtain a transient static response in an analysis with time-dependent material behavior (creep, swelling, and viscoelasticity). */
#define ads_Step_Gen_Visco (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 103))

/** Use automatic stabilization if the problem is expected to be unstable due to local instabilities. */
#define ads_Step_Gen_automaticStepStabilization (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 104))

#define ads_Step_Gen_contourIntegrals (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 105))

#define ads_Step_Gen_controls_ContactPair (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 106))

#define ads_Step_Gen_controls_GeneralContact (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 107))

#define ads_Step_Gen_controls_SolutionTechnique (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 108))

#define ads_Step_Gen_explicitDynamicsSolverControls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 109))

#define ads_Step_Gen_stoppingCriterion (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 110))

#define ads_Step_Gen_timeIncrement (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 111))

#define ads_Step_Lin_Electromagnetic (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 112))

/** This is meaningful only for LCP_CONTACT solution technique. */
#define ads_Step_Lin_controls_SolutionTechnique (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 113))

/** The StoppingCriterion plus its relationship to FieldTypePrimary indicate that the Step should be stopped if it exceeds a certain value. We know for sure that this is not the final schema. The most likely generalizations are towards: Introducing a SteeringCriterion type which might allow actions beyond stopping. Instead of simple fieldQuantities, we will probably refer to sensors, but we need to allow sensors to be defined as max(PEEQ), and more general expressions. While we have some idea of these generalizations, we feel we cannot possibly know what is right until we have enough feedback from customers (methods people, not designers). So, the idea is to keep it as simple as possible with the understanding that we will need to evolve it. Part of the idea of using StoppingCriterion rather than SteeringCriterion now is that we could ADD SteeringCriterion later on and then make StoppingCriterion obsolete later on so that schema evolution should be simpler than having more complex schema now that turns out to be wrong. */
#define ads_StoppingCriterion (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 114))

/** The Step is stopped if the associated (scalar) field exceeds this value. */
#define ads_StoppingCriterion_cutoff (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 115))

#define ads_TaskControls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 116))

#define ads_TaskControls_Solution (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 117))

#define ads_TaskControls_Solution_Constraints (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 118))

#define ads_TaskControls_Solution_Field (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 119))

/** Initial value of the time average flux for this step. */
#define ads_TaskControls_Solution_Field_initValTimeAvgFlux (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 120))

/** User-defined average flux. */
#define ads_TaskControls_Solution_Field_userDefAvgFlux (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 121))

#define ads_TaskControls_Solution_LineSearch (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 122))

#define ads_TaskControls_Solution_NoCutBackScaling (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 123))

#define ads_TaskControls_Solution_Reset (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 124))

#define ads_TaskControls_Solution_TypeDirectCyclc (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 125))

#define ads_TaskControls_Solution_VcctLinearScaling (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 126))

#define ads_TaskTerminationCriterion_MAX (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 127))

#define ads_TaskTerminationCriterion_MAX_value (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 128))

#define ads_TaskTerminationCriterion_MIN (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 129))

#define ads_TaskTerminationCriterion_MIN_value (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 130))

#define ads_Task_controls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 131))

/** Controls that might be propogated through tasks. */
#define ads_Task_taskControls (ads_CoreFragmentTypeIndex(ads_CoreStepsFragment, 132))

/** 
Enum with record members. */
enum ads_AutomaticStepStabilizationMembersEnm
{
    ads_AutomaticStepStabilization_dampingDefinition,
    ads_AutomaticStepStabilization_dampingFactor,
    ads_AutomaticStepStabilization_dissipatedEnergyFraction,
    ads_AutomaticStepStabilization_energyRatioTolerance,
    ads_AutomaticStepStabilization_stabilizationType
};

enum ads_AutomaticStepStabilization_dampingDefinitionEnm
{
    ads_AutomaticStepStabilization_dampingDefinition_DEFAULT_ENERGY_FRACTION,
    ads_AutomaticStepStabilization_dampingDefinition_PROPAGATED_FROM_PREVIOUS,
    ads_AutomaticStepStabilization_dampingDefinition_SPECIFIED_DAMPING,
    ads_AutomaticStepStabilization_dampingDefinition_SPECIFIED_ENERGY_FRACTION
};

enum ads_AutomaticStepStabilization_stabilizationTypeEnm
{
    ads_AutomaticStepStabilization_stabilizationType_ADAPTIVE_DAMPING,
    ads_AutomaticStepStabilization_stabilizationType_CONSTANT_DAMPING
};

/** 
Enum with record members. */
enum ads_ContourIntegralMembersEnm
{
    ads_ContourIntegral_CrackName,
    ads_ContourIntegral_Direction,
    ads_ContourIntegral_Frequency,
    ads_ContourIntegral_LengthScalingFactor,
    ads_ContourIntegral_NonPropStressing,
    ads_ContourIntegral_Output,
    ads_ContourIntegral_ResidualStressStep,
    ads_ContourIntegral_RingRadius,
    ads_ContourIntegral_Ringzone,
    ads_ContourIntegral_Smooth,
    ads_ContourIntegral_SurfaceNormal,
    ads_ContourIntegral_Symm,
    ads_ContourIntegral_Type,
    ads_ContourIntegral_nContours
};

enum ads_ContourIntegral_DirectionEnm
{
    ads_ContourIntegral_Direction_KII0,
    ads_ContourIntegral_Direction_MERR,
    ads_ContourIntegral_Direction_MTS
};

enum ads_ContourIntegral_OutputEnm
{
    ads_ContourIntegral_Output_BOTH,
    ads_ContourIntegral_Output_FILE,
    ads_ContourIntegral_Output_NONE
};

enum ads_ContourIntegral_SurfaceNormalEnm
{
    ads_ContourIntegral_SurfaceNormal_BOTH,
    ads_ContourIntegral_SurfaceNormal_CRACK,
    ads_ContourIntegral_SurfaceNormal_FREE,
    ads_ContourIntegral_SurfaceNormal_NONE
};

enum ads_ContourIntegral_TypeEnm
{
    ads_ContourIntegral_Type_C,
    ads_ContourIntegral_Type_J,
    ads_ContourIntegral_Type_K_FACTORS,
    ads_ContourIntegral_Type_T_STRESS
};

/** 
Enum with record members. */
enum ads_ContourIntegralBaseMembersEnm
{
    ads_ContourIntegralBase_CrackName,
    ads_ContourIntegralBase_Direction,
    ads_ContourIntegralBase_Frequency,
    ads_ContourIntegralBase_LengthScalingFactor,
    ads_ContourIntegralBase_NonPropStressing,
    ads_ContourIntegralBase_Output,
    ads_ContourIntegralBase_ResidualStressStep,
    ads_ContourIntegralBase_RingRadius,
    ads_ContourIntegralBase_Ringzone,
    ads_ContourIntegralBase_Smooth,
    ads_ContourIntegralBase_SurfaceNormal,
    ads_ContourIntegralBase_Symm,
    ads_ContourIntegralBase_Type,
    ads_ContourIntegralBase_nContours
};

enum ads_ContourIntegralBase_DirectionEnm
{
    ads_ContourIntegralBase_Direction_KII0,
    ads_ContourIntegralBase_Direction_MERR,
    ads_ContourIntegralBase_Direction_MTS
};

enum ads_ContourIntegralBase_OutputEnm
{
    ads_ContourIntegralBase_Output_BOTH,
    ads_ContourIntegralBase_Output_FILE,
    ads_ContourIntegralBase_Output_NONE
};

enum ads_ContourIntegralBase_SurfaceNormalEnm
{
    ads_ContourIntegralBase_SurfaceNormal_BOTH,
    ads_ContourIntegralBase_SurfaceNormal_CRACK,
    ads_ContourIntegralBase_SurfaceNormal_FREE,
    ads_ContourIntegralBase_SurfaceNormal_NONE
};

enum ads_ContourIntegralBase_TypeEnm
{
    ads_ContourIntegralBase_Type_C,
    ads_ContourIntegralBase_Type_J,
    ads_ContourIntegralBase_Type_K_FACTORS,
    ads_ContourIntegralBase_Type_T_STRESS
};

/** 
Enum with association roles. */
enum ads_ContourIntegralBase_ElsetRolesEnm
{
    ads_ContourIntegralBase_Elset_referent,
    ads_ContourIntegralBase_Elset_referrer
};

/** Enum with record members. */
enum ads_ContourIntegral_CosineDataMembersEnm
{
    ads_ContourIntegral_CosineData_xCosine,
    ads_ContourIntegral_CosineData_yCosine,
    ads_ContourIntegral_CosineData_zCosine
};

/** Enum with record members. */
enum ads_ContourIntegral_CosineNodeDataMembersEnm
{
    ads_ContourIntegral_CosineNodeData_xCosine,
    ads_ContourIntegral_CosineNodeData_yCosine,
    ads_ContourIntegral_CosineNodeData_zCosine
};

/** Enum with association roles. */
enum ads_ContourIntegral_CosineNodeData_crackFrontNodeSetRolesEnm
{
    ads_ContourIntegral_CosineNodeData_crackFrontNodeSet_referent,
    ads_ContourIntegral_CosineNodeData_crackFrontNodeSet_referrer
};

/** Enum with association roles. */
enum ads_ContourIntegral_CosineNodeData_crackTipNodeSetRolesEnm
{
    ads_ContourIntegral_CosineNodeData_crackTipNodeSet_referent,
    ads_ContourIntegral_CosineNodeData_crackTipNodeSet_referrer
};

/** 
Enum with record members. */
enum ads_ContourIntegral_CrackTipNodesMembersEnm
{
    ads_ContourIntegral_CrackTipNodes_CrackName,
    ads_ContourIntegral_CrackTipNodes_Direction,
    ads_ContourIntegral_CrackTipNodes_Frequency,
    ads_ContourIntegral_CrackTipNodes_LengthScalingFactor,
    ads_ContourIntegral_CrackTipNodes_NonPropStressing,
    ads_ContourIntegral_CrackTipNodes_Output,
    ads_ContourIntegral_CrackTipNodes_ResidualStressStep,
    ads_ContourIntegral_CrackTipNodes_RingRadius,
    ads_ContourIntegral_CrackTipNodes_Ringzone,
    ads_ContourIntegral_CrackTipNodes_Smooth,
    ads_ContourIntegral_CrackTipNodes_SurfaceNormal,
    ads_ContourIntegral_CrackTipNodes_Symm,
    ads_ContourIntegral_CrackTipNodes_Type,
    ads_ContourIntegral_CrackTipNodes_nContours
};

enum ads_ContourIntegral_CrackTipNodes_DirectionEnm
{
    ads_ContourIntegral_CrackTipNodes_Direction_KII0,
    ads_ContourIntegral_CrackTipNodes_Direction_MERR,
    ads_ContourIntegral_CrackTipNodes_Direction_MTS
};

enum ads_ContourIntegral_CrackTipNodes_OutputEnm
{
    ads_ContourIntegral_CrackTipNodes_Output_BOTH,
    ads_ContourIntegral_CrackTipNodes_Output_FILE,
    ads_ContourIntegral_CrackTipNodes_Output_NONE
};

enum ads_ContourIntegral_CrackTipNodes_SurfaceNormalEnm
{
    ads_ContourIntegral_CrackTipNodes_SurfaceNormal_BOTH,
    ads_ContourIntegral_CrackTipNodes_SurfaceNormal_CRACK,
    ads_ContourIntegral_CrackTipNodes_SurfaceNormal_FREE,
    ads_ContourIntegral_CrackTipNodes_SurfaceNormal_NONE
};

enum ads_ContourIntegral_CrackTipNodes_TypeEnm
{
    ads_ContourIntegral_CrackTipNodes_Type_C,
    ads_ContourIntegral_CrackTipNodes_Type_J,
    ads_ContourIntegral_CrackTipNodes_Type_K_FACTORS,
    ads_ContourIntegral_CrackTipNodes_Type_T_STRESS
};

/** Enum with association roles. */
enum ads_ContourIntegral_NodeData_crackFrontNodeSetRolesEnm
{
    ads_ContourIntegral_NodeData_crackFrontNodeSet_referent,
    ads_ContourIntegral_NodeData_crackFrontNodeSet_referrer
};

/** Enum with association roles. */
enum ads_ContourIntegral_NodeData_crackTipNodeSetRolesEnm
{
    ads_ContourIntegral_NodeData_crackTipNodeSet_referent,
    ads_ContourIntegral_NodeData_crackTipNodeSet_referrer
};

/** 
Enum with record members. */
enum ads_ContourIntegral_NormalMembersEnm
{
    ads_ContourIntegral_Normal_CrackName,
    ads_ContourIntegral_Normal_Direction,
    ads_ContourIntegral_Normal_Frequency,
    ads_ContourIntegral_Normal_LengthScalingFactor,
    ads_ContourIntegral_Normal_NonPropStressing,
    ads_ContourIntegral_Normal_Output,
    ads_ContourIntegral_Normal_ResidualStressStep,
    ads_ContourIntegral_Normal_RingRadius,
    ads_ContourIntegral_Normal_Ringzone,
    ads_ContourIntegral_Normal_Smooth,
    ads_ContourIntegral_Normal_SurfaceNormal,
    ads_ContourIntegral_Normal_Symm,
    ads_ContourIntegral_Normal_Type,
    ads_ContourIntegral_Normal_nContours
};

enum ads_ContourIntegral_Normal_DirectionEnm
{
    ads_ContourIntegral_Normal_Direction_KII0,
    ads_ContourIntegral_Normal_Direction_MERR,
    ads_ContourIntegral_Normal_Direction_MTS
};

enum ads_ContourIntegral_Normal_OutputEnm
{
    ads_ContourIntegral_Normal_Output_BOTH,
    ads_ContourIntegral_Normal_Output_FILE,
    ads_ContourIntegral_Normal_Output_NONE
};

enum ads_ContourIntegral_Normal_SurfaceNormalEnm
{
    ads_ContourIntegral_Normal_SurfaceNormal_BOTH,
    ads_ContourIntegral_Normal_SurfaceNormal_CRACK,
    ads_ContourIntegral_Normal_SurfaceNormal_FREE,
    ads_ContourIntegral_Normal_SurfaceNormal_NONE
};

enum ads_ContourIntegral_Normal_TypeEnm
{
    ads_ContourIntegral_Normal_Type_C,
    ads_ContourIntegral_Normal_Type_J,
    ads_ContourIntegral_Normal_Type_K_FACTORS,
    ads_ContourIntegral_Normal_Type_T_STRESS
};

/** 
Enum with record members. */
enum ads_ContourIntegral_NormalCrackTipNodesMembersEnm
{
    ads_ContourIntegral_NormalCrackTipNodes_CrackName,
    ads_ContourIntegral_NormalCrackTipNodes_Direction,
    ads_ContourIntegral_NormalCrackTipNodes_Frequency,
    ads_ContourIntegral_NormalCrackTipNodes_LengthScalingFactor,
    ads_ContourIntegral_NormalCrackTipNodes_NonPropStressing,
    ads_ContourIntegral_NormalCrackTipNodes_Output,
    ads_ContourIntegral_NormalCrackTipNodes_ResidualStressStep,
    ads_ContourIntegral_NormalCrackTipNodes_RingRadius,
    ads_ContourIntegral_NormalCrackTipNodes_Ringzone,
    ads_ContourIntegral_NormalCrackTipNodes_Smooth,
    ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormal,
    ads_ContourIntegral_NormalCrackTipNodes_Symm,
    ads_ContourIntegral_NormalCrackTipNodes_Type,
    ads_ContourIntegral_NormalCrackTipNodes_nContours
};

enum ads_ContourIntegral_NormalCrackTipNodes_DirectionEnm
{
    ads_ContourIntegral_NormalCrackTipNodes_Direction_KII0,
    ads_ContourIntegral_NormalCrackTipNodes_Direction_MERR,
    ads_ContourIntegral_NormalCrackTipNodes_Direction_MTS
};

enum ads_ContourIntegral_NormalCrackTipNodes_OutputEnm
{
    ads_ContourIntegral_NormalCrackTipNodes_Output_BOTH,
    ads_ContourIntegral_NormalCrackTipNodes_Output_FILE,
    ads_ContourIntegral_NormalCrackTipNodes_Output_NONE
};

enum ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormalEnm
{
    ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormal_BOTH,
    ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormal_CRACK,
    ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormal_FREE,
    ads_ContourIntegral_NormalCrackTipNodes_SurfaceNormal_NONE
};

enum ads_ContourIntegral_NormalCrackTipNodes_TypeEnm
{
    ads_ContourIntegral_NormalCrackTipNodes_Type_C,
    ads_ContourIntegral_NormalCrackTipNodes_Type_J,
    ads_ContourIntegral_NormalCrackTipNodes_Type_K_FACTORS,
    ads_ContourIntegral_NormalCrackTipNodes_Type_T_STRESS
};

/** Enum with association roles. */
enum ads_ContourIntegral_Normal_cosineDataRolesEnm
{
    ads_ContourIntegral_Normal_cosineData_child,
    ads_ContourIntegral_Normal_cosineData_parent
};

/** Enum with association roles. */
enum ads_ContourIntegral_Normal_nodeDataRolesEnm
{
    ads_ContourIntegral_Normal_nodeData_child,
    ads_ContourIntegral_Normal_nodeData_parent
};

/** 
Enum with record members. */
enum ads_ContourIntegral_XFEMMembersEnm
{
    ads_ContourIntegral_XFEM_CrackName,
    ads_ContourIntegral_XFEM_Direction,
    ads_ContourIntegral_XFEM_Frequency,
    ads_ContourIntegral_XFEM_LengthScalingFactor,
    ads_ContourIntegral_XFEM_NonPropStressing,
    ads_ContourIntegral_XFEM_Output,
    ads_ContourIntegral_XFEM_ResidualStressStep,
    ads_ContourIntegral_XFEM_RingRadius,
    ads_ContourIntegral_XFEM_Ringzone,
    ads_ContourIntegral_XFEM_Smooth,
    ads_ContourIntegral_XFEM_SurfaceNormal,
    ads_ContourIntegral_XFEM_Symm,
    ads_ContourIntegral_XFEM_Type,
    ads_ContourIntegral_XFEM_nContours,
    ads_ContourIntegral_XFEM_Method
};

enum ads_ContourIntegral_XFEM_DirectionEnm
{
    ads_ContourIntegral_XFEM_Direction_KII0,
    ads_ContourIntegral_XFEM_Direction_MERR,
    ads_ContourIntegral_XFEM_Direction_MTS
};

enum ads_ContourIntegral_XFEM_OutputEnm
{
    ads_ContourIntegral_XFEM_Output_BOTH,
    ads_ContourIntegral_XFEM_Output_FILE,
    ads_ContourIntegral_XFEM_Output_NONE
};

enum ads_ContourIntegral_XFEM_SurfaceNormalEnm
{
    ads_ContourIntegral_XFEM_SurfaceNormal_BOTH,
    ads_ContourIntegral_XFEM_SurfaceNormal_CRACK,
    ads_ContourIntegral_XFEM_SurfaceNormal_FREE,
    ads_ContourIntegral_XFEM_SurfaceNormal_NONE
};

enum ads_ContourIntegral_XFEM_TypeEnm
{
    ads_ContourIntegral_XFEM_Type_C,
    ads_ContourIntegral_XFEM_Type_J,
    ads_ContourIntegral_XFEM_Type_K_FACTORS,
    ads_ContourIntegral_XFEM_Type_T_STRESS
};

enum ads_ContourIntegral_XFEM_MethodEnm
{
    ads_ContourIntegral_XFEM_Method_DOMAIN,
    ads_ContourIntegral_XFEM_Method_LINE
};

/** Enum with association roles. */
enum ads_ContourIntegral_cosineNodeDataRolesEnm
{
    ads_ContourIntegral_cosineNodeData_child,
    ads_ContourIntegral_cosineNodeData_parent
};

/** Enum with record members. */
enum ads_ControlsRelayMembersEnm
{
    ads_ControlsRelay_autoPropogated
};

/** Enum with association roles. */
enum ads_ControlsRelay_controlsRolesEnm
{
    ads_ControlsRelay_controls_referent,
    ads_ControlsRelay_controls_referrer
};

/** Enum with association roles. */
enum ads_ControlsRelay_relayRolesEnm
{
    ads_ControlsRelay_relay_referent,
    ads_ControlsRelay_relay_referrer
};

/** Enum with record members. */
enum ads_Controls_ContactPairMembersEnm
{
    ads_Controls_ContactPair_reset,
    ads_Controls_ContactPair_stabilizeMethod,
    ads_Controls_ContactPair_stabilizeScaleFactors
};

enum ads_Controls_ContactPair_stabilizeMethodEnm
{
    ads_Controls_ContactPair_stabilizeMethod_AUTO_COMPUTE,
    ads_Controls_ContactPair_stabilizeMethod_AUTO_COMPUTE_WITH_ADAPTIVE_SCALE,
    ads_Controls_ContactPair_stabilizeMethod_AUTO_COMPUTE_WITH_SCALE,
    ads_Controls_ContactPair_stabilizeMethod_NONE,
    ads_Controls_ContactPair_stabilizeMethod_USER_DEFINED,
    ads_Controls_ContactPair_stabilizeMethod_USER_DEFINED_WITH_ADAPTIVE_SCALE
};

/** Enum with association roles. */
enum ads_Controls_ContactPair_cpSetsRolesEnm
{
    ads_Controls_ContactPair_cpSets_referent,
    ads_Controls_ContactPair_cpSets_referrer
};

/** Enum with association roles. */
enum ads_Controls_ContactPair_dampingCoefficientRolesEnm
{
    ads_Controls_ContactPair_dampingCoefficient_child,
    ads_Controls_ContactPair_dampingCoefficient_parent
};

/** Enum with association roles. */
enum ads_Controls_ContactPair_stepEndDampingFactorRolesEnm
{
    ads_Controls_ContactPair_stepEndDampingFactor_child,
    ads_Controls_ContactPair_stepEndDampingFactor_parent
};

/** Enum with association roles. */
enum ads_Controls_ContactPair_tangentFractionRolesEnm
{
    ads_Controls_ContactPair_tangentFraction_child,
    ads_Controls_ContactPair_tangentFraction_parent
};

/** Enum with association roles. */
enum ads_Controls_ContactPair_zeroDampingClearanceRolesEnm
{
    ads_Controls_ContactPair_zeroDampingClearance_child,
    ads_Controls_ContactPair_zeroDampingClearance_parent
};

/** Enum with record members. */
enum ads_Controls_ExplicitDynamicsSolverMembersEnm
{
    ads_Controls_ExplicitDynamicsSolver_linearBulkViscosityCoeff,
    ads_Controls_ExplicitDynamicsSolver_quadraticBulkViscosityCoeff
};

/** Enum with record members. */
enum ads_Controls_GeneralContactMembersEnm
{
    ads_Controls_GeneralContact_reset,
    ads_Controls_GeneralContact_stabilizeMethod,
    ads_Controls_GeneralContact_stabilizeScaleFactors
};

enum ads_Controls_GeneralContact_stabilizeMethodEnm
{
    ads_Controls_GeneralContact_stabilizeMethod_AUTO_COMPUTE,
    ads_Controls_GeneralContact_stabilizeMethod_AUTO_COMPUTE_WITH_ADAPTIVE_SCALE,
    ads_Controls_GeneralContact_stabilizeMethod_AUTO_COMPUTE_WITH_SCALE
};

/** Enum with association roles. */
enum ads_Controls_GeneralContact_incrementEndDampingFactorRolesEnm
{
    ads_Controls_GeneralContact_incrementEndDampingFactor_child,
    ads_Controls_GeneralContact_incrementEndDampingFactor_parent
};

/** Enum with association roles. */
enum ads_Controls_GeneralContact_zeroDampingClearanceRolesEnm
{
    ads_Controls_GeneralContact_zeroDampingClearance_child,
    ads_Controls_GeneralContact_zeroDampingClearance_parent
};

/** Enum with record members. */
enum ads_Controls_MassScalingMembersEnm
{
    ads_Controls_MassScaling_type
};

enum ads_Controls_MassScaling_typeEnm
{
    ads_Controls_MassScaling_type_BELOW_MIN,
    ads_Controls_MassScaling_type_NULL,
    ads_Controls_MassScaling_type_ROLLING,
    ads_Controls_MassScaling_type_SET_EQUAL_DT,
    ads_Controls_MassScaling_type_UNIFORM
};

/** Enum with record members. */
enum ads_Controls_MassScaling_FixedMembersEnm
{
    ads_Controls_MassScaling_Fixed_type,
    ads_Controls_MassScaling_Fixed_factor
};

enum ads_Controls_MassScaling_Fixed_typeEnm
{
    ads_Controls_MassScaling_Fixed_type_BELOW_MIN,
    ads_Controls_MassScaling_Fixed_type_NULL,
    ads_Controls_MassScaling_Fixed_type_ROLLING,
    ads_Controls_MassScaling_Fixed_type_SET_EQUAL_DT,
    ads_Controls_MassScaling_Fixed_type_UNIFORM
};

/** Enum with record members. */
enum ads_Controls_MassScaling_VariableMembersEnm
{
    ads_Controls_MassScaling_Variable_type,
    ads_Controls_MassScaling_Variable_frequency,
    ads_Controls_MassScaling_Variable_numberInterval
};

enum ads_Controls_MassScaling_Variable_typeEnm
{
    ads_Controls_MassScaling_Variable_type_BELOW_MIN,
    ads_Controls_MassScaling_Variable_type_NULL,
    ads_Controls_MassScaling_Variable_type_ROLLING,
    ads_Controls_MassScaling_Variable_type_SET_EQUAL_DT,
    ads_Controls_MassScaling_Variable_type_UNIFORM
};

/** Enum with association roles. */
enum ads_Controls_MassScaling_Variable_massScalingRollingOptionRolesEnm
{
    ads_Controls_MassScaling_Variable_massScalingRollingOption_child,
    ads_Controls_MassScaling_Variable_massScalingRollingOption_parent
};

/** Enum with association roles. */
enum ads_Controls_MassScaling_dtRolesEnm
{
    ads_Controls_MassScaling_dt_child,
    ads_Controls_MassScaling_dt_parent
};

/** Enum with association roles. */
enum ads_Controls_MassScaling_elementsRolesEnm
{
    ads_Controls_MassScaling_elements_referent,
    ads_Controls_MassScaling_elements_referrer
};

/** Enum with record members. */
enum ads_Controls_SolutionTechniqueMembersEnm
{
    ads_Controls_SolutionTechnique_reformKernel,
    ads_Controls_SolutionTechnique_type
};

enum ads_Controls_SolutionTechnique_typeEnm
{
    ads_Controls_SolutionTechnique_type_LCP_CONTACT,
    ads_Controls_SolutionTechnique_type_QUASI_NEWTON,
    ads_Controls_SolutionTechnique_type_SEPARATED
};

/** Enum with record members. */
enum ads_Controls_Task_TerminationMembersEnm
{
    ads_Controls_Task_Termination_terminateAnalysis
};

/** Enum with association roles. */
enum ads_Controls_Task_Termination_criteriaRolesEnm
{
    ads_Controls_Task_Termination_criteria_child,
    ads_Controls_Task_Termination_criteria_parent
};

/** Enum with record members. */
enum ads_Controls_TimeIncrementMembersEnm
{
    ads_Controls_TimeIncrement_cutbackDiverging,
    ads_Controls_TimeIncrement_cutbackExcessDistort,
    ads_Controls_TimeIncrement_cutbackExcessEquilIter,
    ads_Controls_TimeIncrement_cutbackExcessSDI,
    ads_Controls_TimeIncrement_cutbackLogRate,
    ads_Controls_TimeIncrement_cutbackTimeIntegAcc,
    ads_Controls_TimeIncrement_fractionStabLimit,
    ads_Controls_TimeIncrement_increaseFactorTimePointApprox,
    ads_Controls_TimeIncrement_increaseFactorTimePointExact,
    ads_Controls_TimeIncrement_increaseFewEquilIter,
    ads_Controls_TimeIncrement_increaseTimeAccuracy,
    ads_Controls_TimeIncrement_maxNumberContactAugments,
    ads_Controls_TimeIncrement_maxNumberCutbacksForInc,
    ads_Controls_TimeIncrement_maxNumberEquilIter,
    ads_Controls_TimeIncrement_maxNumberEquilIterIncreasedInc,
    ads_Controls_TimeIncrement_maxNumberSDIWithoutConvert,
    ads_Controls_TimeIncrement_maxNumberSDIwithConvert,
    ads_Controls_TimeIncrement_maxRatioIncToStabLimit,
    ads_Controls_TimeIncrement_maxSDIIncreaseWithConvert,
    ads_Controls_TimeIncrement_maxSDIIncreaseWithoutConvert,
    ads_Controls_TimeIncrement_maxTimeIncIncrease,
    ads_Controls_TimeIncrement_maxTimeIncIncreaseDiffusion,
    ads_Controls_TimeIncrement_maxTimeIncIncreaseDynStress,
    ads_Controls_TimeIncrement_minIncNoCutbacksIncreasedInc,
    ads_Controls_TimeIncrement_minRatioForExtrapolation,
    ads_Controls_TimeIncrement_minRatioNextToCurrentInc,
    ads_Controls_TimeIncrement_numberEquilIter,
    ads_Controls_TimeIncrement_numberEquilIterAltResid,
    ads_Controls_TimeIncrement_numberEquilIterLogRate,
    ads_Controls_TimeIncrement_numberEquilIterReducedInc,
    ads_Controls_TimeIncrement_ratioAveTimeAccuracy
};

/** 
Enum with grid dimensions. */
enum ads_CycleGridDimensionsEnm
{
    ads_CycleGrid_cycle
};

/** 
Enum with grid dimensions. */
enum ads_CycleTimeGridDimensionsEnm
{
    ads_CycleTimeGrid_cycle,
    ads_CycleTimeGrid_time
};

/** Enum with record members. */
enum ads_Interaction_ContactInterferenceMembersEnm
{
    ads_Interaction_ContactInterference_autoPropagate,
    ads_Interaction_ContactInterference_shrink
};

/** 
Enum with association roles. */
enum ads_Interaction_ContactInterference_contactElementsRolesEnm
{
    ads_Interaction_ContactInterference_contactElements_referent,
    ads_Interaction_ContactInterference_contactElements_referrer
};

/** 
Enum with association roles. */
enum ads_Interaction_ContactInterference_contactPairRolesEnm
{
    ads_Interaction_ContactInterference_contactPair_referent,
    ads_Interaction_ContactInterference_contactPair_referrer
};

/** Enum with association roles. */
enum ads_Interaction_ContactInterference_cosineOfShiftDirectionVectorRolesEnm
{
    ads_Interaction_ContactInterference_cosineOfShiftDirectionVector_child,
    ads_Interaction_ContactInterference_cosineOfShiftDirectionVector_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_ContactInterference_refInterferenceRolesEnm
{
    ads_Interaction_ContactInterference_refInterference_child,
    ads_Interaction_ContactInterference_refInterference_parent
};

/** Enum with record members. */
enum ads_MassScalingRollingOptionMembersEnm
{
    ads_MassScalingRollingOption_crossSectionNodes
};

/** Enum with association roles. */
enum ads_MassScalingRollingOption_extrudedLengthRolesEnm
{
    ads_MassScalingRollingOption_extrudedLength_child,
    ads_MassScalingRollingOption_extrudedLength_parent
};

/** Enum with association roles. */
enum ads_MassScalingRollingOption_feedRateRolesEnm
{
    ads_MassScalingRollingOption_feedRate_child,
    ads_MassScalingRollingOption_feedRate_parent
};

/** Enum with association roles. */
enum ads_Model_contactInterferencesRolesEnm
{
    ads_Model_contactInterferences_child,
    ads_Model_contactInterferences_parent
};

/** Enum with association roles. */
enum ads_Model_taskControlsRolesEnm
{
    ads_Model_taskControls_child,
    ads_Model_taskControls_parent
};

/** Enum with association roles. */
enum ads_SensorCriterion_sensorRolesEnm
{
    ads_SensorCriterion_sensor_referent,
    ads_SensorCriterion_sensor_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_AnnealMembersEnm
{
    ads_Step_Gen_Anneal_designSensitivity,
    ads_Step_Gen_Anneal_dsa,
    ads_Step_Gen_Anneal_beginningTime
};

enum ads_Step_Gen_Anneal_designSensitivityEnm
{
    ads_Step_Gen_Anneal_designSensitivity_ADJOINT,
    ads_Step_Gen_Anneal_designSensitivity_NONE
};

/** Enum with association roles. */
enum ads_Step_Gen_Anneal_temperatureRolesEnm
{
    ads_Step_Gen_Anneal_temperature_child,
    ads_Step_Gen_Anneal_temperature_parent
};

/** 
Enum with record members. */
enum ads_Step_Gen_BaseStateResolutionMembersEnm
{
    ads_Step_Gen_BaseStateResolution_designSensitivity,
    ads_Step_Gen_BaseStateResolution_dsa,
    ads_Step_Gen_BaseStateResolution_beginningTime
};

enum ads_Step_Gen_BaseStateResolution_designSensitivityEnm
{
    ads_Step_Gen_BaseStateResolution_designSensitivity_ADJOINT,
    ads_Step_Gen_BaseStateResolution_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_DirectCyclicMembersEnm
{
    ads_Step_Gen_DirectCyclic_designSensitivity,
    ads_Step_Gen_DirectCyclic_dsa,
    ads_Step_Gen_DirectCyclic_beginningTime,
    ads_Step_Gen_DirectCyclic_continue,
    ads_Step_Gen_DirectCyclic_increment,
    ads_Step_Gen_DirectCyclic_initialNumber,
    ads_Step_Gen_DirectCyclic_matrixSymmetry,
    ads_Step_Gen_DirectCyclic_maxNumberIterations,
    ads_Step_Gen_DirectCyclic_maximumNumber,
    ads_Step_Gen_DirectCyclic_totalTime
};

enum ads_Step_Gen_DirectCyclic_designSensitivityEnm
{
    ads_Step_Gen_DirectCyclic_designSensitivity_ADJOINT,
    ads_Step_Gen_DirectCyclic_designSensitivity_NONE
};

enum ads_Step_Gen_DirectCyclic_matrixSymmetryEnm
{
    ads_Step_Gen_DirectCyclic_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_DirectCyclic_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_DirectCyclic_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_DirectCyclic_FatigueMembersEnm
{
    ads_Step_Gen_DirectCyclic_Fatigue_designSensitivity,
    ads_Step_Gen_DirectCyclic_Fatigue_dsa,
    ads_Step_Gen_DirectCyclic_Fatigue_beginningTime,
    ads_Step_Gen_DirectCyclic_Fatigue_continue,
    ads_Step_Gen_DirectCyclic_Fatigue_increment,
    ads_Step_Gen_DirectCyclic_Fatigue_initialNumber,
    ads_Step_Gen_DirectCyclic_Fatigue_matrixSymmetry,
    ads_Step_Gen_DirectCyclic_Fatigue_maxNumberIterations,
    ads_Step_Gen_DirectCyclic_Fatigue_maximumNumber,
    ads_Step_Gen_DirectCyclic_Fatigue_totalTime,
    ads_Step_Gen_DirectCyclic_Fatigue_cylcleIncrementBasedTol,
    ads_Step_Gen_DirectCyclic_Fatigue_damageBasedTol,
    ads_Step_Gen_DirectCyclic_Fatigue_damageExtrapolationTol,
    ads_Step_Gen_DirectCyclic_Fatigue_maxIncrementInNoOfCylcles,
    ads_Step_Gen_DirectCyclic_Fatigue_minIncrementInNoOfCylcles,
    ads_Step_Gen_DirectCyclic_Fatigue_totalNoOfCycles
};

enum ads_Step_Gen_DirectCyclic_Fatigue_designSensitivityEnm
{
    ads_Step_Gen_DirectCyclic_Fatigue_designSensitivity_ADJOINT,
    ads_Step_Gen_DirectCyclic_Fatigue_designSensitivity_NONE
};

enum ads_Step_Gen_DirectCyclic_Fatigue_matrixSymmetryEnm
{
    ads_Step_Gen_DirectCyclic_Fatigue_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_DirectCyclic_Fatigue_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_DirectCyclic_Fatigue_matrixSymmetry_UNSYMMETRIC
};

/** Enum with association roles. */
enum ads_Step_Gen_DirectCyclic_cycleTimesRolesEnm
{
    ads_Step_Gen_DirectCyclic_cycleTimes_child,
    ads_Step_Gen_DirectCyclic_cycleTimes_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_DirectCyclic_cycleValuesRolesEnm
{
    ads_Step_Gen_DirectCyclic_cycleValues_child,
    ads_Step_Gen_DirectCyclic_cycleValues_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_DirectCyclic_timePointsRolesEnm
{
    ads_Step_Gen_DirectCyclic_timePoints_referent,
    ads_Step_Gen_DirectCyclic_timePoints_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_DynamicMembersEnm
{
    ads_Step_Gen_Dynamic_designSensitivity,
    ads_Step_Gen_Dynamic_dsa,
    ads_Step_Gen_Dynamic_beginningTime,
    ads_Step_Gen_Dynamic_adiabatic,
    ads_Step_Gen_Dynamic_applicationType,
    ads_Step_Gen_Dynamic_totalTime
};

enum ads_Step_Gen_Dynamic_designSensitivityEnm
{
    ads_Step_Gen_Dynamic_designSensitivity_ADJOINT,
    ads_Step_Gen_Dynamic_designSensitivity_NONE
};

enum ads_Step_Gen_Dynamic_applicationTypeEnm
{
    ads_Step_Gen_Dynamic_applicationType_MODERATE_DISSIPATION,
    ads_Step_Gen_Dynamic_applicationType_NONE,
    ads_Step_Gen_Dynamic_applicationType_QUASI_STATIC,
    ads_Step_Gen_Dynamic_applicationType_TRANSIENT_FIDELITY
};

/** 
Enum with record members. */
enum ads_Step_Gen_Dynamic_ExplicitMembersEnm
{
    ads_Step_Gen_Dynamic_Explicit_designSensitivity,
    ads_Step_Gen_Dynamic_Explicit_dsa,
    ads_Step_Gen_Dynamic_Explicit_beginningTime,
    ads_Step_Gen_Dynamic_Explicit_adiabatic,
    ads_Step_Gen_Dynamic_Explicit_applicationType,
    ads_Step_Gen_Dynamic_Explicit_totalTime,
    ads_Step_Gen_Dynamic_Explicit_geometricNonlinearity
};

enum ads_Step_Gen_Dynamic_Explicit_designSensitivityEnm
{
    ads_Step_Gen_Dynamic_Explicit_designSensitivity_ADJOINT,
    ads_Step_Gen_Dynamic_Explicit_designSensitivity_NONE
};

enum ads_Step_Gen_Dynamic_Explicit_applicationTypeEnm
{
    ads_Step_Gen_Dynamic_Explicit_applicationType_MODERATE_DISSIPATION,
    ads_Step_Gen_Dynamic_Explicit_applicationType_NONE,
    ads_Step_Gen_Dynamic_Explicit_applicationType_QUASI_STATIC,
    ads_Step_Gen_Dynamic_Explicit_applicationType_TRANSIENT_FIDELITY
};

/** 
Enum with record members. */
enum ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacementMembersEnm
{
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_designSensitivity,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_dsa,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_beginningTime,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_adiabatic,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationType,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_totalTime,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_geometricNonlinearity
};

enum ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_designSensitivityEnm
{
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_designSensitivity_ADJOINT,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_designSensitivity_NONE
};

enum ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationTypeEnm
{
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationType_MODERATE_DISSIPATION,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationType_NONE,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationType_QUASI_STATIC,
    ads_Step_Gen_Dynamic_Explicit_TemperatureDisplacement_applicationType_TRANSIENT_FIDELITY
};

/** Enum with association roles. */
enum ads_Step_Gen_Dynamic_Explicit_massScalingsRolesEnm
{
    ads_Step_Gen_Dynamic_Explicit_massScalings_child,
    ads_Step_Gen_Dynamic_Explicit_massScalings_parent
};

/** 
Enum with record members. */
enum ads_Step_Gen_Dynamic_ImplicitMembersEnm
{
    ads_Step_Gen_Dynamic_Implicit_designSensitivity,
    ads_Step_Gen_Dynamic_Implicit_dsa,
    ads_Step_Gen_Dynamic_Implicit_beginningTime,
    ads_Step_Gen_Dynamic_Implicit_adiabatic,
    ads_Step_Gen_Dynamic_Implicit_applicationType,
    ads_Step_Gen_Dynamic_Implicit_totalTime,
    ads_Step_Gen_Dynamic_Implicit_alpha,
    ads_Step_Gen_Dynamic_Implicit_amplitude,
    ads_Step_Gen_Dynamic_Implicit_beta,
    ads_Step_Gen_Dynamic_Implicit_calculateHalfStepResiduals,
    ads_Step_Gen_Dynamic_Implicit_convertSDI,
    ads_Step_Gen_Dynamic_Implicit_extrapolation,
    ads_Step_Gen_Dynamic_Implicit_gamma,
    ads_Step_Gen_Dynamic_Implicit_geometricNonlinearity,
    ads_Step_Gen_Dynamic_Implicit_impact,
    ads_Step_Gen_Dynamic_Implicit_matrixSymmetry,
    ads_Step_Gen_Dynamic_Implicit_recalculateAccelerations,
    ads_Step_Gen_Dynamic_Implicit_singularMassPolicy,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator
};

enum ads_Step_Gen_Dynamic_Implicit_designSensitivityEnm
{
    ads_Step_Gen_Dynamic_Implicit_designSensitivity_ADJOINT,
    ads_Step_Gen_Dynamic_Implicit_designSensitivity_NONE
};

enum ads_Step_Gen_Dynamic_Implicit_applicationTypeEnm
{
    ads_Step_Gen_Dynamic_Implicit_applicationType_MODERATE_DISSIPATION,
    ads_Step_Gen_Dynamic_Implicit_applicationType_NONE,
    ads_Step_Gen_Dynamic_Implicit_applicationType_QUASI_STATIC,
    ads_Step_Gen_Dynamic_Implicit_applicationType_TRANSIENT_FIDELITY
};

enum ads_Step_Gen_Dynamic_Implicit_amplitudeEnm
{
    ads_Step_Gen_Dynamic_Implicit_amplitude_RAMP,
    ads_Step_Gen_Dynamic_Implicit_amplitude_STEP
};

enum ads_Step_Gen_Dynamic_Implicit_extrapolationEnm
{
    ads_Step_Gen_Dynamic_Implicit_extrapolation_LINEAR,
    ads_Step_Gen_Dynamic_Implicit_extrapolation_NONE,
    ads_Step_Gen_Dynamic_Implicit_extrapolation_PARABOLIC,
    ads_Step_Gen_Dynamic_Implicit_extrapolation_VELOCITY_PARABOLIC
};

enum ads_Step_Gen_Dynamic_Implicit_impactEnm
{
    ads_Step_Gen_Dynamic_Implicit_impact_AVERAGE_TIME,
    ads_Step_Gen_Dynamic_Implicit_impact_CURRENT_TIME,
    ads_Step_Gen_Dynamic_Implicit_impact_NONE,
    ads_Step_Gen_Dynamic_Implicit_impact_SOLVER_DEFAULT
};

enum ads_Step_Gen_Dynamic_Implicit_matrixSymmetryEnm
{
    ads_Step_Gen_Dynamic_Implicit_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Dynamic_Implicit_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Dynamic_Implicit_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_Dynamic_Implicit_singularMassPolicyEnm
{
    ads_Step_Gen_Dynamic_Implicit_singularMassPolicy_ERROR,
    ads_Step_Gen_Dynamic_Implicit_singularMassPolicy_MAKE_ADJUSTMENTS,
    ads_Step_Gen_Dynamic_Implicit_singularMassPolicy_WARNING
};

enum ads_Step_Gen_Dynamic_Implicit_timeIntegratorEnm
{
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_BWE,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_CH_MD,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_CH_TF,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_HHT,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_HHT_MD,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_HHT_TF,
    ads_Step_Gen_Dynamic_Implicit_timeIntegrator_HYBRID
};

/** 
Enum with record members. */
enum ads_Step_Gen_Dynamic_SubspaceProjectionMembersEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_designSensitivity,
    ads_Step_Gen_Dynamic_SubspaceProjection_dsa,
    ads_Step_Gen_Dynamic_SubspaceProjection_beginningTime,
    ads_Step_Gen_Dynamic_SubspaceProjection_adiabatic,
    ads_Step_Gen_Dynamic_SubspaceProjection_applicationType,
    ads_Step_Gen_Dynamic_SubspaceProjection_totalTime,
    ads_Step_Gen_Dynamic_SubspaceProjection_amplitude,
    ads_Step_Gen_Dynamic_SubspaceProjection_calculateHalfStepResiduals,
    ads_Step_Gen_Dynamic_SubspaceProjection_convertSDI,
    ads_Step_Gen_Dynamic_SubspaceProjection_extrapolation,
    ads_Step_Gen_Dynamic_SubspaceProjection_geometricNonlinearity,
    ads_Step_Gen_Dynamic_SubspaceProjection_matrixSymmetry
};

enum ads_Step_Gen_Dynamic_SubspaceProjection_designSensitivityEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_designSensitivity_ADJOINT,
    ads_Step_Gen_Dynamic_SubspaceProjection_designSensitivity_NONE
};

enum ads_Step_Gen_Dynamic_SubspaceProjection_applicationTypeEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_applicationType_MODERATE_DISSIPATION,
    ads_Step_Gen_Dynamic_SubspaceProjection_applicationType_NONE,
    ads_Step_Gen_Dynamic_SubspaceProjection_applicationType_QUASI_STATIC,
    ads_Step_Gen_Dynamic_SubspaceProjection_applicationType_TRANSIENT_FIDELITY
};

enum ads_Step_Gen_Dynamic_SubspaceProjection_amplitudeEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_amplitude_RAMP,
    ads_Step_Gen_Dynamic_SubspaceProjection_amplitude_STEP
};

enum ads_Step_Gen_Dynamic_SubspaceProjection_extrapolationEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_extrapolation_LINEAR,
    ads_Step_Gen_Dynamic_SubspaceProjection_extrapolation_NONE,
    ads_Step_Gen_Dynamic_SubspaceProjection_extrapolation_PARABOLIC
};

enum ads_Step_Gen_Dynamic_SubspaceProjection_matrixSymmetryEnm
{
    ads_Step_Gen_Dynamic_SubspaceProjection_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Dynamic_SubspaceProjection_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Dynamic_SubspaceProjection_matrixSymmetry_UNSYMMETRIC
};

/** Enum with record members. */
enum ads_Step_Gen_ElectromagneticMembersEnm
{
    ads_Step_Gen_Electromagnetic_designSensitivity,
    ads_Step_Gen_Electromagnetic_dsa,
    ads_Step_Gen_Electromagnetic_beginningTime
};

enum ads_Step_Gen_Electromagnetic_designSensitivityEnm
{
    ads_Step_Gen_Electromagnetic_designSensitivity_ADJOINT,
    ads_Step_Gen_Electromagnetic_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_FatigueCrackGrowthMembersEnm
{
    ads_Step_Gen_FatigueCrackGrowth_designSensitivity,
    ads_Step_Gen_FatigueCrackGrowth_dsa,
    ads_Step_Gen_FatigueCrackGrowth_beginningTime,
    ads_Step_Gen_FatigueCrackGrowth_crackLengthRatio,
    ads_Step_Gen_FatigueCrackGrowth_cylcleIncrementBasedTol,
    ads_Step_Gen_FatigueCrackGrowth_damageBasedTol,
    ads_Step_Gen_FatigueCrackGrowth_direct,
    ads_Step_Gen_FatigueCrackGrowth_matrixSymmetry,
    ads_Step_Gen_FatigueCrackGrowth_maxIncrementInNoOfCylcles,
    ads_Step_Gen_FatigueCrackGrowth_minIncrementInNoOfCylcles,
    ads_Step_Gen_FatigueCrackGrowth_totalNoOfCycles,
    ads_Step_Gen_FatigueCrackGrowth_totalTime,
    ads_Step_Gen_FatigueCrackGrowth_type
};

enum ads_Step_Gen_FatigueCrackGrowth_designSensitivityEnm
{
    ads_Step_Gen_FatigueCrackGrowth_designSensitivity_ADJOINT,
    ads_Step_Gen_FatigueCrackGrowth_designSensitivity_NONE
};

enum ads_Step_Gen_FatigueCrackGrowth_matrixSymmetryEnm
{
    ads_Step_Gen_FatigueCrackGrowth_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_FatigueCrackGrowth_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_FatigueCrackGrowth_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_FatigueCrackGrowth_typeEnm
{
    ads_Step_Gen_FatigueCrackGrowth_type_CONSTANT_AMPLITUDE,
    ads_Step_Gen_FatigueCrackGrowth_type_SIMPLIFIED
};

/** Enum with association roles. */
enum ads_Step_Gen_FatigueCrackGrowth_cycleTimesRolesEnm
{
    ads_Step_Gen_FatigueCrackGrowth_cycleTimes_child,
    ads_Step_Gen_FatigueCrackGrowth_cycleTimes_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_FatigueCrackGrowth_cycleValuesRolesEnm
{
    ads_Step_Gen_FatigueCrackGrowth_cycleValues_child,
    ads_Step_Gen_FatigueCrackGrowth_cycleValues_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_FatigueCrackGrowth_timePointsRolesEnm
{
    ads_Step_Gen_FatigueCrackGrowth_timePoints_referent,
    ads_Step_Gen_FatigueCrackGrowth_timePoints_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_GeostaticMembersEnm
{
    ads_Step_Gen_Geostatic_designSensitivity,
    ads_Step_Gen_Geostatic_dsa,
    ads_Step_Gen_Geostatic_beginningTime,
    ads_Step_Gen_Geostatic_amplitude,
    ads_Step_Gen_Geostatic_convertSDI,
    ads_Step_Gen_Geostatic_extrapolation,
    ads_Step_Gen_Geostatic_geometricNonlinearity,
    ads_Step_Gen_Geostatic_matrixSymmetry
};

enum ads_Step_Gen_Geostatic_designSensitivityEnm
{
    ads_Step_Gen_Geostatic_designSensitivity_ADJOINT,
    ads_Step_Gen_Geostatic_designSensitivity_NONE
};

enum ads_Step_Gen_Geostatic_amplitudeEnm
{
    ads_Step_Gen_Geostatic_amplitude_RAMP,
    ads_Step_Gen_Geostatic_amplitude_STEP
};

enum ads_Step_Gen_Geostatic_extrapolationEnm
{
    ads_Step_Gen_Geostatic_extrapolation_LINEAR,
    ads_Step_Gen_Geostatic_extrapolation_NONE,
    ads_Step_Gen_Geostatic_extrapolation_PARABOLIC
};

enum ads_Step_Gen_Geostatic_matrixSymmetryEnm
{
    ads_Step_Gen_Geostatic_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Geostatic_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Geostatic_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_HeatTransferMembersEnm
{
    ads_Step_Gen_HeatTransfer_designSensitivity,
    ads_Step_Gen_HeatTransfer_dsa,
    ads_Step_Gen_HeatTransfer_beginningTime,
    ads_Step_Gen_HeatTransfer_amplitude,
    ads_Step_Gen_HeatTransfer_centering,
    ads_Step_Gen_HeatTransfer_extrapolation,
    ads_Step_Gen_HeatTransfer_matrixSymmetry,
    ads_Step_Gen_HeatTransfer_totalTime,
    ads_Step_Gen_HeatTransfer_type
};

enum ads_Step_Gen_HeatTransfer_designSensitivityEnm
{
    ads_Step_Gen_HeatTransfer_designSensitivity_ADJOINT,
    ads_Step_Gen_HeatTransfer_designSensitivity_NONE
};

enum ads_Step_Gen_HeatTransfer_amplitudeEnm
{
    ads_Step_Gen_HeatTransfer_amplitude_RAMP,
    ads_Step_Gen_HeatTransfer_amplitude_STEP
};

enum ads_Step_Gen_HeatTransfer_centeringEnm
{
    ads_Step_Gen_HeatTransfer_centering_ELEMENT,
    ads_Step_Gen_HeatTransfer_centering_NODE
};

enum ads_Step_Gen_HeatTransfer_extrapolationEnm
{
    ads_Step_Gen_HeatTransfer_extrapolation_LINEAR,
    ads_Step_Gen_HeatTransfer_extrapolation_NONE,
    ads_Step_Gen_HeatTransfer_extrapolation_PARABOLIC
};

enum ads_Step_Gen_HeatTransfer_matrixSymmetryEnm
{
    ads_Step_Gen_HeatTransfer_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_HeatTransfer_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_HeatTransfer_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_HeatTransfer_typeEnm
{
    ads_Step_Gen_HeatTransfer_type_THERMAL_FLOW,
    ads_Step_Gen_HeatTransfer_type_THERMAL_MECHANICAL
};

/** 
Enum with record members. */
enum ads_Step_Gen_HeatTransferSteadyStateMembersEnm
{
    ads_Step_Gen_HeatTransferSteadyState_designSensitivity,
    ads_Step_Gen_HeatTransferSteadyState_dsa,
    ads_Step_Gen_HeatTransferSteadyState_beginningTime,
    ads_Step_Gen_HeatTransferSteadyState_amplitude,
    ads_Step_Gen_HeatTransferSteadyState_centering,
    ads_Step_Gen_HeatTransferSteadyState_extrapolation,
    ads_Step_Gen_HeatTransferSteadyState_matrixSymmetry,
    ads_Step_Gen_HeatTransferSteadyState_totalTime,
    ads_Step_Gen_HeatTransferSteadyState_type
};

enum ads_Step_Gen_HeatTransferSteadyState_designSensitivityEnm
{
    ads_Step_Gen_HeatTransferSteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_HeatTransferSteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_HeatTransferSteadyState_amplitudeEnm
{
    ads_Step_Gen_HeatTransferSteadyState_amplitude_RAMP,
    ads_Step_Gen_HeatTransferSteadyState_amplitude_STEP
};

enum ads_Step_Gen_HeatTransferSteadyState_centeringEnm
{
    ads_Step_Gen_HeatTransferSteadyState_centering_ELEMENT,
    ads_Step_Gen_HeatTransferSteadyState_centering_NODE
};

enum ads_Step_Gen_HeatTransferSteadyState_extrapolationEnm
{
    ads_Step_Gen_HeatTransferSteadyState_extrapolation_LINEAR,
    ads_Step_Gen_HeatTransferSteadyState_extrapolation_NONE,
    ads_Step_Gen_HeatTransferSteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_HeatTransferSteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_HeatTransferSteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_HeatTransferSteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_HeatTransferSteadyState_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_HeatTransferSteadyState_typeEnm
{
    ads_Step_Gen_HeatTransferSteadyState_type_THERMAL_FLOW,
    ads_Step_Gen_HeatTransferSteadyState_type_THERMAL_MECHANICAL
};

/** Enum with record members. */
enum ads_Step_Gen_MagnetostaticMembersEnm
{
    ads_Step_Gen_Magnetostatic_designSensitivity,
    ads_Step_Gen_Magnetostatic_dsa,
    ads_Step_Gen_Magnetostatic_beginningTime
};

enum ads_Step_Gen_Magnetostatic_designSensitivityEnm
{
    ads_Step_Gen_Magnetostatic_designSensitivity_ADJOINT,
    ads_Step_Gen_Magnetostatic_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_MassDiffusionMembersEnm
{
    ads_Step_Gen_MassDiffusion_designSensitivity,
    ads_Step_Gen_MassDiffusion_dsa,
    ads_Step_Gen_MassDiffusion_beginningTime,
    ads_Step_Gen_MassDiffusion_amplitude,
    ads_Step_Gen_MassDiffusion_extrapolation,
    ads_Step_Gen_MassDiffusion_matrixSymmetry,
    ads_Step_Gen_MassDiffusion_totalTime
};

enum ads_Step_Gen_MassDiffusion_designSensitivityEnm
{
    ads_Step_Gen_MassDiffusion_designSensitivity_ADJOINT,
    ads_Step_Gen_MassDiffusion_designSensitivity_NONE
};

enum ads_Step_Gen_MassDiffusion_amplitudeEnm
{
    ads_Step_Gen_MassDiffusion_amplitude_RAMP,
    ads_Step_Gen_MassDiffusion_amplitude_STEP
};

enum ads_Step_Gen_MassDiffusion_extrapolationEnm
{
    ads_Step_Gen_MassDiffusion_extrapolation_LINEAR,
    ads_Step_Gen_MassDiffusion_extrapolation_NONE,
    ads_Step_Gen_MassDiffusion_extrapolation_PARABOLIC
};

enum ads_Step_Gen_MassDiffusion_matrixSymmetryEnm
{
    ads_Step_Gen_MassDiffusion_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_MassDiffusion_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_MassDiffusion_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_MassDiffusionSteadyStateMembersEnm
{
    ads_Step_Gen_MassDiffusionSteadyState_designSensitivity,
    ads_Step_Gen_MassDiffusionSteadyState_dsa,
    ads_Step_Gen_MassDiffusionSteadyState_beginningTime,
    ads_Step_Gen_MassDiffusionSteadyState_amplitude,
    ads_Step_Gen_MassDiffusionSteadyState_extrapolation,
    ads_Step_Gen_MassDiffusionSteadyState_matrixSymmetry,
    ads_Step_Gen_MassDiffusionSteadyState_totalTime
};

enum ads_Step_Gen_MassDiffusionSteadyState_designSensitivityEnm
{
    ads_Step_Gen_MassDiffusionSteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_MassDiffusionSteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_MassDiffusionSteadyState_amplitudeEnm
{
    ads_Step_Gen_MassDiffusionSteadyState_amplitude_RAMP,
    ads_Step_Gen_MassDiffusionSteadyState_amplitude_STEP
};

enum ads_Step_Gen_MassDiffusionSteadyState_extrapolationEnm
{
    ads_Step_Gen_MassDiffusionSteadyState_extrapolation_LINEAR,
    ads_Step_Gen_MassDiffusionSteadyState_extrapolation_NONE,
    ads_Step_Gen_MassDiffusionSteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_MassDiffusionSteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_MassDiffusionSteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_MassDiffusionSteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_MassDiffusionSteadyState_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_OneStepInverseMembersEnm
{
    ads_Step_Gen_OneStepInverse_designSensitivity,
    ads_Step_Gen_OneStepInverse_dsa,
    ads_Step_Gen_OneStepInverse_beginningTime,
    ads_Step_Gen_OneStepInverse_totalTime,
    ads_Step_Gen_OneStepInverse_unfold
};

enum ads_Step_Gen_OneStepInverse_designSensitivityEnm
{
    ads_Step_Gen_OneStepInverse_designSensitivity_ADJOINT,
    ads_Step_Gen_OneStepInverse_designSensitivity_NONE
};

/** Enum with association roles. */
enum ads_Step_Gen_OneStepInverse_nodesRolesEnm
{
    ads_Step_Gen_OneStepInverse_nodes_referent,
    ads_Step_Gen_OneStepInverse_nodes_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_OptimizationMembersEnm
{
    ads_Step_Gen_Optimization_designSensitivity,
    ads_Step_Gen_Optimization_dsa,
    ads_Step_Gen_Optimization_beginningTime
};

enum ads_Step_Gen_Optimization_designSensitivityEnm
{
    ads_Step_Gen_Optimization_designSensitivity_ADJOINT,
    ads_Step_Gen_Optimization_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_SoilsMembersEnm
{
    ads_Step_Gen_Soils_designSensitivity,
    ads_Step_Gen_Soils_dsa,
    ads_Step_Gen_Soils_beginningTime,
    ads_Step_Gen_Soils_amplitude,
    ads_Step_Gen_Soils_convertSDI,
    ads_Step_Gen_Soils_creep,
    ads_Step_Gen_Soils_extrapolation,
    ads_Step_Gen_Soils_geometricNonlinearity,
    ads_Step_Gen_Soils_matrixSymmetry,
    ads_Step_Gen_Soils_totalTime
};

enum ads_Step_Gen_Soils_designSensitivityEnm
{
    ads_Step_Gen_Soils_designSensitivity_ADJOINT,
    ads_Step_Gen_Soils_designSensitivity_NONE
};

enum ads_Step_Gen_Soils_amplitudeEnm
{
    ads_Step_Gen_Soils_amplitude_RAMP,
    ads_Step_Gen_Soils_amplitude_STEP
};

enum ads_Step_Gen_Soils_extrapolationEnm
{
    ads_Step_Gen_Soils_extrapolation_LINEAR,
    ads_Step_Gen_Soils_extrapolation_NONE,
    ads_Step_Gen_Soils_extrapolation_PARABOLIC
};

enum ads_Step_Gen_Soils_matrixSymmetryEnm
{
    ads_Step_Gen_Soils_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Soils_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Soils_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_SoilsSteadyStateMembersEnm
{
    ads_Step_Gen_SoilsSteadyState_designSensitivity,
    ads_Step_Gen_SoilsSteadyState_dsa,
    ads_Step_Gen_SoilsSteadyState_beginningTime,
    ads_Step_Gen_SoilsSteadyState_amplitude,
    ads_Step_Gen_SoilsSteadyState_convertSDI,
    ads_Step_Gen_SoilsSteadyState_creep,
    ads_Step_Gen_SoilsSteadyState_extrapolation,
    ads_Step_Gen_SoilsSteadyState_geometricNonlinearity,
    ads_Step_Gen_SoilsSteadyState_matrixSymmetry,
    ads_Step_Gen_SoilsSteadyState_totalTime
};

enum ads_Step_Gen_SoilsSteadyState_designSensitivityEnm
{
    ads_Step_Gen_SoilsSteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_SoilsSteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_SoilsSteadyState_amplitudeEnm
{
    ads_Step_Gen_SoilsSteadyState_amplitude_RAMP,
    ads_Step_Gen_SoilsSteadyState_amplitude_STEP
};

enum ads_Step_Gen_SoilsSteadyState_extrapolationEnm
{
    ads_Step_Gen_SoilsSteadyState_extrapolation_LINEAR,
    ads_Step_Gen_SoilsSteadyState_extrapolation_NONE,
    ads_Step_Gen_SoilsSteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_SoilsSteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_SoilsSteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_SoilsSteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_SoilsSteadyState_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_StaticMembersEnm
{
    ads_Step_Gen_Static_designSensitivity,
    ads_Step_Gen_Static_dsa,
    ads_Step_Gen_Static_beginningTime,
    ads_Step_Gen_Static_adiabatic,
    ads_Step_Gen_Static_amplitude,
    ads_Step_Gen_Static_convertSDI,
    ads_Step_Gen_Static_extrapolation,
    ads_Step_Gen_Static_geometricNonlinearity,
    ads_Step_Gen_Static_longTerm,
    ads_Step_Gen_Static_matrixSymmetry,
    ads_Step_Gen_Static_totalTime
};

enum ads_Step_Gen_Static_designSensitivityEnm
{
    ads_Step_Gen_Static_designSensitivity_ADJOINT,
    ads_Step_Gen_Static_designSensitivity_NONE
};

enum ads_Step_Gen_Static_amplitudeEnm
{
    ads_Step_Gen_Static_amplitude_RAMP,
    ads_Step_Gen_Static_amplitude_STEP
};

enum ads_Step_Gen_Static_extrapolationEnm
{
    ads_Step_Gen_Static_extrapolation_LINEAR,
    ads_Step_Gen_Static_extrapolation_NONE,
    ads_Step_Gen_Static_extrapolation_PARABOLIC
};

enum ads_Step_Gen_Static_matrixSymmetryEnm
{
    ads_Step_Gen_Static_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Static_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Static_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_StaticRiksMembersEnm
{
    ads_Step_Gen_StaticRiks_designSensitivity,
    ads_Step_Gen_StaticRiks_dsa,
    ads_Step_Gen_StaticRiks_beginningTime,
    ads_Step_Gen_StaticRiks_adiabatic,
    ads_Step_Gen_StaticRiks_amplitude,
    ads_Step_Gen_StaticRiks_convertSDI,
    ads_Step_Gen_StaticRiks_extrapolation,
    ads_Step_Gen_StaticRiks_geometricNonlinearity,
    ads_Step_Gen_StaticRiks_longTerm,
    ads_Step_Gen_StaticRiks_matrixSymmetry
};

enum ads_Step_Gen_StaticRiks_designSensitivityEnm
{
    ads_Step_Gen_StaticRiks_designSensitivity_ADJOINT,
    ads_Step_Gen_StaticRiks_designSensitivity_NONE
};

enum ads_Step_Gen_StaticRiks_amplitudeEnm
{
    ads_Step_Gen_StaticRiks_amplitude_RAMP,
    ads_Step_Gen_StaticRiks_amplitude_STEP
};

enum ads_Step_Gen_StaticRiks_extrapolationEnm
{
    ads_Step_Gen_StaticRiks_extrapolation_LINEAR,
    ads_Step_Gen_StaticRiks_extrapolation_PARABOLIC
};

enum ads_Step_Gen_StaticRiks_matrixSymmetryEnm
{
    ads_Step_Gen_StaticRiks_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_StaticRiks_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_StaticRiks_matrixSymmetry_UNSYMMETRIC
};

/** Enum with association roles. */
enum ads_Step_Gen_StaticRiks_arcLengthsRolesEnm
{
    ads_Step_Gen_StaticRiks_arcLengths_referent,
    ads_Step_Gen_StaticRiks_arcLengths_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Gen_StaticRiks_maximumDisplacementRolesEnm
{
    ads_Step_Gen_StaticRiks_maximumDisplacement_child,
    ads_Step_Gen_StaticRiks_maximumDisplacement_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_Static_fullyPlasticRolesEnm
{
    ads_Step_Gen_Static_fullyPlastic_referent,
    ads_Step_Gen_Static_fullyPlastic_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_SteadyStateTransportMembersEnm
{
    ads_Step_Gen_SteadyStateTransport_designSensitivity,
    ads_Step_Gen_SteadyStateTransport_dsa,
    ads_Step_Gen_SteadyStateTransport_beginningTime,
    ads_Step_Gen_SteadyStateTransport_amplitude,
    ads_Step_Gen_SteadyStateTransport_convertSDI,
    ads_Step_Gen_SteadyStateTransport_extrapolation,
    ads_Step_Gen_SteadyStateTransport_geometricNonlinearity,
    ads_Step_Gen_SteadyStateTransport_inertia,
    ads_Step_Gen_SteadyStateTransport_inertiaStabilizationFactor,
    ads_Step_Gen_SteadyStateTransport_inertiaSym,
    ads_Step_Gen_SteadyStateTransport_longTerm,
    ads_Step_Gen_SteadyStateTransport_matrixSymmetry,
    ads_Step_Gen_SteadyStateTransport_mullins,
    ads_Step_Gen_SteadyStateTransport_passByPass,
    ads_Step_Gen_SteadyStateTransport_totalTime
};

enum ads_Step_Gen_SteadyStateTransport_designSensitivityEnm
{
    ads_Step_Gen_SteadyStateTransport_designSensitivity_ADJOINT,
    ads_Step_Gen_SteadyStateTransport_designSensitivity_NONE
};

enum ads_Step_Gen_SteadyStateTransport_amplitudeEnm
{
    ads_Step_Gen_SteadyStateTransport_amplitude_RAMP,
    ads_Step_Gen_SteadyStateTransport_amplitude_STEP
};

enum ads_Step_Gen_SteadyStateTransport_extrapolationEnm
{
    ads_Step_Gen_SteadyStateTransport_extrapolation_LINEAR,
    ads_Step_Gen_SteadyStateTransport_extrapolation_NONE,
    ads_Step_Gen_SteadyStateTransport_extrapolation_PARABOLIC
};

enum ads_Step_Gen_SteadyStateTransport_inertiaSymEnm
{
    ads_Step_Gen_SteadyStateTransport_inertiaSym_LOW_SPEED,
    ads_Step_Gen_SteadyStateTransport_inertiaSym_NO,
    ads_Step_Gen_SteadyStateTransport_inertiaSym_YES
};

enum ads_Step_Gen_SteadyStateTransport_matrixSymmetryEnm
{
    ads_Step_Gen_SteadyStateTransport_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_SteadyStateTransport_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_SteadyStateTransport_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_SteadyStateTransport_mullinsEnm
{
    ads_Step_Gen_SteadyStateTransport_mullins_RAMP,
    ads_Step_Gen_SteadyStateTransport_mullins_STEP
};

/** Enum with association roles. */
enum ads_Step_Gen_SteadyStateTransport_eulerianElementsRolesEnm
{
    ads_Step_Gen_SteadyStateTransport_eulerianElements_referent,
    ads_Step_Gen_SteadyStateTransport_eulerianElements_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureDisplacementMembersEnm
{
    ads_Step_Gen_TemperatureDisplacement_designSensitivity,
    ads_Step_Gen_TemperatureDisplacement_dsa,
    ads_Step_Gen_TemperatureDisplacement_beginningTime,
    ads_Step_Gen_TemperatureDisplacement_advection,
    ads_Step_Gen_TemperatureDisplacement_continue,
    ads_Step_Gen_TemperatureDisplacement_convertSDI,
    ads_Step_Gen_TemperatureDisplacement_electrical,
    ads_Step_Gen_TemperatureDisplacement_electrochemical,
    ads_Step_Gen_TemperatureDisplacement_explicitIntegration,
    ads_Step_Gen_TemperatureDisplacement_extrapolation,
    ads_Step_Gen_TemperatureDisplacement_geometricNonlinearity,
    ads_Step_Gen_TemperatureDisplacement_matrixSymmetry,
    ads_Step_Gen_TemperatureDisplacement_porePressure,
    ads_Step_Gen_TemperatureDisplacement_poreptol,
    ads_Step_Gen_TemperatureDisplacement_rateDependence,
    ads_Step_Gen_TemperatureDisplacement_totalTime
};

enum ads_Step_Gen_TemperatureDisplacement_designSensitivityEnm
{
    ads_Step_Gen_TemperatureDisplacement_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureDisplacement_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_advectionEnm
{
    ads_Step_Gen_TemperatureDisplacement_advection_NO,
    ads_Step_Gen_TemperatureDisplacement_advection_NONE,
    ads_Step_Gen_TemperatureDisplacement_advection_YES
};

enum ads_Step_Gen_TemperatureDisplacement_continueEnm
{
    ads_Step_Gen_TemperatureDisplacement_continue_NO,
    ads_Step_Gen_TemperatureDisplacement_continue_NONE,
    ads_Step_Gen_TemperatureDisplacement_continue_YES
};

enum ads_Step_Gen_TemperatureDisplacement_explicitIntegrationEnm
{
    ads_Step_Gen_TemperatureDisplacement_explicitIntegration_EXPLICIT,
    ads_Step_Gen_TemperatureDisplacement_explicitIntegration_IMPLICIT,
    ads_Step_Gen_TemperatureDisplacement_explicitIntegration_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_extrapolationEnm
{
    ads_Step_Gen_TemperatureDisplacement_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureDisplacement_extrapolation_NONE,
    ads_Step_Gen_TemperatureDisplacement_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureDisplacement_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureDisplacement_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureDisplacement_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureDisplacement_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_TemperatureDisplacement_rateDependenceEnm
{
    ads_Step_Gen_TemperatureDisplacement_rateDependence_NONE,
    ads_Step_Gen_TemperatureDisplacement_rateDependence_OFF,
    ads_Step_Gen_TemperatureDisplacement_rateDependence_ON
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureDisplacement_SteadyStateMembersEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_designSensitivity,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_dsa,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_beginningTime,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_advection,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_continue,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_convertSDI,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_electrical,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_electrochemical,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_explicitIntegration,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_extrapolation,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_geometricNonlinearity,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_matrixSymmetry,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_porePressure,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_poreptol,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_rateDependence,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_totalTime,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_amplitude
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_designSensitivityEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_advectionEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_advection_NO,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_advection_NONE,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_advection_YES
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_continueEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_continue_NO,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_continue_NONE,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_continue_YES
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_explicitIntegrationEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_explicitIntegration_EXPLICIT,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_explicitIntegration_IMPLICIT,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_explicitIntegration_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_extrapolationEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_extrapolation_NONE,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_rateDependenceEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_rateDependence_NONE,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_rateDependence_OFF,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_rateDependence_ON
};

enum ads_Step_Gen_TemperatureDisplacement_SteadyState_amplitudeEnm
{
    ads_Step_Gen_TemperatureDisplacement_SteadyState_amplitude_RAMP,
    ads_Step_Gen_TemperatureDisplacement_SteadyState_amplitude_STEP
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureDisplacement_TransientMembersEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_designSensitivity,
    ads_Step_Gen_TemperatureDisplacement_Transient_dsa,
    ads_Step_Gen_TemperatureDisplacement_Transient_beginningTime,
    ads_Step_Gen_TemperatureDisplacement_Transient_advection,
    ads_Step_Gen_TemperatureDisplacement_Transient_continue,
    ads_Step_Gen_TemperatureDisplacement_Transient_convertSDI,
    ads_Step_Gen_TemperatureDisplacement_Transient_electrical,
    ads_Step_Gen_TemperatureDisplacement_Transient_electrochemical,
    ads_Step_Gen_TemperatureDisplacement_Transient_explicitIntegration,
    ads_Step_Gen_TemperatureDisplacement_Transient_extrapolation,
    ads_Step_Gen_TemperatureDisplacement_Transient_geometricNonlinearity,
    ads_Step_Gen_TemperatureDisplacement_Transient_matrixSymmetry,
    ads_Step_Gen_TemperatureDisplacement_Transient_porePressure,
    ads_Step_Gen_TemperatureDisplacement_Transient_poreptol,
    ads_Step_Gen_TemperatureDisplacement_Transient_rateDependence,
    ads_Step_Gen_TemperatureDisplacement_Transient_totalTime,
    ads_Step_Gen_TemperatureDisplacement_Transient_amplitude
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_designSensitivityEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureDisplacement_Transient_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_advectionEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_advection_NO,
    ads_Step_Gen_TemperatureDisplacement_Transient_advection_NONE,
    ads_Step_Gen_TemperatureDisplacement_Transient_advection_YES
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_continueEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_continue_NO,
    ads_Step_Gen_TemperatureDisplacement_Transient_continue_NONE,
    ads_Step_Gen_TemperatureDisplacement_Transient_continue_YES
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_explicitIntegrationEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_explicitIntegration_EXPLICIT,
    ads_Step_Gen_TemperatureDisplacement_Transient_explicitIntegration_IMPLICIT,
    ads_Step_Gen_TemperatureDisplacement_Transient_explicitIntegration_NONE
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_extrapolationEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureDisplacement_Transient_extrapolation_NONE,
    ads_Step_Gen_TemperatureDisplacement_Transient_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureDisplacement_Transient_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureDisplacement_Transient_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_rateDependenceEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_rateDependence_NONE,
    ads_Step_Gen_TemperatureDisplacement_Transient_rateDependence_OFF,
    ads_Step_Gen_TemperatureDisplacement_Transient_rateDependence_ON
};

enum ads_Step_Gen_TemperatureDisplacement_Transient_amplitudeEnm
{
    ads_Step_Gen_TemperatureDisplacement_Transient_amplitude_RAMP,
    ads_Step_Gen_TemperatureDisplacement_Transient_amplitude_STEP
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureElectricalMembersEnm
{
    ads_Step_Gen_TemperatureElectrical_designSensitivity,
    ads_Step_Gen_TemperatureElectrical_dsa,
    ads_Step_Gen_TemperatureElectrical_beginningTime,
    ads_Step_Gen_TemperatureElectrical_amplitude,
    ads_Step_Gen_TemperatureElectrical_convertSDI,
    ads_Step_Gen_TemperatureElectrical_extrapolation,
    ads_Step_Gen_TemperatureElectrical_matrixSymmetry,
    ads_Step_Gen_TemperatureElectrical_totalTime
};

enum ads_Step_Gen_TemperatureElectrical_designSensitivityEnm
{
    ads_Step_Gen_TemperatureElectrical_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureElectrical_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureElectrical_amplitudeEnm
{
    ads_Step_Gen_TemperatureElectrical_amplitude_RAMP,
    ads_Step_Gen_TemperatureElectrical_amplitude_STEP
};

enum ads_Step_Gen_TemperatureElectrical_extrapolationEnm
{
    ads_Step_Gen_TemperatureElectrical_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureElectrical_extrapolation_NONE,
    ads_Step_Gen_TemperatureElectrical_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureElectrical_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureElectrical_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureElectrical_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureElectrical_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureElectricalSteadyStateMembersEnm
{
    ads_Step_Gen_TemperatureElectricalSteadyState_designSensitivity,
    ads_Step_Gen_TemperatureElectricalSteadyState_dsa,
    ads_Step_Gen_TemperatureElectricalSteadyState_beginningTime,
    ads_Step_Gen_TemperatureElectricalSteadyState_amplitude,
    ads_Step_Gen_TemperatureElectricalSteadyState_convertSDI,
    ads_Step_Gen_TemperatureElectricalSteadyState_extrapolation,
    ads_Step_Gen_TemperatureElectricalSteadyState_matrixSymmetry,
    ads_Step_Gen_TemperatureElectricalSteadyState_totalTime
};

enum ads_Step_Gen_TemperatureElectricalSteadyState_designSensitivityEnm
{
    ads_Step_Gen_TemperatureElectricalSteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureElectricalSteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureElectricalSteadyState_amplitudeEnm
{
    ads_Step_Gen_TemperatureElectricalSteadyState_amplitude_RAMP,
    ads_Step_Gen_TemperatureElectricalSteadyState_amplitude_STEP
};

enum ads_Step_Gen_TemperatureElectricalSteadyState_extrapolationEnm
{
    ads_Step_Gen_TemperatureElectricalSteadyState_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureElectricalSteadyState_extrapolation_NONE,
    ads_Step_Gen_TemperatureElectricalSteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureElectricalSteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureElectricalSteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureElectricalSteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureElectricalSteadyState_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureElectroChemicalMembersEnm
{
    ads_Step_Gen_TemperatureElectroChemical_designSensitivity,
    ads_Step_Gen_TemperatureElectroChemical_dsa,
    ads_Step_Gen_TemperatureElectroChemical_beginningTime,
    ads_Step_Gen_TemperatureElectroChemical_amplitude,
    ads_Step_Gen_TemperatureElectroChemical_convertSDI,
    ads_Step_Gen_TemperatureElectroChemical_extrapolation,
    ads_Step_Gen_TemperatureElectroChemical_matrixSymmetry,
    ads_Step_Gen_TemperatureElectroChemical_totalTime
};

enum ads_Step_Gen_TemperatureElectroChemical_designSensitivityEnm
{
    ads_Step_Gen_TemperatureElectroChemical_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureElectroChemical_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureElectroChemical_amplitudeEnm
{
    ads_Step_Gen_TemperatureElectroChemical_amplitude_RAMP,
    ads_Step_Gen_TemperatureElectroChemical_amplitude_STEP
};

enum ads_Step_Gen_TemperatureElectroChemical_extrapolationEnm
{
    ads_Step_Gen_TemperatureElectroChemical_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureElectroChemical_extrapolation_NONE,
    ads_Step_Gen_TemperatureElectroChemical_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureElectroChemical_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureElectroChemical_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureElectroChemical_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureElectroChemical_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_TemperatureElectroChemicalSteadyStateMembersEnm
{
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_designSensitivity,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_dsa,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_beginningTime,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_amplitude,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_convertSDI,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_extrapolation,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_matrixSymmetry,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_totalTime
};

enum ads_Step_Gen_TemperatureElectroChemicalSteadyState_designSensitivityEnm
{
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_designSensitivity_ADJOINT,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_designSensitivity_NONE
};

enum ads_Step_Gen_TemperatureElectroChemicalSteadyState_amplitudeEnm
{
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_amplitude_RAMP,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_amplitude_STEP
};

enum ads_Step_Gen_TemperatureElectroChemicalSteadyState_extrapolationEnm
{
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_extrapolation_LINEAR,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_extrapolation_NONE,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_extrapolation_PARABOLIC
};

enum ads_Step_Gen_TemperatureElectroChemicalSteadyState_matrixSymmetryEnm
{
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_TemperatureElectroChemicalSteadyState_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Gen_ViscoMembersEnm
{
    ads_Step_Gen_Visco_designSensitivity,
    ads_Step_Gen_Visco_dsa,
    ads_Step_Gen_Visco_beginningTime,
    ads_Step_Gen_Visco_amplitude,
    ads_Step_Gen_Visco_convertSDI,
    ads_Step_Gen_Visco_explicitIntegration,
    ads_Step_Gen_Visco_extrapolation,
    ads_Step_Gen_Visco_geometricNonlinearity,
    ads_Step_Gen_Visco_matrixSymmetry,
    ads_Step_Gen_Visco_totalTime
};

enum ads_Step_Gen_Visco_designSensitivityEnm
{
    ads_Step_Gen_Visco_designSensitivity_ADJOINT,
    ads_Step_Gen_Visco_designSensitivity_NONE
};

enum ads_Step_Gen_Visco_amplitudeEnm
{
    ads_Step_Gen_Visco_amplitude_RAMP,
    ads_Step_Gen_Visco_amplitude_STEP
};

enum ads_Step_Gen_Visco_extrapolationEnm
{
    ads_Step_Gen_Visco_extrapolation_LINEAR,
    ads_Step_Gen_Visco_extrapolation_NONE,
    ads_Step_Gen_Visco_extrapolation_PARABOLIC
};

enum ads_Step_Gen_Visco_matrixSymmetryEnm
{
    ads_Step_Gen_Visco_matrixSymmetry_AUTOMATIC,
    ads_Step_Gen_Visco_matrixSymmetry_SYMMETRIC,
    ads_Step_Gen_Visco_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_Step_Gen_automaticStepStabilizationRolesEnm
{
    ads_Step_Gen_automaticStepStabilization_child,
    ads_Step_Gen_automaticStepStabilization_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_contourIntegralsRolesEnm
{
    ads_Step_Gen_contourIntegrals_child,
    ads_Step_Gen_contourIntegrals_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_controls_ContactPairRolesEnm
{
    ads_Step_Gen_controls_ContactPair_child,
    ads_Step_Gen_controls_ContactPair_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_controls_GeneralContactRolesEnm
{
    ads_Step_Gen_controls_GeneralContact_child,
    ads_Step_Gen_controls_GeneralContact_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_controls_SolutionTechniqueRolesEnm
{
    ads_Step_Gen_controls_SolutionTechnique_child,
    ads_Step_Gen_controls_SolutionTechnique_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_explicitDynamicsSolverControlsRolesEnm
{
    ads_Step_Gen_explicitDynamicsSolverControls_child,
    ads_Step_Gen_explicitDynamicsSolverControls_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_stoppingCriterionRolesEnm
{
    ads_Step_Gen_stoppingCriterion_child,
    ads_Step_Gen_stoppingCriterion_parent
};

/** Enum with association roles. */
enum ads_Step_Gen_timeIncrementRolesEnm
{
    ads_Step_Gen_timeIncrement_child,
    ads_Step_Gen_timeIncrement_parent
};

/** Enum with record members. */
enum ads_Step_Lin_ElectromagneticMembersEnm
{
    ads_Step_Lin_Electromagnetic_designSensitivity,
    ads_Step_Lin_Electromagnetic_dsa
};

enum ads_Step_Lin_Electromagnetic_designSensitivityEnm
{
    ads_Step_Lin_Electromagnetic_designSensitivity_ADJOINT,
    ads_Step_Lin_Electromagnetic_designSensitivity_NONE
};

/** 
Enum with association roles. */
enum ads_Step_Lin_controls_SolutionTechniqueRolesEnm
{
    ads_Step_Lin_controls_SolutionTechnique_child,
    ads_Step_Lin_controls_SolutionTechnique_parent
};

/** 
Enum with association roles. */
enum ads_StoppingCriterion_cutoffRolesEnm
{
    ads_StoppingCriterion_cutoff_child,
    ads_StoppingCriterion_cutoff_parent
};

/** Enum with record members. */
enum ads_TaskControls_SolutionMembersEnm
{
    ads_TaskControls_Solution_discontinuous
};

/** Enum with record members. */
enum ads_TaskControls_Solution_ConstraintsMembersEnm
{
    ads_TaskControls_Solution_Constraints_discontinuous,
    ads_TaskControls_Solution_Constraints_axialStrainTolHybridBeamEls,
    ads_TaskControls_Solution_Constraints_contForceErrTolSDI,
    ads_TaskControls_Solution_Constraints_contactSlipTol,
    ads_TaskControls_Solution_Constraints_dispTolDistrCouplingEls,
    ads_TaskControls_Solution_Constraints_rotationTolDistrCouplingEls,
    ads_TaskControls_Solution_Constraints_softContactTolLowPressure,
    ads_TaskControls_Solution_Constraints_transverseShearStrainTolHybridbBeamEls,
    ads_TaskControls_Solution_Constraints_volStrainTolHybridSolidEls
};

/** Enum with record members. */
enum ads_TaskControls_Solution_FieldMembersEnm
{
    ads_TaskControls_Solution_Field_discontinuous,
    ads_TaskControls_Solution_Field_alternaticeResidualConvergeCrit,
    ads_TaskControls_Solution_Field_convRatioTwoFields,
    ads_TaskControls_Solution_Field_critZeroDispIncToCharElemLen,
    ads_TaskControls_Solution_Field_critZeroFlux,
    ads_TaskControls_Solution_Field_critZeroFluxToAvgValueLargeFlux,
    ads_TaskControls_Solution_Field_field,
    ads_TaskControls_Solution_Field_ratioResidualToAvgFlux,
    ads_TaskControls_Solution_Field_ratioResidualToAvgFluxAcceptIter,
    ads_TaskControls_Solution_Field_ratioSolCorrectToSolValueZeroFlux,
    ads_TaskControls_Solution_Field_ratioSolCorrectionToSolValue
};

enum ads_TaskControls_Solution_Field_fieldEnm
{
    ads_TaskControls_Solution_Field_field_CONCENTRATION,
    ads_TaskControls_Solution_Field_field_DISPLACEMENT,
    ads_TaskControls_Solution_Field_field_ELECTRICAL_POTENTIAL,
    ads_TaskControls_Solution_Field_field_ELECTRICAL_POTENTIAL_PIEZO,
    ads_TaskControls_Solution_Field_field_FLUID_ELECTRIC_POTENTIAL,
    ads_TaskControls_Solution_Field_field_HYDROSTATIC_FLUID_PRESSURE,
    ads_TaskControls_Solution_Field_field_ION_CONCENTRATION,
    ads_TaskControls_Solution_Field_field_MATERIAL_FLOW,
    ads_TaskControls_Solution_Field_field_NONE,
    ads_TaskControls_Solution_Field_field_PORE_FLUID_PRESSURE,
    ads_TaskControls_Solution_Field_field_PRESSURE_LAGRANGE_MULTIPLIER,
    ads_TaskControls_Solution_Field_field_ROTATION,
    ads_TaskControls_Solution_Field_field_SLURRYVF,
    ads_TaskControls_Solution_Field_field_SPECIES_CONCENTRATION,
    ads_TaskControls_Solution_Field_field_TEMPERATURE,
    ads_TaskControls_Solution_Field_field_VOLUMETRIC_LAGRANGE_MULTIPLIER
};

/** 
Enum with association roles. */
enum ads_TaskControls_Solution_Field_initValTimeAvgFluxRolesEnm
{
    ads_TaskControls_Solution_Field_initValTimeAvgFlux_child,
    ads_TaskControls_Solution_Field_initValTimeAvgFlux_parent
};

/** 
Enum with association roles. */
enum ads_TaskControls_Solution_Field_userDefAvgFluxRolesEnm
{
    ads_TaskControls_Solution_Field_userDefAvgFlux_child,
    ads_TaskControls_Solution_Field_userDefAvgFlux_parent
};

/** Enum with record members. */
enum ads_TaskControls_Solution_LineSearchMembersEnm
{
    ads_TaskControls_Solution_LineSearch_discontinuous,
    ads_TaskControls_Solution_LineSearch_maxCorrectionScale,
    ads_TaskControls_Solution_LineSearch_maxNoLineSearchIters,
    ads_TaskControls_Solution_LineSearch_minCorrectionScale,
    ads_TaskControls_Solution_LineSearch_ratioNewToOldCorrection,
    ads_TaskControls_Solution_LineSearch_residualReduction
};

/** Enum with record members. */
enum ads_TaskControls_Solution_NoCutBackScalingMembersEnm
{
    ads_TaskControls_Solution_NoCutBackScaling_discontinuous,
    ads_TaskControls_Solution_NoCutBackScaling_alpha,
    ads_TaskControls_Solution_NoCutBackScaling_beta,
    ads_TaskControls_Solution_NoCutBackScaling_nm
};

/** Enum with record members. */
enum ads_TaskControls_Solution_ResetMembersEnm
{
    ads_TaskControls_Solution_Reset_discontinuous,
    ads_TaskControls_Solution_Reset_reset
};

/** Enum with record members. */
enum ads_TaskControls_Solution_TypeDirectCyclcMembersEnm
{
    ads_TaskControls_Solution_TypeDirectCyclc_discontinuous,
    ads_TaskControls_Solution_TypeDirectCyclc_iterNoPeriodicity,
    ads_TaskControls_Solution_TypeDirectCyclc_plasDetRatDispCoeffToDispCoeff,
    ads_TaskControls_Solution_TypeDirectCyclc_plasDetRatResCoeffToAvgFluxNorm,
    ads_TaskControls_Solution_TypeDirectCyclc_ratCorDispCoeffToDispCoeff,
    ads_TaskControls_Solution_TypeDirectCyclc_stabDetRatResCoeffToAvgFluxNorm
};

/** Enum with record members. */
enum ads_TaskControls_Solution_VcctLinearScalingMembersEnm
{
    ads_TaskControls_Solution_VcctLinearScaling_discontinuous,
    ads_TaskControls_Solution_VcctLinearScaling_beta
};

/** Enum with association roles. */
enum ads_TaskTerminationCriterion_MAX_valueRolesEnm
{
    ads_TaskTerminationCriterion_MAX_value_child,
    ads_TaskTerminationCriterion_MAX_value_parent
};

/** Enum with association roles. */
enum ads_TaskTerminationCriterion_MIN_valueRolesEnm
{
    ads_TaskTerminationCriterion_MIN_value_child,
    ads_TaskTerminationCriterion_MIN_value_parent
};

/** Enum with association roles. */
enum ads_Task_controlsRolesEnm
{
    ads_Task_controls_child,
    ads_Task_controls_parent
};

/** 
Enum with association roles. */
enum ads_Task_taskControlsRolesEnm
{
    ads_Task_taskControls_child,
    ads_Task_taskControls_parent
};

#endif
