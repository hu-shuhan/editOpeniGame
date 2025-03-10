//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreMeshC_h
#define ads_CoreMeshC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Mesh of the latest level of form Core */

/** This is the top level cyclic symmetry record that links all of the cyclic symmetry modes for various steps together. It also contains the number of segments defined for cyclic symmetry. */
#define ads_CyclicSymmetry (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 0))

/** This record represents a degree of freedom. Dofs are most of the time used as c-members. */
#define ads_Dof (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 1))

/** A collection of degrees of freedom */
#define ads_DofCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 2))

#define ads_DofDofGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 3))

#define ads_DofGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 4))

/** An FEA mesh element. */
#define ads_Element (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 5))

/** This data record is used to define a set of elements sharing same criteria. */
#define ads_ElementClass (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 6))

#define ads_ElementClassCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 7))

/** This grid's members decompose the elements of a Mesh based on the intersection all the criteria of an ElementClassification. Each Mesh Element is associated to one and only one ElementClass, and each ElementClass is associated to at least one Element. */
#define ads_ElementClassElementGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 8))

/** Associates a elementSpec decomposition criterion to a ElementClass. */
#define ads_ElementClass_elementSpec (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 9))

/** The Sections of the elements in this ElementClass. For a given section in this set, not all elements that the Section is assigned to are necessarily in this class. If they have a different element type, they will be in a different element class. Two sections will be referred by the same element class if they have the same values for the criteria that defines the current element classification. */
#define ads_ElementClass_section (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 10))

/** This data record is used to define sets of elements that share the same properties and characteristics. */
#define ads_ElementClassification (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 11))

/** Defines the intersection of the criteria of an ElementClassification. */
#define ads_ElementClassification_elementClass (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 12))

/** Associates Mesh Element decomposition according to the MeshClassification criteria. */
#define ads_ElementClassification_membership (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 13))

#define ads_ElementCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 14))

/** Can be used to assign vectors and tensors to elements, for example. */
#define ads_ElementComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 15))

/** Grid spanning edges of mesh elements */
#define ads_ElementEdgeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 16))

/** Grid spanning elements, faces and components. An example usage of such a grid is when traction forces are applied to a surface. */
#define ads_ElementFaceComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 17))

/** The edges local to a face. */
#define ads_ElementFaceEdgeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 18))

/** Grid spanning faces of mesh elements */
#define ads_ElementFaceGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 19))

/** Grid spanning mesh elements */
#define ads_ElementGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 20))

#define ads_ElementNodeComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 21))

#define ads_ElementNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 22))

/** This anchor data record is used to aggregate element-node normal data. Specifically, it has three synchronized child distributions: the first to capture a sequence of elements, the second to capture a corresponding sequence of nodes, and the third to capture a corresponding sequence of normalized direction vectors. By synchronized, we mean the indexing of the three sequences is identical. The indexing CSet is from the GenericCollection. The distributions should each consist of a singe drawers. */
#define ads_ElementNodeNormals (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 23))

/** Distribution to store the sequence of elements for which element-node normals are defined. The domain is a GenericGrid, used as a counting collection. The range values are members of ElementCollection. */
#define ads_ElementNodeNormals_elements (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 24))

/** Distribution to store the nodes for which elementXnode based normals are defined. The domain is a GenericGrid, used as a counting collection. The range is members of NodeCollection. */
#define ads_ElementNodeNormals_nodes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 25))

/** Distribution to store the sequence of normals for which element-node normals are defined. The domain is a GenericComponentGrid, used as a counting collection. The range values are normalized direction vectors. */
#define ads_ElementNodeNormals_normals (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 26))

/** Element x Point x Component */
#define ads_ElementPointComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 27))

/** The local nodes of an element. */
#define ads_ElementPointGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 28))

/** The local nodes of an element. */
#define ads_ElementPointNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 29))

/** The local nodes of an element. */
#define ads_ElementPointTemperaturePointGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 30))

/** This data record is used to define ElementSpec. */
#define ads_ElementSpec (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 31))

#define ads_ElementSpecCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 32))

/** Elements of this grid are element specs to elements. */
#define ads_ElementSpecElementGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 33))

/** This data record is used to define the type of a substructure element in a substructure usage scenario. The data members in this record are currently very simple, and will most likely be revised at a later date to more efficiently connect to the substructure generation SIM file. */
#define ads_ElementSpec_Substructure (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 34))

/** Associates a topology decomposition criterion to a ElementSpec. */
#define ads_ElementSpec_elementTopology (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 35))

/** Associate local point set to the ElementSpec record. */
#define ads_ElementSpec_pointSet (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 36))

/** For user defined elementspec, the format of name must be Un, where n is a positive integer less than 10000. */
#define ads_Focus_elementSpecs (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 37))

/** Data type for each Geometric Body. */
#define ads_GBody (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 38))

/** Collection for the GBody type. */
#define ads_GBodyCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 39))

/** Data type for each 3DCell in a 3DShape. */
#define ads_GCell (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 40))

/** Collection for the GCell type. */
#define ads_GCellCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 41))

/** Data type for each 3DShape in a FEMRep. */
#define ads_GShape (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 42))

/** Collection for the GShape type. */
#define ads_GShapeCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 43))

#define ads_GShapeComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 44))

#define ads_GShapeGBodyGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 45))

#define ads_GShapeGCellDomainGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 46))

#define ads_GShapeGCellElementEdgeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 47))

#define ads_GShapeGCellElementFaceGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 48))

#define ads_GShapeGCellElementGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 49))

#define ads_GShapeGCellNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 50))

#define ads_GShapeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 51))

#define ads_GenericComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 52))

/** Collection of element specs. */
#define ads_GlobalCollections_elementSpecCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 53))

/** The Mesh record aggregates the collections and distributions that define a mesh, and the records that describe its characteristics. */
#define ads_Mesh (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 54))

/** A mesh domain is a set of elements which belongs to a given mesh part. */
#define ads_MeshDomain (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 55))

#define ads_MeshDomainCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 56))

#define ads_MeshDomain_elementSet (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 57))

/** A mesh part is a set a elements which has been generated by a given mesh algorithm. A mesh part generally corresponds to a given geometrical support. For stand-alone HPViz purposes, inp2sim create 1 MeshPart and 1 MeshDomain. */
#define ads_MeshPart (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 58))

/** Associates the mesh part with an element connectivity dset. */
#define ads_MeshPart_connectivity (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 59))

/** Associates the mesh part with the coordinates distribution */
#define ads_MeshPart_coordinates (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 60))

/** Associates the mesh part with an element spec assignment dset. */
#define ads_MeshPart_elementSpecAssignment (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 61))

#define ads_MeshPart_frozenElementSet (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 62))

/** DSet to store the associativity of geometric edge to FE edge with(GShape X GCell X Element X Edge)grid. */
#define ads_MeshPart_gEdgeEdgeAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 63))

/** DSet to store the associativity of geometric edge to FE element with(GShape X GCell X Element)grid. */
#define ads_MeshPart_gEdgeElementAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 64))

/** DSet to store the associativity of geometric face to FE element with(GShape X GCell X Element)grid. */
#define ads_MeshPart_gFaceElementAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 65))

/** DSet to store the associativity of geometric face to FE face with(GShape X GCell X Element X Face)grid. */
#define ads_MeshPart_gFaceFaceAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 66))

/** DSet to store the associativity of geometric vertex to FE node with (GShape X GCell X Node)grid. */
#define ads_MeshPart_gVertexNodeAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 67))

/** DSet to store the associativity of geometric cell to MeshDomain with(GShape X GCell X MeshDomain)grid. */
#define ads_MeshPart_gVolumeDomainAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 68))

/** A given mesh part is partitioned by its mesh domains. */
#define ads_MeshPart_meshDomains (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 69))

#define ads_MeshPart_nodeSet (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 70))

/** Captures the GShapes and GBodies with which the elements of the MeshPart are associated. */
#define ads_MeshPart_shapeBodies (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 71))

/** Aggregates statistical information about the mesh. */
#define ads_MeshStatistics (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 72))

/** Implicit analysis processing (Abaqus/Standard) requires that element connectivity be expanded to include all node associated with the element through constraints and through nodes added for Lagrange multipliers. Rather than replacing the connectivity based on the nodes used to define the basic geometry of the element, we have separated out the added nodes into an auxiliary connectivity Dset that is then associated with the Mesh. Local node ordinals within the auxiliary connectivity begin at 0 (NOT at the ordinal after the last geometry node of the element). */
#define ads_Mesh_auxiliaryConnectivity (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 73))

/** Composition to Dset to store information for cases where beam orientation is specified through extra node. */
#define ads_Mesh_beamOrientationExtraNodes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 74))

/** n1 direction for beam sections. */
#define ads_Mesh_beamSectionNormals (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 75))

/** Element connectivity. For some use of SIM meshes, the element connectivity may be expanded through the MeshAuxiliaryConnectivity association. */
#define ads_Mesh_connectivity (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 76))

/** The global nodes coordinates. */
#define ads_Mesh_coordinates (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 77))

/** This is a top level composition that binds a mesh with a cyclic symmetry definition */
#define ads_Mesh_cyclicSymmetry (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 78))

/** A collection of degrees of freedom of the mesh. */
#define ads_Mesh_dofCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 79))

/** The set of all degrees of freedom in the finite element model. */
#define ads_Mesh_dofs (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 80))

/** The ElementClass datarecord exists as a datarecord and as a cmember in ElementClassCollection. */
#define ads_Mesh_elementClasses (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 81))

/** The elements of a mesh. */
#define ads_Mesh_elementCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 82))

#define ads_Mesh_elementNodeNormals (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 83))

/** Associates the mesh with a element Spec assignment dset. */
#define ads_Mesh_elementSpecAssignment (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 84))

/** The set of all elements in the finite element model. */
#define ads_Mesh_elements (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 85))

/** This composition represents mapping from the element indices in the occurence mesh to the user labels in the occurence labels mesh. */
#define ads_Mesh_elementsMeshToLabelsMeshMap (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 86))

/** DSet to store the associativity of geometric edge to FE edge with(GShape X GCell X Element X Edge)grid. */
#define ads_Mesh_gEdgeEdgeAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 87))

/** DSet to store the associativity of geometric edge to FE element with(GShape X GCell X Element)grid. */
#define ads_Mesh_gEdgeElementAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 88))

/** DSet to store the associativity of geometric face to FE element with(GShape X GCell X Element)grid. */
#define ads_Mesh_gFaceElementAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 89))

/** DSet to store the associativity of geometric face to FE face with(GShape X GCell X Element X Face)grid. */
#define ads_Mesh_gFaceFaceAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 90))

/** DSet to store the associativity of geometric vertex to FE node with (GShape X GCell X Node)grid. */
#define ads_Mesh_gVertexNodeAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 91))

/** DSet to store the associativity of geometric cell to MeshDomain with(GShape X GCell X MeshDomain)grid. */
#define ads_Mesh_gVolumeDomainAssociativities (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 92))

/** Cimposition to Dset to store the reference node information for the IRS* and drag chain (3D) elements. Note- MeshBeamOrientationExtraNodes could have been used to store this refrence node but these element types are on verge of extinction so it was decided to use different dset association for these elements. */
#define ads_Mesh_irsDragchainReferenceNodes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 93))

/** A mesh has a User label Mesh. This Mesh is used to store user labels. */
#define ads_Mesh_labelsMesh (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 94))

/** The MeshDomains of a mesh. */
#define ads_Mesh_meshDomainCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 95))

/** A given mesh is partitioned by its mesh parts. */
#define ads_Mesh_meshParts (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 96))

/** The nodalDofType d-set associates nodes with DofType records activated in those nodes. This d-set is computed from connectivity and element types. */
#define ads_Mesh_nodalDofTypes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 97))

/** The Mesh nodes. */
#define ads_Mesh_nodeCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 98))

/** A local coordinate system for displacement and rotation degrees of freedom at a node. These data are independent of the way loads, BCs, equations, retained DOFs, etc. allow users to specify local coordinate systems. */
#define ads_Mesh_nodeInternalLocalSys (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 99))

/** The global nodes normals. */
#define ads_Mesh_nodeNormals (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 100))

/** The set of all nodes in the finite element model. */
#define ads_Mesh_nodes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 101))

/** This composition represents mapping from the node indices in the occurence mesh to the user labels in the occurence labels mesh. */
#define ads_Mesh_nodesMeshToLabelsMeshMap (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 102))

/** It was decided in a meeting between all the SIM clients and the data modelers that the tracer particles in a SIM mesh would be represented by just a node set, not by a special collection. At this point we are leaving the door open for the existence of several sets (maxOccurs=unbounded) with the understanding that the set of tracer particles is the union of all sets in this association. */
#define ads_Mesh_set (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 103))

/** Defines an ElementClassification based on shapes, formulation, layers, section points and material criteria. */
#define ads_Mesh_shapeFormulationLogicalSectionClassification (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 104))

/** A mesh contains a statistics record. */
#define ads_Mesh_statistics (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 105))

/** This composition represents mapping from the element indices in the occurence mesh to the user labels in the occurence labels mesh. Using the distribution instead of the CMemberMap in elementsMeshToLabelsMeshMap relation is more efficient. This distribution typically contains one drawer per MeshPart. */
#define ads_Mesh_userElementLabels (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 106))

/** The set of elements in the original mesh created by the user, before any elaboration. This is frequently also the set of elements which have user labels, but not always (the labels are optional). */
#define ads_Mesh_userElements (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 107))

/** This composition represents mapping from the node indices in the occurence mesh to the user labels in the occurence labels mesh. Using the distribution instead of the CMemberMap in nodesMeshToLabelsMeshMap relation is more efficient. This distribution typically contains one drawer per MeshPart. */
#define ads_Mesh_userNodeLabels (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 108))

/** The set of nodes in the original mesh created by the user, before any elaboration. This is frequently also the set of nodes which have user labels, but not always (the labels are optional). */
#define ads_Mesh_userNodes (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 109))

/** An FEA mesh node. */
#define ads_Node (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 110))

#define ads_NodeCollection (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 111))

/** Node x Component */
#define ads_NodeComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 112))

/** Elements of this grid are dof types active in nodes. */
#define ads_NodeDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 113))

/** Grid spanning mesh nodes */
#define ads_NodeGrid (ads_CoreFragmentTypeIndex(ads_CoreMeshFragment, 114))

/** 
Enum with record members. */
enum ads_CyclicSymmetryMembersEnm
{
    ads_CyclicSymmetry_numberOfSectors,
    ads_CyclicSymmetry_point1x,
    ads_CyclicSymmetry_point1y,
    ads_CyclicSymmetry_point1z,
    ads_CyclicSymmetry_point2x,
    ads_CyclicSymmetry_point2y,
    ads_CyclicSymmetry_point2z
};

/** Enum with grid dimensions. */
enum ads_DofDofGridDimensionsEnm
{
    ads_DofDofGrid_column,
    ads_DofDofGrid_row
};

/** Enum with grid dimensions. */
enum ads_DofGridDimensionsEnm
{
    ads_DofGrid_dofs
};

/** 
Enum with grid dimensions. */
enum ads_ElementClassElementGridDimensionsEnm
{
    ads_ElementClassElementGrid_element,
    ads_ElementClassElementGrid_elementClass
};

/** 
Enum with association roles. */
enum ads_ElementClass_elementSpecRolesEnm
{
    ads_ElementClass_elementSpec_referent,
    ads_ElementClass_elementSpec_referrer
};

/** 
Enum with association roles. */
enum ads_ElementClass_sectionRolesEnm
{
    ads_ElementClass_section_referent,
    ads_ElementClass_section_referrer
};

/** 
Enum with association roles. */
enum ads_ElementClassification_elementClassRolesEnm
{
    ads_ElementClassification_elementClass_child,
    ads_ElementClassification_elementClass_parent
};

/** 
Enum with association roles. */
enum ads_ElementClassification_membershipRolesEnm
{
    ads_ElementClassification_membership_child,
    ads_ElementClassification_membership_parent
};

/** 
Enum with grid dimensions. */
enum ads_ElementComponentGridDimensionsEnm
{
    ads_ElementComponentGrid_component,
    ads_ElementComponentGrid_element
};

/** 
Enum with grid dimensions. */
enum ads_ElementEdgeGridDimensionsEnm
{
    ads_ElementEdgeGrid_edge,
    ads_ElementEdgeGrid_element
};

/** 
Enum with grid dimensions. */
enum ads_ElementFaceComponentGridDimensionsEnm
{
    ads_ElementFaceComponentGrid_component,
    ads_ElementFaceComponentGrid_element,
    ads_ElementFaceComponentGrid_face
};

/** 
Enum with grid dimensions. */
enum ads_ElementFaceEdgeGridDimensionsEnm
{
    ads_ElementFaceEdgeGrid_edge,
    ads_ElementFaceEdgeGrid_element,
    ads_ElementFaceEdgeGrid_face
};

/** 
Enum with grid dimensions. */
enum ads_ElementFaceGridDimensionsEnm
{
    ads_ElementFaceGrid_element,
    ads_ElementFaceGrid_face
};

/** 
Enum with grid dimensions. */
enum ads_ElementGridDimensionsEnm
{
    ads_ElementGrid_element
};

/** Enum with grid dimensions. */
enum ads_ElementNodeComponentGridDimensionsEnm
{
    ads_ElementNodeComponentGrid_component,
    ads_ElementNodeComponentGrid_element,
    ads_ElementNodeComponentGrid_node
};

/** Enum with grid dimensions. */
enum ads_ElementNodeGridDimensionsEnm
{
    ads_ElementNodeGrid_element,
    ads_ElementNodeGrid_node
};

/** 
Enum with association roles. */
enum ads_ElementNodeNormals_elementsRolesEnm
{
    ads_ElementNodeNormals_elements_child,
    ads_ElementNodeNormals_elements_parent
};

/** 
Enum with association roles. */
enum ads_ElementNodeNormals_nodesRolesEnm
{
    ads_ElementNodeNormals_nodes_child,
    ads_ElementNodeNormals_nodes_parent
};

/** 
Enum with association roles. */
enum ads_ElementNodeNormals_normalsRolesEnm
{
    ads_ElementNodeNormals_normals_child,
    ads_ElementNodeNormals_normals_parent
};

/** 
Enum with grid dimensions. */
enum ads_ElementPointComponentGridDimensionsEnm
{
    ads_ElementPointComponentGrid_component,
    ads_ElementPointComponentGrid_element,
    ads_ElementPointComponentGrid_point
};

/** 
Enum with grid dimensions. */
enum ads_ElementPointGridDimensionsEnm
{
    ads_ElementPointGrid_element,
    ads_ElementPointGrid_point
};

/** 
Enum with grid dimensions. */
enum ads_ElementPointNodeGridDimensionsEnm
{
    ads_ElementPointNodeGrid_element,
    ads_ElementPointNodeGrid_node,
    ads_ElementPointNodeGrid_point
};

/** 
Enum with grid dimensions. */
enum ads_ElementPointTemperaturePointGridDimensionsEnm
{
    ads_ElementPointTemperaturePointGrid_element,
    ads_ElementPointTemperaturePointGrid_point,
    ads_ElementPointTemperaturePointGrid_temperaturePoint
};

/** 
Enum with record members. */
enum ads_ElementSpecMembersEnm
{
    ads_ElementSpec_spaceDimension,
    ads_ElementSpec_usage
};

enum ads_ElementSpec_spaceDimensionEnm
{
    ads_ElementSpec_spaceDimension_SPACE_1D,
    ads_ElementSpec_spaceDimension_SPACE_2D,
    ads_ElementSpec_spaceDimension_SPACE_3D
};

enum ads_ElementSpec_usageEnm
{
    ads_ElementSpec_usage_ABAQUS,
    ads_ElementSpec_usage_MESH
};

/** 
Enum with grid dimensions. */
enum ads_ElementSpecElementGridDimensionsEnm
{
    ads_ElementSpecElementGrid_element,
    ads_ElementSpecElementGrid_elementSpec
};

/** 
Enum with record members. */
enum ads_ElementSpec_SubstructureMembersEnm
{
    ads_ElementSpec_Substructure_spaceDimension,
    ads_ElementSpec_Substructure_usage,
    ads_ElementSpec_Substructure_file
};

enum ads_ElementSpec_Substructure_spaceDimensionEnm
{
    ads_ElementSpec_Substructure_spaceDimension_SPACE_1D,
    ads_ElementSpec_Substructure_spaceDimension_SPACE_2D,
    ads_ElementSpec_Substructure_spaceDimension_SPACE_3D
};

enum ads_ElementSpec_Substructure_usageEnm
{
    ads_ElementSpec_Substructure_usage_ABAQUS,
    ads_ElementSpec_Substructure_usage_MESH
};

/** 
Enum with association roles. */
enum ads_ElementSpec_elementTopologyRolesEnm
{
    ads_ElementSpec_elementTopology_referent,
    ads_ElementSpec_elementTopology_referrer
};

/** 
Enum with association roles. */
enum ads_ElementSpec_pointSetRolesEnm
{
    ads_ElementSpec_pointSet_referent,
    ads_ElementSpec_pointSet_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_elementSpecsRolesEnm
{
    ads_Focus_elementSpecs_child,
    ads_Focus_elementSpecs_parent
};

/** Enum with grid dimensions. */
enum ads_GShapeComponentGridDimensionsEnm
{
    ads_GShapeComponentGrid_component,
    ads_GShapeComponentGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGBodyGridDimensionsEnm
{
    ads_GShapeGBodyGrid_gbody,
    ads_GShapeGBodyGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGCellDomainGridDimensionsEnm
{
    ads_GShapeGCellDomainGrid_domain,
    ads_GShapeGCellDomainGrid_gcell,
    ads_GShapeGCellDomainGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGCellElementEdgeGridDimensionsEnm
{
    ads_GShapeGCellElementEdgeGrid_edge,
    ads_GShapeGCellElementEdgeGrid_element,
    ads_GShapeGCellElementEdgeGrid_gcell,
    ads_GShapeGCellElementEdgeGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGCellElementFaceGridDimensionsEnm
{
    ads_GShapeGCellElementFaceGrid_element,
    ads_GShapeGCellElementFaceGrid_face,
    ads_GShapeGCellElementFaceGrid_gcell,
    ads_GShapeGCellElementFaceGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGCellElementGridDimensionsEnm
{
    ads_GShapeGCellElementGrid_element,
    ads_GShapeGCellElementGrid_gcell,
    ads_GShapeGCellElementGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GShapeGCellNodeGridDimensionsEnm
{
    ads_GShapeGCellNodeGrid_gcell,
    ads_GShapeGCellNodeGrid_gshape,
    ads_GShapeGCellNodeGrid_node
};

/** Enum with grid dimensions. */
enum ads_GShapeGridDimensionsEnm
{
    ads_GShapeGrid_gshape
};

/** Enum with grid dimensions. */
enum ads_GenericComponentGridDimensionsEnm
{
    ads_GenericComponentGrid_component,
    ads_GenericComponentGrid_generic
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_elementSpecCollectionRolesEnm
{
    ads_GlobalCollections_elementSpecCollection_child,
    ads_GlobalCollections_elementSpecCollection_parent
};

/** 
Enum with record members. */
enum ads_MeshDomainMembersEnm
{
    ads_MeshDomain_domainTag
};

/** Enum with association roles. */
enum ads_MeshDomain_elementSetRolesEnm
{
    ads_MeshDomain_elementSet_referent,
    ads_MeshDomain_elementSet_referrer
};

/** 
Enum with record members. */
enum ads_MeshPartMembersEnm
{
    ads_MeshPart_isActive,
    ads_MeshPart_mptype,
    ads_MeshPart_stringTag
};

/** 
Enum with association roles. */
enum ads_MeshPart_connectivityRolesEnm
{
    ads_MeshPart_connectivity_child,
    ads_MeshPart_connectivity_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_coordinatesRolesEnm
{
    ads_MeshPart_coordinates_child,
    ads_MeshPart_coordinates_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_elementSpecAssignmentRolesEnm
{
    ads_MeshPart_elementSpecAssignment_child,
    ads_MeshPart_elementSpecAssignment_parent
};

/** Enum with association roles. */
enum ads_MeshPart_frozenElementSetRolesEnm
{
    ads_MeshPart_frozenElementSet_referent,
    ads_MeshPart_frozenElementSet_referrer
};

/** 
Enum with association roles. */
enum ads_MeshPart_gEdgeEdgeAssociativitiesRolesEnm
{
    ads_MeshPart_gEdgeEdgeAssociativities_child,
    ads_MeshPart_gEdgeEdgeAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_gEdgeElementAssociativitiesRolesEnm
{
    ads_MeshPart_gEdgeElementAssociativities_child,
    ads_MeshPart_gEdgeElementAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_gFaceElementAssociativitiesRolesEnm
{
    ads_MeshPart_gFaceElementAssociativities_child,
    ads_MeshPart_gFaceElementAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_gFaceFaceAssociativitiesRolesEnm
{
    ads_MeshPart_gFaceFaceAssociativities_child,
    ads_MeshPart_gFaceFaceAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_gVertexNodeAssociativitiesRolesEnm
{
    ads_MeshPart_gVertexNodeAssociativities_child,
    ads_MeshPart_gVertexNodeAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_gVolumeDomainAssociativitiesRolesEnm
{
    ads_MeshPart_gVolumeDomainAssociativities_child,
    ads_MeshPart_gVolumeDomainAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_MeshPart_meshDomainsRolesEnm
{
    ads_MeshPart_meshDomains_child,
    ads_MeshPart_meshDomains_parent
};

/** Enum with association roles. */
enum ads_MeshPart_nodeSetRolesEnm
{
    ads_MeshPart_nodeSet_referent,
    ads_MeshPart_nodeSet_referrer
};

/** 
Enum with association roles. */
enum ads_MeshPart_shapeBodiesRolesEnm
{
    ads_MeshPart_shapeBodies_child,
    ads_MeshPart_shapeBodies_parent
};

/** 
Enum with record members. */
enum ads_MeshStatisticsMembersEnm
{
    ads_MeshStatistics_averageElementSize
};

/** 
Enum with association roles. */
enum ads_Mesh_auxiliaryConnectivityRolesEnm
{
    ads_Mesh_auxiliaryConnectivity_child,
    ads_Mesh_auxiliaryConnectivity_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_beamOrientationExtraNodesRolesEnm
{
    ads_Mesh_beamOrientationExtraNodes_child,
    ads_Mesh_beamOrientationExtraNodes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_beamSectionNormalsRolesEnm
{
    ads_Mesh_beamSectionNormals_child,
    ads_Mesh_beamSectionNormals_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_connectivityRolesEnm
{
    ads_Mesh_connectivity_child,
    ads_Mesh_connectivity_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_coordinatesRolesEnm
{
    ads_Mesh_coordinates_child,
    ads_Mesh_coordinates_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_cyclicSymmetryRolesEnm
{
    ads_Mesh_cyclicSymmetry_child,
    ads_Mesh_cyclicSymmetry_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_dofCollectionRolesEnm
{
    ads_Mesh_dofCollection_child,
    ads_Mesh_dofCollection_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_dofsRolesEnm
{
    ads_Mesh_dofs_referent,
    ads_Mesh_dofs_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_elementClassesRolesEnm
{
    ads_Mesh_elementClasses_child,
    ads_Mesh_elementClasses_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_elementCollectionRolesEnm
{
    ads_Mesh_elementCollection_child,
    ads_Mesh_elementCollection_parent
};

/** Enum with association roles. */
enum ads_Mesh_elementNodeNormalsRolesEnm
{
    ads_Mesh_elementNodeNormals_child,
    ads_Mesh_elementNodeNormals_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_elementSpecAssignmentRolesEnm
{
    ads_Mesh_elementSpecAssignment_child,
    ads_Mesh_elementSpecAssignment_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_elementsRolesEnm
{
    ads_Mesh_elements_referent,
    ads_Mesh_elements_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_elementsMeshToLabelsMeshMapRolesEnm
{
    ads_Mesh_elementsMeshToLabelsMeshMap_child,
    ads_Mesh_elementsMeshToLabelsMeshMap_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gEdgeEdgeAssociativitiesRolesEnm
{
    ads_Mesh_gEdgeEdgeAssociativities_child,
    ads_Mesh_gEdgeEdgeAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gEdgeElementAssociativitiesRolesEnm
{
    ads_Mesh_gEdgeElementAssociativities_child,
    ads_Mesh_gEdgeElementAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gFaceElementAssociativitiesRolesEnm
{
    ads_Mesh_gFaceElementAssociativities_child,
    ads_Mesh_gFaceElementAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gFaceFaceAssociativitiesRolesEnm
{
    ads_Mesh_gFaceFaceAssociativities_child,
    ads_Mesh_gFaceFaceAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gVertexNodeAssociativitiesRolesEnm
{
    ads_Mesh_gVertexNodeAssociativities_child,
    ads_Mesh_gVertexNodeAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_gVolumeDomainAssociativitiesRolesEnm
{
    ads_Mesh_gVolumeDomainAssociativities_child,
    ads_Mesh_gVolumeDomainAssociativities_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_irsDragchainReferenceNodesRolesEnm
{
    ads_Mesh_irsDragchainReferenceNodes_child,
    ads_Mesh_irsDragchainReferenceNodes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_labelsMeshRolesEnm
{
    ads_Mesh_labelsMesh_child,
    ads_Mesh_labelsMesh_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_meshDomainCollectionRolesEnm
{
    ads_Mesh_meshDomainCollection_child,
    ads_Mesh_meshDomainCollection_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_meshPartsRolesEnm
{
    ads_Mesh_meshParts_child,
    ads_Mesh_meshParts_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_nodalDofTypesRolesEnm
{
    ads_Mesh_nodalDofTypes_child,
    ads_Mesh_nodalDofTypes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_nodeCollectionRolesEnm
{
    ads_Mesh_nodeCollection_child,
    ads_Mesh_nodeCollection_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_nodeInternalLocalSysRolesEnm
{
    ads_Mesh_nodeInternalLocalSys_child,
    ads_Mesh_nodeInternalLocalSys_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_nodeNormalsRolesEnm
{
    ads_Mesh_nodeNormals_child,
    ads_Mesh_nodeNormals_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_nodesRolesEnm
{
    ads_Mesh_nodes_referent,
    ads_Mesh_nodes_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_nodesMeshToLabelsMeshMapRolesEnm
{
    ads_Mesh_nodesMeshToLabelsMeshMap_child,
    ads_Mesh_nodesMeshToLabelsMeshMap_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_setRolesEnm
{
    ads_Mesh_set_referent,
    ads_Mesh_set_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_shapeFormulationLogicalSectionClassificationRolesEnm
{
    ads_Mesh_shapeFormulationLogicalSectionClassification_child,
    ads_Mesh_shapeFormulationLogicalSectionClassification_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_statisticsRolesEnm
{
    ads_Mesh_statistics_child,
    ads_Mesh_statistics_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_userElementLabelsRolesEnm
{
    ads_Mesh_userElementLabels_child,
    ads_Mesh_userElementLabels_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_userElementsRolesEnm
{
    ads_Mesh_userElements_referent,
    ads_Mesh_userElements_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_userNodeLabelsRolesEnm
{
    ads_Mesh_userNodeLabels_child,
    ads_Mesh_userNodeLabels_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_userNodesRolesEnm
{
    ads_Mesh_userNodes_referent,
    ads_Mesh_userNodes_referrer
};

/** 
Enum with grid dimensions. */
enum ads_NodeComponentGridDimensionsEnm
{
    ads_NodeComponentGrid_component,
    ads_NodeComponentGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_NodeDofTypeGridDimensionsEnm
{
    ads_NodeDofTypeGrid_dofType,
    ads_NodeDofTypeGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_NodeGridDimensionsEnm
{
    ads_NodeGrid_node
};

#endif
