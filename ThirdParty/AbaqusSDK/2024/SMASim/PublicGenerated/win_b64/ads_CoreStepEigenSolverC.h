//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreStepEigenSolverC_h
#define ads_CoreStepEigenSolverC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment StepEigenSolver of the latest level of form Core */

/** Frequency interval bias record. */
#define ads_BiasFrequencyInterval (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 0))

/** Eigensolver base class record. The current eigensolution options available are subspace, Lanczos and AMS. */
#define ads_Eigensolver (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 1))

/** AMS eigensolver record. */
#define ads_Eigensolver_AMS (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 2))

/** Set identifying the nodes to compute eigenvectors. */
#define ads_Eigensolver_AMS_recoveryNodes (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 3))

/** System dofs for calculating residual modes. */
#define ads_Eigensolver_AMS_residualDofs (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 4))

/** Nodal dofs for calculating residual modes. */
#define ads_Eigensolver_AMS_residualNodeDofs (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 5))

/** Lanczos eigensolver record. */
#define ads_Eigensolver_Lanczos (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 6))

/** Biased frequency interval. */
#define ads_Eigensolver_Lanczos_biasFrequencyInterval (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 7))

/** System dofs for calculating residual modes. */
#define ads_Eigensolver_Lanczos_residualDofs (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 8))

/** User frequency interval. TBD: Switch to f-table if needed -glv */
#define ads_Eigensolver_Lanczos_userFrequencyInterval (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 9))

/** Subspace eigensolver record. */
#define ads_Eigensolver_Subspace (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 10))

/** Buckle task eigensolver. */
#define ads_Step_Lin_Buckle_eigensolver (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 11))

/** Frequency task eigensolver. */
#define ads_Step_Lin_Frequency_eigensolver (ads_CoreFragmentTypeIndex(ads_CoreStepEigenSolverFragment, 12))

/** 
Enum with record members. */
enum ads_BiasFrequencyIntervalMembersEnm
{
    ads_BiasFrequencyInterval_bias,
    ads_BiasFrequencyInterval_numberIntervals
};

/** 
Enum with record members. */
enum ads_Eigensolver_AMSMembersEnm
{
    ads_Eigensolver_AMS_acousticCoupling,
    ads_Eigensolver_AMS_computeResidualModes,
    ads_Eigensolver_AMS_cutoff1,
    ads_Eigensolver_AMS_cutoff2,
    ads_Eigensolver_AMS_cutoff3,
    ads_Eigensolver_AMS_dampingProjection,
    ads_Eigensolver_AMS_maximumFrequency,
    ads_Eigensolver_AMS_minimumFrequency,
    ads_Eigensolver_AMS_numberModes,
    ads_Eigensolver_AMS_propertyEvaluation,
    ads_Eigensolver_AMS_recoveryMode
};

enum ads_Eigensolver_AMS_recoveryModeEnm
{
    ads_Eigensolver_AMS_recoveryMode_AUTOMATED,
    ads_Eigensolver_AMS_recoveryMode_FULL,
    ads_Eigensolver_AMS_recoveryMode_SPECIFIED
};

/** 
Enum with association roles. */
enum ads_Eigensolver_AMS_recoveryNodesRolesEnm
{
    ads_Eigensolver_AMS_recoveryNodes_referent,
    ads_Eigensolver_AMS_recoveryNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Eigensolver_AMS_residualDofsRolesEnm
{
    ads_Eigensolver_AMS_residualDofs_referent,
    ads_Eigensolver_AMS_residualDofs_referrer
};

/** 
Enum with association roles. */
enum ads_Eigensolver_AMS_residualNodeDofsRolesEnm
{
    ads_Eigensolver_AMS_residualNodeDofs_child,
    ads_Eigensolver_AMS_residualNodeDofs_parent
};

/** 
Enum with record members. */
enum ads_Eigensolver_LanczosMembersEnm
{
    ads_Eigensolver_Lanczos_acousticCoupling,
    ads_Eigensolver_Lanczos_blockSize,
    ads_Eigensolver_Lanczos_computeResidualModes,
    ads_Eigensolver_Lanczos_dampingProjection,
    ads_Eigensolver_Lanczos_maximumBlockSteps,
    ads_Eigensolver_Lanczos_maximumFrequency,
    ads_Eigensolver_Lanczos_minimumFrequency,
    ads_Eigensolver_Lanczos_normalization,
    ads_Eigensolver_Lanczos_numberModes,
    ads_Eigensolver_Lanczos_optimizeConstraints,
    ads_Eigensolver_Lanczos_propertyEvaluation,
    ads_Eigensolver_Lanczos_shift,
    ads_Eigensolver_Lanczos_sim
};

enum ads_Eigensolver_Lanczos_normalizationEnm
{
    ads_Eigensolver_Lanczos_normalization_DISPLACEMENT,
    ads_Eigensolver_Lanczos_normalization_MASS
};

enum ads_Eigensolver_Lanczos_optimizeConstraintsEnm
{
    ads_Eigensolver_Lanczos_optimizeConstraints_ACTIVE_WITHOUT_RBM,
    ads_Eigensolver_Lanczos_optimizeConstraints_ACTIVE_WITH_RBM,
    ads_Eigensolver_Lanczos_optimizeConstraints_INACTIVE
};

/** 
Enum with association roles. */
enum ads_Eigensolver_Lanczos_biasFrequencyIntervalRolesEnm
{
    ads_Eigensolver_Lanczos_biasFrequencyInterval_child,
    ads_Eigensolver_Lanczos_biasFrequencyInterval_parent
};

/** 
Enum with association roles. */
enum ads_Eigensolver_Lanczos_residualDofsRolesEnm
{
    ads_Eigensolver_Lanczos_residualDofs_referent,
    ads_Eigensolver_Lanczos_residualDofs_referrer
};

/** 
Enum with association roles. */
enum ads_Eigensolver_Lanczos_userFrequencyIntervalRolesEnm
{
    ads_Eigensolver_Lanczos_userFrequencyInterval_child,
    ads_Eigensolver_Lanczos_userFrequencyInterval_parent
};

/** 
Enum with record members. */
enum ads_Eigensolver_SubspaceMembersEnm
{
    ads_Eigensolver_Subspace_computeResidualModes,
    ads_Eigensolver_Subspace_maximumFrequency,
    ads_Eigensolver_Subspace_maximumIterations,
    ads_Eigensolver_Subspace_normalization,
    ads_Eigensolver_Subspace_numberModes,
    ads_Eigensolver_Subspace_numberVectors,
    ads_Eigensolver_Subspace_propertyEvaluation,
    ads_Eigensolver_Subspace_shift,
    ads_Eigensolver_Subspace_sim
};

enum ads_Eigensolver_Subspace_normalizationEnm
{
    ads_Eigensolver_Subspace_normalization_DISPLACEMENT,
    ads_Eigensolver_Subspace_normalization_MASS
};

/** 
Enum with association roles. */
enum ads_Step_Lin_Buckle_eigensolverRolesEnm
{
    ads_Step_Lin_Buckle_eigensolver_child,
    ads_Step_Lin_Buckle_eigensolver_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_Frequency_eigensolverRolesEnm
{
    ads_Step_Lin_Frequency_eigensolver_child,
    ads_Step_Lin_Frequency_eigensolver_parent
};

#endif
