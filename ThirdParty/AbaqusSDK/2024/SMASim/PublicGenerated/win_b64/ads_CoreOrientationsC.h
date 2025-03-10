//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreOrientationsC_h
#define ads_CoreOrientationsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Orientations of the latest level of form Core */

/** This data record describes an orientation of cartesian type. */
#define ads_CartesianOrientation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 0))

#define ads_CartesianOrientation_rotation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 1))

/** This data record describes an orientation of cylindrical type. */
#define ads_CylindricalOrientation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 2))

#define ads_CylindricalOrientation_axisPointA (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 3))

#define ads_CylindricalOrientation_axisPointB (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 4))

/** Elements of this grid are axis numbers for orientation in layers of elements. */
#define ads_ElementLayerOrientationAxisGrid (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 5))

/** A collection of OrientationAxis. */
#define ads_GlobalCollections_orientationAxisCollection (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 6))

/** Collection of all orientations. */
#define ads_Model_orientationCollection (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 7))

/** The abstract data record to anchor a coordinate system definition. */
#define ads_Orientation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 8))

/** Axes used in orientation definition. */
#define ads_OrientationAxis (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 9))

/** Collection of orientation axes. */
#define ads_OrientationAxisCollection (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 10))

/** Elements of this grid are axis numbers for orientation. */
#define ads_OrientationAxisGrid (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 11))

/** A collection of orientation records. */
#define ads_OrientationCollection (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 12))

/** Allows element sets to be assigned orientation */
#define ads_OrientationElementGrid (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 13))

/** Allows node sets to be assigned orientation */
#define ads_OrientationNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 14))

#define ads_Orientation_additionalAngle (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 15))

/** The second-order tensor for the fiber dispersion. */
#define ads_Orientation_dispersion (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 16))

/** Permuted CSet of points (local nodes) to capture the points A, B and/or C for the orientation. */
#define ads_Orientation_localNodes (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 17))

/** Permuted CSet of nodes to capture the points A, B and/or C for the orientation. */
#define ads_Orientation_nodes (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 18))

#define ads_Orientation_origin (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 19))

/** This data record describes an orientation of spherical type. */
#define ads_SphericalOrientation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 20))

#define ads_SphericalOrientation_axisPointA (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 21))

#define ads_SphericalOrientation_axisPointB (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 22))

/** This data record describes an orientation of spherical type. */
#define ads_UserOrientation (ads_CoreFragmentTypeIndex(ads_CoreOrientationsFragment, 23))

/** 
Enum with record members. */
enum ads_CartesianOrientationMembersEnm
{
    ads_CartesianOrientation_additionalAngleAxis
};

enum ads_CartesianOrientation_additionalAngleAxisEnm
{
    ads_CartesianOrientation_additionalAngleAxis_LOCAL_1,
    ads_CartesianOrientation_additionalAngleAxis_LOCAL_2,
    ads_CartesianOrientation_additionalAngleAxis_LOCAL_3
};

/** Enum with association roles. */
enum ads_CartesianOrientation_rotationRolesEnm
{
    ads_CartesianOrientation_rotation_child,
    ads_CartesianOrientation_rotation_parent
};

/** 
Enum with record members. */
enum ads_CylindricalOrientationMembersEnm
{
    ads_CylindricalOrientation_additionalAngleAxis
};

enum ads_CylindricalOrientation_additionalAngleAxisEnm
{
    ads_CylindricalOrientation_additionalAngleAxis_LOCAL_1,
    ads_CylindricalOrientation_additionalAngleAxis_LOCAL_2,
    ads_CylindricalOrientation_additionalAngleAxis_LOCAL_3
};

/** Enum with association roles. */
enum ads_CylindricalOrientation_axisPointARolesEnm
{
    ads_CylindricalOrientation_axisPointA_child,
    ads_CylindricalOrientation_axisPointA_parent
};

/** Enum with association roles. */
enum ads_CylindricalOrientation_axisPointBRolesEnm
{
    ads_CylindricalOrientation_axisPointB_child,
    ads_CylindricalOrientation_axisPointB_parent
};

/** 
Enum with grid dimensions. */
enum ads_ElementLayerOrientationAxisGridDimensionsEnm
{
    ads_ElementLayerOrientationAxisGrid_element,
    ads_ElementLayerOrientationAxisGrid_orientationAxis,
    ads_ElementLayerOrientationAxisGrid_sectionLayer
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_orientationAxisCollectionRolesEnm
{
    ads_GlobalCollections_orientationAxisCollection_child,
    ads_GlobalCollections_orientationAxisCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_orientationCollectionRolesEnm
{
    ads_Model_orientationCollection_child,
    ads_Model_orientationCollection_parent
};

/** 
Enum with record members. */
enum ads_OrientationMembersEnm
{
    ads_Orientation_additionalAngleAxis
};

enum ads_Orientation_additionalAngleAxisEnm
{
    ads_Orientation_additionalAngleAxis_LOCAL_1,
    ads_Orientation_additionalAngleAxis_LOCAL_2,
    ads_Orientation_additionalAngleAxis_LOCAL_3
};

/** 
Enum with grid dimensions. */
enum ads_OrientationAxisGridDimensionsEnm
{
    ads_OrientationAxisGrid_orientationAxis
};

/** 
Enum with grid dimensions. */
enum ads_OrientationElementGridDimensionsEnm
{
    ads_OrientationElementGrid_element,
    ads_OrientationElementGrid_orientation
};

/** 
Enum with grid dimensions. */
enum ads_OrientationNodeGridDimensionsEnm
{
    ads_OrientationNodeGrid_node,
    ads_OrientationNodeGrid_orientation
};

/** Enum with association roles. */
enum ads_Orientation_additionalAngleRolesEnm
{
    ads_Orientation_additionalAngle_child,
    ads_Orientation_additionalAngle_parent
};

/** 
Enum with association roles. */
enum ads_Orientation_dispersionRolesEnm
{
    ads_Orientation_dispersion_child,
    ads_Orientation_dispersion_parent
};

/** 
Enum with association roles. */
enum ads_Orientation_localNodesRolesEnm
{
    ads_Orientation_localNodes_referent,
    ads_Orientation_localNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Orientation_nodesRolesEnm
{
    ads_Orientation_nodes_referent,
    ads_Orientation_nodes_referrer
};

/** Enum with association roles. */
enum ads_Orientation_originRolesEnm
{
    ads_Orientation_origin_child,
    ads_Orientation_origin_parent
};

/** 
Enum with record members. */
enum ads_SphericalOrientationMembersEnm
{
    ads_SphericalOrientation_additionalAngleAxis
};

enum ads_SphericalOrientation_additionalAngleAxisEnm
{
    ads_SphericalOrientation_additionalAngleAxis_LOCAL_1,
    ads_SphericalOrientation_additionalAngleAxis_LOCAL_2,
    ads_SphericalOrientation_additionalAngleAxis_LOCAL_3
};

/** Enum with association roles. */
enum ads_SphericalOrientation_axisPointARolesEnm
{
    ads_SphericalOrientation_axisPointA_child,
    ads_SphericalOrientation_axisPointA_parent
};

/** Enum with association roles. */
enum ads_SphericalOrientation_axisPointBRolesEnm
{
    ads_SphericalOrientation_axisPointB_child,
    ads_SphericalOrientation_axisPointB_parent
};

/** 
Enum with record members. */
enum ads_UserOrientationMembersEnm
{
    ads_UserOrientation_additionalAngleAxis
};

enum ads_UserOrientation_additionalAngleAxisEnm
{
    ads_UserOrientation_additionalAngleAxis_LOCAL_1,
    ads_UserOrientation_additionalAngleAxis_LOCAL_2,
    ads_UserOrientation_additionalAngleAxis_LOCAL_3
};

#endif
