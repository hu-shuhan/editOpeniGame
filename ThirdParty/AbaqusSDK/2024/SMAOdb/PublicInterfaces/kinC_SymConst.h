//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kinC_SymConst_h
#define kinC_SymConst_h

//
// Enumerated values and symbolic constants for interactions
//

// Begin local includes

#include <mdl_propagationStatus.h>
#include <mdl_propAttributeState.h>
#include <omu_PrimEnum.h>

// Enums

enum kinC_AnalysisTypeEnm
{
    kinC_EXPLICIT_ANALYSIS = 0,
    kinC_STANDARD_ANALYSIS
};

enum kinC_SearchDomainTypeEnm
{
    kinC_WholeModelSearchDomain = 0,
    kinC_InstanceSearchDomain,
    kinC_ViewportSearchDomain,
    kinC_SurfaceSearchDomain,
    kinC_ManualSurfacePick
};

enum kinC_ModelEnm
{
    kinC_MODEL = 0,
    kinC_DISPLAYED_ENTITIES
};

enum kinC_CategoryEnm
{
    kinC_MECHANICAL_INTERACTION,
    kinC_THERMAL_INTERACTION,
    kinC_OTHER_INTERACTION
};

enum kinC_ClearanceTypeEnm
{
    kinC_OMIT,
    kinC_BOLT,
    kinC_UNIFORM
};

enum kinC_SlidingEnm
{
    kinC_SMALL_SLIDING,
    kinC_FINITE_SLIDING
};

enum kinC_SlaveTypeEnm
{
    kinC_SLAVE_SURFACE,
    kinC_SLAVE_SET
};

enum kinC_InterferenceTypeEnm
{
    kinC_NO_INTERFERENCE,
    kinC_SHRINK_FIT_INTERFERENCE,
    kinC_UNIFORM_INTERFERENCE
};

enum kinC_InterferenceDirectionTypeEnm
{
    kinC_COMPUTED_INTERFERENCE_DIRECTION,
    kinC_SPECIFIED_INTERFERENCE_DIRECTION
};

enum kinC_InitialClearanceEnm
{
  kinC_OMITTED_CLEARANCE,
  kinC_COMPUTED_CLEARANCE
};
enum kinC_ComputedEnm
{
  kinC_COMPUTED_DEFAULT
};

enum kinC_CavityRadiationBlockingEnm
{
    kinC_BLOCKING_ALL,
    kinC_BLOCKING_NONE,
    kinC_BLOCKING_PARTIAL
};

enum kinC_DefinitionEnm
{
    kinC_EMBEDDED_COEFF,
    kinC_PROPERTY_REF,
    kinC_USER_SUB,
    kinC_FIELD,
    kinC_DISCRETE_FIELD
};

enum kinC_DistributionEnm
{
    kinC_UNIFORM_DISTRIB,
    kinC_ANALYTICAL_FIELD_DISTRIB,
    kinC_DISCRETE_FIELD_DISTRIB
};

enum kinC_MechanicalConstraintEnm
{
    kinC_KINEMATIC,
    kinC_PENALTY
};

enum kinC_StdAdjustEnm
{
    kinC_ADJUST_NONE,
    kinC_ADJUST_OVERCLOSED,
    kinC_ADJUST_TOLERANCE,
    kinC_ADJUST_SET
};

enum kinC_DefaultOrValueEnm
{
    kinC_DEFAULT,
    kinC_SPECIFIED
};

enum kinC_ExplicitRegionTypeEnm
{
    kinC_LAGRANGIAN,
    kinC_SLIDING,
    kinC_EULERIAN
};

enum kinC_ConstrEnforcementEnm
{
    kinC_NODE_TO_SURFACE,
    kinC_SURFACE_TO_SURFACE
};

enum kinC_AcousticDefEnm
{
    kinC_TABULAR,
    kinC_NONREFLECTING
};

enum kinC_TableTypeEnm
{
    kinC_IMPEDANCE,
    kinC_ADMITTANCE
};

enum kinC_NonreflTypeEnm
{
    kinC_PLANAR,
    kinC_IMPROVED,
    kinC_CIRCULAR,
    kinC_SPHERICAL,
    kinC_ELLIPTICAL,
    kinC_PROLATE
};

enum kinC_IncidentDefEnm
{
    kinC_PRESSURE,
    kinC_ACCELERATION,
    kinC_UNDEX,
    kinC_CONWEP
};

enum kinC_AutoContactDefaultTypeEnm
{
    kinC_CONTACT,
    kinC_TIE,
    kinC_CONTACT_STANDARD,
    kinC_CONTACT_EXPLICIT
};

enum kinC_MeshedGeometrySearchTechniqueEnm
{
    kinC_USE_GEOMETRY,
    kinC_USE_MESH
};

enum kinC_ConstraintPositionEnm
{
    kinC_NODE_CENTERED,
    kinC_FACE_CENTERED
};

enum kinC_ContactTrackingEnm
{
    kinC_ONE_CONFIG,
    kinC_TWO_CONFIG
};

enum kinC_SupplementaryContactEnm
{
    kinC_SELECTIVE,
    kinC_NEVER,
    kinC_ALWAYS
};

enum kinC_SurfaceSmoothingEnm
{
    kinC_NONE,
    kinC_AUTOMATIC,
//     kinC_MASTER,  // NYI in 6.8
//     kinC_SLAVE,  // NYI in 6.8
//     kinC_BOTH,  // NYI in 6.8
    kinC_SMOOTHING_NA // Intentionally not available for Python commands
};

enum kinC_NodalDiameterEnm
{
    kinC_NODAL_DIAMETER_ALL,
    kinC_NODAL_DIAMETER_SPECIFIED
};

enum kinC_CosimulationIncrementationEnum
{
    kinC_INCREMENTATION_NONE,
    kinC_ALLOW_SUBCYCLING,
    kinC_LOCKSTEP
};

enum kinC_CosimulationTimeMarksEnum
{
    kinC_TIMEMARKS_NONE,
    kinC_EXACT_TARGETS,
    kinC_LOOSE_COUPLING
};

enum kinC_CosimulationCouplingEnum
{
    kinC_COUPLING_NONE,
    kinC_GAUSS_COUPLING,
    kinC_JACOBI_COUPLING
};

enum kinC_CosimulationStepSizeEnum
{
    kinC_STEPSIZE_NONE,
    kinC_IMPORT_STEP_SIZE,
    kinC_EXPORT_STEP_SIZE,
    kinC_MAX_STEP_SIZE,
    kinC_MIN_STEP_SIZE,
    kinC_SPECIFY_STEP_SIZE
};

enum kinC_CosimulationSchemeOptionsEnum
{
    kinC_SCHEMEOPTIONS_NONE,
    kinC_LEAD_ANALYSIS,
    kinC_LAG_ANALYSIS,
    kinC_SEND_PREDICTION,
    kinC_RECIEVE_PREDICTION
};

enum kinC_BoltHandednessEnm
{
	kinC_RIGHT_HANDEDNESS,
	kinC_LEFT_HANDEDNESS
};

enum kinC_BoltNormalAdjustmentEnm
{
	kinC_UNIFORM_AXIAL_COMPONENT,
	kinC_LOCATION_DEPENDENT
};

enum kinC_ModelTypeEnm
{
    kinC_MODELTYPE_DEFAULT,
    kinC_MODELTYPE_USER,
    kinC_MODELTYPE_USER_INTERACTION
};

enum kinC_EdgeToEdgeFormulationTypeEnm
{
    kinC_CROSS,
    kinC_RADIAL,
    kinC_BOTH,
    kinC_NO
};

enum kinC_BeamCrossSectionTypeEnm
{
    kinC_CIRCUMSCRIBED_CIRCLE,
    kinC_EXACT
};

/*
enum kinC_ContactMassScalinglocationEnm
{
    kinC_MASS_SCALING_NONE,
    kinC_ELEMENT_MASS_SCALING,
    kinC_ALL_CONTACT_SURFACES,
    kinC_SPECIFIED_SURFACES
};
*/

// Symbolic constants

typedef omu_PrimEnum<kinC_AnalysisTypeEnm,1> kinC_AnalysisTypeConst;
typedef omu_PrimEnum<kinC_AutoContactDefaultTypeEnm,1> kinC_AutoContactDefaultTypeConst;
typedef omu_PrimEnum<kinC_MeshedGeometrySearchTechniqueEnm,1> kinC_MeshedGeometrySearchTechniqueConst;

typedef omu_PrimEnum<kinC_ModelEnm,1> kinC_ModelConst;
typedef omu_PrimEnum<kinC_CategoryEnm,1> kinC_InteractionCategoryConst;
typedef omu_PrimEnum<kinC_ClearanceTypeEnm,1> kinC_ClearanceTypeConst;
typedef omu_PrimEnum<kinC_InitialClearanceEnm,1> kinC_InitialClearanceConst;
typedef omu_PrimEnum<kinC_ComputedEnm,1> kinC_ComputedConst;

typedef omu_PrimEnum<kinC_SlidingEnm,1> kinC_SlidingConst;
typedef omu_PrimEnum<kinC_SlaveTypeEnm,1> kinC_SlaveTypeConst;
typedef omu_PrimEnum<kinC_InterferenceTypeEnm,1> kinC_InterferenceTypeConst;
typedef omu_PrimEnum<kinC_InterferenceDirectionTypeEnm,1> kinC_InterferenceDirectionTypeConst;
typedef omu_PrimEnum<kinC_CavityRadiationBlockingEnm,1> kinC_CavityRadiationBlockingConst;
typedef omu_PrimEnum<kinC_DefinitionEnm,1> kinC_DefinitionConst;
typedef omu_PrimEnum<kinC_DistributionEnm,1> kinC_DistributionConst;
typedef omu_PrimEnum<kinC_MechanicalConstraintEnm,1> kinC_MechanicalConstraintConst;
typedef omu_PrimEnum<kinC_StdAdjustEnm,1> kinC_StdAdjustConst;
typedef omu_PrimEnum<kinC_DefaultOrValueEnm, 1>kinC_DefaultOrValueConst;
typedef omu_PrimEnum<kinC_ExplicitRegionTypeEnm, 1>kinC_ExplicitRegionTypeConst;
typedef omu_PrimEnum<kinC_ConstrEnforcementEnm,1> kinC_ConstrEnforcementConst;
typedef omu_PrimEnum<kinC_AcousticDefEnm,1> kinC_AcousticDefConst;
typedef omu_PrimEnum<kinC_TableTypeEnm,1> kinC_TableTypeConst;
typedef omu_PrimEnum<kinC_NonreflTypeEnm,1> kinC_NonreflTypeConst;
typedef omu_PrimEnum<kinC_IncidentDefEnm,1> kinC_IncidentDefConst;
typedef omu_PrimEnum<kinC_ConstraintPositionEnm,1> kinC_ConstraintPositionConst;
typedef omu_PrimEnum<kinC_ContactTrackingEnm,1> kinC_ContactTrackingConst;
typedef omu_PrimEnum<kinC_SupplementaryContactEnm,1> kinC_SupplementaryContactConst;
typedef omu_PrimEnum<kinC_SurfaceSmoothingEnm,1> kinC_SurfaceSmoothingConst;
typedef omu_PrimEnum<kinC_NodalDiameterEnm, 1> kinC_NodalDiameterConst;
typedef omu_PrimEnum<kinC_CosimulationIncrementationEnum, 1> kinC_CosimulationIncrementationConst;
typedef omu_PrimEnum<kinC_CosimulationTimeMarksEnum, 1> kinC_CosimulationTimeMarksConst;
typedef omu_PrimEnum<kinC_CosimulationCouplingEnum, 1> kinC_CosimulationCouplingConst;
typedef omu_PrimEnum<kinC_CosimulationStepSizeEnum, 1> kinC_CosimulationStepSizeConst;
typedef omu_PrimEnum<kinC_CosimulationSchemeOptionsEnum, 1> kinC_CosimulationSchemeOptionsConst;
typedef omu_PrimEnum<kinC_BoltHandednessEnm, 1> kinC_BoltHandednessConst;
typedef omu_PrimEnum<kinC_BoltNormalAdjustmentEnm, 1> kinC_BoltNormalAdjustmentConst;    
typedef omu_PrimEnum<kinC_ModelTypeEnm, 1> kinC_ModelTypeConst;	 
typedef omu_PrimEnum <kinC_EdgeToEdgeFormulationTypeEnm, 1> kinC_EdgeToEdgeFormulationTypeConst;
typedef omu_PrimEnum <kinC_BeamCrossSectionTypeEnm, 1> kinC_BeamCrossSectionTypeConst;
// typedef omu_PrimEnum <kinC_ContactMassScalinglocationEnm, 1> kinC_ContactMassScalinglocationConst;

#endif
