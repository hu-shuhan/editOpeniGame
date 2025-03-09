//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreDiagnosticsC_h
#define ads_CoreDiagnosticsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Diagnostics of the latest level of form Core */

/** Attempt is a sequence of iterations aiming to find equilibrium solution in an increment. */
#define ads_Attempt (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 0))

/** A collection of attempts. */
#define ads_AttemptCollection (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 1))

/** A sequence of iteration in an attempt to find an equilibrium solution. */
#define ads_Attempt_iterations (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 2))

/** The Increment/Attempt/Iteration of a diagnostic. The minOccurs is 0 because not all Conditions are Diagnostics. */
#define ads_Condition_frame (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 3))

/** Base record for diagnostic parameters. */
#define ads_DiagnosticControls (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 4))

/** Diagnostic parameters for CFD. */
#define ads_DiagnosticControls_CFD (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 5))

/** Diagnostic parameters for Explicit. */
#define ads_DiagnosticControls_Explicit (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 6))

/** Diagnostic parameters for Standard. */
#define ads_DiagnosticControls_Standard (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 7))

/** An identifier for an occasion at which a diagnostic can occur. A given DiagnosticFrame relates to exactly one of the following three; (i) a Time, (ii) an Iteration (iii) a ContactIteration. Three different DSets are used to indicate which of the above three a given DiagnosticFrame is related to. A given DiagnosticFrame must participate in exactly one of the three DSets. Note that DiagnosticFrame is not associated with Attempts. This is only because no diagnostics are currently issued at the level of attempts. */
#define ads_DiagnosticFrame (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 8))

/** A collection of diagnosticFrames. */
#define ads_DiagnosticFrameCollection (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 9))

/** A grid of DiagnosticFrames. */
#define ads_DiagnosticFrameGrid (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 10))

/** Formalizes the fact Increment, Attempt, and Iteration are all "ticks" in the analysis "ticker". "Frame" is the base type of all three. In the future we would like to unify the SIM diagnostics ticker (Frame) with the SIM results ticker (Time, ArcLenght, and FrequencyPoint.) Both a record-based and an index-based representation of the Frame are used. The records are used to carry data (for example, the Iteration's Boolean "isConverged". The c-members are used in the discrete domain of Fields related with Residuals (more on that later). */
#define ads_Frame (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 11))

/** See the Frame documentation. */
#define ads_FrameCollection (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 12))

/** A grid of Frames x Elements. */
#define ads_FrameElementGrid (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 13))

/** A grid of Frames. */
#define ads_FrameGrid (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 14))

/** A grid of Frames x Nodes x DofTypes. */
#define ads_FrameNodeDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 15))

/** A grid of Frames x Nodes. */
#define ads_FrameNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 16))

/** An increment is part of a step. In nonlinear analyses each step is broken into increments so that the nonlinear solution path can be followed. */
#define ads_Increment (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 17))

/** An increment can have a number of attempts to find an equilibrium solution. */
#define ads_Increment_attempts (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 18))

/** Iteration is an attempt at finding an equilibrium solution in an increment. */
#define ads_Iteration (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 19))

/** A collection of iterations. */
#define ads_IterationCollection (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 20))

/** Diagnostic summary for the whole job. */
#define ads_JobDiagnosticSummary (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 21))

/** Diagnostic controls defined in the model. */
#define ads_Model_diagnosticControls (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 22))

/** Model level diagnostic conditions (ass opposed to Task level diagnostics) */
#define ads_Model_diagnostics (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 23))

/** Residual data (residuals/correction) of a conjugate pair */
#define ads_Residual (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 24))

/** Average flux values */
#define ads_Residual_averageFlux (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 25))

/** Correction in the solution during interations */
#define ads_Residual_correction (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 26))

/** Bools (ints) indicating if the correction is accepted */
#define ads_Residual_correctionCheck (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 27))

/** Total increment of the solution during iteration */
#define ads_Residual_increment (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 28))

/** For each step, we keep a dset expressing the maxCorrection location in Frame x Node x DofType relationship. */
#define ads_Residual_maxCorrectionNodeDof (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 29))

/** For each step, we keep a dset expressing the maxIncrement location in Frame x Node x DofType relationship. */
#define ads_Residual_maxIncrementNodeDof (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 30))

/** For each step, we keep a dset expressing the maxResidual location in Frame x Node x DofType relationship. */
#define ads_Residual_maxResidualNodeDof (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 31))

/** "Force" residual values for a step (iterations) */
#define ads_Residual_residual (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 32))

/** Bools (ints) indicating if the residual is accepted */
#define ads_Residual_residualCheck (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 33))

/** Time average flux values */
#define ads_Residual_timeAverageFlux (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 34))

/** Time of the step at the beginning of an attempt */
#define ads_Task_attemptTime (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 35))

/** Diagnostic controls defined for the task. */
#define ads_Task_diagnosticControls (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 36))

/** Diagnostic fields in a step. */
#define ads_Task_diagnosticField (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 37))

/** A collection of diagnosticFrames. */
#define ads_Task_diagnosticFramesSet (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 38))

/** In this Diagnostics data model (as oppposed to the ODB one, for example) we are adopting a flat structure. All diagnostics (warnings, errors, element diagnostics, etc.) are stored in the same container. Separation by type (e.g.: "all warnings in a step") or by frame (e.g.: "all diagnostics of an iteration") should be obtained through services. */
#define ads_Task_diagnostics (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 39))

/** A collection of frames does not extend across more than one step. */
#define ads_Task_frames (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 40))

/** Increments in a step. */
#define ads_Task_increments (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 41))

/** Residuals in a step. */
#define ads_Task_residuals (ads_CoreFragmentTypeIndex(ads_CoreDiagnosticsFragment, 42))

/** 
Enum with record members. */
enum ads_AttemptMembersEnm
{
    ads_Attempt_needsReordering
};

/** 
Enum with association roles. */
enum ads_Attempt_iterationsRolesEnm
{
    ads_Attempt_iterations_child,
    ads_Attempt_iterations_parent
};

/** 
Enum with association roles. */
enum ads_Condition_frameRolesEnm
{
    ads_Condition_frame_referent,
    ads_Condition_frame_referrer
};

/** 
Enum with record members. */
enum ads_DiagnosticControls_CFDMembersEnm
{
    ads_DiagnosticControls_CFD_unconvergedDiagnostics
};

/** 
Enum with record members. */
enum ads_DiagnosticControls_ExplicitMembersEnm
{
    ads_DiagnosticControls_Explicit_adaptiveMesh,
    ads_DiagnosticControls_Explicit_contactInitialOverclosureDetail,
    ads_DiagnosticControls_Explicit_criticalElements,
    ads_DiagnosticControls_Explicit_cutoffRatio,
    ads_DiagnosticControls_Explicit_deepPenetrationFactor,
    ads_DiagnosticControls_Explicit_deformationSpeedCheck,
    ads_DiagnosticControls_Explicit_detectCrossedSurfaces,
    ads_DiagnosticControls_Explicit_largeImplicitSystemError,
    ads_DiagnosticControls_Explicit_plasticity,
    ads_DiagnosticControls_Explicit_warningRatio,
    ads_DiagnosticControls_Explicit_warpedSurface
};

enum ads_DiagnosticControls_Explicit_adaptiveMeshEnm
{
    ads_DiagnosticControls_Explicit_adaptiveMesh_DETAIL,
    ads_DiagnosticControls_Explicit_adaptiveMesh_STEP_SUMMARY,
    ads_DiagnosticControls_Explicit_adaptiveMesh_SUMMARY
};

enum ads_DiagnosticControls_Explicit_deformationSpeedCheckEnm
{
    ads_DiagnosticControls_Explicit_deformationSpeedCheck_DETAIL,
    ads_DiagnosticControls_Explicit_deformationSpeedCheck_OFF,
    ads_DiagnosticControls_Explicit_deformationSpeedCheck_SUMMARY
};

enum ads_DiagnosticControls_Explicit_plasticityEnm
{
    ads_DiagnosticControls_Explicit_plasticity_DETAIL,
    ads_DiagnosticControls_Explicit_plasticity_OFF,
    ads_DiagnosticControls_Explicit_plasticity_SUMMARY
};

enum ads_DiagnosticControls_Explicit_warpedSurfaceEnm
{
    ads_DiagnosticControls_Explicit_warpedSurface_DETAIL,
    ads_DiagnosticControls_Explicit_warpedSurface_OFF,
    ads_DiagnosticControls_Explicit_warpedSurface_SUMMARY
};

/** 
Enum with record members. */
enum ads_DiagnosticControls_StandardMembersEnm
{
    ads_DiagnosticControls_Standard_nonhybridIncompressibleError,
    ads_DiagnosticControls_Standard_oldContactControlsError
};

/** 
Enum with grid dimensions. */
enum ads_DiagnosticFrameGridDimensionsEnm
{
    ads_DiagnosticFrameGrid_diagnosticFrame
};

/** 
Enum with grid dimensions. */
enum ads_FrameElementGridDimensionsEnm
{
    ads_FrameElementGrid_element,
    ads_FrameElementGrid_frame
};

/** 
Enum with grid dimensions. */
enum ads_FrameGridDimensionsEnm
{
    ads_FrameGrid_frame
};

/** 
Enum with grid dimensions. */
enum ads_FrameNodeDofTypeGridDimensionsEnm
{
    ads_FrameNodeDofTypeGrid_dofType,
    ads_FrameNodeDofTypeGrid_frame,
    ads_FrameNodeDofTypeGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_FrameNodeGridDimensionsEnm
{
    ads_FrameNodeGrid_frame,
    ads_FrameNodeGrid_node
};

/** 
Enum with record members. */
enum ads_IncrementMembersEnm
{
    ads_Increment_number
};

/** 
Enum with association roles. */
enum ads_Increment_attemptsRolesEnm
{
    ads_Increment_attempts_child,
    ads_Increment_attempts_parent
};

/** 
Enum with record members. */
enum ads_IterationMembersEnm
{
    ads_Iteration_converged,
    ads_Iteration_severeDiscontinuity
};

/** 
Enum with record members. */
enum ads_JobDiagnosticSummaryMembersEnm
{
    ads_JobDiagnosticSummary_status,
    ads_JobDiagnosticSummary_systemTime,
    ads_JobDiagnosticSummary_userTime,
    ads_JobDiagnosticSummary_wallclockTime
};

/** 
Enum with association roles. */
enum ads_Model_diagnosticControlsRolesEnm
{
    ads_Model_diagnosticControls_child,
    ads_Model_diagnosticControls_parent
};

/** 
Enum with association roles. */
enum ads_Model_diagnosticsRolesEnm
{
    ads_Model_diagnostics_child,
    ads_Model_diagnostics_parent
};

/** 
Enum with association roles. */
enum ads_Residual_averageFluxRolesEnm
{
    ads_Residual_averageFlux_child,
    ads_Residual_averageFlux_parent
};

/** 
Enum with association roles. */
enum ads_Residual_correctionRolesEnm
{
    ads_Residual_correction_child,
    ads_Residual_correction_parent
};

/** 
Enum with association roles. */
enum ads_Residual_correctionCheckRolesEnm
{
    ads_Residual_correctionCheck_child,
    ads_Residual_correctionCheck_parent
};

/** 
Enum with association roles. */
enum ads_Residual_incrementRolesEnm
{
    ads_Residual_increment_child,
    ads_Residual_increment_parent
};

/** 
Enum with association roles. */
enum ads_Residual_maxCorrectionNodeDofRolesEnm
{
    ads_Residual_maxCorrectionNodeDof_child,
    ads_Residual_maxCorrectionNodeDof_parent
};

/** 
Enum with association roles. */
enum ads_Residual_maxIncrementNodeDofRolesEnm
{
    ads_Residual_maxIncrementNodeDof_child,
    ads_Residual_maxIncrementNodeDof_parent
};

/** 
Enum with association roles. */
enum ads_Residual_maxResidualNodeDofRolesEnm
{
    ads_Residual_maxResidualNodeDof_child,
    ads_Residual_maxResidualNodeDof_parent
};

/** 
Enum with association roles. */
enum ads_Residual_residualRolesEnm
{
    ads_Residual_residual_child,
    ads_Residual_residual_parent
};

/** 
Enum with association roles. */
enum ads_Residual_residualCheckRolesEnm
{
    ads_Residual_residualCheck_child,
    ads_Residual_residualCheck_parent
};

/** 
Enum with association roles. */
enum ads_Residual_timeAverageFluxRolesEnm
{
    ads_Residual_timeAverageFlux_child,
    ads_Residual_timeAverageFlux_parent
};

/** 
Enum with association roles. */
enum ads_Task_attemptTimeRolesEnm
{
    ads_Task_attemptTime_child,
    ads_Task_attemptTime_parent
};

/** 
Enum with association roles. */
enum ads_Task_diagnosticControlsRolesEnm
{
    ads_Task_diagnosticControls_referent,
    ads_Task_diagnosticControls_referrer
};

/** 
Enum with association roles. */
enum ads_Task_diagnosticFieldRolesEnm
{
    ads_Task_diagnosticField_child,
    ads_Task_diagnosticField_parent
};

/** 
Enum with association roles. */
enum ads_Task_diagnosticFramesSetRolesEnm
{
    ads_Task_diagnosticFramesSet_referent,
    ads_Task_diagnosticFramesSet_referrer
};

/** 
Enum with association roles. */
enum ads_Task_diagnosticsRolesEnm
{
    ads_Task_diagnostics_child,
    ads_Task_diagnostics_parent
};

/** 
Enum with association roles. */
enum ads_Task_framesRolesEnm
{
    ads_Task_frames_child,
    ads_Task_frames_parent
};

/** 
Enum with association roles. */
enum ads_Task_incrementsRolesEnm
{
    ads_Task_increments_child,
    ads_Task_increments_parent
};

/** 
Enum with association roles. */
enum ads_Task_residualsRolesEnm
{
    ads_Task_residuals_child,
    ads_Task_residuals_parent
};

#endif
