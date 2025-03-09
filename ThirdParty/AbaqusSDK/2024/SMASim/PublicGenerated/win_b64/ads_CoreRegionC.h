//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreRegionC_h
#define ads_CoreRegionC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Region of the latest level of form Core */

/** Reference to Region records corresponding to the Mesh Group features owned by the corresponding Mesh Part feature. */
#define ads_MeshPart_regionRef (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 0))

/** Relationship to capture the design area of an optimization. */
#define ads_Model_designArea (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 1))

/** This is temporary, this will be removed in favour of Model_region relationship. */
#define ads_Model_elementSets (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 2))

/** Element CSets created for diagnostics. */
#define ads_Model_elementSetsDiagnostic (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 3))

/** This is temporary, this will be removed in favour of Model_region relationship. */
#define ads_Model_elementSetsInternal (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 4))

/** Relationship to capture frozen regions of an optimization. */
#define ads_Model_frozenRegions (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 5))

/** This is temporary, this will be removed in favour of Model_region relationship. */
#define ads_Model_nodeSets (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 6))

/** Node CSets created for diagnostics. */
#define ads_Model_nodeSetsDiagnostic (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 7))

/** This is temporary, this will be removed in favour of Model_region relationship. */
#define ads_Model_nodeSetsInternal (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 8))

/** A global namespace for Region names. Please note that this namespace cannot be used with any of the MDB, INP, or ODB objects or keywords. The older data models employ namespaces that specific to certain uses. For example, the INP has separate namespaces are for Elements sets, node sets, surfaces, Lagrangian regions in Eulerian anlayses ("material instance"), fluid cavities, etc.. These namespaces that are specific to a region-type are represented by other SIM associations. A link can be added to those associations with no link being added to this one. In terms of multiplicities, we believe that a Region will not be shared across Products. It can certainly be shared across the different meshes of a Product, but not between two Products. The string key of this association defines the name of the Region in the global Region namespace. This name is constrained in the case of an associativity region (see Model_associativityRegion), otherwise not. */
#define ads_Model_region (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 9))

/** The collection version of the regions. */
#define ads_Model_regionCollection (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 10))

/** Any model should have one and only one whole model region. This association links to that instance. The link and the instance are created by the data dictonaries. */
#define ads_Model_regionVolumeWholeModel (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 11))

/** This is temporary, this will be removed in favour of Occurrence_regionRef relationship.. */
#define ads_Occurrence_elementSetsRef (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 12))

/** This is temporary, this will be removed in favour of Occurrence_regionRef relationship.. */
#define ads_Occurrence_nodeSetsRef (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 13))

/** Defines the Occurrence of an assembly sub-component Model on which the region is defined. This link allows to define regions on different occurence of a reference geometrical entity. */
#define ads_Occurrence_regionRef (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 14))

/** The concept of a SIM Region data type is a combination of two characteristics--one top-down, the other bottom-up: Each Region record corresponds to a physical region in the system being modeled. The Region data type is the generalization of all geometric representation data types regardless of dimensionality or geometric modeling system. When we compare a physical region to its representation in CAD and to its representation in FE, the dimensionalities of the three may be all different: a physical volume, if it is thin enough, may be represented in CAD by a surface and may be represented in FE as a line (a shell in a 2D mesh). Of these three, the dimensionality that guides the upper layers of the SIM Region data model is the physical regions dimensionality. We believe this is the right approach because the usage of a region in simulation (for assigning properties, applying excitations, requesting output, etc.) is mostly dependent on the physical regions dimensionality. For example, when defining a surface to use in contact, the dimensionality of the physical region is always 2D, but the dimensionality of the FE representation of that surface can be 1D (in 2D meshes) or 2D (in 3D meshes). Similar examples exist for Loads, Boundary Conditions, etc. */
#define ads_Region (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 15))

/** Example of use: some results use the region as a qualifying dimension in a distribution. */
#define ads_RegionCollection (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 16))

/** An abstract type representing regions that are obtained by combination operations (union, intersection, etc) on other regions. That kind of region must have the externalRegionID member set to 'null'. */
#define ads_RegionCombined (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 17))

/** Grid spanning regions and components. */
#define ads_RegionComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 18))

/** Grid spanning regions */
#define ads_RegionGrid (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 19))

/** A SIM region which corresponds to a physical region of dimension 1. Note that the CAD representation or the Mesh representation of that physical region may be done with entities of even lower dimensionality. */
#define ads_RegionLine (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 20))

/** A SIM region which corresponds to a physical region of dimension 0. Note that a region can correspond to more than one point. */
#define ads_RegionPoint (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 21))

/** Defines a Region generated by a union of two or more regions. RegionUnion might be linked to a discrete region that represents the union of the individual discrete region of the combined regions. That will happen only when the union operation of the individual discrete regions would lead to the removal of duplicates entities. */
#define ads_RegionUnion (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 22))

/** Defines at least two regions involved in the union operation. */
#define ads_RegionUnion_region (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 23))

/** A SIM region which corresponds to a physical region of dimension 3. Note that the CAD representation or the Mesh representation of that physical region may be done with entities of lower dimensionality (quads in a 2D mesh). Volume regions may be contiguous or not. Volume regions may be discretized by an element set, a node set, a set of all the elements, faces, edges, and nodes in that volume, by a reference point, or by many other FEA constructs. */
#define ads_RegionVolume (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 24))

/** This record type represents the volume corresponding to the entire model. These records carry no discrete data. The client code should be able to obtain all the elements in the mesh (or all the nodes, or all the faces, etc.) by querying the Mesh directly. */
#define ads_RegionVolumeWholeModel (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 25))

/** Defines the elements of a particular discrete region representation of the region. */
#define ads_Region_elementsCluster (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 26))

/** Defines the elements edges of a particular discrete region representation of the region. */
#define ads_Region_elementsEdgesCluster (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 27))

/** Defines the elements faces of a particular discrete region representation of the region. */
#define ads_Region_elementsFacesCluster (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 28))

/** Defines the nodes of a particular discrete region representation of the region. */
#define ads_Region_nodesCluster (ads_CoreFragmentTypeIndex(ads_CoreRegionFragment, 29))

/** 
Enum with association roles. */
enum ads_MeshPart_regionRefRolesEnm
{
    ads_MeshPart_regionRef_referent,
    ads_MeshPart_regionRef_referrer
};

/** 
Enum with association roles. */
enum ads_Model_designAreaRolesEnm
{
    ads_Model_designArea_referent,
    ads_Model_designArea_referrer
};

/** 
Enum with association roles. */
enum ads_Model_elementSetsRolesEnm
{
    ads_Model_elementSets_referent,
    ads_Model_elementSets_referrer
};

/** 
Enum with association roles. */
enum ads_Model_elementSetsDiagnosticRolesEnm
{
    ads_Model_elementSetsDiagnostic_referent,
    ads_Model_elementSetsDiagnostic_referrer
};

/** 
Enum with association roles. */
enum ads_Model_elementSetsInternalRolesEnm
{
    ads_Model_elementSetsInternal_referent,
    ads_Model_elementSetsInternal_referrer
};

/** 
Enum with association roles. */
enum ads_Model_frozenRegionsRolesEnm
{
    ads_Model_frozenRegions_referent,
    ads_Model_frozenRegions_referrer
};

/** 
Enum with association roles. */
enum ads_Model_nodeSetsRolesEnm
{
    ads_Model_nodeSets_referent,
    ads_Model_nodeSets_referrer
};

/** 
Enum with association roles. */
enum ads_Model_nodeSetsDiagnosticRolesEnm
{
    ads_Model_nodeSetsDiagnostic_referent,
    ads_Model_nodeSetsDiagnostic_referrer
};

/** 
Enum with association roles. */
enum ads_Model_nodeSetsInternalRolesEnm
{
    ads_Model_nodeSetsInternal_referent,
    ads_Model_nodeSetsInternal_referrer
};

/** 
Enum with association roles. */
enum ads_Model_regionRolesEnm
{
    ads_Model_region_child,
    ads_Model_region_parent
};

/** 
Enum with association roles. */
enum ads_Model_regionCollectionRolesEnm
{
    ads_Model_regionCollection_child,
    ads_Model_regionCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_regionVolumeWholeModelRolesEnm
{
    ads_Model_regionVolumeWholeModel_child,
    ads_Model_regionVolumeWholeModel_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_elementSetsRefRolesEnm
{
    ads_Occurrence_elementSetsRef_referent,
    ads_Occurrence_elementSetsRef_referrer
};

/** 
Enum with association roles. */
enum ads_Occurrence_nodeSetsRefRolesEnm
{
    ads_Occurrence_nodeSetsRef_referent,
    ads_Occurrence_nodeSetsRef_referrer
};

/** 
Enum with association roles. */
enum ads_Occurrence_regionRefRolesEnm
{
    ads_Occurrence_regionRef_referent,
    ads_Occurrence_regionRef_referrer
};

/** 
Enum with record members. */
enum ads_RegionMembersEnm
{
    ads_Region_externalRegionID,
    ads_Region_hidden,
    ads_Region_internal,
    ads_Region_roleType
};

enum ads_Region_roleTypeEnm
{
    ads_Region_roleType_GENERIC,
    ads_Region_roleType_USERGROUP
};

/** 
Enum with record members. */
enum ads_RegionCombinedMembersEnm
{
    ads_RegionCombined_externalRegionID,
    ads_RegionCombined_hidden,
    ads_RegionCombined_internal,
    ads_RegionCombined_roleType
};

enum ads_RegionCombined_roleTypeEnm
{
    ads_RegionCombined_roleType_GENERIC,
    ads_RegionCombined_roleType_USERGROUP
};

/** 
Enum with grid dimensions. */
enum ads_RegionComponentGridDimensionsEnm
{
    ads_RegionComponentGrid_component,
    ads_RegionComponentGrid_region
};

/** 
Enum with grid dimensions. */
enum ads_RegionGridDimensionsEnm
{
    ads_RegionGrid_region
};

/** 
Enum with record members. */
enum ads_RegionLineMembersEnm
{
    ads_RegionLine_externalRegionID,
    ads_RegionLine_hidden,
    ads_RegionLine_internal,
    ads_RegionLine_roleType
};

enum ads_RegionLine_roleTypeEnm
{
    ads_RegionLine_roleType_GENERIC,
    ads_RegionLine_roleType_USERGROUP
};

/** 
Enum with record members. */
enum ads_RegionPointMembersEnm
{
    ads_RegionPoint_externalRegionID,
    ads_RegionPoint_hidden,
    ads_RegionPoint_internal,
    ads_RegionPoint_roleType
};

enum ads_RegionPoint_roleTypeEnm
{
    ads_RegionPoint_roleType_GENERIC,
    ads_RegionPoint_roleType_USERGROUP
};

/** 
Enum with record members. */
enum ads_RegionUnionMembersEnm
{
    ads_RegionUnion_externalRegionID,
    ads_RegionUnion_hidden,
    ads_RegionUnion_internal,
    ads_RegionUnion_roleType
};

enum ads_RegionUnion_roleTypeEnm
{
    ads_RegionUnion_roleType_GENERIC,
    ads_RegionUnion_roleType_USERGROUP
};

/** 
Enum with association roles. */
enum ads_RegionUnion_regionRolesEnm
{
    ads_RegionUnion_region_referent,
    ads_RegionUnion_region_referrer
};

/** 
Enum with record members. */
enum ads_RegionVolumeMembersEnm
{
    ads_RegionVolume_externalRegionID,
    ads_RegionVolume_hidden,
    ads_RegionVolume_internal,
    ads_RegionVolume_roleType
};

enum ads_RegionVolume_roleTypeEnm
{
    ads_RegionVolume_roleType_GENERIC,
    ads_RegionVolume_roleType_USERGROUP
};

/** 
Enum with record members. */
enum ads_RegionVolumeWholeModelMembersEnm
{
    ads_RegionVolumeWholeModel_externalRegionID,
    ads_RegionVolumeWholeModel_hidden,
    ads_RegionVolumeWholeModel_internal,
    ads_RegionVolumeWholeModel_roleType
};

enum ads_RegionVolumeWholeModel_roleTypeEnm
{
    ads_RegionVolumeWholeModel_roleType_GENERIC,
    ads_RegionVolumeWholeModel_roleType_USERGROUP
};

/** 
Enum with association roles. */
enum ads_Region_elementsClusterRolesEnm
{
    ads_Region_elementsCluster_referent,
    ads_Region_elementsCluster_referrer
};

/** 
Enum with association roles. */
enum ads_Region_elementsEdgesClusterRolesEnm
{
    ads_Region_elementsEdgesCluster_child,
    ads_Region_elementsEdgesCluster_parent
};

/** 
Enum with association roles. */
enum ads_Region_elementsFacesClusterRolesEnm
{
    ads_Region_elementsFacesCluster_child,
    ads_Region_elementsFacesCluster_parent
};

/** 
Enum with association roles. */
enum ads_Region_nodesClusterRolesEnm
{
    ads_Region_nodesCluster_referent,
    ads_Region_nodesCluster_referrer
};

#endif
