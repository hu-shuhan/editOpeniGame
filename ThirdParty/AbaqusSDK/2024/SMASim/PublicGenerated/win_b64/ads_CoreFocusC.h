//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFocusC_h
#define ads_CoreFocusC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Focus of the latest level of form Core */

/** A simplified representation of an external region */
#define ads_ExternalRegion (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 0))

/** An external region representing a whole model */
#define ads_ExternalRegion_WholeModel (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 1))

/** Represents an external SIM source of data, i.e. allows for loose coupling between SIMDocs. Used primarily to capture the name of another SIMDoc to support use cases corresponding to Abaqus *Keyword, file=... */
#define ads_ExternalSource (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 2))

/** . */
#define ads_ExternalSource_regions (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 3))

/** . */
#define ads_Focus_externalDrawers (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 4))

/** Provide storage for ids of load cases in data models that are external to the SIM data model, such as the V6 Feature objects for load case. */
#define ads_Focus_externalLoadCaseIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 5))

/** Provide storage for ids of Material in data models that are external to the SIM data model, such as the V6 Feature objects for Material. */
#define ads_Focus_externalMaterialIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 6))

/** Provide storage for ids of MeshParts in data models that are external to the SIM data model, such as the V6 meshing objects for MeshPart. */
#define ads_Focus_externalMeshPartIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 7))

/** Provide storage for ids of plys in data models that are external to the SIM data model, such as the V6 Feature objects for Region. */
#define ads_Focus_externalPlyIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 8))

/** Provide storage for ids of regions in data models that are external to the SIM data model, such as the V6 Feature objects for Region. */
#define ads_Focus_externalRegionIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 9))

/** Provide storage for ids of sections in data models that are external to the SIM data model, such as the V6 Feature objects for Task. */
#define ads_Focus_externalSectionIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 10))

/** Links ExternalSource records to the Focus. */
#define ads_Focus_externalSources (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 11))

/** Provide storage for ids of tasks in data models that are external to the SIM data model, such as the V6 Feature objects for Task. */
#define ads_Focus_externalTaskIds (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 12))

/** . */
#define ads_Focus_externallyReferredDrawers (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 13))

/** Global collections. */
#define ads_Focus_globalCollections (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 14))

/** Provides access to the single JobDiagnosticSummary record produced by running the analysis. Please note that the majority of the diagnostics data is associated to individual steps, not to the Results record, and not to the JobDiagnosticSummary record. */
#define ads_Focus_jobDiagnosticSummary (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 15))

/** Focus to results composition. */
#define ads_Focus_results (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 16))

/** Links SymbolsLib record to the Focus. */
#define ads_Focus_symbolsLib (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 17))

/** The anchor record for global collections. Throughout the entire document, there is, at most, one instance of each "global collection" type. Those collection instances are held under this record. */
#define ads_GlobalCollections (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 18))

/** The anchor record for the results data model. */
#define ads_Results (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 19))

#define ads_SymbolsLib (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 20))

/** Aggregation of ComplexNumberPart symbols. */
#define ads_SymbolsLib_complexNumberParts (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 21))

#define ads_SymbolsLib_component (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 22))

/** Aggregation of DistributingRotCouplingMethod symbols. */
#define ads_SymbolsLib_distributingCouplingMethods (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 23))

/** Aggregation of DistributingRotCouplingMethod symbols. */
#define ads_SymbolsLib_distributingRotCouplingMethods (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 24))

/** Aggregation of DistributingWeightingMethod symbols. */
#define ads_SymbolsLib_distributingWeightingMethods (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 25))

/** Aggregation of DofType symbols. */
#define ads_SymbolsLib_dofTypes (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 26))

/** Aggregation of Invariant symbols. */
#define ads_SymbolsLib_invariants (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 27))

/** Aggregation of LimitType symbols. */
#define ads_SymbolsLib_limitTypes (ads_CoreFragmentTypeIndex(ads_CoreFocusFragment, 28))

/** 
Enum with record members. */
enum ads_ExternalSourceMembersEnm
{
    ads_ExternalSource_sourceName
};

/** 
Enum with association roles. */
enum ads_ExternalSource_regionsRolesEnm
{
    ads_ExternalSource_regions_child,
    ads_ExternalSource_regions_parent
};

/** 
Enum with association roles. */
enum ads_Focus_externalDrawersRolesEnm
{
    ads_Focus_externalDrawers_referent,
    ads_Focus_externalDrawers_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalLoadCaseIdsRolesEnm
{
    ads_Focus_externalLoadCaseIds_referent,
    ads_Focus_externalLoadCaseIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalMaterialIdsRolesEnm
{
    ads_Focus_externalMaterialIds_referent,
    ads_Focus_externalMaterialIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalMeshPartIdsRolesEnm
{
    ads_Focus_externalMeshPartIds_referent,
    ads_Focus_externalMeshPartIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalPlyIdsRolesEnm
{
    ads_Focus_externalPlyIds_referent,
    ads_Focus_externalPlyIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalRegionIdsRolesEnm
{
    ads_Focus_externalRegionIds_referent,
    ads_Focus_externalRegionIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalSectionIdsRolesEnm
{
    ads_Focus_externalSectionIds_referent,
    ads_Focus_externalSectionIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externalSourcesRolesEnm
{
    ads_Focus_externalSources_child,
    ads_Focus_externalSources_parent
};

/** 
Enum with association roles. */
enum ads_Focus_externalTaskIdsRolesEnm
{
    ads_Focus_externalTaskIds_referent,
    ads_Focus_externalTaskIds_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_externallyReferredDrawersRolesEnm
{
    ads_Focus_externallyReferredDrawers_referent,
    ads_Focus_externallyReferredDrawers_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_globalCollectionsRolesEnm
{
    ads_Focus_globalCollections_child,
    ads_Focus_globalCollections_parent
};

/** 
Enum with association roles. */
enum ads_Focus_jobDiagnosticSummaryRolesEnm
{
    ads_Focus_jobDiagnosticSummary_child,
    ads_Focus_jobDiagnosticSummary_parent
};

/** 
Enum with association roles. */
enum ads_Focus_resultsRolesEnm
{
    ads_Focus_results_child,
    ads_Focus_results_parent
};

/** 
Enum with association roles. */
enum ads_Focus_symbolsLibRolesEnm
{
    ads_Focus_symbolsLib_child,
    ads_Focus_symbolsLib_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_complexNumberPartsRolesEnm
{
    ads_SymbolsLib_complexNumberParts_child,
    ads_SymbolsLib_complexNumberParts_parent
};

/** Enum with association roles. */
enum ads_SymbolsLib_componentRolesEnm
{
    ads_SymbolsLib_component_child,
    ads_SymbolsLib_component_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_distributingCouplingMethodsRolesEnm
{
    ads_SymbolsLib_distributingCouplingMethods_child,
    ads_SymbolsLib_distributingCouplingMethods_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_distributingRotCouplingMethodsRolesEnm
{
    ads_SymbolsLib_distributingRotCouplingMethods_child,
    ads_SymbolsLib_distributingRotCouplingMethods_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_distributingWeightingMethodsRolesEnm
{
    ads_SymbolsLib_distributingWeightingMethods_child,
    ads_SymbolsLib_distributingWeightingMethods_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_dofTypesRolesEnm
{
    ads_SymbolsLib_dofTypes_child,
    ads_SymbolsLib_dofTypes_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_invariantsRolesEnm
{
    ads_SymbolsLib_invariants_child,
    ads_SymbolsLib_invariants_parent
};

/** 
Enum with association roles. */
enum ads_SymbolsLib_limitTypesRolesEnm
{
    ads_SymbolsLib_limitTypes_child,
    ads_SymbolsLib_limitTypes_parent
};

#endif
