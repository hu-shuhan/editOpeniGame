//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreConstraintsC_h
#define ads_CoreConstraintsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Constraints of the latest level of form Core */

/** Base Constraint record. */
#define ads_Constraint (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 0))

/** A kinematic or distributing coupling constraint. The MeshDistributingCouplingCharacteristics is used to distinguish between kinematic and distributing. If a constraint is absent in this DSet, we can assume it to be of type kinematic constraint. */
#define ads_Constraint_Coupling (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 1))

/** This record, when associated with a set of elements, marks those elements as a display body. Display bodies do not participate in the analysis; their elements and nodes are ignored. A DisplayBody can only be associated with those C-sets that are C-sets of elements of some Instance records. This requirement may be relaxed in future. */
#define ads_Constraint_DisplayBody (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 2))

/** The region designating the reference nodes of the display body. The nodesCluster of the associated DiscreteRegion should be unsorted nodes. The size of the nodesCluster must be at least 1 and at most 3. */
#define ads_Constraint_DisplayBody_referenceRegion (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 3))

/** The region designating the display body. The clients should populate both the nodesCluster and elementsCluster of the DiscreteRegion associated with the Region. This makes HPViz processing easier and more efficient. These nodes and elements are the cmembers from the flat mesh collections. */
#define ads_Constraint_DisplayBody_region (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 4))

/** *Embedded Element. */
#define ads_Constraint_EmbeddedElement (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 5))

/** Region of the host eleemnts. */
#define ads_Constraint_EmbeddedElement_host (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 6))

/** Regions of embedded eleemnts. */
#define ads_Constraint_EmbeddedElement_region (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 7))

/** This option is used to bind a set of elements and/or a set of nodes and/or an analytical surface into a rigid body and assign a reference node to the rigid body, which can optionally be declared as an isothermal rigid body for fully coupled thermal-stress analysis. It is also used to specify density, thickness, and offset for rigid elements that are part of a rigid body in an Abaqus/Explicit analysis. */
#define ads_Constraint_RigidBody (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 8))

/** The analytic surface assigned to the rigid body. */
#define ads_Constraint_RigidBody_analyticSurface (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 9))

/** The elements assigned to the rigid body. An element cannot appear in more than one rigid body. */
#define ads_Constraint_RigidBody_elements (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 10))

/** The pin-type nodes assigned to the rigid body. It can be used to add nodes to a rigid body or to redefine node types of nodes on elements included in the rigid body by the ELSET parameter. Pin-type nodes have only their translational degrees of freedom associated with the rigid body. A node cannot appear in more than one rigid body definition. */
#define ads_Constraint_RigidBody_pinNodes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 11))

/** The pin-type region assigned to the rigid body. The nodes cluster in this region capture the pin nodes. It can be used to add nodes to a rigid body or to redefine node types of nodes on elements included in the rigid body by the ELSET parameter. Pin-type nodes have only their translational degrees of freedom associated with the rigid body. A node cannot appear in more than one rigid body definition. */
#define ads_Constraint_RigidBody_pinRegion (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 12))

/** The region capturing the elements assigned to the rigid body. */
#define ads_Constraint_RigidBody_region (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 13))

/** The tie-type nodes assigned to the rigid body. It can be used to add nodes to a rigid body or to redefine node types of nodes on elements included in the rigid body by the ELSET parameter. Tie-type nodes have both their translational and rotational degrees of freedom associated with the rigid body. . A node cannot appear in more than one rigid body definition. */
#define ads_Constraint_RigidBody_tieNodes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 14))

/** The tie-type region assigned to the rigid body. The nodes cluster in this region capture the tie nodes. It can be used to add nodes to a rigid body or to redefine node types of nodes on elements included in the rigid body by the ELSET parameter. Tie-type nodes have both their translational and rotational degrees of freedom associated with the rigid body. . A node cannot appear in more than one rigid body definition. */
#define ads_Constraint_RigidBody_tieRegion (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 15))

/** Representative Volume Element. */
#define ads_Constraint_Rve (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 16))

/** Rve constraints. */
#define ads_Constraint_Rve_constraints (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 17))

/** The region designating the representative volume element body. */
#define ads_Constraint_Rve_region (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 18))

/** Shell to solid coupling constraint. */
#define ads_Constraint_Shell2Solid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 19))

/** Tthe perpendicular distance from the edge-based surface within which all nodes or element facets on the solid surface (depending on the solid surface type) must lie to be included in the coupling constraint. The default value is half the thickness of the shell that was used to define the edge-based surface. */
#define ads_Constraint_Shell2Solid_influenceDistance (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 20))

/** The distance within which nodes on the edge-based surface must lie from the solid surface to be included in the coupling definition. The default tolerance is 5% of the length of a typical facet on the shell edge. */
#define ads_Constraint_Shell2Solid_positionTolerance (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 21))

/** The first or odd numbered surface is edge-surface, and second or even numbered surface is solid surface. */
#define ads_Constraint_Shell2Solid_regions (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 22))

/** Tie Constraint. */
#define ads_Constraint_Tie (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 23))

/** The cutoff distance that is used to determine which nodes on the slave surface are tied to the master surface. The calculation of the distance between the slave and master surface for a particular slave node depends on factors such as shell element thickness, the type of the tie constraint, and the types of surfaces involved. Slave nodes that do not satisfy the position tolerance are not tied to the master surface. */
#define ads_Constraint_Tie_positionTolerance (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 24))

/** The first surface is master, and second surface is slave. */
#define ads_Constraint_Tie_regions (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 25))

/** The node set that includes the nodes on the slave surface that will be tied to the master surface. Nodes not included in this node set will not be tied. positionTolerance and tiedNodeSet are mutually exclusive options. */
#define ads_Constraint_Tie_tiedNodeSet (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 26))

/** Reference to store the elaborated constraints for a specific input (user) constraint. */
#define ads_Constraint_elaboratedConstraints (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 27))

/** Collection of all coupling constraints. Typically, large number of these constraints are defined in a simulation and this collection helps to bulkify constraint data. */
#define ads_CouplingConstraintCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 28))

/** Elements of this grid are dof types active in coupling constraints. */
#define ads_CouplingConstraintDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 29))

/** Grid spanning mesh coupling constraints */
#define ads_CouplingConstraintGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 30))

/** Relationships between constraints and nodes. */
#define ads_CouplingConstraintNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 31))

/** Grid spanning orientations associated with coupling constraints. */
#define ads_CouplingConstraintOrientationGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 32))

/** Grid spanning regions (surfaces) associated with coupling constraints. */
#define ads_CouplingConstraintRegionGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 33))

/** Grid spanning properties associated with distributing coupling constraints. */
#define ads_DistributingCouplingCharacteristicsGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 34))

/** The coupling method used to couple the displacement and rotation of the reference node to the average motion of the surface nodes within the influence radius. */
#define ads_DistributingCouplingMethod (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 35))

/** A collection of various distributing coupling coupling-methods. This collection is required because distributions cannot have a range of dynamic_ids. A workaround is to put all these dynamic_ids(of the same type in to a collection). */
#define ads_DistributingCouplingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 36))

/** Grid spanning properties associated with distributing coupling constraints. */
#define ads_DistributingRotCouplingCharacteristicsGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 37))

/** A collection of various distributing coupling rotational coupling-methods. This collection is required because distributions cannot have a range of dynamic_ids. A workaround is to put all these dynamic_ids(of the same type in to a collection). */
#define ads_DistributingRotCouplingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 38))

/** An optional weighting method to modify the default weight distribution at the coupling nodes. */
#define ads_DistributingWeightingMethod (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 39))

/** A collection of various distributing coupling weighting methods. This collection is required because distributions cannot have a range of dynamic_ids. A workaround is to put all these dynamic_ids(of the same type in to a collection). */
#define ads_DistributingWeightingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 40))

/** Collection for ElaboratedConstraints. Note that NO data record should be created off of this collection. */
#define ads_ElaboratedConstraintCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 41))

/** An anchor for all the elaborated constraint data. */
#define ads_ElaboratedConstraintDetails (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 42))

/** Distribution to store the slaveNode1, cloud1MasterNode1, cloud1MasterNode2, slaveNode2, cloud2MasterNode1, cloud2MasterNode2, cloud2MasterNode3,.... The range of this distribution is a CMemberType of NodeCollection from the Mesh. The domain is a GenericCollection cset used as a counting collection. */
#define ads_ElaboratedConstraintDetails_constraintsNodes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 43))

/** Distribution to store the offsets of the slaveNodes in the constraintsNodes distribution. The domain is the ElaboratedConstraintCollection cset and the range is a CMemberType of GenericCollection. */
#define ads_ElaboratedConstraintDetails_constraintsSlaveNodeOffsets (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 44))

#define ads_ElaboratedConstraintDetails_elaboratedConstraintCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 45))

#define ads_ElaboratedConstraintGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 46))

/** This collection groups all permitted distributing coupling methods. */
#define ads_GlobalCollections_distributingCouplingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 47))

/** This collection groups all permitted distributing rotational coupling methods. */
#define ads_GlobalCollections_distributingRotCouplingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 48))

/** This collection groups all permitted distributing coupling weighting methods. */
#define ads_GlobalCollections_distributingWeightingMethodCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 49))

/** The dof types constrained by each coupling constraint definition. Each coupling constraint could constrain more than one dof type. */
#define ads_Mesh_couplingConstraintDofTypes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 50))

/** Influence radius specified for the coupling constraint. */
#define ads_Mesh_couplingConstraintInfluenceRadius (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 51))

/** The local coordinate system associated with the coupling constraint definition. */
#define ads_Mesh_couplingConstraintOrientation (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 52))

/** The reference node associated with a coupling constraint definition. */
#define ads_Mesh_couplingConstraintReferenceNode (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 53))

/** The surface associated with a coupling constraint definition. */
#define ads_Mesh_couplingConstraintSurface (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 54))

/** The dof types constrained by each coupling constraint definition. Each coupling constraint could constrain more than one dof type. */
#define ads_Mesh_couplingDofTypes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 55))

/** Associates the mesh with display bodies present in this mesh. */
#define ads_Mesh_displayBodies (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 56))

/** The properties associated with distributing couplings defined for a mesh. */
#define ads_Mesh_distributingCouplingCharacteristics (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 57))

/** This distribution stores the weight factors of the nodes constrained by each distributing coupling constraint. These data are typically obtained by elaboration of a surface-based definition of the same constraint. Alternatively, the cloud node data may come directly from an input file. */
#define ads_Mesh_distributingCouplingConstraintNodeWeightFactors (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 58))

/** The properties associated with distributing couplings (with rotational coupling parameter) defined for a mesh. */
#define ads_Mesh_distributingRotCouplingCharacteristics (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 59))

/** Composition from Mesh to the ElaboratedConstraintDetails anchor. */
#define ads_Mesh_elaboratedConstraintDetails (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 60))

/** Influence radius specified for the coupling constraint. */
#define ads_Mesh_kinematicAlpha (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 61))

/** This D-Set stores the nodes constrained by each kinematic coupling constraint. These data are typically obtained by elaboration of a surface-based definition of the same constraint. Alternatively, the cloud node data may come directly from an input file. Even though the CouplingConstraintCollection collection is used for kinematic coupling constraints and for distributing coupling constraints, this D-Set is intended for kinematic constraints only. The elaborated version of the distributed coupling constraints is stored in the MeshDistributingCouplingConstraintNodeWeightFactors distribution. */
#define ads_Mesh_kinematicConstraintNodes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 62))

/** The elements of a mesh. */
#define ads_Mesh_rigidBodies (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 63))

/** The elements of a mesh. */
#define ads_Mesh_rigidBodyReferenceNode (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 64))

/** Model constraints. */
#define ads_Model_constraints (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 65))

/** All coupling constraints associated with a simulation are anchored with the model. */
#define ads_Model_couplingConstraints (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 66))

#define ads_Model_rveCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 67))

#define ads_RigidBodyCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 68))

/** Elements of this grid are reference nodes associated with a rigid body. */
#define ads_RigidBodyNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 69))

/** Collection for Representative Volume Elements. */
#define ads_RveCollection (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 70))

/** Constraint for a representative volume element. */
#define ads_RveConstraint (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 71))

/** Periodic type constraint for a representative volume element. */
#define ads_RveConstraint_Periodic (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 72))

/** Periodic type constraint data item. */
#define ads_RveConstraint_PeriodicData (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 73))

/** Periodicity for a periodic type Rve constraint. */
#define ads_RveConstraint_PeriodicData_periodicity (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 74))

/** The first surface is main, and second surface is secondary. */
#define ads_RveConstraint_PeriodicData_regions (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 75))

/** Rve periodic constraint data set. */
#define ads_RveConstraint_Periodic_dataset (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 76))

/** Surface Gradient type constraint for a representative volume element. */
#define ads_RveConstraint_SurfaceGradient (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 77))

/** Region with a driven nodes for Surface Gradient type constraint. */
#define ads_RveConstraint_SurfaceGradient_drivenNodes (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 78))

/** Taylor type constraint for a representative volume element. */
#define ads_RveConstraint_Taylor (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 79))

#define ads_RveGrid (ads_CoreFragmentTypeIndex(ads_CoreConstraintsFragment, 80))

/** 
Enum with association roles. */
enum ads_Constraint_DisplayBody_referenceRegionRolesEnm
{
    ads_Constraint_DisplayBody_referenceRegion_referent,
    ads_Constraint_DisplayBody_referenceRegion_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_DisplayBody_regionRolesEnm
{
    ads_Constraint_DisplayBody_region_referent,
    ads_Constraint_DisplayBody_region_referrer
};

/** 
Enum with record members. */
enum ads_Constraint_EmbeddedElementMembersEnm
{
    ads_Constraint_EmbeddedElement_absoluteExteriorTolerance,
    ads_Constraint_EmbeddedElement_embedNodes,
    ads_Constraint_EmbeddedElement_exteriorTolerance,
    ads_Constraint_EmbeddedElement_partialEmbed,
    ads_Constraint_EmbeddedElement_roundoffTolerance
};

/** 
Enum with association roles. */
enum ads_Constraint_EmbeddedElement_hostRolesEnm
{
    ads_Constraint_EmbeddedElement_host_referent,
    ads_Constraint_EmbeddedElement_host_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_EmbeddedElement_regionRolesEnm
{
    ads_Constraint_EmbeddedElement_region_referent,
    ads_Constraint_EmbeddedElement_region_referrer
};

/** 
Enum with record members. */
enum ads_Constraint_RigidBodyMembersEnm
{
    ads_Constraint_RigidBody_isothermal,
    ads_Constraint_RigidBody_referenceNodePosition
};

enum ads_Constraint_RigidBody_referenceNodePositionEnm
{
    ads_Constraint_RigidBody_referenceNodePosition_CENTER_OF_MASS,
    ads_Constraint_RigidBody_referenceNodePosition_INPUT
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_analyticSurfaceRolesEnm
{
    ads_Constraint_RigidBody_analyticSurface_referent,
    ads_Constraint_RigidBody_analyticSurface_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_elementsRolesEnm
{
    ads_Constraint_RigidBody_elements_referent,
    ads_Constraint_RigidBody_elements_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_pinNodesRolesEnm
{
    ads_Constraint_RigidBody_pinNodes_referent,
    ads_Constraint_RigidBody_pinNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_pinRegionRolesEnm
{
    ads_Constraint_RigidBody_pinRegion_referent,
    ads_Constraint_RigidBody_pinRegion_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_regionRolesEnm
{
    ads_Constraint_RigidBody_region_referent,
    ads_Constraint_RigidBody_region_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_tieNodesRolesEnm
{
    ads_Constraint_RigidBody_tieNodes_referent,
    ads_Constraint_RigidBody_tieNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_RigidBody_tieRegionRolesEnm
{
    ads_Constraint_RigidBody_tieRegion_referent,
    ads_Constraint_RigidBody_tieRegion_referrer
};

/** 
Enum with record members. */
enum ads_Constraint_RveMembersEnm
{
    ads_Constraint_Rve_application,
    ads_Constraint_Rve_area,
    ads_Constraint_Rve_displacement,
    ads_Constraint_Rve_rotation,
    ads_Constraint_Rve_temperature,
    ads_Constraint_Rve_volume
};

enum ads_Constraint_Rve_applicationEnm
{
    ads_Constraint_Rve_application_FULL3D,
    ads_Constraint_Rve_application_SHELL
};

enum ads_Constraint_Rve_rotationEnm
{
    ads_Constraint_Rve_rotation_NONE,
    ads_Constraint_Rve_rotation_X,
    ads_Constraint_Rve_rotation_XY,
    ads_Constraint_Rve_rotation_XYZ,
    ads_Constraint_Rve_rotation_XZ,
    ads_Constraint_Rve_rotation_Y,
    ads_Constraint_Rve_rotation_YZ,
    ads_Constraint_Rve_rotation_Z
};

/** 
Enum with association roles. */
enum ads_Constraint_Rve_constraintsRolesEnm
{
    ads_Constraint_Rve_constraints_child,
    ads_Constraint_Rve_constraints_parent
};

/** 
Enum with association roles. */
enum ads_Constraint_Rve_regionRolesEnm
{
    ads_Constraint_Rve_region_referent,
    ads_Constraint_Rve_region_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_Shell2Solid_influenceDistanceRolesEnm
{
    ads_Constraint_Shell2Solid_influenceDistance_child,
    ads_Constraint_Shell2Solid_influenceDistance_parent
};

/** 
Enum with association roles. */
enum ads_Constraint_Shell2Solid_positionToleranceRolesEnm
{
    ads_Constraint_Shell2Solid_positionTolerance_child,
    ads_Constraint_Shell2Solid_positionTolerance_parent
};

/** 
Enum with association roles. */
enum ads_Constraint_Shell2Solid_regionsRolesEnm
{
    ads_Constraint_Shell2Solid_regions_referent,
    ads_Constraint_Shell2Solid_regions_referrer
};

/** 
Enum with record members. */
enum ads_Constraint_TieMembersEnm
{
    ads_Constraint_Tie_adjust,
    ads_Constraint_Tie_constraintRatio,
    ads_Constraint_Tie_constraintRatioSpecified,
    ads_Constraint_Tie_cyclicSymmetry,
    ads_Constraint_Tie_thickness,
    ads_Constraint_Tie_tieElectricalPotential,
    ads_Constraint_Tie_tieFluidElectricalPotential,
    ads_Constraint_Tie_tieIonConcentration,
    ads_Constraint_Tie_tiePore,
    ads_Constraint_Tie_tieRotations,
    ads_Constraint_Tie_tieTemperature,
    ads_Constraint_Tie_type
};

enum ads_Constraint_Tie_typeEnm
{
    ads_Constraint_Tie_type_NODE_TO_SURF,
    ads_Constraint_Tie_type_SURF_TO_SURF
};

/** 
Enum with association roles. */
enum ads_Constraint_Tie_positionToleranceRolesEnm
{
    ads_Constraint_Tie_positionTolerance_child,
    ads_Constraint_Tie_positionTolerance_parent
};

/** 
Enum with association roles. */
enum ads_Constraint_Tie_regionsRolesEnm
{
    ads_Constraint_Tie_regions_referent,
    ads_Constraint_Tie_regions_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_Tie_tiedNodeSetRolesEnm
{
    ads_Constraint_Tie_tiedNodeSet_referent,
    ads_Constraint_Tie_tiedNodeSet_referrer
};

/** 
Enum with association roles. */
enum ads_Constraint_elaboratedConstraintsRolesEnm
{
    ads_Constraint_elaboratedConstraints_referent,
    ads_Constraint_elaboratedConstraints_referrer
};

/** 
Enum with grid dimensions. */
enum ads_CouplingConstraintDofTypeGridDimensionsEnm
{
    ads_CouplingConstraintDofTypeGrid_couplingConstraint,
    ads_CouplingConstraintDofTypeGrid_dofType
};

/** 
Enum with grid dimensions. */
enum ads_CouplingConstraintGridDimensionsEnm
{
    ads_CouplingConstraintGrid_couplingConstraints
};

/** 
Enum with grid dimensions. */
enum ads_CouplingConstraintNodeGridDimensionsEnm
{
    ads_CouplingConstraintNodeGrid_couplingConstraint,
    ads_CouplingConstraintNodeGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_CouplingConstraintOrientationGridDimensionsEnm
{
    ads_CouplingConstraintOrientationGrid_couplingConstraint,
    ads_CouplingConstraintOrientationGrid_orientation
};

/** 
Enum with grid dimensions. */
enum ads_CouplingConstraintRegionGridDimensionsEnm
{
    ads_CouplingConstraintRegionGrid_couplingConstraint,
    ads_CouplingConstraintRegionGrid_surface
};

/** 
Enum with grid dimensions. */
enum ads_DistributingCouplingCharacteristicsGridDimensionsEnm
{
    ads_DistributingCouplingCharacteristicsGrid_couplingConstraint,
    ads_DistributingCouplingCharacteristicsGrid_couplingMethod,
    ads_DistributingCouplingCharacteristicsGrid_weightingMethod
};

/** Enum with the symbols of data type DistributingCouplingMethod*/
enum ads_DistributingCouplingMethodSymbolsEnm
{
    ads_DistributingCouplingMethod_CONTINUUM,
    ads_DistributingCouplingMethod_STRUCTURAL
};

/** 
Enum with grid dimensions. */
enum ads_DistributingRotCouplingCharacteristicsGridDimensionsEnm
{
    ads_DistributingRotCouplingCharacteristicsGrid_couplingConstraint,
    ads_DistributingRotCouplingCharacteristicsGrid_couplingMethod,
    ads_DistributingRotCouplingCharacteristicsGrid_rotationalCouplingMethod,
    ads_DistributingRotCouplingCharacteristicsGrid_weightingMethod
};

/** Enum with the symbols of data type DistributingWeightingMethod*/
enum ads_DistributingWeightingMethodSymbolsEnm
{
    ads_DistributingWeightingMethod_CUBIC,
    ads_DistributingWeightingMethod_LINEAR,
    ads_DistributingWeightingMethod_QUADRATIC,
    ads_DistributingWeightingMethod_UNIFORM
};

/** 
Enum with association roles. */
enum ads_ElaboratedConstraintDetails_constraintsNodesRolesEnm
{
    ads_ElaboratedConstraintDetails_constraintsNodes_child,
    ads_ElaboratedConstraintDetails_constraintsNodes_parent
};

/** 
Enum with association roles. */
enum ads_ElaboratedConstraintDetails_constraintsSlaveNodeOffsetsRolesEnm
{
    ads_ElaboratedConstraintDetails_constraintsSlaveNodeOffsets_child,
    ads_ElaboratedConstraintDetails_constraintsSlaveNodeOffsets_parent
};

/** Enum with association roles. */
enum ads_ElaboratedConstraintDetails_elaboratedConstraintCollectionRolesEnm
{
    ads_ElaboratedConstraintDetails_elaboratedConstraintCollection_child,
    ads_ElaboratedConstraintDetails_elaboratedConstraintCollection_parent
};

/** Enum with grid dimensions. */
enum ads_ElaboratedConstraintGridDimensionsEnm
{
    ads_ElaboratedConstraintGrid_elaboratedConstraint
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_distributingCouplingMethodCollectionRolesEnm
{
    ads_GlobalCollections_distributingCouplingMethodCollection_child,
    ads_GlobalCollections_distributingCouplingMethodCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_distributingRotCouplingMethodCollectionRolesEnm
{
    ads_GlobalCollections_distributingRotCouplingMethodCollection_child,
    ads_GlobalCollections_distributingRotCouplingMethodCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_distributingWeightingMethodCollectionRolesEnm
{
    ads_GlobalCollections_distributingWeightingMethodCollection_child,
    ads_GlobalCollections_distributingWeightingMethodCollection_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingConstraintDofTypesRolesEnm
{
    ads_Mesh_couplingConstraintDofTypes_child,
    ads_Mesh_couplingConstraintDofTypes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingConstraintInfluenceRadiusRolesEnm
{
    ads_Mesh_couplingConstraintInfluenceRadius_child,
    ads_Mesh_couplingConstraintInfluenceRadius_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingConstraintOrientationRolesEnm
{
    ads_Mesh_couplingConstraintOrientation_child,
    ads_Mesh_couplingConstraintOrientation_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingConstraintReferenceNodeRolesEnm
{
    ads_Mesh_couplingConstraintReferenceNode_child,
    ads_Mesh_couplingConstraintReferenceNode_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingConstraintSurfaceRolesEnm
{
    ads_Mesh_couplingConstraintSurface_child,
    ads_Mesh_couplingConstraintSurface_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_couplingDofTypesRolesEnm
{
    ads_Mesh_couplingDofTypes_child,
    ads_Mesh_couplingDofTypes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_displayBodiesRolesEnm
{
    ads_Mesh_displayBodies_referent,
    ads_Mesh_displayBodies_referrer
};

/** 
Enum with association roles. */
enum ads_Mesh_distributingCouplingCharacteristicsRolesEnm
{
    ads_Mesh_distributingCouplingCharacteristics_child,
    ads_Mesh_distributingCouplingCharacteristics_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_distributingCouplingConstraintNodeWeightFactorsRolesEnm
{
    ads_Mesh_distributingCouplingConstraintNodeWeightFactors_child,
    ads_Mesh_distributingCouplingConstraintNodeWeightFactors_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_distributingRotCouplingCharacteristicsRolesEnm
{
    ads_Mesh_distributingRotCouplingCharacteristics_child,
    ads_Mesh_distributingRotCouplingCharacteristics_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_elaboratedConstraintDetailsRolesEnm
{
    ads_Mesh_elaboratedConstraintDetails_child,
    ads_Mesh_elaboratedConstraintDetails_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_kinematicAlphaRolesEnm
{
    ads_Mesh_kinematicAlpha_child,
    ads_Mesh_kinematicAlpha_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_kinematicConstraintNodesRolesEnm
{
    ads_Mesh_kinematicConstraintNodes_child,
    ads_Mesh_kinematicConstraintNodes_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_rigidBodiesRolesEnm
{
    ads_Mesh_rigidBodies_child,
    ads_Mesh_rigidBodies_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_rigidBodyReferenceNodeRolesEnm
{
    ads_Mesh_rigidBodyReferenceNode_child,
    ads_Mesh_rigidBodyReferenceNode_parent
};

/** 
Enum with association roles. */
enum ads_Model_constraintsRolesEnm
{
    ads_Model_constraints_child,
    ads_Model_constraints_parent
};

/** 
Enum with association roles. */
enum ads_Model_couplingConstraintsRolesEnm
{
    ads_Model_couplingConstraints_child,
    ads_Model_couplingConstraints_parent
};

/** Enum with association roles. */
enum ads_Model_rveCollectionRolesEnm
{
    ads_Model_rveCollection_child,
    ads_Model_rveCollection_parent
};

/** 
Enum with grid dimensions. */
enum ads_RigidBodyNodeGridDimensionsEnm
{
    ads_RigidBodyNodeGrid_node,
    ads_RigidBodyNodeGrid_rigidBody
};

/** 
Enum with record members. */
enum ads_RveConstraintMembersEnm
{
    ads_RveConstraint_fields
};

enum ads_RveConstraint_fieldsEnm
{
    ads_RveConstraint_fields_ALL,
    ads_RveConstraint_fields_DISPLACEMENT,
    ads_RveConstraint_fields_TEMPERATURE
};

/** 
Enum with record members. */
enum ads_RveConstraint_PeriodicMembersEnm
{
    ads_RveConstraint_Periodic_fields
};

enum ads_RveConstraint_Periodic_fieldsEnm
{
    ads_RveConstraint_Periodic_fields_ALL,
    ads_RveConstraint_Periodic_fields_DISPLACEMENT,
    ads_RveConstraint_Periodic_fields_TEMPERATURE
};

/** 
Enum with association roles. */
enum ads_RveConstraint_PeriodicData_periodicityRolesEnm
{
    ads_RveConstraint_PeriodicData_periodicity_child,
    ads_RveConstraint_PeriodicData_periodicity_parent
};

/** 
Enum with association roles. */
enum ads_RveConstraint_PeriodicData_regionsRolesEnm
{
    ads_RveConstraint_PeriodicData_regions_referent,
    ads_RveConstraint_PeriodicData_regions_referrer
};

/** 
Enum with association roles. */
enum ads_RveConstraint_Periodic_datasetRolesEnm
{
    ads_RveConstraint_Periodic_dataset_child,
    ads_RveConstraint_Periodic_dataset_parent
};

/** 
Enum with record members. */
enum ads_RveConstraint_SurfaceGradientMembersEnm
{
    ads_RveConstraint_SurfaceGradient_fields
};

enum ads_RveConstraint_SurfaceGradient_fieldsEnm
{
    ads_RveConstraint_SurfaceGradient_fields_ALL,
    ads_RveConstraint_SurfaceGradient_fields_DISPLACEMENT,
    ads_RveConstraint_SurfaceGradient_fields_TEMPERATURE
};

/** 
Enum with association roles. */
enum ads_RveConstraint_SurfaceGradient_drivenNodesRolesEnm
{
    ads_RveConstraint_SurfaceGradient_drivenNodes_referent,
    ads_RveConstraint_SurfaceGradient_drivenNodes_referrer
};

/** 
Enum with record members. */
enum ads_RveConstraint_TaylorMembersEnm
{
    ads_RveConstraint_Taylor_fields
};

enum ads_RveConstraint_Taylor_fieldsEnm
{
    ads_RveConstraint_Taylor_fields_ALL,
    ads_RveConstraint_Taylor_fields_DISPLACEMENT,
    ads_RveConstraint_Taylor_fields_TEMPERATURE
};

/** Enum with grid dimensions. */
enum ads_RveGridDimensionsEnm
{
    ads_RveGrid_rve
};

#endif
