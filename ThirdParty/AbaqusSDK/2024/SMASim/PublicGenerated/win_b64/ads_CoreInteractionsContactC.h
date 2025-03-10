//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreInteractionsContactC_h
#define ads_CoreInteractionsContactC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment InteractionsContact of the latest level of form Core */

#define ads_ContactControlsAssignment (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 0))

#define ads_ContactControlsAssignment_AltEdgeTracking (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 1))

#define ads_ContactControlsAssignment_AutoOverclosure (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 2))

/** If the first surface is omitted, a default surface that encompasses the entire general contact domain (including all nodes and facets) is assumed. */
#define ads_ContactControlsAssignment_AutoOverclosure_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 3))

/** If the second surface is omitted or is the same as the first surface, the specified contact controls are assigned to contact interactions between the first surface and itself. */
#define ads_ContactControlsAssignment_AutoOverclosure_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 4))

#define ads_ContactControlsAssignment_BeamCrossSection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 5))

#define ads_ContactControlsAssignment_EnhEdgeTracking (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 6))

#define ads_ContactControlsAssignment_FoldInversCheck (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 7))

/** The surface for which the fold inversion check should be activated. If the surface is omitted, a default surface that encompasses the entire general contact domain (including all nodes and facets) is assumed */
#define ads_ContactControlsAssignment_FoldInversCheck_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 8))

#define ads_ContactControlsAssignment_FoldTracking (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 9))

/** The surface whose nodes will be tracked using the nondefault node-to-face tracking algorithm. If the surface is omitted, a default surface that encompasses the entire general contact domain (including all nodes and facets) is assumed */
#define ads_ContactControlsAssignment_FoldTracking_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 10))

#define ads_ContactControlsAssignment_NodalErosion (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 11))

#define ads_ContactControlsAssignment_RotationalTerms (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 12))

#define ads_ContactControlsAssignment_ScalePenalty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 13))

/** If the first surface is omitted, a default surface that encompasses the entire general contact domain (including all nodes and facets) is assumed. */
#define ads_ContactControlsAssignment_ScalePenalty_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 14))

/** If the second surface is omitted or is the same as the first surface, the specified contact controls are assigned to contact interactions between the first surface and itself. */
#define ads_ContactControlsAssignment_ScalePenalty_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 15))

#define ads_ContactControlsAssignment_ThickReduction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 16))

#define ads_ContactExclusion (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 17))

/** The first surface. If the first surface is omitted, the default all-inclusive, surface defined by Abaqus is assumed. */
#define ads_ContactExclusion_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 18))

/** The second surface. If the second surface is omitted or is the same as the first surface, Abaqus assumes that self-contact is defined. */
#define ads_ContactExclusion_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 19))

#define ads_ContactFormulation (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 20))

#define ads_ContactFormulation_EdgeToEdge (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 21))

#define ads_ContactFormulation_MasterSlaveRoles (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 22))

#define ads_ContactFormulation_Polarity (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 23))

#define ads_ContactFormulation_SlidingFormulation (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 24))

#define ads_ContactFormulation_SlidingTransition (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 25))

/** The first surface. If the first surface is omitted, a default surface that encompasses the entire contact domain is assumed */
#define ads_ContactFormulation_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 26))

#define ads_ContactFormulation_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 27))

#define ads_ContactInclusion (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 28))

/** The first surface. If the first surface is omitted, the default all-inclusive, surface defined by Abaqus is assumed. */
#define ads_ContactInclusion_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 29))

/** The second surface. If the second surface is omitted or is the same as the first surface, Abaqus assumes that self-contact is defined. If both surfaces are omited then self-contact for a default unnamed, all-inclusive surface that includes all element-based surface facets and, in Abaqus/Explicit only, all analytical rigid surfaces, is assumed. */
#define ads_ContactInclusion_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 30))

#define ads_ContactInitializationAssignment (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 31))

/** The contact initialization data definition to be assigned. */
#define ads_ContactInitializationAssignment_contactInitializationMethod (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 32))

/** If the first surface is omitted, a default surface that encompasses the entire general contact domain is assumed */
#define ads_ContactInitializationAssignment_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 33))

/** If the second surface is omitted or is the same as the first surface, the specified contact initialization method definition is assigned to contact interactions between the first surface and itself */
#define ads_ContactInitializationAssignment_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 34))

#define ads_ContactInitializationData (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 35))

#define ads_ContactInitializationData_Clearance (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 36))

/** initial clearance distance */
#define ads_ContactInitializationData_Clearance_clearance (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 37))

/** *CONTACT INITIALIZATION DATA, INTERFERENCE FIT */
#define ads_ContactInitializationData_Interference (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 38))

/** This field can be omited to treat initial overclosures as interference fits. Set this field equal to a positive value to specify an interference distance */
#define ads_ContactInitializationData_Interference_interferenceValue (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 39))

/** Set this field equal to a positive value to ensure that the search zone for contact initialization includes gaps at least as large as the specified value */
#define ads_ContactInitializationData_searchAbove (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 40))

/** Set this field equal to a positive value to ensure that the search zone for contact initialization includes overclosures at least as large as the specified value. */
#define ads_ContactInitializationData_searchBelow (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 41))

#define ads_ContactMassScaling (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 42))

/** List of contact surfaces, surfaces defined by Abaqus is assumed. */
#define ads_ContactMassScaling_contactSurfaces (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 43))

#define ads_ContactPropertyAssignment (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 44))

/** Property to be assigned. */
#define ads_ContactPropertyAssignment_interactionProperty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 45))

/** The first surface. If the first surface is omitted, a default surface that encompasses the entire general contact domain is assumed. */
#define ads_ContactPropertyAssignment_surf1 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 46))

/** The second surface. If the second surface is omitted or is the same as the first surface, the specified contact property definition is assigned to contact interactions between the first surface and itself. */
#define ads_ContactPropertyAssignment_surf2 (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 47))

#define ads_Interaction_Contact (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 48))

/** If the contact controls assignments overlap, the last assignment applies in the overlap region. */
#define ads_Interaction_Contact_contactControlsAssignments (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 49))

#define ads_Interaction_Contact_contactExclusions (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 50))

#define ads_Interaction_Contact_contactFormulations (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 51))

#define ads_Interaction_Contact_contactInclusions (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 52))

/** If the contact initialization method assignments overlap, the last assignment applies in the overlap region. */
#define ads_Interaction_Contact_contactInitializationAssignments (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 53))

#define ads_Interaction_Contact_contactMassScaling (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 54))

/** If the contact property assignments overlap, the last assignment applies in the overlap region. */
#define ads_Interaction_Contact_contactPropertyAssignments (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 55))

/** If the feature edge criteria assignments overlap, the last assignment applies in the overlap region. */
#define ads_Interaction_Contact_surfPropAssignments (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 56))

/** A self contact interaction */
#define ads_Interaction_SelfContact (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 57))

/** An association between the interaction and the surface tangential slip directions. */
#define ads_Interaction_SelfContact_tangentialSlipDirection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 58))

/** A surface interaction */
#define ads_Interaction_SurfaceToSurface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 59))

/** Adjust slave nodes from this set. */
#define ads_Interaction_SurfaceToSurface_adjustedNodeSet (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 60))

/** An association between the interaction and the master surface tangential slip directions. */
#define ads_Interaction_SurfaceToSurface_masterTangentialSlipDirection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 61))

/** An association between the interaction and the slave surface tangential slip directions. */
#define ads_Interaction_SurfaceToSurface_slaveTangentialSlipDirection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 62))

#define ads_Model_contactInitializationMethods (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 63))

#define ads_SurfPropAssign (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 64))

#define ads_SurfPropAssign_BeamSmoothing (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 65))

#define ads_SurfPropAssign_BeamSmoothing_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 66))

#define ads_SurfPropAssign_FeatureEdgeCriteria (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 67))

#define ads_SurfPropAssign_FeatureEdgeCriteria_primaryCutoffFeatureAngle (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 68))

#define ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCutoffFeatureAngle (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 69))

/** The Surface. A surface is required if the ALL_EDGES or PICKED_EDGES options are specified. If the surface is omitted when using the PERIMETER_EDGES, NO_FEATURE_EDGES, or cutoff feature angle options, a default surface that encompasses the entire general contact domain is assumed. */
#define ads_SurfPropAssign_FeatureEdgeCriteria_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 70))

#define ads_SurfPropAssign_GeometricCorrection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 71))

/** Global coordinates. */
#define ads_SurfPropAssign_GeometricCorrection_pointACoordinate (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 72))

/** Global coordinates. */
#define ads_SurfPropAssign_GeometricCorrection_pointBCoordinate (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 73))

/** The surface. */
#define ads_SurfPropAssign_GeometricCorrection_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 74))

/** The toroidal radius from point A. */
#define ads_SurfPropAssign_GeometricCorrection_toroidalRadius (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 75))

#define ads_SurfPropAssign_OffsetFraction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 76))

/** The Surface. If the surfaceis omitted, a default surface that encompasses the entire general contact domain is assumed. Faces specified on elements other than shell elements, membrane elements, rigid elements, and surface elements will be ignored. */
#define ads_SurfPropAssign_OffsetFraction_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 77))

#define ads_SurfPropAssign_ThicknessAssignment (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 78))

#define ads_SurfPropAssign_ThicknessAssignment_nominalThicknessValue (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 79))

/** The surface. If the surface is omitted, a default surface that encompasses the entire general contact domain is assumed */
#define ads_SurfPropAssign_ThicknessAssignment_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 80))

#define ads_SurfPropAssign_VertexCriteria (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 81))

/** The Surface. If the surface is omitted, a default surface that encompasses the entire general contact domain is assumed. */
#define ads_SurfPropAssign_VertexCriteria_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 82))

#define ads_SurfPropAssign_VertexCriteria_vertexAngleThreshold (ads_CoreFragmentTypeIndex(ads_CoreInteractionsContactFragment, 83))

/** Enum with record members. */
enum ads_ContactControlsAssignmentMembersEnm
{
    ads_ContactControlsAssignment_contactSeeding
};

enum ads_ContactControlsAssignment_contactSeedingEnm
{
    ads_ContactControlsAssignment_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_contactSeeding_NONE
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_AltEdgeTrackingMembersEnm
{
    ads_ContactControlsAssignment_AltEdgeTracking_contactSeeding
};

enum ads_ContactControlsAssignment_AltEdgeTracking_contactSeedingEnm
{
    ads_ContactControlsAssignment_AltEdgeTracking_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_AltEdgeTracking_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_AltEdgeTracking_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_AltEdgeTracking_contactSeeding_NONE
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_AutoOverclosureMembersEnm
{
    ads_ContactControlsAssignment_AutoOverclosure_contactSeeding,
    ads_ContactControlsAssignment_AutoOverclosure_overclosureResolutionMethod
};

enum ads_ContactControlsAssignment_AutoOverclosure_contactSeedingEnm
{
    ads_ContactControlsAssignment_AutoOverclosure_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_AutoOverclosure_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_AutoOverclosure_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_AutoOverclosure_contactSeeding_NONE
};

enum ads_ContactControlsAssignment_AutoOverclosure_overclosureResolutionMethodEnm
{
    ads_ContactControlsAssignment_AutoOverclosure_overclosureResolutionMethod_ADJUST_NODES,
    ads_ContactControlsAssignment_AutoOverclosure_overclosureResolutionMethod_STORE_OFFSETS
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_AutoOverclosure_surf1RolesEnm
{
    ads_ContactControlsAssignment_AutoOverclosure_surf1_referent,
    ads_ContactControlsAssignment_AutoOverclosure_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_AutoOverclosure_surf2RolesEnm
{
    ads_ContactControlsAssignment_AutoOverclosure_surf2_referent,
    ads_ContactControlsAssignment_AutoOverclosure_surf2_referrer
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_BeamCrossSectionMembersEnm
{
    ads_ContactControlsAssignment_BeamCrossSection_contactSeeding,
    ads_ContactControlsAssignment_BeamCrossSection_contactBeamCrossSection
};

enum ads_ContactControlsAssignment_BeamCrossSection_contactSeedingEnm
{
    ads_ContactControlsAssignment_BeamCrossSection_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_BeamCrossSection_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_BeamCrossSection_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_BeamCrossSection_contactSeeding_NONE
};

enum ads_ContactControlsAssignment_BeamCrossSection_contactBeamCrossSectionEnm
{
    ads_ContactControlsAssignment_BeamCrossSection_contactBeamCrossSection_CIRCUMSCRIBED_CIRCLE,
    ads_ContactControlsAssignment_BeamCrossSection_contactBeamCrossSection_EXACT
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_EnhEdgeTrackingMembersEnm
{
    ads_ContactControlsAssignment_EnhEdgeTracking_contactSeeding
};

enum ads_ContactControlsAssignment_EnhEdgeTracking_contactSeedingEnm
{
    ads_ContactControlsAssignment_EnhEdgeTracking_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_EnhEdgeTracking_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_EnhEdgeTracking_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_EnhEdgeTracking_contactSeeding_NONE
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_FoldInversCheckMembersEnm
{
    ads_ContactControlsAssignment_FoldInversCheck_contactSeeding
};

enum ads_ContactControlsAssignment_FoldInversCheck_contactSeedingEnm
{
    ads_ContactControlsAssignment_FoldInversCheck_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_FoldInversCheck_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_FoldInversCheck_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_FoldInversCheck_contactSeeding_NONE
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_FoldInversCheck_surfaceRolesEnm
{
    ads_ContactControlsAssignment_FoldInversCheck_surface_referent,
    ads_ContactControlsAssignment_FoldInversCheck_surface_referrer
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_FoldTrackingMembersEnm
{
    ads_ContactControlsAssignment_FoldTracking_contactSeeding
};

enum ads_ContactControlsAssignment_FoldTracking_contactSeedingEnm
{
    ads_ContactControlsAssignment_FoldTracking_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_FoldTracking_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_FoldTracking_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_FoldTracking_contactSeeding_NONE
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_FoldTracking_surfaceRolesEnm
{
    ads_ContactControlsAssignment_FoldTracking_surface_referent,
    ads_ContactControlsAssignment_FoldTracking_surface_referrer
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_NodalErosionMembersEnm
{
    ads_ContactControlsAssignment_NodalErosion_contactSeeding,
    ads_ContactControlsAssignment_NodalErosion_nodalErosion
};

enum ads_ContactControlsAssignment_NodalErosion_contactSeedingEnm
{
    ads_ContactControlsAssignment_NodalErosion_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_NodalErosion_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_NodalErosion_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_NodalErosion_contactSeeding_NONE
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_RotationalTermsMembersEnm
{
    ads_ContactControlsAssignment_RotationalTerms_contactSeeding,
    ads_ContactControlsAssignment_RotationalTerms_contactRotationalTerms
};

enum ads_ContactControlsAssignment_RotationalTerms_contactSeedingEnm
{
    ads_ContactControlsAssignment_RotationalTerms_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_RotationalTerms_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_RotationalTerms_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_RotationalTerms_contactSeeding_NONE
};

enum ads_ContactControlsAssignment_RotationalTerms_contactRotationalTermsEnm
{
    ads_ContactControlsAssignment_RotationalTerms_contactRotationalTerms_NONE,
    ads_ContactControlsAssignment_RotationalTerms_contactRotationalTerms_STRUCTURAL
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_ScalePenaltyMembersEnm
{
    ads_ContactControlsAssignment_ScalePenalty_contactSeeding,
    ads_ContactControlsAssignment_ScalePenalty_scaleFactor
};

enum ads_ContactControlsAssignment_ScalePenalty_contactSeedingEnm
{
    ads_ContactControlsAssignment_ScalePenalty_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_ScalePenalty_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_ScalePenalty_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_ScalePenalty_contactSeeding_NONE
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_ScalePenalty_surf1RolesEnm
{
    ads_ContactControlsAssignment_ScalePenalty_surf1_referent,
    ads_ContactControlsAssignment_ScalePenalty_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactControlsAssignment_ScalePenalty_surf2RolesEnm
{
    ads_ContactControlsAssignment_ScalePenalty_surf2_referent,
    ads_ContactControlsAssignment_ScalePenalty_surf2_referrer
};

/** Enum with record members. */
enum ads_ContactControlsAssignment_ThickReductionMembersEnm
{
    ads_ContactControlsAssignment_ThickReduction_contactSeeding,
    ads_ContactControlsAssignment_ThickReduction_contactThicknessReduction
};

enum ads_ContactControlsAssignment_ThickReduction_contactSeedingEnm
{
    ads_ContactControlsAssignment_ThickReduction_contactSeeding_DYNAMIC,
    ads_ContactControlsAssignment_ThickReduction_contactSeeding_GLOBAL,
    ads_ContactControlsAssignment_ThickReduction_contactSeeding_LOCAL,
    ads_ContactControlsAssignment_ThickReduction_contactSeeding_NONE
};

enum ads_ContactControlsAssignment_ThickReduction_contactThicknessReductionEnm
{
    ads_ContactControlsAssignment_ThickReduction_contactThicknessReduction_NOPERIMSELF,
    ads_ContactControlsAssignment_ThickReduction_contactThicknessReduction_SELF
};

/** 
Enum with association roles. */
enum ads_ContactExclusion_surf1RolesEnm
{
    ads_ContactExclusion_surf1_referent,
    ads_ContactExclusion_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactExclusion_surf2RolesEnm
{
    ads_ContactExclusion_surf2_referent,
    ads_ContactExclusion_surf2_referrer
};

/** Enum with record members. */
enum ads_ContactFormulation_EdgeToEdgeMembersEnm
{
    ads_ContactFormulation_EdgeToEdge_formulation
};

enum ads_ContactFormulation_EdgeToEdge_formulationEnm
{
    ads_ContactFormulation_EdgeToEdge_formulation_BOTH,
    ads_ContactFormulation_EdgeToEdge_formulation_CROSS,
    ads_ContactFormulation_EdgeToEdge_formulation_NONE,
    ads_ContactFormulation_EdgeToEdge_formulation_RADIAL
};

/** Enum with record members. */
enum ads_ContactFormulation_MasterSlaveRolesMembersEnm
{
    ads_ContactFormulation_MasterSlaveRoles_formulationType
};

enum ads_ContactFormulation_MasterSlaveRoles_formulationTypeEnm
{
    ads_ContactFormulation_MasterSlaveRoles_formulationType_BALANCED,
    ads_ContactFormulation_MasterSlaveRoles_formulationType_MASTER,
    ads_ContactFormulation_MasterSlaveRoles_formulationType_SLAVE
};

/** Enum with record members. */
enum ads_ContactFormulation_PolarityMembersEnm
{
    ads_ContactFormulation_Polarity_secondSurfaceElementsSide
};

enum ads_ContactFormulation_Polarity_secondSurfaceElementsSideEnm
{
    ads_ContactFormulation_Polarity_secondSurfaceElementsSide_NONE,
    ads_ContactFormulation_Polarity_secondSurfaceElementsSide_SNEG,
    ads_ContactFormulation_Polarity_secondSurfaceElementsSide_SPOS,
    ads_ContactFormulation_Polarity_secondSurfaceElementsSide_TWO_SIDED
};

/** Enum with record members. */
enum ads_ContactFormulation_SlidingFormulationMembersEnm
{
    ads_ContactFormulation_SlidingFormulation_smallSliding
};

/** Enum with record members. */
enum ads_ContactFormulation_SlidingTransitionMembersEnm
{
    ads_ContactFormulation_SlidingTransition_smoothing
};

enum ads_ContactFormulation_SlidingTransition_smoothingEnm
{
    ads_ContactFormulation_SlidingTransition_smoothing_ELEMENT_ORDER_SMOOTHING,
    ads_ContactFormulation_SlidingTransition_smoothing_LINEAR_SMOOTHING,
    ads_ContactFormulation_SlidingTransition_smoothing_QUADRATIC_SMOOTHING
};

/** 
Enum with association roles. */
enum ads_ContactFormulation_surf1RolesEnm
{
    ads_ContactFormulation_surf1_referent,
    ads_ContactFormulation_surf1_referrer
};

/** Enum with association roles. */
enum ads_ContactFormulation_surf2RolesEnm
{
    ads_ContactFormulation_surf2_referent,
    ads_ContactFormulation_surf2_referrer
};

/** 
Enum with association roles. */
enum ads_ContactInclusion_surf1RolesEnm
{
    ads_ContactInclusion_surf1_referent,
    ads_ContactInclusion_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactInclusion_surf2RolesEnm
{
    ads_ContactInclusion_surf2_referent,
    ads_ContactInclusion_surf2_referrer
};

/** 
Enum with association roles. */
enum ads_ContactInitializationAssignment_contactInitializationMethodRolesEnm
{
    ads_ContactInitializationAssignment_contactInitializationMethod_referent,
    ads_ContactInitializationAssignment_contactInitializationMethod_referrer
};

/** 
Enum with association roles. */
enum ads_ContactInitializationAssignment_surf1RolesEnm
{
    ads_ContactInitializationAssignment_surf1_referent,
    ads_ContactInitializationAssignment_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactInitializationAssignment_surf2RolesEnm
{
    ads_ContactInitializationAssignment_surf2_referent,
    ads_ContactInitializationAssignment_surf2_referrer
};

/** Enum with record members. */
enum ads_ContactInitializationDataMembersEnm
{
    ads_ContactInitializationData_adjust,
    ads_ContactInitializationData_minDistance
};

/** Enum with record members. */
enum ads_ContactInitializationData_ClearanceMembersEnm
{
    ads_ContactInitializationData_Clearance_adjust,
    ads_ContactInitializationData_Clearance_minDistance
};

/** 
Enum with association roles. */
enum ads_ContactInitializationData_Clearance_clearanceRolesEnm
{
    ads_ContactInitializationData_Clearance_clearance_child,
    ads_ContactInitializationData_Clearance_clearance_parent
};

/** 
Enum with record members. */
enum ads_ContactInitializationData_InterferenceMembersEnm
{
    ads_ContactInitializationData_Interference_adjust,
    ads_ContactInitializationData_Interference_minDistance
};

/** 
Enum with association roles. */
enum ads_ContactInitializationData_Interference_interferenceValueRolesEnm
{
    ads_ContactInitializationData_Interference_interferenceValue_child,
    ads_ContactInitializationData_Interference_interferenceValue_parent
};

/** 
Enum with association roles. */
enum ads_ContactInitializationData_searchAboveRolesEnm
{
    ads_ContactInitializationData_searchAbove_child,
    ads_ContactInitializationData_searchAbove_parent
};

/** 
Enum with association roles. */
enum ads_ContactInitializationData_searchBelowRolesEnm
{
    ads_ContactInitializationData_searchBelow_child,
    ads_ContactInitializationData_searchBelow_parent
};

/** Enum with record members. */
enum ads_ContactMassScalingMembersEnm
{
    ads_ContactMassScaling_location
};

enum ads_ContactMassScaling_locationEnm
{
    ads_ContactMassScaling_location_ALL_CONTACT_SURFACES,
    ads_ContactMassScaling_location_ELEMENT_MASS_SCALING,
    ads_ContactMassScaling_location_NONE,
    ads_ContactMassScaling_location_SPECIFIED_SURFACES
};

/** 
Enum with association roles. */
enum ads_ContactMassScaling_contactSurfacesRolesEnm
{
    ads_ContactMassScaling_contactSurfaces_referent,
    ads_ContactMassScaling_contactSurfaces_referrer
};

/** 
Enum with association roles. */
enum ads_ContactPropertyAssignment_interactionPropertyRolesEnm
{
    ads_ContactPropertyAssignment_interactionProperty_referent,
    ads_ContactPropertyAssignment_interactionProperty_referrer
};

/** 
Enum with association roles. */
enum ads_ContactPropertyAssignment_surf1RolesEnm
{
    ads_ContactPropertyAssignment_surf1_referent,
    ads_ContactPropertyAssignment_surf1_referrer
};

/** 
Enum with association roles. */
enum ads_ContactPropertyAssignment_surf2RolesEnm
{
    ads_ContactPropertyAssignment_surf2_referent,
    ads_ContactPropertyAssignment_surf2_referrer
};

/** Enum with record members. */
enum ads_Interaction_ContactMembersEnm
{
    ads_Interaction_Contact_autoPropagate
};

/** 
Enum with association roles. */
enum ads_Interaction_Contact_contactControlsAssignmentsRolesEnm
{
    ads_Interaction_Contact_contactControlsAssignments_child,
    ads_Interaction_Contact_contactControlsAssignments_parent
};

/** Enum with association roles. */
enum ads_Interaction_Contact_contactExclusionsRolesEnm
{
    ads_Interaction_Contact_contactExclusions_child,
    ads_Interaction_Contact_contactExclusions_parent
};

/** Enum with association roles. */
enum ads_Interaction_Contact_contactFormulationsRolesEnm
{
    ads_Interaction_Contact_contactFormulations_child,
    ads_Interaction_Contact_contactFormulations_parent
};

/** Enum with association roles. */
enum ads_Interaction_Contact_contactInclusionsRolesEnm
{
    ads_Interaction_Contact_contactInclusions_child,
    ads_Interaction_Contact_contactInclusions_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Contact_contactInitializationAssignmentsRolesEnm
{
    ads_Interaction_Contact_contactInitializationAssignments_child,
    ads_Interaction_Contact_contactInitializationAssignments_parent
};

/** Enum with association roles. */
enum ads_Interaction_Contact_contactMassScalingRolesEnm
{
    ads_Interaction_Contact_contactMassScaling_child,
    ads_Interaction_Contact_contactMassScaling_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Contact_contactPropertyAssignmentsRolesEnm
{
    ads_Interaction_Contact_contactPropertyAssignments_child,
    ads_Interaction_Contact_contactPropertyAssignments_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Contact_surfPropAssignmentsRolesEnm
{
    ads_Interaction_Contact_surfPropAssignments_child,
    ads_Interaction_Contact_surfPropAssignments_parent
};

/** 
Enum with record members. */
enum ads_Interaction_SelfContactMembersEnm
{
    ads_Interaction_SelfContact_autoPropagate,
    ads_Interaction_SelfContact_inpOrder
};

/** 
Enum with association roles. */
enum ads_Interaction_SelfContact_tangentialSlipDirectionRolesEnm
{
    ads_Interaction_SelfContact_tangentialSlipDirection_referent,
    ads_Interaction_SelfContact_tangentialSlipDirection_referrer
};

/** 
Enum with record members. */
enum ads_Interaction_SurfaceToSurfaceMembersEnm
{
    ads_Interaction_SurfaceToSurface_autoPropagate,
    ads_Interaction_SurfaceToSurface_cpSet,
    ads_Interaction_SurfaceToSurface_extensionZone,
    ads_Interaction_SurfaceToSurface_hCrit,
    ads_Interaction_SurfaceToSurface_initiallyAdjustedSurface,
    ads_Interaction_SurfaceToSurface_inpOrder,
    ads_Interaction_SurfaceToSurface_mechanicalConstraint,
    ads_Interaction_SurfaceToSurface_minimumDistance,
    ads_Interaction_SurfaceToSurface_positionTolerance,
    ads_Interaction_SurfaceToSurface_smallSliding,
    ads_Interaction_SurfaceToSurface_smooth,
    ads_Interaction_SurfaceToSurface_supplementaryConstraints,
    ads_Interaction_SurfaceToSurface_thickness,
    ads_Interaction_SurfaceToSurface_tied,
    ads_Interaction_SurfaceToSurface_tracking,
    ads_Interaction_SurfaceToSurface_type,
    ads_Interaction_SurfaceToSurface_weight
};

enum ads_Interaction_SurfaceToSurface_mechanicalConstraintEnm
{
    ads_Interaction_SurfaceToSurface_mechanicalConstraint_KINEMATIC,
    ads_Interaction_SurfaceToSurface_mechanicalConstraint_PENALTY
};

enum ads_Interaction_SurfaceToSurface_supplementaryConstraintsEnm
{
    ads_Interaction_SurfaceToSurface_supplementaryConstraints_NO,
    ads_Interaction_SurfaceToSurface_supplementaryConstraints_SELECTIVE,
    ads_Interaction_SurfaceToSurface_supplementaryConstraints_YES
};

enum ads_Interaction_SurfaceToSurface_trackingEnm
{
    ads_Interaction_SurfaceToSurface_tracking_PATH,
    ads_Interaction_SurfaceToSurface_tracking_STATE
};

enum ads_Interaction_SurfaceToSurface_typeEnm
{
    ads_Interaction_SurfaceToSurface_type_NODE_TO_SURF,
    ads_Interaction_SurfaceToSurface_type_SURF_TO_SURF
};

/** 
Enum with association roles. */
enum ads_Interaction_SurfaceToSurface_adjustedNodeSetRolesEnm
{
    ads_Interaction_SurfaceToSurface_adjustedNodeSet_referent,
    ads_Interaction_SurfaceToSurface_adjustedNodeSet_referrer
};

/** 
Enum with association roles. */
enum ads_Interaction_SurfaceToSurface_masterTangentialSlipDirectionRolesEnm
{
    ads_Interaction_SurfaceToSurface_masterTangentialSlipDirection_referent,
    ads_Interaction_SurfaceToSurface_masterTangentialSlipDirection_referrer
};

/** 
Enum with association roles. */
enum ads_Interaction_SurfaceToSurface_slaveTangentialSlipDirectionRolesEnm
{
    ads_Interaction_SurfaceToSurface_slaveTangentialSlipDirection_referent,
    ads_Interaction_SurfaceToSurface_slaveTangentialSlipDirection_referrer
};

/** Enum with association roles. */
enum ads_Model_contactInitializationMethodsRolesEnm
{
    ads_Model_contactInitializationMethods_child,
    ads_Model_contactInitializationMethods_parent
};

/** Enum with record members. */
enum ads_SurfPropAssign_BeamSmoothingMembersEnm
{
    ads_SurfPropAssign_BeamSmoothing_value
};

/** Enum with association roles. */
enum ads_SurfPropAssign_BeamSmoothing_surfaceRolesEnm
{
    ads_SurfPropAssign_BeamSmoothing_surface_referent,
    ads_SurfPropAssign_BeamSmoothing_surface_referrer
};

/** Enum with record members. */
enum ads_SurfPropAssign_FeatureEdgeCriteriaMembersEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_criteriaAssignment,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion
};

enum ads_SurfPropAssign_FeatureEdgeCriteria_criteriaAssignmentEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_criteriaAssignment_CURRENT,
    ads_SurfPropAssign_FeatureEdgeCriteria_criteriaAssignment_NONE,
    ads_SurfPropAssign_FeatureEdgeCriteria_criteriaAssignment_ORIGINAL
};

enum ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterionEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion_ALL_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion_CUTOFF_ANGLE,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion_NO_FEATURE_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion_PERIMETER_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCriterion_PICKED_EDGES
};

enum ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterionEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_ALL_REMAINING_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_CUTOFF_ANGLE,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_NONE,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_NO_FEATURE_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_PERIMETER_EDGES,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCriterion_PICKED_EDGES
};

/** Enum with association roles. */
enum ads_SurfPropAssign_FeatureEdgeCriteria_primaryCutoffFeatureAngleRolesEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCutoffFeatureAngle_child,
    ads_SurfPropAssign_FeatureEdgeCriteria_primaryCutoffFeatureAngle_parent
};

/** Enum with association roles. */
enum ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCutoffFeatureAngleRolesEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCutoffFeatureAngle_child,
    ads_SurfPropAssign_FeatureEdgeCriteria_secondaryCutoffFeatureAngle_parent
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_FeatureEdgeCriteria_surfaceRolesEnm
{
    ads_SurfPropAssign_FeatureEdgeCriteria_surface_referent,
    ads_SurfPropAssign_FeatureEdgeCriteria_surface_referrer
};

/** Enum with record members. */
enum ads_SurfPropAssign_GeometricCorrectionMembersEnm
{
    ads_SurfPropAssign_GeometricCorrection_surfaceType
};

enum ads_SurfPropAssign_GeometricCorrection_surfaceTypeEnm
{
    ads_SurfPropAssign_GeometricCorrection_surfaceType_CGM,
    ads_SurfPropAssign_GeometricCorrection_surfaceType_CIRCUMFERENTIAL,
    ads_SurfPropAssign_GeometricCorrection_surfaceType_NONE,
    ads_SurfPropAssign_GeometricCorrection_surfaceType_SPHERICAL,
    ads_SurfPropAssign_GeometricCorrection_surfaceType_TOROIDAL
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_GeometricCorrection_pointACoordinateRolesEnm
{
    ads_SurfPropAssign_GeometricCorrection_pointACoordinate_child,
    ads_SurfPropAssign_GeometricCorrection_pointACoordinate_parent
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_GeometricCorrection_pointBCoordinateRolesEnm
{
    ads_SurfPropAssign_GeometricCorrection_pointBCoordinate_child,
    ads_SurfPropAssign_GeometricCorrection_pointBCoordinate_parent
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_GeometricCorrection_surfaceRolesEnm
{
    ads_SurfPropAssign_GeometricCorrection_surface_referent,
    ads_SurfPropAssign_GeometricCorrection_surface_referrer
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_GeometricCorrection_toroidalRadiusRolesEnm
{
    ads_SurfPropAssign_GeometricCorrection_toroidalRadius_child,
    ads_SurfPropAssign_GeometricCorrection_toroidalRadius_parent
};

/** Enum with record members. */
enum ads_SurfPropAssign_OffsetFractionMembersEnm
{
    ads_SurfPropAssign_OffsetFraction_offset,
    ads_SurfPropAssign_OffsetFraction_offsetValue
};

enum ads_SurfPropAssign_OffsetFraction_offsetEnm
{
    ads_SurfPropAssign_OffsetFraction_offset_ORIGINAL,
    ads_SurfPropAssign_OffsetFraction_offset_SNEG,
    ads_SurfPropAssign_OffsetFraction_offset_SPOS,
    ads_SurfPropAssign_OffsetFraction_offset_VAL
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_OffsetFraction_surfaceRolesEnm
{
    ads_SurfPropAssign_OffsetFraction_surface_referent,
    ads_SurfPropAssign_OffsetFraction_surface_referrer
};

/** Enum with record members. */
enum ads_SurfPropAssign_ThicknessAssignmentMembersEnm
{
    ads_SurfPropAssign_ThicknessAssignment_nominalThickness,
    ads_SurfPropAssign_ThicknessAssignment_scaleFactor
};

enum ads_SurfPropAssign_ThicknessAssignment_nominalThicknessEnm
{
    ads_SurfPropAssign_ThicknessAssignment_nominalThickness_ORIGINAL,
    ads_SurfPropAssign_ThicknessAssignment_nominalThickness_THINNING,
    ads_SurfPropAssign_ThicknessAssignment_nominalThickness_VAL
};

/** Enum with association roles. */
enum ads_SurfPropAssign_ThicknessAssignment_nominalThicknessValueRolesEnm
{
    ads_SurfPropAssign_ThicknessAssignment_nominalThicknessValue_child,
    ads_SurfPropAssign_ThicknessAssignment_nominalThicknessValue_parent
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_ThicknessAssignment_surfaceRolesEnm
{
    ads_SurfPropAssign_ThicknessAssignment_surface_referent,
    ads_SurfPropAssign_ThicknessAssignment_surface_referrer
};

/** Enum with record members. */
enum ads_SurfPropAssign_VertexCriteriaMembersEnm
{
    ads_SurfPropAssign_VertexCriteria_vertexToSurface
};

enum ads_SurfPropAssign_VertexCriteria_vertexToSurfaceEnm
{
    ads_SurfPropAssign_VertexCriteria_vertexToSurface_ALL_VERTICES,
    ads_SurfPropAssign_VertexCriteria_vertexToSurface_NO_VERTICES,
    ads_SurfPropAssign_VertexCriteria_vertexToSurface_VAL
};

/** 
Enum with association roles. */
enum ads_SurfPropAssign_VertexCriteria_surfaceRolesEnm
{
    ads_SurfPropAssign_VertexCriteria_surface_referent,
    ads_SurfPropAssign_VertexCriteria_surface_referrer
};

/** Enum with association roles. */
enum ads_SurfPropAssign_VertexCriteria_vertexAngleThresholdRolesEnm
{
    ads_SurfPropAssign_VertexCriteria_vertexAngleThreshold_child,
    ads_SurfPropAssign_VertexCriteria_vertexAngleThreshold_parent
};

#endif
