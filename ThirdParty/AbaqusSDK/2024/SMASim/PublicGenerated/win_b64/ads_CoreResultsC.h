//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreResultsC_h
#define ads_CoreResultsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Results of the latest level of form Core */

/** An instance of this type represents an arc length. */
#define ads_ArcLength (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 0))

/** This is used with StaticRiks. */
#define ads_ArcLengthCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 1))

#define ads_ArcLengthGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 2))

/** This record represents a single cyclic symmetry mode. A c-set of these will be associated with the CyclicSymmetryModeSelection record. Each of these records will be associated with a c-set of modes. */
#define ads_CyclicSymmetryMode (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 3))

/** This collection represents a collection of cyclic symmetry modes. A c-set from this collection will be associated with the CyclicSymmetryModeSelection record. */
#define ads_CyclicSymmetryModeCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 4))

#define ads_DofFrequencyComplexNumberPartCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 5))

#define ads_DofFrequencyGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 6))

#define ads_DofModeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 7))

#define ads_DofTimeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 8))

/** An instance of this type represents a frequency point. */
#define ads_Frequency (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 9))

/** Frequency points contained in a sweep over a frequency range defined for a frequency domain procedures. */
#define ads_FrequencyCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 10))

#define ads_FrequencyComplexNumberPartCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 11))

#define ads_FrequencyGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 12))

/** An instance of the Mode type represents an eigenmode. */
#define ads_Mode (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 13))

/** Mode number in an eigenvalue analysis. */
#define ads_ModeCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 14))

/** Elements of this grid represent complex modal values. */
#define ads_ModeComplexNumberPartGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 15))

/** Elements of this grid represent influence of a mode on a degree of freedom type. */
#define ads_ModeDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 16))

/** Elements of this grid represent complex modal values for frequency points and load cases. */
#define ads_ModeFrequencyComplexNumberPartCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 17))

#define ads_ModeFrequencyGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 18))

#define ads_ModeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 19))

#define ads_ModeModeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 20))

/** Elements of this grid represent complex modal values for Parameters. */
#define ads_ModeParameterComplexNumberPartGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 21))

#define ads_ModeTimeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 22))

/** During the analysis a reference is created between the output request and the field representing the data for this request. This field also contains the quantity that is being output. In most cases there will be a single field is referenced by each output request, because the (elaborated) output request is very specific--one clock, one varaible, etc. However, there are cases where there could be more than one field. The first known case is with eulerian output. A single stress request leads to the creation of a stress field for each 'material instance'. */
#define ads_OutputRequest_field (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 23))

/** This D-Set associates one SurfaceOutputContribution to one Surface. If there is contact output, this indicates for a given node what surface are we considering that node to be in for the purposes of that output. The D-Set SurfaceOutputContributionInteraction is not enough to (implicitly) represent this relationship because of surface to surface contact. A surface to surface interaction may have contact output on both master and slave. In this case the interaction will be associated with two SurfaceOutputContribution c-members and the only way to tell which one is master and which one is slave is through the SurfaceOutputContributionSurface D-Set. */
#define ads_Results_contactOutputSurfaces (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 24))

/** Clients such as the Viewer, given a SurfaceOutputContribution member found in a contact output distribution, need to find the associated Interaction, because they need to display Interaction name, for example. This DSet associates one or more SurfaceOutputContribution entities with an Interaction. Initially all SurfaceOutputContribution entities are associated with some Interaction, but in the future, for surface output not related to contact for example, we may have a given SurfaceOutputContribution c-member associated with something else other than an Interaction. More than one SurfaceOutputContribution may be associated to the same Interaction. For example, Explicit outputs CPRESS and other contact variables on both the master and the slave Note that just SurfaceOutputContributionSurface D-Set is not enough because given a Surface it is not possible to find the appropriate Interaction. Going from Interaction to Surface is always possible, but going unequivocally from Surface to Interaction may not be possible if two Interactions share the same slave surface. */
#define ads_Results_interactionOutputContributions (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 25))

/** This provides direct access to the surface output contribution collection. */
#define ads_Results_surfaceOutputContributionCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 26))

/** Contact data is basically nodal in nature. The primary dimension of a contact output distribution is of type NodeCollection. However, there may be multiple groupings of nodal data representing output from different contact pairs or from different facets within a contact pair and these groupings may share nodes. In order to have all the groupings stored in the same distribution, an extra dimension is required. The c-members along that dimension are of type SurfaceOutputContribution. In other words, if we have 5 sets of nodes representing output for contact regions, the extra dimension will have 5 members of type SurfaceOutputContribution. Please note that, during analysis, both the time dimension and the SurfaceOutputContribution may be extended. */
#define ads_SurfaceOutputContribution (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 27))

/** See documentation of the SurfaceOutputContribution data type */
#define ads_SurfaceOutputContributionCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 28))

/** This grid is used to relate SurfaceOutputContribution to Interaction. For more on this relationship, see the SurfaceOutputContributionInteraction association. */
#define ads_SurfaceOutputContributionInteractionGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 29))

/** This grid is used to relate SurfaceOutputContribution to Surface. For more on this relationship, see the SurfaceOutputContributionSurface association. */
#define ads_SurfaceOutputContributionRegionGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 30))

/** Composition from a task (step, event) to the field results fields. */
#define ads_Task_fieldResults (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 31))

/** Composition from a task (step, event) to the history results fields. */
#define ads_Task_historyResults (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 32))

/** Composition from a task (step, event) to the state results fields. */
#define ads_Task_stateField (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 33))

/** An instance of this type represents a physical time point within the physical time period of a simulation. */
#define ads_Time (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 34))

#define ads_TimeCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 35))

/** Physical time points within the physical time period of a simulation. */
#define ads_TimeCollection (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 36))

#define ads_TimeGrid (ads_CoreFragmentTypeIndex(ads_CoreResultsFragment, 37))

/** Enum with grid dimensions. */
enum ads_ArcLengthGridDimensionsEnm
{
    ads_ArcLengthGrid_arcLength
};

/** 
Enum with record members. */
enum ads_CyclicSymmetryModeMembersEnm
{
    ads_CyclicSymmetryMode_modeNumber
};

/** Enum with grid dimensions. */
enum ads_DofFrequencyComplexNumberPartCaseGridDimensionsEnm
{
    ads_DofFrequencyComplexNumberPartCaseGrid_case,
    ads_DofFrequencyComplexNumberPartCaseGrid_complexPart,
    ads_DofFrequencyComplexNumberPartCaseGrid_dof,
    ads_DofFrequencyComplexNumberPartCaseGrid_frequency
};

/** Enum with grid dimensions. */
enum ads_DofFrequencyGridDimensionsEnm
{
    ads_DofFrequencyGrid_dof,
    ads_DofFrequencyGrid_frequency
};

/** Enum with grid dimensions. */
enum ads_DofModeGridDimensionsEnm
{
    ads_DofModeGrid_dof,
    ads_DofModeGrid_mode
};

/** Enum with grid dimensions. */
enum ads_DofTimeGridDimensionsEnm
{
    ads_DofTimeGrid_dof,
    ads_DofTimeGrid_time
};

/** Enum with grid dimensions. */
enum ads_FrequencyComplexNumberPartCaseGridDimensionsEnm
{
    ads_FrequencyComplexNumberPartCaseGrid_case,
    ads_FrequencyComplexNumberPartCaseGrid_complexPart,
    ads_FrequencyComplexNumberPartCaseGrid_frequency
};

/** Enum with grid dimensions. */
enum ads_FrequencyGridDimensionsEnm
{
    ads_FrequencyGrid_frequency
};

/** 
Enum with grid dimensions. */
enum ads_ModeComplexNumberPartGridDimensionsEnm
{
    ads_ModeComplexNumberPartGrid_complexPart,
    ads_ModeComplexNumberPartGrid_mode
};

/** 
Enum with grid dimensions. */
enum ads_ModeDofTypeGridDimensionsEnm
{
    ads_ModeDofTypeGrid_dofType,
    ads_ModeDofTypeGrid_mode
};

/** 
Enum with grid dimensions. */
enum ads_ModeFrequencyComplexNumberPartCaseGridDimensionsEnm
{
    ads_ModeFrequencyComplexNumberPartCaseGrid_case,
    ads_ModeFrequencyComplexNumberPartCaseGrid_complexPart,
    ads_ModeFrequencyComplexNumberPartCaseGrid_frequency,
    ads_ModeFrequencyComplexNumberPartCaseGrid_mode
};

/** Enum with grid dimensions. */
enum ads_ModeFrequencyGridDimensionsEnm
{
    ads_ModeFrequencyGrid_frequency,
    ads_ModeFrequencyGrid_mode
};

/** Enum with grid dimensions. */
enum ads_ModeGridDimensionsEnm
{
    ads_ModeGrid_mode
};

/** Enum with grid dimensions. */
enum ads_ModeModeGridDimensionsEnm
{
    ads_ModeModeGrid_column,
    ads_ModeModeGrid_row
};

/** 
Enum with grid dimensions. */
enum ads_ModeParameterComplexNumberPartGridDimensionsEnm
{
    ads_ModeParameterComplexNumberPartGrid_complexNumberPart,
    ads_ModeParameterComplexNumberPartGrid_mode,
    ads_ModeParameterComplexNumberPartGrid_parameter
};

/** Enum with grid dimensions. */
enum ads_ModeTimeGridDimensionsEnm
{
    ads_ModeTimeGrid_mode,
    ads_ModeTimeGrid_time
};

/** 
Enum with association roles. */
enum ads_OutputRequest_fieldRolesEnm
{
    ads_OutputRequest_field_referent,
    ads_OutputRequest_field_referrer
};

/** 
Enum with association roles. */
enum ads_Results_contactOutputSurfacesRolesEnm
{
    ads_Results_contactOutputSurfaces_child,
    ads_Results_contactOutputSurfaces_parent
};

/** 
Enum with association roles. */
enum ads_Results_interactionOutputContributionsRolesEnm
{
    ads_Results_interactionOutputContributions_child,
    ads_Results_interactionOutputContributions_parent
};

/** 
Enum with association roles. */
enum ads_Results_surfaceOutputContributionCollectionRolesEnm
{
    ads_Results_surfaceOutputContributionCollection_child,
    ads_Results_surfaceOutputContributionCollection_parent
};

/** 
Enum with grid dimensions. */
enum ads_SurfaceOutputContributionInteractionGridDimensionsEnm
{
    ads_SurfaceOutputContributionInteractionGrid_interaction,
    ads_SurfaceOutputContributionInteractionGrid_outputContribution
};

/** 
Enum with grid dimensions. */
enum ads_SurfaceOutputContributionRegionGridDimensionsEnm
{
    ads_SurfaceOutputContributionRegionGrid_outputContribution,
    ads_SurfaceOutputContributionRegionGrid_surface
};

/** 
Enum with association roles. */
enum ads_Task_fieldResultsRolesEnm
{
    ads_Task_fieldResults_child,
    ads_Task_fieldResults_parent
};

/** 
Enum with association roles. */
enum ads_Task_historyResultsRolesEnm
{
    ads_Task_historyResults_child,
    ads_Task_historyResults_parent
};

/** 
Enum with association roles. */
enum ads_Task_stateFieldRolesEnm
{
    ads_Task_stateField_child,
    ads_Task_stateField_parent
};

/** Enum with grid dimensions. */
enum ads_TimeCaseGridDimensionsEnm
{
    ads_TimeCaseGrid_case,
    ads_TimeCaseGrid_frame
};

/** Enum with grid dimensions. */
enum ads_TimeGridDimensionsEnm
{
    ads_TimeGrid_time
};

#endif
