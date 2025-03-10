//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreMicrotopologyC_h
#define ads_CoreMicrotopologyC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Microtopology of the latest level of form Core */

/** Cell number of a shape. */
#define ads_Cell (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 0))

/** Counting collection of shape cells. */
#define ads_CellCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 1))

/** A cluster for View factor. */
#define ads_Cluster (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 2))

#define ads_ClusterCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 3))

/** DOF type describes a degree of freedom. The major use of DOF types is to indicate what degrees of freedom are shared between elements in shared nodes. If two neighboring elements have an identical DOF type record assigned to a shared node, then we need for it only one global DOF. */
#define ads_DofType (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 4))

/** Collection of DOF types */
#define ads_DofTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 5))

#define ads_DofType_temperaturePoint (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 6))

/** Edge number of a shape. */
#define ads_Edge (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 7))

/** Counting collection of shape edges. */
#define ads_EdgeCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 8))

/** Element topology is a way to capture the mathematical notion of topology (properties that are preserved under continuous deformation). The ElementTopology record defines a set of symbols that capture the variety of element topologies in use in SIMULIA code. The naming convention for the symbols is as follows: <D|L|S|C|U>_<Descriptive short string>*_{{<min # nodes>{_<max # nodes>}?}|{VAR}} The first letter is an abbreviation for the following: D: Discrete L: Line/curve S: Surface C: Cell/continuum U: User element topology For user element topologies, one of the U_A through U_J symbols must be used. Most ElementTopologies dictate a fixed number of nodes. Some, like the D_SPIDER_VAR, C_HEX_22_27 allow for a variable number of nodes. For these latter cases, ElementTopology alone is insufficient to form the element classification needed for mesh generators. To classify elements for mesh generators, the set of Points (element-local nodes) used to capture the element connectivity must be paired with the ElementTopology. Given an ElementTopology symbol, client code can uniquely instantiate the corresponding concrete C++ ElementTopology objects that provide an uniform interface to data and operations - shape function values, shape function derivative values and connectivity information (boundary set ElementTopology, surface set ElementTopology, line set ElementTopology). */
#define ads_ElementTopology (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 9))

/** Face number of a shape. */
#define ads_Face (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 10))

/** Counting collection of shape faces. */
#define ads_FaceCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 11))

/** The cell shapes collection. */
#define ads_GlobalCollections_cellCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 12))

#define ads_GlobalCollections_clusterCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 13))

/** A dof category collection. */
#define ads_GlobalCollections_dofTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 14))

/** The collection of edeg shapes. */
#define ads_GlobalCollections_edgeCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 15))

/** The collection of face shapes. */
#define ads_GlobalCollections_faceCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 16))

/** A collection of integration stations. */
#define ads_GlobalCollections_integrationStationCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 17))

/** The collection of point shapes. */
#define ads_GlobalCollections_pointCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 18))

/** A collection of quadratures. */
#define ads_GlobalCollections_quadratureCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 19))

/** The way to get THE collection of temperature points. */
#define ads_GlobalCollections_temperaturePointCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 20))

/** Integration station number of an element type. */
#define ads_IntegrationStation (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 21))

/** Counting collection of integration stations. */
#define ads_IntegrationStationCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 22))

/** Points local to a shape (think element local nodes). */
#define ads_Point (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 23))

/** Counting collection of shape points. */
#define ads_PointCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 24))

/** Elements of this grid are coordinates of points. */
#define ads_PointCoordinateGrid (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 25))

/** Elements of this grid are dof types activated in local nodes. */
#define ads_PointDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 26))

/** Outside of SIM, 'Quadrature' is a rectangular grid of points in 3D space, primarily used for numerical integration. In SIM, Quadratures are also used to specify points that will hold element state or points for result output. Material Points are an example of quadrature points. The cross product of Quadrature points is partially defined by ElementFormulation records, and partially by section topology data. ElementFormulation supplies IntegrationStations and section topology supplies either SectionLayers and SectionPoints to form together a grid of material points or SectionLayers and TemperaturePoints to form a grid of temperature points. Some Quadratures, such as Quadratures for 3D element faces, may have no SectionLayer or SectionPoint dimensions. The same element may have input data (material properties, predefined temperature fields) or output data (state, results) defined over different quadratures. Here are examples of different quadratures for the same element: The S4R element type is a shell element with a single structural integration station. However, for the application of a non-uniform pressure load, four points are defined. The material points and the load points are thus from different quadratures. For a structural analysis, when the user applies a predefined field to a shell, the temperature field may have been defined over a different set of section points (called temperature point) than the section points that will be used for the structural analysis. In this case, besides the structural quadrature used with the structural analysis, the same element is also associated with the thermal quadrature just to be able to apply the predefined field (see documentation for the TEMPERATURE parameter of the SHELL SECTION ABAQUS keyword.) And here is an example of different quadratures for the same element formulation (but not for the same element): The SIMPSON and GAUSS rules position the section points differently (with SIMPSON, section points are equally spaced). The particular quadratures of an element spec are defined in code, not in SIM data. The section point factor of the quadrature Cartesian product is provided by the MeshElementLayerSectionPoints and MeshShellElementLayerTemperaturePoints D-Sets. */
#define ads_Quadrature (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 27))

/** Counting collection of quadratures. */
#define ads_QuadratureCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 28))

/** Temperature points, generally at a node, through an element section. */
#define ads_TemperaturePoint (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 29))

/** Counting collection of temperature points. */
#define ads_TemperaturePointCollection (ads_CoreFragmentTypeIndex(ads_CoreMicrotopologyFragment, 30))

/** Enum with the symbols of data type DofType*/
enum ads_DofTypeSymbolsEnm
{
    ads_DofType_ACOUSTIC_PRESSURE,
    ads_DofType_CONCENTRATION,
    ads_DofType_DISPLACEMENT_1,
    ads_DofType_DISPLACEMENT_2,
    ads_DofType_DISPLACEMENT_3,
    ads_DofType_ELECTRIC_POTENTIAL,
    ads_DofType_ELECTRIC_POTENTIAL_FLUID,
    ads_DofType_IONIC_CONCENTRATION_FLUID,
    ads_DofType_IONIC_CONCENTRATION_SOLID,
    ads_DofType_MATERIAL_FLOW,
    ads_DofType_PORE_CONCENTRATION_1,
    ads_DofType_PORE_CONCENTRATION_2,
    ads_DofType_PORE_CONCENTRATION_3,
    ads_DofType_PORE_PRESSURE,
    ads_DofType_PRESSURE,
    ads_DofType_ROTATION_ANGLE_1,
    ads_DofType_ROTATION_ANGLE_2,
    ads_DofType_ROTATION_ANGLE_3,
    ads_DofType_SCALAR_DOF,
    ads_DofType_TEMPERATURE,
    ads_DofType_VELOCITY_1,
    ads_DofType_VELOCITY_2,
    ads_DofType_VELOCITY_3,
    ads_DofType_WARPING_AMPLITUDE,
    ads_DofType_WEARN_1,
    ads_DofType_WEARN_2,
    ads_DofType_WEARN_3,
    ads_DofType_WEARP_1,
    ads_DofType_WEARP_2,
    ads_DofType_WEARP_3
};

/** Enum with association roles. */
enum ads_DofType_temperaturePointRolesEnm
{
    ads_DofType_temperaturePoint_referent,
    ads_DofType_temperaturePoint_referrer
};

/** Enum with the symbols of data type ElementTopology*/
enum ads_ElementTopologySymbolsEnm
{
    ads_ElementTopology_D_1,
    ads_ElementTopology_D_1_2,
    ads_ElementTopology_D_2,
    ads_ElementTopology_D_GEN_VAR,
    ads_ElementTopology_D_SNAKE_VAR,
    ads_ElementTopology_D_SPIDER_VAR,
    ads_ElementTopology_L_2,
    ads_ElementTopology_L_3,
    ads_ElementTopology_L_4,
    ads_ElementTopology_L_CYL_3,
    ads_ElementTopology_S_CYL_6,
    ads_ElementTopology_S_CYL_9,
    ads_ElementTopology_S_QUADINF_4,
    ads_ElementTopology_S_QUADINF_5,
    ads_ElementTopology_S_QUAD_4,
    ads_ElementTopology_S_QUAD_8,
    ads_ElementTopology_S_QUAD_9,
    ads_ElementTopology_S_TRI_3,
    ads_ElementTopology_S_TRI_6,
    ads_ElementTopology_U_A,
    ads_ElementTopology_U_B,
    ads_ElementTopology_U_C,
    ads_ElementTopology_U_D,
    ads_ElementTopology_U_E,
    ads_ElementTopology_U_F,
    ads_ElementTopology_U_G,
    ads_ElementTopology_U_H,
    ads_ElementTopology_U_I,
    ads_ElementTopology_U_J,
    ads_ElementTopology_V_CYL_12,
    ads_ElementTopology_V_CYL_18,
    ads_ElementTopology_V_CYL_24,
    ads_ElementTopology_V_CYL_9,
    ads_ElementTopology_V_HEXINF_12,
    ads_ElementTopology_V_HEXINF_18,
    ads_ElementTopology_V_HEX_12,
    ads_ElementTopology_V_HEX_20,
    ads_ElementTopology_V_HEX_22_27,
    ads_ElementTopology_V_HEX_27,
    ads_ElementTopology_V_HEX_8,
    ads_ElementTopology_V_PYR_13,
    ads_ElementTopology_V_PYR_5,
    ads_ElementTopology_V_TETM_10,
    ads_ElementTopology_V_TET_10,
    ads_ElementTopology_V_TET_4,
    ads_ElementTopology_V_WEDG_12,
    ads_ElementTopology_V_WEDG_15,
    ads_ElementTopology_V_WEDG_16_18,
    ads_ElementTopology_V_WEDG_6,
    ads_ElementTopology_V_WEDG_9
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_cellCollectionRolesEnm
{
    ads_GlobalCollections_cellCollection_child,
    ads_GlobalCollections_cellCollection_parent
};

/** Enum with association roles. */
enum ads_GlobalCollections_clusterCollectionRolesEnm
{
    ads_GlobalCollections_clusterCollection_child,
    ads_GlobalCollections_clusterCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_dofTypeCollectionRolesEnm
{
    ads_GlobalCollections_dofTypeCollection_child,
    ads_GlobalCollections_dofTypeCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_edgeCollectionRolesEnm
{
    ads_GlobalCollections_edgeCollection_child,
    ads_GlobalCollections_edgeCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_faceCollectionRolesEnm
{
    ads_GlobalCollections_faceCollection_child,
    ads_GlobalCollections_faceCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_integrationStationCollectionRolesEnm
{
    ads_GlobalCollections_integrationStationCollection_child,
    ads_GlobalCollections_integrationStationCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_pointCollectionRolesEnm
{
    ads_GlobalCollections_pointCollection_child,
    ads_GlobalCollections_pointCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_quadratureCollectionRolesEnm
{
    ads_GlobalCollections_quadratureCollection_child,
    ads_GlobalCollections_quadratureCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_temperaturePointCollectionRolesEnm
{
    ads_GlobalCollections_temperaturePointCollection_child,
    ads_GlobalCollections_temperaturePointCollection_parent
};

/** 
Enum with grid dimensions. */
enum ads_PointCoordinateGridDimensionsEnm
{
    ads_PointCoordinateGrid_coordinate,
    ads_PointCoordinateGrid_point
};

/** 
Enum with grid dimensions. */
enum ads_PointDofTypeGridDimensionsEnm
{
    ads_PointDofTypeGrid_dofType,
    ads_PointDofTypeGrid_point
};

/** Enum with the symbols of data type TemperaturePoint*/
enum ads_TemperaturePointSymbolsEnm
{
    ads_TemperaturePoint_TEMPERATURE_POINT_1
};

#endif
