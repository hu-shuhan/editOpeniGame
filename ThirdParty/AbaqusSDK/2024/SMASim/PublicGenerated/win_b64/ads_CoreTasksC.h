//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreTasksC_h
#define ads_CoreTasksC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Tasks of the latest level of form Core */

/** Settings for composite modal damping for modal analysis. */
#define ads_CompositeModalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 0))

/** Regional settings for composite modal damping. */
#define ads_CompositeModalDampingRegional (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 1))

/** Region for composite modal damping settings. */
#define ads_CompositeModalDampingRegional_elset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 2))

/** Regional settings for composite modal damping. */
#define ads_CompositeModalDamping_regionalData (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 3))

/** This record is associated with a frequency extraction task and is associated with a cset of cyclic symmetry modes that represent the cyclic symmetry modes for this task. It also contains input data such as min and max modes specified. */
#define ads_CyclicSymmetryModeSelection (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 4))

/** This reference associates the actual cyclic symmetry modes extracted in a frequency task with the cyclic symmetry modes selection record. */
#define ads_CyclicSymmetryModeSelection_cyclicSymmetryModes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 5))

/** This reference binds a cyclic symmetry mode to the eigenvalues extracted for this mode. The eigenvalues contained in this c-set must be a subset of the eigenvalues defined under the frequency task. */
#define ads_CyclicSymmetryMode_modes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 6))

/** To control the type (viscous, structural) and source of damping (material, global) within the step definition and with substructures. */
#define ads_DampingControls (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 7))

/** Active eigenmode information used in a dynamic analysis based on modes or in a complex eigenvalue extraction analysis. */
#define ads_EigenmodeSelection (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 8))

/** Association between active used modes record and the modal frequency range to be used. */
#define ads_EigenmodeSelection_frequencyRange (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 9))

/** Reference between active used modes record and the modal frequency range to be used. */
#define ads_EigenmodeSelection_modeSet (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 10))

/** Specifies the element recovery matrix output for substructure generation. */
#define ads_ElementRecoveryMatrix (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 11))

/** A reference to the region of the substructure for which the element results recovery should be enabled. */
#define ads_ElementRecoveryMatrix_region (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 12))

/** A list of section points in the substructure for the element recover matrix output. */
#define ads_ElementRecoveryMatrix_sectionPoints (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 13))

/** A list of elements in the regions of the substructure for the element recover matrix output. */
#define ads_ElementRecoveryMatrix_selectiveElements (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 14))

/** A list of element recover strain (E) components to output. */
#define ads_ElementRecoveryMatrix_strain (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 15))

/** A list of element recover stress (S) components to output. */
#define ads_ElementRecoveryMatrix_stress (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 16))

/** Values of minimum and maximum frequencies to be used for active modes. */
#define ads_FrequencyRange (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 17))

/** Global damping factors. */
#define ads_GlobalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 18))

/** Base record for linear equation solver type. The derived type represents the actual linear equation solver. */
#define ads_LinearEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 19))

/** Direct linear equation solver record. */
#define ads_LinearEquationSolver_Direct (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 20))

/** Iterative linear equation solver record. */
#define ads_LinearEquationSolver_Iterative (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 21))

/** Check the quality of the generated stiffness and mass matrices. The option can be used only in matrix generation or substructure generation analyses. */
#define ads_MatrixCheck (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 22))

/** Reference point for matrix quality check. */
#define ads_MatrixCheck_referencePoint (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 23))

/** Matrix export according to workflow. */
#define ads_MatrixExport (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 24))

/** Instances of the operator matrices to be generated at substructure generation analyses. */
#define ads_MatrixInstances (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 25))

/** Matrices to output to text file(s). */
#define ads_MatrixOutput (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 26))

/** To specify damping for mode-based procedures. */
#define ads_ModalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 27))

/** Definitions for damping values. Columns are defined by the modal damping type and definition. */
#define ads_ModalDampingTable (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 28))

/** Damping values data table. */
#define ads_ModalDamping_data (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 29))

/** The elaboration task for the simulation. */
#define ads_Model_elaborationTask (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 30))

/** The task(s) that start the simulation. */
#define ads_Model_initialTask (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 31))

/** This sequence of all tasks represents the traditional order of the *Step keywords in the input file. This relationship is redundant with the ??? reference. There really are no plans to deprecate the sequencing of tasks any time soon. Ideally we would combine this association with the string-keyed Model HAS Tasks composition, but a keyed relationship cannot also be sequenced. */
#define ads_Model_stepSequence (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 32))

/** A link to all of the tasks. The tasks are keyed by a name. */
#define ads_Model_task (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 33))

/** Reference point for matrix quality check, etc. */
#define ads_ReferencePoint (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 34))

#define ads_RegionActivationRelay (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 35))

#define ads_RegionActivationRelay_regionActivation (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 36))

/** A Step is a Task that reaches equilibrium. Weather it advances state is optional. For steps that do advance state, this happens if and only if there are excitations. Consequently, advancing time is also an optional characteristic of the data types derived from Step. This definition of step is a subset of what step means in ABAQUS INP. The unit of computational work in the old definition is being replaced by task. The concept of a step that does not modify the state is being replaced by event. That leaves the steps that just reach an equilibrium and those that reach an initial equilibrium and then advance state. */
#define ads_Step_Gen (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 37))

/** Reference to the previous general analysis step. */
#define ads_Step_Gen_previousStep (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 38))

/** An event is a task that computes a solution, e.g. displacement. However, it does not advance the state. An example of an event is a static linear perturbation step. The hallmark of an event is that superimposition holds. This feature is used when combining load cases between cases within an event as well as between sibling events to compute a solution envelope. */
#define ads_Step_Lin (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 39))

/** Calculate the cross-section warping function, to define the centroid and shear center, and to generate the stiffness and inertia properties for a meshed cross-section */
#define ads_Step_Lin_BeamSectionGeneration (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 40))

/** Eigenvalue buckling estimation task. */
#define ads_Step_Lin_Buckle (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 41))

/** This option is used to perform eigenvalue extraction to calculate the complex eigenvalues and corresponding complex mode shapes of a system. */
#define ads_Step_Lin_ComplexFrequency (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 42))

/** Export matrices according to workflow. */
#define ads_Step_Lin_ComplexFrequency_export (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 43))

/** Task used to perform eigenvalue extraction to calculate the natural frequencies and corresponding mode shapes of a system. */
#define ads_Step_Lin_Frequency (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 44))

/** Settings for composite modal damping. */
#define ads_Step_Lin_Frequency_compositeModalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 45))

/** This binds a cyclic symmetry modes record with its frequency task. The modes associated with the frequency task will be divided among the cyclic symmetry mode records associated with this record. */
#define ads_Step_Lin_Frequency_cyclicSymmetryModeSelection (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 46))

/** The list of system dofs constrained with the secondary base motion boundary conditions that are specific for modal analyses. */
#define ads_Step_Lin_Frequency_secondaryBaseNodalDofs (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 47))

/** Task to generate system level operators. */
#define ads_Step_Lin_MatrixGeneration (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 48))

/** Reference point for matrix quality check. */
#define ads_Step_Lin_MatrixGeneration_checkRefPoint (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 49))

/** The set of elements in the original mesh for matrix generation. */
#define ads_Step_Lin_MatrixGeneration_elset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 50))

/** Export matrices according to workflow. */
#define ads_Step_Lin_MatrixGeneration_export (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 51))

/** A set of nodes of the model that will be used for interface in the usage model. */
#define ads_Step_Lin_MatrixGeneration_interfaceNodes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 52))

/** Matrix quality check. */
#define ads_Step_Lin_MatrixGeneration_matrixCheck (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 53))

/** Definitions for PLOT-type elements. */
#define ads_Step_Lin_MatrixGeneration_matrixPlot (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 54))

/** Matrices to output to text file(s). */
#define ads_Step_Lin_MatrixGeneration_output (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 55))

/** A set of nodes of the model that will be public (visible) at the usage model. */
#define ads_Step_Lin_MatrixGeneration_publicNodes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 56))

/** Step definition for reduced basis generation. */
#define ads_Step_Lin_ReducedBasisGeneration (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 57))

/** The task of generation of a substructure. */
#define ads_Step_Lin_SubstructureGeneration (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 58))

/** Reference point for matrix quality check. */
#define ads_Step_Lin_SubstructureGeneration_checkRefPoint (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 59))

/** A list of the elements defining a coarse substructure display representation. */
#define ads_Step_Lin_SubstructureGeneration_displayElements (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 60))

/** Reference to region of the elements defining a coarse substructure display representation. */
#define ads_Step_Lin_SubstructureGeneration_displayElset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 61))

/** Element recovery matrices to output. */
#define ads_Step_Lin_SubstructureGeneration_elementRecovery (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 62))

/** Matrix quality check. */
#define ads_Step_Lin_SubstructureGeneration_matrixCheck (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 63))

/** Instances of the operator matrices to be generated. */
#define ads_Step_Lin_SubstructureGeneration_matrixInstances (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 64))

/** A list of the nodes to monitor displacements. */
#define ads_Step_Lin_SubstructureGeneration_monitorNodes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 65))

/** Matrices to output to text or binary file(s). */
#define ads_Step_Lin_SubstructureGeneration_output (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 66))

/** The list of the elements in the regions of the substructure where user wants to recover results. */
#define ads_Step_Lin_SubstructureGeneration_recoveryElements (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 67))

/** The element set that contains all the elements in the regions of the substructure where the results have to be recovered. */
#define ads_Step_Lin_SubstructureGeneration_recoveryElset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 68))

/** The list of the nodes of the substructure where user wants to recover results. */
#define ads_Step_Lin_SubstructureGeneration_recoveryNodes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 69))

/** The node set that contains all the nodes in the regions of the substructure where the results have to be recovered. */
#define ads_Step_Lin_SubstructureGeneration_recoveryNset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 70))

/** Reference to region record to define node cluster to specify node wise retained all dof on the substructure. */
#define ads_Step_Lin_SubstructureGeneration_region (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 71))

/** The list of system dofs retained. May contain internal dofs in addition to user dofs. */
#define ads_Step_Lin_SubstructureGeneration_retainedNodalDofs (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 72))

/** The list of the nodes retained by the user (connection points). */
#define ads_Step_Lin_SubstructureGeneration_retainedNodes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 73))

/** Miscellanious data from SUP file. */
#define ads_Step_Lin_SubstructureGeneration_supData (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 74))

/** Reference to load case collection. */
#define ads_Step_Lin_cases (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 75))

/** Applicable to some modal events. */
#define ads_Step_Lin_dampingControls (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 76))

/** Selected modes to be used in a dynamic analysis based on modes, in a complex eigenvalue extraction analysis, or in a substructure generation analysis. */
#define ads_Step_Lin_eigenmodeSelection (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 77))

/** Applicable to some modal events. */
#define ads_Step_Lin_globalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 78))

/** Applicable to some modal events. */
#define ads_Step_Lin_modalDamping (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 79))

/** Change the substructure properties from step to step in substructure usage analysis. */
#define ads_SubstructureChange (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 80))

/** A set of substructure elements for the change. */
#define ads_SubstructureChange_elset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 81))

/** Output solution at substructure interface in substructure usage analysis. */
#define ads_SubstructureOutput (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 82))

/** The set of (substructure) elements for output. */
#define ads_SubstructureOutput_elset (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 83))

/** A unit of of simulation work that can be scheduled, run in a particular sequence with other tasks, etc. The net result of this work will be represented in one (or more) new SIM edition view. This task will be the new base class from which we derive other step-like concepts. */
#define ads_Task (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 84))

/** The list of system dofs constrained with boundary conditions. */
#define ads_Task_constrainedNodalDofs (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 85))

/** Reference to the set of elements that are inactive in the current step. For Abaqus/Standard and Explicit, see *Model Change, REMOVE. */
#define ads_Task_inactiveElements (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 86))

#define ads_Task_linearEquationSolver (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 87))

/** For a given task, this reference provides the set of modes (EigenValues) which were created by running that task. */
#define ads_Task_modes (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 88))

#define ads_Task_regionActivationRelays (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 89))

/** Change for substructure properties in substructure usage analysis. */
#define ads_Task_substructureChange (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 90))

/** Output solution in substructure usage analysis. */
#define ads_Task_substructureOutput (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 91))

/** For a given Step, this provides the set of time points which were created by running that step. */
#define ads_Task_times (ads_CoreFragmentTypeIndex(ads_CoreTasksFragment, 92))

/** 
Enum with record members. */
enum ads_CompositeModalDampingMembersEnm
{
    ads_CompositeModalDamping_massMatrixInput,
    ads_CompositeModalDamping_stiffnessMatrixInput
};

/** 
Enum with record members. */
enum ads_CompositeModalDampingRegionalMembersEnm
{
    ads_CompositeModalDampingRegional_massFraction,
    ads_CompositeModalDampingRegional_stiffnessFraction
};

/** 
Enum with association roles. */
enum ads_CompositeModalDampingRegional_elsetRolesEnm
{
    ads_CompositeModalDampingRegional_elset_referent,
    ads_CompositeModalDampingRegional_elset_referrer
};

/** 
Enum with association roles. */
enum ads_CompositeModalDamping_regionalDataRolesEnm
{
    ads_CompositeModalDamping_regionalData_child,
    ads_CompositeModalDamping_regionalData_parent
};

/** 
Enum with record members. */
enum ads_CyclicSymmetryModeSelectionMembersEnm
{
    ads_CyclicSymmetryModeSelection_even,
    ads_CyclicSymmetryModeSelection_maximumMode,
    ads_CyclicSymmetryModeSelection_minimumMode
};

/** 
Enum with association roles. */
enum ads_CyclicSymmetryModeSelection_cyclicSymmetryModesRolesEnm
{
    ads_CyclicSymmetryModeSelection_cyclicSymmetryModes_referent,
    ads_CyclicSymmetryModeSelection_cyclicSymmetryModes_referrer
};

/** 
Enum with association roles. */
enum ads_CyclicSymmetryMode_modesRolesEnm
{
    ads_CyclicSymmetryMode_modes_referent,
    ads_CyclicSymmetryMode_modes_referrer
};

/** 
Enum with record members. */
enum ads_DampingControlsMembersEnm
{
    ads_DampingControls_lowFrequency,
    ads_DampingControls_structural,
    ads_DampingControls_viscous
};

enum ads_DampingControls_structuralEnm
{
    ads_DampingControls_structural_COMBINED,
    ads_DampingControls_structural_ELEMENT,
    ads_DampingControls_structural_FACTOR,
    ads_DampingControls_structural_NONE
};

enum ads_DampingControls_viscousEnm
{
    ads_DampingControls_viscous_COMBINED,
    ads_DampingControls_viscous_ELEMENT,
    ads_DampingControls_viscous_FACTOR,
    ads_DampingControls_viscous_NONE
};

/** 
Enum with association roles. */
enum ads_EigenmodeSelection_frequencyRangeRolesEnm
{
    ads_EigenmodeSelection_frequencyRange_child,
    ads_EigenmodeSelection_frequencyRange_parent
};

/** 
Enum with association roles. */
enum ads_EigenmodeSelection_modeSetRolesEnm
{
    ads_EigenmodeSelection_modeSet_referent,
    ads_EigenmodeSelection_modeSet_referrer
};

/** 
Enum with record members. */
enum ads_ElementRecoveryMatrixMembersEnm
{
    ads_ElementRecoveryMatrix_position
};

enum ads_ElementRecoveryMatrix_positionEnm
{
    ads_ElementRecoveryMatrix_position_AVEREGED,
    ads_ElementRecoveryMatrix_position_CENTROIDAL
};

/** 
Enum with association roles. */
enum ads_ElementRecoveryMatrix_regionRolesEnm
{
    ads_ElementRecoveryMatrix_region_referent,
    ads_ElementRecoveryMatrix_region_referrer
};

/** 
Enum with association roles. */
enum ads_ElementRecoveryMatrix_sectionPointsRolesEnm
{
    ads_ElementRecoveryMatrix_sectionPoints_referent,
    ads_ElementRecoveryMatrix_sectionPoints_referrer
};

/** 
Enum with association roles. */
enum ads_ElementRecoveryMatrix_selectiveElementsRolesEnm
{
    ads_ElementRecoveryMatrix_selectiveElements_referent,
    ads_ElementRecoveryMatrix_selectiveElements_referrer
};

/** 
Enum with association roles. */
enum ads_ElementRecoveryMatrix_strainRolesEnm
{
    ads_ElementRecoveryMatrix_strain_referent,
    ads_ElementRecoveryMatrix_strain_referrer
};

/** 
Enum with association roles. */
enum ads_ElementRecoveryMatrix_stressRolesEnm
{
    ads_ElementRecoveryMatrix_stress_referent,
    ads_ElementRecoveryMatrix_stress_referrer
};

/** 
Enum with record members. */
enum ads_FrequencyRangeMembersEnm
{
    ads_FrequencyRange_maximum,
    ads_FrequencyRange_minimum
};

/** 
Enum with record members. */
enum ads_GlobalDampingMembersEnm
{
    ads_GlobalDamping_alpha,
    ads_GlobalDamping_alphaAcoustic,
    ads_GlobalDamping_beta,
    ads_GlobalDamping_betaAcoustic,
    ads_GlobalDamping_structural,
    ads_GlobalDamping_structuralAcoustic
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_DirectMembersEnm
{
    ads_LinearEquationSolver_Direct_optimizeConstraints
};

enum ads_LinearEquationSolver_Direct_optimizeConstraintsEnm
{
    ads_LinearEquationSolver_Direct_optimizeConstraints_ACTIVE_WITHOUT_RBM,
    ads_LinearEquationSolver_Direct_optimizeConstraints_ACTIVE_WITH_RBM,
    ads_LinearEquationSolver_Direct_optimizeConstraints_INACTIVE
};

/** 
Enum with record members. */
enum ads_LinearEquationSolver_IterativeMembersEnm
{
    ads_LinearEquationSolver_Iterative_maximumNumberOfIterations,
    ads_LinearEquationSolver_Iterative_numberOfDomains,
    ads_LinearEquationSolver_Iterative_relativeTolerance
};

/** 
Enum with record members. */
enum ads_MatrixCheckMembersEnm
{
    ads_MatrixCheck_errJob,
    ads_MatrixCheck_massConditionMinimum,
    ads_MatrixCheck_massDiagonalTolerance,
    ads_MatrixCheck_massTolerance,
    ads_MatrixCheck_seTol,
    ads_MatrixCheck_stiffnessConditionMinimum,
    ads_MatrixCheck_stiffnessDiagonalTolerance,
    ads_MatrixCheck_stiffnessTolerance
};

/** 
Enum with association roles. */
enum ads_MatrixCheck_referencePointRolesEnm
{
    ads_MatrixCheck_referencePoint_child,
    ads_MatrixCheck_referencePoint_parent
};

/** 
Enum with record members. */
enum ads_MatrixExportMembersEnm
{
    ads_MatrixExport_format,
    ads_MatrixExport_modeShapes,
    ads_MatrixExport_workflow
};

enum ads_MatrixExport_formatEnm
{
    ads_MatrixExport_format_OP4,
    ads_MatrixExport_format_SIM
};

enum ads_MatrixExport_workflowEnm
{
    ads_MatrixExport_workflow_MODAL,
    ads_MatrixExport_workflow_ZAERO
};

/** 
Enum with record members. */
enum ads_MatrixInstancesMembersEnm
{
    ads_MatrixInstances_mass,
    ads_MatrixInstances_stiffness,
    ads_MatrixInstances_structuralDamping,
    ads_MatrixInstances_viscousDamping
};

enum ads_MatrixInstances_massEnm
{
    ads_MatrixInstances_mass_AUTOMATIC,
    ads_MatrixInstances_mass_BOTH,
    ads_MatrixInstances_mass_SYMMETRIC,
    ads_MatrixInstances_mass_UNSYMMETRIC
};

enum ads_MatrixInstances_stiffnessEnm
{
    ads_MatrixInstances_stiffness_AUTOMATIC,
    ads_MatrixInstances_stiffness_BOTH,
    ads_MatrixInstances_stiffness_SYMMETRIC,
    ads_MatrixInstances_stiffness_UNSYMMETRIC
};

enum ads_MatrixInstances_structuralDampingEnm
{
    ads_MatrixInstances_structuralDamping_AUTOMATIC,
    ads_MatrixInstances_structuralDamping_BOTH,
    ads_MatrixInstances_structuralDamping_SYMMETRIC,
    ads_MatrixInstances_structuralDamping_UNSYMMETRIC
};

enum ads_MatrixInstances_viscousDampingEnm
{
    ads_MatrixInstances_viscousDamping_AUTOMATIC,
    ads_MatrixInstances_viscousDamping_BOTH,
    ads_MatrixInstances_viscousDamping_SYMMETRIC,
    ads_MatrixInstances_viscousDamping_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_MatrixOutputMembersEnm
{
    ads_MatrixOutput_file,
    ads_MatrixOutput_format,
    ads_MatrixOutput_gravityLoad,
    ads_MatrixOutput_load,
    ads_MatrixOutput_mass,
    ads_MatrixOutput_recovery,
    ads_MatrixOutput_stiffness,
    ads_MatrixOutput_structuralDamping,
    ads_MatrixOutput_symmetric,
    ads_MatrixOutput_viscousDamping
};

enum ads_MatrixOutput_formatEnm
{
    ads_MatrixOutput_format_COORD,
    ads_MatrixOutput_format_DMIG,
    ads_MatrixOutput_format_LABELS,
    ads_MatrixOutput_format_MINP,
    ads_MatrixOutput_format_OP4BINARY,
    ads_MatrixOutput_format_OP4TEXT,
    ads_MatrixOutput_format_USER
};

/** 
Enum with record members. */
enum ads_ModalDampingMembersEnm
{
    ads_ModalDamping_definition,
    ads_ModalDamping_field,
    ads_ModalDamping_type
};

enum ads_ModalDamping_definitionEnm
{
    ads_ModalDamping_definition_FREQUENCY,
    ads_ModalDamping_definition_MODES
};

enum ads_ModalDamping_fieldEnm
{
    ads_ModalDamping_field_ACOUSTIC,
    ads_ModalDamping_field_ALL,
    ads_ModalDamping_field_MECHANICAL
};

enum ads_ModalDamping_typeEnm
{
    ads_ModalDamping_type_STRUCTURAL,
    ads_ModalDamping_type_VISCOUS_COMPOSITE,
    ads_ModalDamping_type_VISCOUS_FRACTION,
    ads_ModalDamping_type_VISCOUS_RAYLEIGH
};

/** 
Enum with association roles. */
enum ads_ModalDamping_dataRolesEnm
{
    ads_ModalDamping_data_child,
    ads_ModalDamping_data_parent
};

/** 
Enum with association roles. */
enum ads_Model_elaborationTaskRolesEnm
{
    ads_Model_elaborationTask_referent,
    ads_Model_elaborationTask_referrer
};

/** 
Enum with association roles. */
enum ads_Model_initialTaskRolesEnm
{
    ads_Model_initialTask_referent,
    ads_Model_initialTask_referrer
};

/** 
Enum with association roles. */
enum ads_Model_stepSequenceRolesEnm
{
    ads_Model_stepSequence_referent,
    ads_Model_stepSequence_referrer
};

/** 
Enum with association roles. */
enum ads_Model_taskRolesEnm
{
    ads_Model_task_child,
    ads_Model_task_parent
};

/** 
Enum with record members. */
enum ads_ReferencePointMembersEnm
{
    ads_ReferencePoint_X,
    ads_ReferencePoint_Y,
    ads_ReferencePoint_Z
};

/** Enum with record members. */
enum ads_RegionActivationRelayMembersEnm
{
    ads_RegionActivationRelay_status
};

enum ads_RegionActivationRelay_statusEnm
{
    ads_RegionActivationRelay_status_ADDED_STRAIN_FREE,
    ads_RegionActivationRelay_status_ADDED_WITH_STRAIN,
    ads_RegionActivationRelay_status_CREATED,
    ads_RegionActivationRelay_status_PROPAGATED,
    ads_RegionActivationRelay_status_REMOVED
};

/** Enum with association roles. */
enum ads_RegionActivationRelay_regionActivationRolesEnm
{
    ads_RegionActivationRelay_regionActivation_referent,
    ads_RegionActivationRelay_regionActivation_referrer
};

/** 
Enum with record members. */
enum ads_Step_GenMembersEnm
{
    ads_Step_Gen_designSensitivity,
    ads_Step_Gen_dsa,
    ads_Step_Gen_beginningTime
};

enum ads_Step_Gen_designSensitivityEnm
{
    ads_Step_Gen_designSensitivity_ADJOINT,
    ads_Step_Gen_designSensitivity_NONE
};

/** 
Enum with association roles. */
enum ads_Step_Gen_previousStepRolesEnm
{
    ads_Step_Gen_previousStep_referent,
    ads_Step_Gen_previousStep_referrer
};

/** 
Enum with record members. */
enum ads_Step_LinMembersEnm
{
    ads_Step_Lin_designSensitivity,
    ads_Step_Lin_dsa
};

enum ads_Step_Lin_designSensitivityEnm
{
    ads_Step_Lin_designSensitivity_ADJOINT,
    ads_Step_Lin_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Lin_BeamSectionGenerationMembersEnm
{
    ads_Step_Lin_BeamSectionGeneration_designSensitivity,
    ads_Step_Lin_BeamSectionGeneration_dsa
};

enum ads_Step_Lin_BeamSectionGeneration_designSensitivityEnm
{
    ads_Step_Lin_BeamSectionGeneration_designSensitivity_ADJOINT,
    ads_Step_Lin_BeamSectionGeneration_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Lin_BuckleMembersEnm
{
    ads_Step_Lin_Buckle_designSensitivity,
    ads_Step_Lin_Buckle_dsa
};

enum ads_Step_Lin_Buckle_designSensitivityEnm
{
    ads_Step_Lin_Buckle_designSensitivity_ADJOINT,
    ads_Step_Lin_Buckle_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Lin_ComplexFrequencyMembersEnm
{
    ads_Step_Lin_ComplexFrequency_designSensitivity,
    ads_Step_Lin_ComplexFrequency_dsa,
    ads_Step_Lin_ComplexFrequency_frictionDamping,
    ads_Step_Lin_ComplexFrequency_matrixSymmetry,
    ads_Step_Lin_ComplexFrequency_maximumFrequencyOfInterest,
    ads_Step_Lin_ComplexFrequency_minimumFrequencyOfInterest,
    ads_Step_Lin_ComplexFrequency_numberOfComplexEigenmodes,
    ads_Step_Lin_ComplexFrequency_propertyEvaluation,
    ads_Step_Lin_ComplexFrequency_shiftPoint,
    ads_Step_Lin_ComplexFrequency_unstableModesOnly
};

enum ads_Step_Lin_ComplexFrequency_designSensitivityEnm
{
    ads_Step_Lin_ComplexFrequency_designSensitivity_ADJOINT,
    ads_Step_Lin_ComplexFrequency_designSensitivity_NONE
};

enum ads_Step_Lin_ComplexFrequency_matrixSymmetryEnm
{
    ads_Step_Lin_ComplexFrequency_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_ComplexFrequency_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_ComplexFrequency_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_Step_Lin_ComplexFrequency_exportRolesEnm
{
    ads_Step_Lin_ComplexFrequency_export_child,
    ads_Step_Lin_ComplexFrequency_export_parent
};

/** 
Enum with record members. */
enum ads_Step_Lin_FrequencyMembersEnm
{
    ads_Step_Lin_Frequency_designSensitivity,
    ads_Step_Lin_Frequency_dsa,
    ads_Step_Lin_Frequency_flexibleBody,
    ads_Step_Lin_Frequency_propertyEvaluation
};

enum ads_Step_Lin_Frequency_designSensitivityEnm
{
    ads_Step_Lin_Frequency_designSensitivity_ADJOINT,
    ads_Step_Lin_Frequency_designSensitivity_NONE
};

enum ads_Step_Lin_Frequency_flexibleBodyEnm
{
    ads_Step_Lin_Frequency_flexibleBody_GENERIC,
    ads_Step_Lin_Frequency_flexibleBody_NONE,
    ads_Step_Lin_Frequency_flexibleBody_SID,
    ads_Step_Lin_Frequency_flexibleBody_SIMPACK
};

/** 
Enum with association roles. */
enum ads_Step_Lin_Frequency_compositeModalDampingRolesEnm
{
    ads_Step_Lin_Frequency_compositeModalDamping_child,
    ads_Step_Lin_Frequency_compositeModalDamping_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_Frequency_cyclicSymmetryModeSelectionRolesEnm
{
    ads_Step_Lin_Frequency_cyclicSymmetryModeSelection_child,
    ads_Step_Lin_Frequency_cyclicSymmetryModeSelection_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_Frequency_secondaryBaseNodalDofsRolesEnm
{
    ads_Step_Lin_Frequency_secondaryBaseNodalDofs_referent,
    ads_Step_Lin_Frequency_secondaryBaseNodalDofs_referrer
};

/** 
Enum with record members. */
enum ads_Step_Lin_MatrixGenerationMembersEnm
{
    ads_Step_Lin_MatrixGeneration_designSensitivity,
    ads_Step_Lin_MatrixGeneration_dsa,
    ads_Step_Lin_MatrixGeneration_checkMatrices,
    ads_Step_Lin_MatrixGeneration_elementByElement,
    ads_Step_Lin_MatrixGeneration_field,
    ads_Step_Lin_MatrixGeneration_frictionDamping,
    ads_Step_Lin_MatrixGeneration_load,
    ads_Step_Lin_MatrixGeneration_mass,
    ads_Step_Lin_MatrixGeneration_mpc,
    ads_Step_Lin_MatrixGeneration_name,
    ads_Step_Lin_MatrixGeneration_propertyEvaluation,
    ads_Step_Lin_MatrixGeneration_solidInfiniteFormulation,
    ads_Step_Lin_MatrixGeneration_source,
    ads_Step_Lin_MatrixGeneration_stiffness,
    ads_Step_Lin_MatrixGeneration_structuralDamping,
    ads_Step_Lin_MatrixGeneration_symmetric,
    ads_Step_Lin_MatrixGeneration_transformation,
    ads_Step_Lin_MatrixGeneration_viscousDamping
};

enum ads_Step_Lin_MatrixGeneration_designSensitivityEnm
{
    ads_Step_Lin_MatrixGeneration_designSensitivity_ADJOINT,
    ads_Step_Lin_MatrixGeneration_designSensitivity_NONE
};

enum ads_Step_Lin_MatrixGeneration_fieldEnm
{
    ads_Step_Lin_MatrixGeneration_field_ACOUSTIC,
    ads_Step_Lin_MatrixGeneration_field_ALL,
    ads_Step_Lin_MatrixGeneration_field_ASI,
    ads_Step_Lin_MatrixGeneration_field_MECHANICAL
};

enum ads_Step_Lin_MatrixGeneration_solidInfiniteFormulationEnm
{
    ads_Step_Lin_MatrixGeneration_solidInfiniteFormulation_DYNAMIC,
    ads_Step_Lin_MatrixGeneration_solidInfiniteFormulation_STATIC
};

enum ads_Step_Lin_MatrixGeneration_sourceEnm
{
    ads_Step_Lin_MatrixGeneration_source_ALL,
    ads_Step_Lin_MatrixGeneration_source_ELEMENTS,
    ads_Step_Lin_MatrixGeneration_source_MATRIX_INPUT
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_checkRefPointRolesEnm
{
    ads_Step_Lin_MatrixGeneration_checkRefPoint_child,
    ads_Step_Lin_MatrixGeneration_checkRefPoint_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_elsetRolesEnm
{
    ads_Step_Lin_MatrixGeneration_elset_referent,
    ads_Step_Lin_MatrixGeneration_elset_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_exportRolesEnm
{
    ads_Step_Lin_MatrixGeneration_export_child,
    ads_Step_Lin_MatrixGeneration_export_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_interfaceNodesRolesEnm
{
    ads_Step_Lin_MatrixGeneration_interfaceNodes_referent,
    ads_Step_Lin_MatrixGeneration_interfaceNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_matrixCheckRolesEnm
{
    ads_Step_Lin_MatrixGeneration_matrixCheck_child,
    ads_Step_Lin_MatrixGeneration_matrixCheck_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_matrixPlotRolesEnm
{
    ads_Step_Lin_MatrixGeneration_matrixPlot_child,
    ads_Step_Lin_MatrixGeneration_matrixPlot_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_outputRolesEnm
{
    ads_Step_Lin_MatrixGeneration_output_child,
    ads_Step_Lin_MatrixGeneration_output_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_MatrixGeneration_publicNodesRolesEnm
{
    ads_Step_Lin_MatrixGeneration_publicNodes_referent,
    ads_Step_Lin_MatrixGeneration_publicNodes_referrer
};

/** 
Enum with record members. */
enum ads_Step_Lin_ReducedBasisGenerationMembersEnm
{
    ads_Step_Lin_ReducedBasisGeneration_designSensitivity,
    ads_Step_Lin_ReducedBasisGeneration_dsa,
    ads_Step_Lin_ReducedBasisGeneration_deformationVectorSingularValuesCutOff,
    ads_Step_Lin_ReducedBasisGeneration_flexibleBody,
    ads_Step_Lin_ReducedBasisGeneration_name,
    ads_Step_Lin_ReducedBasisGeneration_residualVectorSingularValuesCutOff
};

enum ads_Step_Lin_ReducedBasisGeneration_designSensitivityEnm
{
    ads_Step_Lin_ReducedBasisGeneration_designSensitivity_ADJOINT,
    ads_Step_Lin_ReducedBasisGeneration_designSensitivity_NONE
};

enum ads_Step_Lin_ReducedBasisGeneration_flexibleBodyEnm
{
    ads_Step_Lin_ReducedBasisGeneration_flexibleBody_NONE,
    ads_Step_Lin_ReducedBasisGeneration_flexibleBody_SIMPACK
};

/** 
Enum with record members. */
enum ads_Step_Lin_SubstructureGenerationMembersEnm
{
    ads_Step_Lin_SubstructureGeneration_designSensitivity,
    ads_Step_Lin_SubstructureGeneration_dsa,
    ads_Step_Lin_SubstructureGeneration_checkMatrices,
    ads_Step_Lin_SubstructureGeneration_eigenproblem,
    ads_Step_Lin_SubstructureGeneration_flexibleBody,
    ads_Step_Lin_SubstructureGeneration_frictionDamping,
    ads_Step_Lin_SubstructureGeneration_gravityLoad,
    ads_Step_Lin_SubstructureGeneration_library,
    ads_Step_Lin_SubstructureGeneration_mass,
    ads_Step_Lin_SubstructureGeneration_matrixSymmetry,
    ads_Step_Lin_SubstructureGeneration_modelData,
    ads_Step_Lin_SubstructureGeneration_name,
    ads_Step_Lin_SubstructureGeneration_overwrite,
    ads_Step_Lin_SubstructureGeneration_propertyEvaluation,
    ads_Step_Lin_SubstructureGeneration_recovery,
    ads_Step_Lin_SubstructureGeneration_reducedFormulation,
    ads_Step_Lin_SubstructureGeneration_residualModes,
    ads_Step_Lin_SubstructureGeneration_solidInfiniteFormulation,
    ads_Step_Lin_SubstructureGeneration_stiffness,
    ads_Step_Lin_SubstructureGeneration_structuralDamping,
    ads_Step_Lin_SubstructureGeneration_symmetric,
    ads_Step_Lin_SubstructureGeneration_type,
    ads_Step_Lin_SubstructureGeneration_viscousDamping
};

enum ads_Step_Lin_SubstructureGeneration_designSensitivityEnm
{
    ads_Step_Lin_SubstructureGeneration_designSensitivity_ADJOINT,
    ads_Step_Lin_SubstructureGeneration_designSensitivity_NONE
};

enum ads_Step_Lin_SubstructureGeneration_flexibleBodyEnm
{
    ads_Step_Lin_SubstructureGeneration_flexibleBody_ADAMS,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_EXCITE,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_EXCITE_SMALL_MOTION,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_GENERIC,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_NONE,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_SID,
    ads_Step_Lin_SubstructureGeneration_flexibleBody_SIMPACK
};

enum ads_Step_Lin_SubstructureGeneration_matrixSymmetryEnm
{
    ads_Step_Lin_SubstructureGeneration_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_SubstructureGeneration_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_SubstructureGeneration_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Lin_SubstructureGeneration_modelDataEnm
{
    ads_Step_Lin_SubstructureGeneration_modelData_AUTO,
    ads_Step_Lin_SubstructureGeneration_modelData_BOTH,
    ads_Step_Lin_SubstructureGeneration_modelData_NONE,
    ads_Step_Lin_SubstructureGeneration_modelData_ODB,
    ads_Step_Lin_SubstructureGeneration_modelData_SIM
};

enum ads_Step_Lin_SubstructureGeneration_solidInfiniteFormulationEnm
{
    ads_Step_Lin_SubstructureGeneration_solidInfiniteFormulation_DYNAMIC,
    ads_Step_Lin_SubstructureGeneration_solidInfiniteFormulation_STATIC
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_checkRefPointRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_checkRefPoint_child,
    ads_Step_Lin_SubstructureGeneration_checkRefPoint_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_displayElementsRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_displayElements_referent,
    ads_Step_Lin_SubstructureGeneration_displayElements_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_displayElsetRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_displayElset_referent,
    ads_Step_Lin_SubstructureGeneration_displayElset_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_elementRecoveryRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_elementRecovery_child,
    ads_Step_Lin_SubstructureGeneration_elementRecovery_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_matrixCheckRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_matrixCheck_child,
    ads_Step_Lin_SubstructureGeneration_matrixCheck_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_matrixInstancesRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_matrixInstances_child,
    ads_Step_Lin_SubstructureGeneration_matrixInstances_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_monitorNodesRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_monitorNodes_referent,
    ads_Step_Lin_SubstructureGeneration_monitorNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_outputRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_output_child,
    ads_Step_Lin_SubstructureGeneration_output_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_recoveryElementsRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_recoveryElements_referent,
    ads_Step_Lin_SubstructureGeneration_recoveryElements_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_recoveryElsetRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_recoveryElset_referent,
    ads_Step_Lin_SubstructureGeneration_recoveryElset_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_recoveryNodesRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_recoveryNodes_referent,
    ads_Step_Lin_SubstructureGeneration_recoveryNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_recoveryNsetRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_recoveryNset_referent,
    ads_Step_Lin_SubstructureGeneration_recoveryNset_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_regionRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_region_referent,
    ads_Step_Lin_SubstructureGeneration_region_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_retainedNodalDofsRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_retainedNodalDofs_referent,
    ads_Step_Lin_SubstructureGeneration_retainedNodalDofs_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_retainedNodesRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_retainedNodes_referent,
    ads_Step_Lin_SubstructureGeneration_retainedNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_SubstructureGeneration_supDataRolesEnm
{
    ads_Step_Lin_SubstructureGeneration_supData_child,
    ads_Step_Lin_SubstructureGeneration_supData_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_casesRolesEnm
{
    ads_Step_Lin_cases_referent,
    ads_Step_Lin_cases_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_dampingControlsRolesEnm
{
    ads_Step_Lin_dampingControls_child,
    ads_Step_Lin_dampingControls_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_eigenmodeSelectionRolesEnm
{
    ads_Step_Lin_eigenmodeSelection_child,
    ads_Step_Lin_eigenmodeSelection_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_globalDampingRolesEnm
{
    ads_Step_Lin_globalDamping_child,
    ads_Step_Lin_globalDamping_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_modalDampingRolesEnm
{
    ads_Step_Lin_modalDamping_child,
    ads_Step_Lin_modalDamping_parent
};

/** 
Enum with record members. */
enum ads_SubstructureChangeMembersEnm
{
    ads_SubstructureChange_stiffnessUnsymmetry
};

/** 
Enum with association roles. */
enum ads_SubstructureChange_elsetRolesEnm
{
    ads_SubstructureChange_elset_referent,
    ads_SubstructureChange_elset_referrer
};

/** 
Enum with record members. */
enum ads_SubstructureOutputMembersEnm
{
    ads_SubstructureOutput_derivatives,
    ads_SubstructureOutput_format,
    ads_SubstructureOutput_print
};

enum ads_SubstructureOutput_formatEnm
{
    ads_SubstructureOutput_format_MDF,
    ads_SubstructureOutput_format_OP4,
    ads_SubstructureOutput_format_SIM
};

/** 
Enum with association roles. */
enum ads_SubstructureOutput_elsetRolesEnm
{
    ads_SubstructureOutput_elset_referent,
    ads_SubstructureOutput_elset_referrer
};

/** 
Enum with record members. */
enum ads_TaskMembersEnm
{
    ads_Task_designSensitivity,
    ads_Task_dsa
};

enum ads_Task_designSensitivityEnm
{
    ads_Task_designSensitivity_ADJOINT,
    ads_Task_designSensitivity_NONE
};

/** 
Enum with association roles. */
enum ads_Task_constrainedNodalDofsRolesEnm
{
    ads_Task_constrainedNodalDofs_referent,
    ads_Task_constrainedNodalDofs_referrer
};

/** 
Enum with association roles. */
enum ads_Task_inactiveElementsRolesEnm
{
    ads_Task_inactiveElements_referent,
    ads_Task_inactiveElements_referrer
};

/** Enum with association roles. */
enum ads_Task_linearEquationSolverRolesEnm
{
    ads_Task_linearEquationSolver_child,
    ads_Task_linearEquationSolver_parent
};

/** 
Enum with association roles. */
enum ads_Task_modesRolesEnm
{
    ads_Task_modes_referent,
    ads_Task_modes_referrer
};

/** Enum with association roles. */
enum ads_Task_regionActivationRelaysRolesEnm
{
    ads_Task_regionActivationRelays_child,
    ads_Task_regionActivationRelays_parent
};

/** 
Enum with association roles. */
enum ads_Task_substructureChangeRolesEnm
{
    ads_Task_substructureChange_child,
    ads_Task_substructureChange_parent
};

/** 
Enum with association roles. */
enum ads_Task_substructureOutputRolesEnm
{
    ads_Task_substructureOutput_child,
    ads_Task_substructureOutput_parent
};

/** 
Enum with association roles. */
enum ads_Task_timesRolesEnm
{
    ads_Task_times_referent,
    ads_Task_times_referrer
};

#endif
