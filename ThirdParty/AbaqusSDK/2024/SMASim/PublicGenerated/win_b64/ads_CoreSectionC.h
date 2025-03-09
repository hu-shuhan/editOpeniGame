//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreSectionC_h
#define ads_CoreSectionC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Section of the latest level of form Core */

/** Data to define property data for input to beam general section when computed from a meshed section analysis. The rows of this table correspond to the sequence of dmembers in the meshedSectionPoints tabular DSet. */
#define ads_BGSMeshedSectionTable (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 0))

/** This data type locates section points in beam general section for which axial stress and axial strain output are required. */
#define ads_BeamSectionPoints (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 1))

/** Sequence of local coordinates of the section points. n-th value is for n-th section point. */
#define ads_BeamSectionPoints_localCoordinates (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 2))

/** Tabular DSet mapping SectionPoint to 2D Element number and IntegrationPoint. Element numbers are from the collection of a 2D mesh. First data line of *Section Points when used with *Beam General Section, Section=MESHED or the data line of *Section Point when used in a *Beam Section Generate analysis. */
#define ads_BeamSectionPoints_meshedSectionPoints (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 3))

#define ads_BeamSectionPoints_table (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 4))

/** Elements of this grid are section points in layers of elements. This can be used for specifying section points coordinates for beam general section. */
#define ads_ElementLayerLayerPointComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 5))

/** Elements of this grid are assignments of orientation to element layers. */
#define ads_ElementLayerOrientationGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 6))

/** Elements of this grid are assignments of orientation to elements. */
#define ads_ElementOrientationGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 7))

/** Elements of this grid are assignments of orientation to element local nodes. */
#define ads_ElementPointOrientationGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 8))

/** Elements of this grid are components of location of layers of finite elements. */
#define ads_ElementSectionLayerComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 9))

/** A value associated with a Section can be overridden by a field at the time of the assignment of that section. */
#define ads_FieldBinding (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 10))

/** Actual Field that replaces the parameter value of the property. */
#define ads_FieldBinding_field (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 11))

/** This is the reference to the record that holds the FieldBinding parameter. */
#define ads_FieldBinding_property (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 12))

/** A counting LayerPoint collection. */
#define ads_GlobalCollections_layerPointCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 13))

/** A Material collection. */
#define ads_GlobalCollections_materialCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 14))

/** A counting SectionLayer collection. */
#define ads_GlobalCollections_sectionLayerCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 15))

/** A counting SectionPoint collection. */
#define ads_GlobalCollections_sectionPointCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 16))

/** Base class for all layer types. */
#define ads_Layer (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 17))

/** A collection of layers. */
#define ads_LayerCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 18))

#define ads_LayerElementGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 19))

/** This data type respresents a layer point in a section layer. Layer points are members of a counting collection thus in distributions they are always qualified with a SectionLayer record. */
#define ads_LayerPoint (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 20))

/** Counting collection of layer points. */
#define ads_LayerPointCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 21))

#define ads_LayerSectionGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 22))

/** List of material instances. Typically each layer consists of only one MaterialInstance object. However, the Eularian layer can contain more than one MaterialInstance per layer. */
#define ads_Layer_materialInstances (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 23))

/** Ply that defines the layer. */
#define ads_Layer_ply (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 24))

/** A coordinate system associated with a Section can be overridden by a distribution at the time of the assignment of that section. */
#define ads_LocalSysBinding (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 25))

/** Actual LoacalSys that replaces the parameter value of the property. */
#define ads_LocalSysBinding_localSysBinding (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 26))

/** This is the reference to the record that holds the LocalSysBinding parameter. */
#define ads_LocalSysBinding_property (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 27))

/** A collection of materials. */
#define ads_MaterialCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 28))

/** Elements of this grid are material assignments to element layers. */
#define ads_MaterialElementLayerGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 29))

/** Material instance. */
#define ads_MaterialInstance (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 30))

/** Solid portion of porous media flow material. */
#define ads_MaterialInstance_PorousMedia (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 31))

/** Local Coordinate System for the material instance. */
#define ads_MaterialInstance_localSys (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 32))

/** Material that defines the MaterialInstance. */
#define ads_MaterialInstance_material (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 33))

/** Set this parameter equal to the orientation angle. */
#define ads_MaterialInstance_orientationAngle (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 34))

#define ads_Model_layerCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 35))

#define ads_Model_plyCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 36))

#define ads_Model_sectionCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 37))

/** Composition from Model to the section controls. */
#define ads_Model_sectionControls (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 38))

/** Data to define pressure velocity table */
#define ads_PerfPlate_PressureVelocityTable (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 39))

/** Record to denote the type of region for injection molding domain. */
#define ads_PlasticsRegion (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 40))

/** Used to capture the input fields for plastics region. The different fields are distinguished via the string key. Most input fields are likely to be of type Field_Scalar, but the client code should be designed to handle any subtype of Field, e.g. Field_Vector, MeshedField, etc. */
#define ads_PlasticsRegion_inputFields (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 41))

/** Layer ply. */
#define ads_Ply (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 42))

/** A collection of plies. */
#define ads_PlyCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 43))

/** Elements of this grid are section points in layers of elements. Quadratures serve only a classification purpose, so that section points can be matched with integration stations on the elements. SectionPoints within the same layer must be unique regardless of whether they belong to the same Quadrature or not. */
#define ads_QuadratureElementLayerLayerPointGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 44))

/** Elements of this grid are temperature points in layers of elements. Quadratures serve only a classification purpose, so that temperature points can be matched with integration stations on the elements. TemperaturePoints within the same layer must be unique regardless of whether they belong to the same Quadrature or not. */
#define ads_QuadratureElementLayerTemperaturePointGrid (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 45))

#define ads_ScaleMass (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 46))

/** To specify scale factors using a distribution for material mass */
#define ads_ScaleMass_scaleFactors (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 47))

#define ads_ScaleStiffness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 48))

/** To specify scale factors using a distribution for material stiffness */
#define ads_ScaleStiffness_scaleFactors (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 49))

#define ads_ScaleStressDesign (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 50))

/** To specify scale factors using a distribution for stress design */
#define ads_ScaleStressDesign_scaleFactors (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 51))

#define ads_ScaleThermalConductivity (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 52))

/** To specify scale factors using a distribution for material thermal conductivity */
#define ads_ScaleThermalConductivity_scaleFactors (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 53))

/** Generalization of all Abaqus section types. */
#define ads_Section (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 54))

/** ... */
#define ads_SectionAssignment (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 55))

/** ... */
#define ads_SectionAssignment_fieldBindings (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 56))

/** ... */
#define ads_SectionAssignment_localSysBindings (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 57))

/** ... */
#define ads_SectionAssignment_region (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 58))

/** ... */
#define ads_SectionAssignment_section (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 59))

/** A collection of sections. */
#define ads_SectionCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 60))

/** Section Controls type. This data type aggregates all the section control options. */
#define ads_SectionControls (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 61))

/** Abstract base section controls option type. */
#define ads_SectionControlsOption (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 62))

/** Section controls options to capture AM related data. All the options have been moved to the base datatype SectionControlsOption. */
#define ads_SectionControlsOption_Activation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 63))

/** Section controls options to capture SPH related data. */
#define ads_SectionControlsOption_SPH (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 64))

#define ads_SectionControlsOption_SPH_backgroundGridOrientation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 65))

#define ads_SectionControlsOption_SPH_backgroundGridSpacing (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 66))

#define ads_SectionControlsOption_SPH_conversionThresholdValue (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 67))

#define ads_SectionControlsOption_SPH_lowerLeftTrackingBoxCorner (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 68))

#define ads_SectionControlsOption_SPH_smoothingLength (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 69))

#define ads_SectionControlsOption_SPH_upperRightTrackingBoxCorner (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 70))

#define ads_SectionControlsOption_rampInitialStress (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 71))

/** Composition to the section controls options. */
#define ads_SectionControls_sectionControlsOptions (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 72))

/** SectionLayers are members of a counting collection. They are unique within the context of SectionLayup or Element records. A SectionLayer denotes an area in an Element cross-section with different attributes, such as specific number of integration points, different Material, Orientation etc. Such areas in beam sections are also represented by SectionLayers. */
#define ads_SectionLayer (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 73))

/** A counting collection of section layers. */
#define ads_SectionLayerCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 74))

/** This data type respresents a section point in a section layer. Section points are members of a counting collection thus in distributions they are always qualified with a SectionLayer record. */
#define ads_SectionPoint (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 75))

/** Counting collection of section points. */
#define ads_SectionPointCollection (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 76))

/** Beam section */
#define ads_Section_Beam (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 77))

/** Gives local 1-direction if extra node is not defined */
#define ads_Section_Beam_n1 (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 78))

/** Gives local 2-direction - default is default. Must be bound to element nodal field otherwise (replaces *NORMAL) */
#define ads_Section_Beam_n2 (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 79))

/** Data to define the offset values for the origin of the beam cross-section from the beam mesh */
#define ads_Section_Beam_offset (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 80))

/** The Poisson's ratio. Possible values are between -1.0 and 0.5. This argument is valid only when poissonOption=SPECIFIED. */
#define ads_Section_Beam_poisson (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 81))

#define ads_Section_Beam_sectionInertia (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 82))

/** Section output points for the beam general section. */
#define ads_Section_Beam_sectionPoints (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 83))

#define ads_Section_Beam_sectionStiffness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 84))

/** Connector section */
#define ads_Section_Connector (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 85))

/** Connector behavior associated with the connector section. */
#define ads_Section_Connector_connectorBehavior (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 86))

/** Orientation 1 to be used with the section elements */
#define ads_Section_Connector_localSys1 (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 87))

/** Orientation 2 to be used with the section elements */
#define ads_Section_Connector_localSys2 (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 88))

/** Continuum section */
#define ads_Section_Continuum (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 89))

/** Use only for plane and generalized plane strain */
#define ads_Section_Continuum_depth (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 90))

/** Composition to the PlasticsRegion from the Continuum Section. */
#define ads_Section_Continuum_plasticsRegion (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 91))

/** Use only for plane and generalized plane strain */
#define ads_Section_Continuum_thetaX (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 92))

/** Use only for plane and generalized plane strain */
#define ads_Section_Continuum_thetaY (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 93))

/** Discrete section */
#define ads_Section_Discrete (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 94))

/** Cluster MassInertiaTable */
#define ads_Section_Discrete_clusterMassInertia (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 95))

/** density parameter */
#define ads_Section_Discrete_density (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 96))

/** Ecooling section (CFD) */
#define ads_Section_ECooling (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 97))

/** Blower */
#define ads_Section_ECooling_Blower (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 98))

/** Blower impeller */
#define ads_Section_ECooling_Blower_Impeller (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 99))

/** Board surface */
#define ads_Section_ECooling_Blower_inlet (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 100))

/** Case surface */
#define ads_Section_ECooling_Blower_outlet (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 101))

/** Compact Heat Sink */
#define ads_Section_ECooling_CHeatSink (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 102))

/** Compact Heat Sink Extruded options */
#define ads_Section_ECooling_CHeatSink_Extruded (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 103))

/** Compact Heat Sink Manual options */
#define ads_Section_ECooling_CHeatSink_Manual (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 104))

/** Compact PCB */
#define ads_Section_ECooling_CompactPCB (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 105))

#define ads_Section_ECooling_CompactPCB_table (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 106))

/** Trace Material */
#define ads_Section_ECooling_CompactPCB_traceMaterial (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 107))

/** Heat pipe */
#define ads_Section_ECooling_HeatPipe (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 108))

/** Case surface */
#define ads_Section_ECooling_HeatPipe_sinkSurface (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 109))

/** Board surface */
#define ads_Section_ECooling_HeatPipe_sourceSurface (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 110))

/** Perforated Plate */
#define ads_Section_ECooling_PerforatedPlate (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 111))

/** Generic pattern type */
#define ads_Section_ECooling_PerforatedPlate_Generic (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 112))

/** Using pressure velocity curve */
#define ads_Section_ECooling_PerforatedPlate_PVCurve (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 113))

#define ads_Section_ECooling_PerforatedPlate_PVCurve_table (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 114))

/** Thermoelectric Cooler */
#define ads_Section_ECooling_ThermoelectricCooler (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 115))

/** Cold side */
#define ads_Section_ECooling_ThermoelectricCooler_coldSide (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 116))

/** Hot side */
#define ads_Section_ECooling_ThermoelectricCooler_hotSide (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 117))

/** Two Resistor */
#define ads_Section_ECooling_TwoResistor (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 118))

/** Board surface */
#define ads_Section_ECooling_TwoResistor_boardSurface (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 119))

/** Case surface */
#define ads_Section_ECooling_TwoResistor_caseSurface (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 120))

/** Local Coordinate System. */
#define ads_Section_ECooling_axisSys (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 121))

/** Material for the support */
#define ads_Section_ECooling_material (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 122))

/** Supports */
#define ads_Section_ECooling_supports (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 123))

/** Interface section */
#define ads_Section_Interface (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 124))

/** Use for link gaskets, gap elements */
#define ads_Section_Interface_area (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 125))

/** Use only for 3D edge based or 2D gaskets and cohesive and axisymmetric link gaskets */
#define ads_Section_Interface_depth (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 126))

/** Orientation to be used with the section elements */
#define ads_Section_Interface_localSys (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 127))

/** Undefined means that normal determined from element geometry. Typically bound to field in assigment if non-default normal is specified */
#define ads_Section_Interface_normal (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 128))

/** This is really a numerical parameter and therefore is not given as a property Used for gaskets (FL-2) */
#define ads_Section_Interface_stabilizationStiffness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 129))

/** point like section */
#define ads_Section_Lumped (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 130))

/** Orientation to be used with the section elements */
#define ads_Section_Lumped_localSys (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 131))

/** Shell section */
#define ads_Section_Shell (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 132))

#define ads_Section_Shell_addedMassDensity (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 133))

#define ads_Section_Shell_offset (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 134))

/** The Poisson's ratio. Possible values are between -1.0 and 0.5. This argument is valid only when poissonOption=SPECIFIED. */
#define ads_Section_Shell_poisson (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 135))

#define ads_Section_Shell_thickness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 136))

#define ads_Section_Shell_thicknessModulus (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 137))

/** Substructure property */
#define ads_Section_Substructure (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 138))

/** This association represents the point on rotational axis. This is stored as a Vector. */
#define ads_Section_Substructure_centerOfRotation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 139))

/** This association represents the rotational positioning. This is stored as a Quaternion. */
#define ads_Section_Substructure_rotation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 140))

/** This association represents the translational positioning. This is stored as a Vector. */
#define ads_Section_Substructure_translation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 141))

/** Each section consists of one or more layer objects. */
#define ads_Section_layers (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 142))

/** Each section consists of set of Property objects. */
#define ads_Section_properties (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 143))

#define ads_Section_scaleMass (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 144))

#define ads_Section_scaleStiffness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 145))

#define ads_Section_scaleStressDesign (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 146))

#define ads_Section_scaleThermalConductivity (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 147))

/** Referrence from a section to the section controls. */
#define ads_Section_sectionControls (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 148))

/** Anchor to store the bulkified shell section data */
#define ads_ShellSectionDetails (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 149))

/** Composition to a MeshedField to store the shell section constant orientation angle. Its grid is LayerXSection to the value. */
#define ads_ShellSectionDetails_layerConstOrientationAngle (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 150))

/** Composition to a MeshedField to store the shell section constant thickness. Its grid is LayerXSection to the value. */
#define ads_ShellSectionDetails_layerConstThickness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 151))

/** Composition to a MeshedField to store the shell section distributed orientation angle. Its grid is LayerXElement to the value. */
#define ads_ShellSectionDetails_layerDistribOrientationAngle (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 152))

/** Composition to a MeshedField to store the shell section distributed thickness. Its grid is LayerXElement to the value. */
#define ads_ShellSectionDetails_layerDistribThickness (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 153))

/** Composition LayerXSection->Material distribution. */
#define ads_ShellSectionDetails_layerMaterial (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 154))

/** Composition LayerXSection->int (number of section points) distribution. */
#define ads_ShellSectionDetails_layerNumPoints (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 155))

/** Composition LayerXSection->Orientation distribution. */
#define ads_ShellSectionDetails_layerOrientation (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 156))

/** Composition LayerXSection->Ply distribution. */
#define ads_ShellSectionDetails_layerPly (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 157))

#define ads_layerTraceAttributesTable (ads_CoreFragmentTypeIndex(ads_CoreSectionFragment, 158))

/** 
Enum with association roles. */
enum ads_BeamSectionPoints_localCoordinatesRolesEnm
{
    ads_BeamSectionPoints_localCoordinates_child,
    ads_BeamSectionPoints_localCoordinates_parent
};

/** 
Enum with association roles. */
enum ads_BeamSectionPoints_meshedSectionPointsRolesEnm
{
    ads_BeamSectionPoints_meshedSectionPoints_child,
    ads_BeamSectionPoints_meshedSectionPoints_parent
};

/** Enum with association roles. */
enum ads_BeamSectionPoints_tableRolesEnm
{
    ads_BeamSectionPoints_table_child,
    ads_BeamSectionPoints_table_parent
};

/** 
Enum with grid dimensions. */
enum ads_ElementLayerLayerPointComponentGridDimensionsEnm
{
    ads_ElementLayerLayerPointComponentGrid_component,
    ads_ElementLayerLayerPointComponentGrid_element,
    ads_ElementLayerLayerPointComponentGrid_layerPoint,
    ads_ElementLayerLayerPointComponentGrid_sectionLayer
};

/** 
Enum with grid dimensions. */
enum ads_ElementLayerOrientationGridDimensionsEnm
{
    ads_ElementLayerOrientationGrid_element,
    ads_ElementLayerOrientationGrid_layer,
    ads_ElementLayerOrientationGrid_orientation
};

/** 
Enum with grid dimensions. */
enum ads_ElementOrientationGridDimensionsEnm
{
    ads_ElementOrientationGrid_element,
    ads_ElementOrientationGrid_orientation
};

/** 
Enum with grid dimensions. */
enum ads_ElementPointOrientationGridDimensionsEnm
{
    ads_ElementPointOrientationGrid_element,
    ads_ElementPointOrientationGrid_orientation,
    ads_ElementPointOrientationGrid_point
};

/** 
Enum with grid dimensions. */
enum ads_ElementSectionLayerComponentGridDimensionsEnm
{
    ads_ElementSectionLayerComponentGrid_component,
    ads_ElementSectionLayerComponentGrid_element,
    ads_ElementSectionLayerComponentGrid_layer
};

/** 
Enum with record members. */
enum ads_FieldBindingMembersEnm
{
    ads_FieldBinding_parameter
};

/** 
Enum with association roles. */
enum ads_FieldBinding_fieldRolesEnm
{
    ads_FieldBinding_field_referent,
    ads_FieldBinding_field_referrer
};

/** 
Enum with association roles. */
enum ads_FieldBinding_propertyRolesEnm
{
    ads_FieldBinding_property_referent,
    ads_FieldBinding_property_referrer
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_layerPointCollectionRolesEnm
{
    ads_GlobalCollections_layerPointCollection_child,
    ads_GlobalCollections_layerPointCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_materialCollectionRolesEnm
{
    ads_GlobalCollections_materialCollection_child,
    ads_GlobalCollections_materialCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_sectionLayerCollectionRolesEnm
{
    ads_GlobalCollections_sectionLayerCollection_child,
    ads_GlobalCollections_sectionLayerCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_sectionPointCollectionRolesEnm
{
    ads_GlobalCollections_sectionPointCollection_child,
    ads_GlobalCollections_sectionPointCollection_parent
};

/** Enum with grid dimensions. */
enum ads_LayerElementGridDimensionsEnm
{
    ads_LayerElementGrid_element,
    ads_LayerElementGrid_layer
};

/** Enum with grid dimensions. */
enum ads_LayerSectionGridDimensionsEnm
{
    ads_LayerSectionGrid_layer,
    ads_LayerSectionGrid_section
};

/** 
Enum with association roles. */
enum ads_Layer_materialInstancesRolesEnm
{
    ads_Layer_materialInstances_child,
    ads_Layer_materialInstances_parent
};

/** 
Enum with association roles. */
enum ads_Layer_plyRolesEnm
{
    ads_Layer_ply_referent,
    ads_Layer_ply_referrer
};

/** 
Enum with record members. */
enum ads_LocalSysBindingMembersEnm
{
    ads_LocalSysBinding_parameter
};

/** 
Enum with association roles. */
enum ads_LocalSysBinding_localSysBindingRolesEnm
{
    ads_LocalSysBinding_localSysBinding_referent,
    ads_LocalSysBinding_localSysBinding_referrer
};

/** 
Enum with association roles. */
enum ads_LocalSysBinding_propertyRolesEnm
{
    ads_LocalSysBinding_property_referent,
    ads_LocalSysBinding_property_referrer
};

/** 
Enum with grid dimensions. */
enum ads_MaterialElementLayerGridDimensionsEnm
{
    ads_MaterialElementLayerGrid_element,
    ads_MaterialElementLayerGrid_material,
    ads_MaterialElementLayerGrid_sectionLayer
};

/** 
Enum with association roles. */
enum ads_MaterialInstance_localSysRolesEnm
{
    ads_MaterialInstance_localSys_referent,
    ads_MaterialInstance_localSys_referrer
};

/** 
Enum with association roles. */
enum ads_MaterialInstance_materialRolesEnm
{
    ads_MaterialInstance_material_referent,
    ads_MaterialInstance_material_referrer
};

/** 
Enum with association roles. */
enum ads_MaterialInstance_orientationAngleRolesEnm
{
    ads_MaterialInstance_orientationAngle_child,
    ads_MaterialInstance_orientationAngle_parent
};

/** Enum with association roles. */
enum ads_Model_layerCollectionRolesEnm
{
    ads_Model_layerCollection_child,
    ads_Model_layerCollection_parent
};

/** Enum with association roles. */
enum ads_Model_plyCollectionRolesEnm
{
    ads_Model_plyCollection_child,
    ads_Model_plyCollection_parent
};

/** Enum with association roles. */
enum ads_Model_sectionCollectionRolesEnm
{
    ads_Model_sectionCollection_child,
    ads_Model_sectionCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_sectionControlsRolesEnm
{
    ads_Model_sectionControls_child,
    ads_Model_sectionControls_parent
};

/** 
Enum with record members. */
enum ads_PlasticsRegionMembersEnm
{
    ads_PlasticsRegion_regionType
};

enum ads_PlasticsRegion_regionTypeEnm
{
    ads_PlasticsRegion_regionType_COOLING_CHANNEL,
    ads_PlasticsRegion_regionType_MOLD,
    ads_PlasticsRegion_regionType_MOLD_INSERT,
    ads_PlasticsRegion_regionType_PART_CAVITY,
    ads_PlasticsRegion_regionType_PART_INSERT,
    ads_PlasticsRegion_regionType_RUNNER
};

/** 
Enum with association roles. */
enum ads_PlasticsRegion_inputFieldsRolesEnm
{
    ads_PlasticsRegion_inputFields_child,
    ads_PlasticsRegion_inputFields_parent
};

/** 
Enum with grid dimensions. */
enum ads_QuadratureElementLayerLayerPointGridDimensionsEnm
{
    ads_QuadratureElementLayerLayerPointGrid_element,
    ads_QuadratureElementLayerLayerPointGrid_layerPoint,
    ads_QuadratureElementLayerLayerPointGrid_quadrature,
    ads_QuadratureElementLayerLayerPointGrid_sectionLayer
};

/** 
Enum with grid dimensions. */
enum ads_QuadratureElementLayerTemperaturePointGridDimensionsEnm
{
    ads_QuadratureElementLayerTemperaturePointGrid_element,
    ads_QuadratureElementLayerTemperaturePointGrid_quadrature,
    ads_QuadratureElementLayerTemperaturePointGrid_sectionLayer,
    ads_QuadratureElementLayerTemperaturePointGrid_temperaturePoint
};

/** 
Enum with association roles. */
enum ads_ScaleMass_scaleFactorsRolesEnm
{
    ads_ScaleMass_scaleFactors_referent,
    ads_ScaleMass_scaleFactors_referrer
};

/** 
Enum with association roles. */
enum ads_ScaleStiffness_scaleFactorsRolesEnm
{
    ads_ScaleStiffness_scaleFactors_referent,
    ads_ScaleStiffness_scaleFactors_referrer
};

/** 
Enum with association roles. */
enum ads_ScaleStressDesign_scaleFactorsRolesEnm
{
    ads_ScaleStressDesign_scaleFactors_referent,
    ads_ScaleStressDesign_scaleFactors_referrer
};

/** 
Enum with association roles. */
enum ads_ScaleThermalConductivity_scaleFactorsRolesEnm
{
    ads_ScaleThermalConductivity_scaleFactors_referent,
    ads_ScaleThermalConductivity_scaleFactors_referrer
};

/** 
Enum with record members. */
enum ads_SectionMembersEnm
{
    ads_Section_numLayers,
    ads_Section_numRebarLayers
};

/** 
Enum with association roles. */
enum ads_SectionAssignment_fieldBindingsRolesEnm
{
    ads_SectionAssignment_fieldBindings_child,
    ads_SectionAssignment_fieldBindings_parent
};

/** 
Enum with association roles. */
enum ads_SectionAssignment_localSysBindingsRolesEnm
{
    ads_SectionAssignment_localSysBindings_child,
    ads_SectionAssignment_localSysBindings_parent
};

/** 
Enum with association roles. */
enum ads_SectionAssignment_regionRolesEnm
{
    ads_SectionAssignment_region_referent,
    ads_SectionAssignment_region_referrer
};

/** 
Enum with association roles. */
enum ads_SectionAssignment_sectionRolesEnm
{
    ads_SectionAssignment_section_referent,
    ads_SectionAssignment_section_referrer
};

/** 
Enum with record members. */
enum ads_SectionControlsOptionMembersEnm
{
    ads_SectionControlsOption_deleteDistortedElement,
    ads_SectionControlsOption_distortionControl,
    ads_SectionControlsOption_drillStiffness,
    ads_SectionControlsOption_elemCharLen,
    ads_SectionControlsOption_elemStableDT,
    ads_SectionControlsOption_elemVolOrAreaDT,
    ads_SectionControlsOption_elementDeletion,
    ads_SectionControlsOption_hourglass,
    ads_SectionControlsOption_htIntegration,
    ads_SectionControlsOption_improvedDTMethod,
    ads_SectionControlsOption_initialGapOpening,
    ads_SectionControlsOption_kinematicSplit,
    ads_SectionControlsOption_lengthRatio,
    ads_SectionControlsOption_linearKinematicConversion,
    ads_SectionControlsOption_maxDegradation,
    ads_SectionControlsOption_perturbation,
    ads_SectionControlsOption_preactivationScaling,
    ads_SectionControlsOption_ratioElemCharLenByOrigElemCharLen,
    ads_SectionControlsOption_ratioElemStableDTByOrigElemStableDT,
    ads_SectionControlsOption_ratioElemVolOrAreaByOrigElemVolOrArea,
    ads_SectionControlsOption_scalingFactorAccuracyCrit,
    ads_SectionControlsOption_scalingFactorDispDOF,
    ads_SectionControlsOption_scalingFactorDrillStiffness,
    ads_SectionControlsOption_scalingFactorDurationCollisionCrit,
    ads_SectionControlsOption_scalingFactorHourglassStiffness,
    ads_SectionControlsOption_scalingFactorJKRModelCrit,
    ads_SectionControlsOption_scalingFactorLinearBulkViscosity,
    ads_SectionControlsOption_scalingFactorOutOfPlaneDispDOF,
    ads_SectionControlsOption_scalingFactorQuadraticBulkViscosity,
    ads_SectionControlsOption_scalingFactorRayleighCrit,
    ads_SectionControlsOption_scalingFactorRotationalDOF,
    ads_SectionControlsOption_scalingFactorStabilityCrit,
    ads_SectionControlsOption_scalingFactorTangentialCrit,
    ads_SectionControlsOption_secondOrderAccuracy,
    ads_SectionControlsOption_shellDeletionNumber,
    ads_SectionControlsOption_viscosity,
    ads_SectionControlsOption_weightFactor
};

enum ads_SectionControlsOption_hourglassEnm
{
    ads_SectionControlsOption_hourglass_COMBINED,
    ads_SectionControlsOption_hourglass_ENHANCED,
    ads_SectionControlsOption_hourglass_RELAX_STIFFNESS,
    ads_SectionControlsOption_hourglass_STIFFNESS,
    ads_SectionControlsOption_hourglass_VISCOUS
};

enum ads_SectionControlsOption_htIntegrationEnm
{
    ads_SectionControlsOption_htIntegration_GAUSS,
    ads_SectionControlsOption_htIntegration_MIXED,
    ads_SectionControlsOption_htIntegration_NODAL,
    ads_SectionControlsOption_htIntegration_OLD
};

enum ads_SectionControlsOption_improvedDTMethodEnm
{
    ads_SectionControlsOption_improvedDTMethod_GLOBAL,
    ads_SectionControlsOption_improvedDTMethod_NO,
    ads_SectionControlsOption_improvedDTMethod_YES
};

enum ads_SectionControlsOption_kinematicSplitEnm
{
    ads_SectionControlsOption_kinematicSplit_AVERAGE_STRAIN,
    ads_SectionControlsOption_kinematicSplit_CENTROID,
    ads_SectionControlsOption_kinematicSplit_ORTHOGONAL
};

/** 
Enum with record members. */
enum ads_SectionControlsOption_ActivationMembersEnm
{
    ads_SectionControlsOption_Activation_deleteDistortedElement,
    ads_SectionControlsOption_Activation_distortionControl,
    ads_SectionControlsOption_Activation_drillStiffness,
    ads_SectionControlsOption_Activation_elemCharLen,
    ads_SectionControlsOption_Activation_elemStableDT,
    ads_SectionControlsOption_Activation_elemVolOrAreaDT,
    ads_SectionControlsOption_Activation_elementDeletion,
    ads_SectionControlsOption_Activation_hourglass,
    ads_SectionControlsOption_Activation_htIntegration,
    ads_SectionControlsOption_Activation_improvedDTMethod,
    ads_SectionControlsOption_Activation_initialGapOpening,
    ads_SectionControlsOption_Activation_kinematicSplit,
    ads_SectionControlsOption_Activation_lengthRatio,
    ads_SectionControlsOption_Activation_linearKinematicConversion,
    ads_SectionControlsOption_Activation_maxDegradation,
    ads_SectionControlsOption_Activation_perturbation,
    ads_SectionControlsOption_Activation_preactivationScaling,
    ads_SectionControlsOption_Activation_ratioElemCharLenByOrigElemCharLen,
    ads_SectionControlsOption_Activation_ratioElemStableDTByOrigElemStableDT,
    ads_SectionControlsOption_Activation_ratioElemVolOrAreaByOrigElemVolOrArea,
    ads_SectionControlsOption_Activation_scalingFactorAccuracyCrit,
    ads_SectionControlsOption_Activation_scalingFactorDispDOF,
    ads_SectionControlsOption_Activation_scalingFactorDrillStiffness,
    ads_SectionControlsOption_Activation_scalingFactorDurationCollisionCrit,
    ads_SectionControlsOption_Activation_scalingFactorHourglassStiffness,
    ads_SectionControlsOption_Activation_scalingFactorJKRModelCrit,
    ads_SectionControlsOption_Activation_scalingFactorLinearBulkViscosity,
    ads_SectionControlsOption_Activation_scalingFactorOutOfPlaneDispDOF,
    ads_SectionControlsOption_Activation_scalingFactorQuadraticBulkViscosity,
    ads_SectionControlsOption_Activation_scalingFactorRayleighCrit,
    ads_SectionControlsOption_Activation_scalingFactorRotationalDOF,
    ads_SectionControlsOption_Activation_scalingFactorStabilityCrit,
    ads_SectionControlsOption_Activation_scalingFactorTangentialCrit,
    ads_SectionControlsOption_Activation_secondOrderAccuracy,
    ads_SectionControlsOption_Activation_shellDeletionNumber,
    ads_SectionControlsOption_Activation_viscosity,
    ads_SectionControlsOption_Activation_weightFactor
};

enum ads_SectionControlsOption_Activation_hourglassEnm
{
    ads_SectionControlsOption_Activation_hourglass_COMBINED,
    ads_SectionControlsOption_Activation_hourglass_ENHANCED,
    ads_SectionControlsOption_Activation_hourglass_RELAX_STIFFNESS,
    ads_SectionControlsOption_Activation_hourglass_STIFFNESS,
    ads_SectionControlsOption_Activation_hourglass_VISCOUS
};

enum ads_SectionControlsOption_Activation_htIntegrationEnm
{
    ads_SectionControlsOption_Activation_htIntegration_GAUSS,
    ads_SectionControlsOption_Activation_htIntegration_MIXED,
    ads_SectionControlsOption_Activation_htIntegration_NODAL,
    ads_SectionControlsOption_Activation_htIntegration_OLD
};

enum ads_SectionControlsOption_Activation_improvedDTMethodEnm
{
    ads_SectionControlsOption_Activation_improvedDTMethod_GLOBAL,
    ads_SectionControlsOption_Activation_improvedDTMethod_NO,
    ads_SectionControlsOption_Activation_improvedDTMethod_YES
};

enum ads_SectionControlsOption_Activation_kinematicSplitEnm
{
    ads_SectionControlsOption_Activation_kinematicSplit_AVERAGE_STRAIN,
    ads_SectionControlsOption_Activation_kinematicSplit_CENTROID,
    ads_SectionControlsOption_Activation_kinematicSplit_ORTHOGONAL
};

/** 
Enum with record members. */
enum ads_SectionControlsOption_SPHMembersEnm
{
    ads_SectionControlsOption_SPH_deleteDistortedElement,
    ads_SectionControlsOption_SPH_distortionControl,
    ads_SectionControlsOption_SPH_drillStiffness,
    ads_SectionControlsOption_SPH_elemCharLen,
    ads_SectionControlsOption_SPH_elemStableDT,
    ads_SectionControlsOption_SPH_elemVolOrAreaDT,
    ads_SectionControlsOption_SPH_elementDeletion,
    ads_SectionControlsOption_SPH_hourglass,
    ads_SectionControlsOption_SPH_htIntegration,
    ads_SectionControlsOption_SPH_improvedDTMethod,
    ads_SectionControlsOption_SPH_initialGapOpening,
    ads_SectionControlsOption_SPH_kinematicSplit,
    ads_SectionControlsOption_SPH_lengthRatio,
    ads_SectionControlsOption_SPH_linearKinematicConversion,
    ads_SectionControlsOption_SPH_maxDegradation,
    ads_SectionControlsOption_SPH_perturbation,
    ads_SectionControlsOption_SPH_preactivationScaling,
    ads_SectionControlsOption_SPH_ratioElemCharLenByOrigElemCharLen,
    ads_SectionControlsOption_SPH_ratioElemStableDTByOrigElemStableDT,
    ads_SectionControlsOption_SPH_ratioElemVolOrAreaByOrigElemVolOrArea,
    ads_SectionControlsOption_SPH_scalingFactorAccuracyCrit,
    ads_SectionControlsOption_SPH_scalingFactorDispDOF,
    ads_SectionControlsOption_SPH_scalingFactorDrillStiffness,
    ads_SectionControlsOption_SPH_scalingFactorDurationCollisionCrit,
    ads_SectionControlsOption_SPH_scalingFactorHourglassStiffness,
    ads_SectionControlsOption_SPH_scalingFactorJKRModelCrit,
    ads_SectionControlsOption_SPH_scalingFactorLinearBulkViscosity,
    ads_SectionControlsOption_SPH_scalingFactorOutOfPlaneDispDOF,
    ads_SectionControlsOption_SPH_scalingFactorQuadraticBulkViscosity,
    ads_SectionControlsOption_SPH_scalingFactorRayleighCrit,
    ads_SectionControlsOption_SPH_scalingFactorRotationalDOF,
    ads_SectionControlsOption_SPH_scalingFactorStabilityCrit,
    ads_SectionControlsOption_SPH_scalingFactorTangentialCrit,
    ads_SectionControlsOption_SPH_secondOrderAccuracy,
    ads_SectionControlsOption_SPH_shellDeletionNumber,
    ads_SectionControlsOption_SPH_viscosity,
    ads_SectionControlsOption_SPH_weightFactor,
    ads_SectionControlsOption_SPH_conversionCriterion,
    ads_SectionControlsOption_SPH_elementConversion,
    ads_SectionControlsOption_SPH_kernel,
    ads_SectionControlsOption_SPH_meanVelocityFilterCoef,
    ads_SectionControlsOption_SPH_minInfluenceParticles,
    ads_SectionControlsOption_SPH_numParticlesPerIsoDir,
    ads_SectionControlsOption_SPH_particleThickness,
    ads_SectionControlsOption_SPH_smoothingLengthScaleFactor,
    ads_SectionControlsOption_SPH_smoothingLengthVariation,
    ads_SectionControlsOption_SPH_sphConversion,
    ads_SectionControlsOption_SPH_sphDimension,
    ads_SectionControlsOption_SPH_sphFormulation,
    ads_SectionControlsOption_SPH_trackingCriterion
};

enum ads_SectionControlsOption_SPH_hourglassEnm
{
    ads_SectionControlsOption_SPH_hourglass_COMBINED,
    ads_SectionControlsOption_SPH_hourglass_ENHANCED,
    ads_SectionControlsOption_SPH_hourglass_RELAX_STIFFNESS,
    ads_SectionControlsOption_SPH_hourglass_STIFFNESS,
    ads_SectionControlsOption_SPH_hourglass_VISCOUS
};

enum ads_SectionControlsOption_SPH_htIntegrationEnm
{
    ads_SectionControlsOption_SPH_htIntegration_GAUSS,
    ads_SectionControlsOption_SPH_htIntegration_MIXED,
    ads_SectionControlsOption_SPH_htIntegration_NODAL,
    ads_SectionControlsOption_SPH_htIntegration_OLD
};

enum ads_SectionControlsOption_SPH_improvedDTMethodEnm
{
    ads_SectionControlsOption_SPH_improvedDTMethod_GLOBAL,
    ads_SectionControlsOption_SPH_improvedDTMethod_NO,
    ads_SectionControlsOption_SPH_improvedDTMethod_YES
};

enum ads_SectionControlsOption_SPH_kinematicSplitEnm
{
    ads_SectionControlsOption_SPH_kinematicSplit_AVERAGE_STRAIN,
    ads_SectionControlsOption_SPH_kinematicSplit_CENTROID,
    ads_SectionControlsOption_SPH_kinematicSplit_ORTHOGONAL
};

enum ads_SectionControlsOption_SPH_conversionCriterionEnm
{
    ads_SectionControlsOption_SPH_conversionCriterion_DAMAGE,
    ads_SectionControlsOption_SPH_conversionCriterion_DISTORTION,
    ads_SectionControlsOption_SPH_conversionCriterion_SENSOR,
    ads_SectionControlsOption_SPH_conversionCriterion_STRAIN,
    ads_SectionControlsOption_SPH_conversionCriterion_STRESS,
    ads_SectionControlsOption_SPH_conversionCriterion_TIME,
    ads_SectionControlsOption_SPH_conversionCriterion_USER
};

enum ads_SectionControlsOption_SPH_elementConversionEnm
{
    ads_SectionControlsOption_SPH_elementConversion_BACKGROUND_GRID,
    ads_SectionControlsOption_SPH_elementConversion_NO,
    ads_SectionControlsOption_SPH_elementConversion_YES
};

enum ads_SectionControlsOption_SPH_kernelEnm
{
    ads_SectionControlsOption_SPH_kernel_CUBIC,
    ads_SectionControlsOption_SPH_kernel_QUADRATIC,
    ads_SectionControlsOption_SPH_kernel_QUINTIC
};

enum ads_SectionControlsOption_SPH_particleThicknessEnm
{
    ads_SectionControlsOption_SPH_particleThickness_UNIFORM,
    ads_SectionControlsOption_SPH_particleThickness_VARIABLE
};

enum ads_SectionControlsOption_SPH_smoothingLengthVariationEnm
{
    ads_SectionControlsOption_SPH_smoothingLengthVariation_CONSTANT,
    ads_SectionControlsOption_SPH_smoothingLengthVariation_VARIABLE
};

enum ads_SectionControlsOption_SPH_sphConversionEnm
{
    ads_SectionControlsOption_SPH_sphConversion_BACKGROUND_GRID,
    ads_SectionControlsOption_SPH_sphConversion_PER_ELEMENT
};

enum ads_SectionControlsOption_SPH_sphDimensionEnm
{
    ads_SectionControlsOption_SPH_sphDimension_THREE_D,
    ads_SectionControlsOption_SPH_sphDimension_TWO_D
};

enum ads_SectionControlsOption_SPH_sphFormulationEnm
{
    ads_SectionControlsOption_SPH_sphFormulation_CLASSICAL,
    ads_SectionControlsOption_SPH_sphFormulation_DELTA,
    ads_SectionControlsOption_SPH_sphFormulation_NSPH,
    ads_SectionControlsOption_SPH_sphFormulation_XSELFDEFINE,
    ads_SectionControlsOption_SPH_sphFormulation_XSPH
};

enum ads_SectionControlsOption_SPH_trackingCriterionEnm
{
    ads_SectionControlsOption_SPH_trackingCriterion_ALL_PARTICLES,
    ads_SectionControlsOption_SPH_trackingCriterion_PARTICLES_IN_TRACKING_BOX
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_backgroundGridOrientationRolesEnm
{
    ads_SectionControlsOption_SPH_backgroundGridOrientation_referent,
    ads_SectionControlsOption_SPH_backgroundGridOrientation_referrer
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_backgroundGridSpacingRolesEnm
{
    ads_SectionControlsOption_SPH_backgroundGridSpacing_child,
    ads_SectionControlsOption_SPH_backgroundGridSpacing_parent
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_conversionThresholdValueRolesEnm
{
    ads_SectionControlsOption_SPH_conversionThresholdValue_child,
    ads_SectionControlsOption_SPH_conversionThresholdValue_parent
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_lowerLeftTrackingBoxCornerRolesEnm
{
    ads_SectionControlsOption_SPH_lowerLeftTrackingBoxCorner_child,
    ads_SectionControlsOption_SPH_lowerLeftTrackingBoxCorner_parent
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_smoothingLengthRolesEnm
{
    ads_SectionControlsOption_SPH_smoothingLength_child,
    ads_SectionControlsOption_SPH_smoothingLength_parent
};

/** Enum with association roles. */
enum ads_SectionControlsOption_SPH_upperRightTrackingBoxCornerRolesEnm
{
    ads_SectionControlsOption_SPH_upperRightTrackingBoxCorner_child,
    ads_SectionControlsOption_SPH_upperRightTrackingBoxCorner_parent
};

/** Enum with association roles. */
enum ads_SectionControlsOption_rampInitialStressRolesEnm
{
    ads_SectionControlsOption_rampInitialStress_referent,
    ads_SectionControlsOption_rampInitialStress_referrer
};

/** 
Enum with association roles. */
enum ads_SectionControls_sectionControlsOptionsRolesEnm
{
    ads_SectionControls_sectionControlsOptions_child,
    ads_SectionControls_sectionControlsOptions_parent
};

/** 
Enum with record members. */
enum ads_Section_BeamMembersEnm
{
    ads_Section_Beam_numLayers,
    ads_Section_Beam_numRebarLayers,
    ads_Section_Beam_lumpedMass,
    ads_Section_Beam_poissonOption,
    ads_Section_Beam_preIntegrate,
    ads_Section_Beam_rotaryInertia,
    ads_Section_Beam_sectionType,
    ads_Section_Beam_temperature
};

enum ads_Section_Beam_poissonOptionEnm
{
    ads_Section_Beam_poissonOption_DEFAULT,
    ads_Section_Beam_poissonOption_ELASTIC,
    ads_Section_Beam_poissonOption_MATERIAL,
    ads_Section_Beam_poissonOption_SPECIFIED
};

enum ads_Section_Beam_rotaryInertiaEnm
{
    ads_Section_Beam_rotaryInertia_EXACT,
    ads_Section_Beam_rotaryInertia_ISOTROPIC
};

enum ads_Section_Beam_sectionTypeEnm
{
    ads_Section_Beam_sectionType_BEAM,
    ads_Section_Beam_sectionType_FRAME
};

enum ads_Section_Beam_temperatureEnm
{
    ads_Section_Beam_temperature_GRADIENTS,
    ads_Section_Beam_temperature_VALUES
};

/** 
Enum with association roles. */
enum ads_Section_Beam_n1RolesEnm
{
    ads_Section_Beam_n1_child,
    ads_Section_Beam_n1_parent
};

/** 
Enum with association roles. */
enum ads_Section_Beam_n2RolesEnm
{
    ads_Section_Beam_n2_child,
    ads_Section_Beam_n2_parent
};

/** 
Enum with association roles. */
enum ads_Section_Beam_offsetRolesEnm
{
    ads_Section_Beam_offset_child,
    ads_Section_Beam_offset_parent
};

/** 
Enum with association roles. */
enum ads_Section_Beam_poissonRolesEnm
{
    ads_Section_Beam_poisson_child,
    ads_Section_Beam_poisson_parent
};

/** Enum with association roles. */
enum ads_Section_Beam_sectionInertiaRolesEnm
{
    ads_Section_Beam_sectionInertia_child,
    ads_Section_Beam_sectionInertia_parent
};

/** 
Enum with association roles. */
enum ads_Section_Beam_sectionPointsRolesEnm
{
    ads_Section_Beam_sectionPoints_child,
    ads_Section_Beam_sectionPoints_parent
};

/** Enum with association roles. */
enum ads_Section_Beam_sectionStiffnessRolesEnm
{
    ads_Section_Beam_sectionStiffness_child,
    ads_Section_Beam_sectionStiffness_parent
};

/** 
Enum with record members. */
enum ads_Section_ConnectorMembersEnm
{
    ads_Section_Connector_numLayers,
    ads_Section_Connector_numRebarLayers
};

/** 
Enum with association roles. */
enum ads_Section_Connector_connectorBehaviorRolesEnm
{
    ads_Section_Connector_connectorBehavior_referent,
    ads_Section_Connector_connectorBehavior_referrer
};

/** 
Enum with association roles. */
enum ads_Section_Connector_localSys1RolesEnm
{
    ads_Section_Connector_localSys1_referent,
    ads_Section_Connector_localSys1_referrer
};

/** 
Enum with association roles. */
enum ads_Section_Connector_localSys2RolesEnm
{
    ads_Section_Connector_localSys2_referent,
    ads_Section_Connector_localSys2_referrer
};

/** 
Enum with record members. */
enum ads_Section_ContinuumMembersEnm
{
    ads_Section_Continuum_numLayers,
    ads_Section_Continuum_numRebarLayers,
    ads_Section_Continuum_infiniteElementOrder,
    ads_Section_Continuum_layupName,
    ads_Section_Continuum_sectionType,
    ads_Section_Continuum_stackDirection,
    ads_Section_Continuum_symmetricLayup
};

enum ads_Section_Continuum_sectionTypeEnm
{
    ads_Section_Continuum_sectionType_FLUID,
    ads_Section_Continuum_sectionType_SOLID
};

enum ads_Section_Continuum_stackDirectionEnm
{
    ads_Section_Continuum_stackDirection_STACK_1,
    ads_Section_Continuum_stackDirection_STACK_2,
    ads_Section_Continuum_stackDirection_STACK_3,
    ads_Section_Continuum_stackDirection_STACK_ORIENTATION
};

/** 
Enum with association roles. */
enum ads_Section_Continuum_depthRolesEnm
{
    ads_Section_Continuum_depth_child,
    ads_Section_Continuum_depth_parent
};

/** 
Enum with association roles. */
enum ads_Section_Continuum_plasticsRegionRolesEnm
{
    ads_Section_Continuum_plasticsRegion_child,
    ads_Section_Continuum_plasticsRegion_parent
};

/** 
Enum with association roles. */
enum ads_Section_Continuum_thetaXRolesEnm
{
    ads_Section_Continuum_thetaX_child,
    ads_Section_Continuum_thetaX_parent
};

/** 
Enum with association roles. */
enum ads_Section_Continuum_thetaYRolesEnm
{
    ads_Section_Continuum_thetaY_child,
    ads_Section_Continuum_thetaY_parent
};

/** 
Enum with record members. */
enum ads_Section_DiscreteMembersEnm
{
    ads_Section_Discrete_numLayers,
    ads_Section_Discrete_numRebarLayers,
    ads_Section_Discrete_alpha
};

/** 
Enum with association roles. */
enum ads_Section_Discrete_clusterMassInertiaRolesEnm
{
    ads_Section_Discrete_clusterMassInertia_child,
    ads_Section_Discrete_clusterMassInertia_parent
};

/** 
Enum with association roles. */
enum ads_Section_Discrete_densityRolesEnm
{
    ads_Section_Discrete_density_child,
    ads_Section_Discrete_density_parent
};

/** 
Enum with record members. */
enum ads_Section_ECoolingMembersEnm
{
    ads_Section_ECooling_numLayers,
    ads_Section_ECooling_numRebarLayers,
    ads_Section_ECooling_direction
};

enum ads_Section_ECooling_directionEnm
{
    ads_Section_ECooling_direction_X,
    ads_Section_ECooling_direction_Y,
    ads_Section_ECooling_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_BlowerMembersEnm
{
    ads_Section_ECooling_Blower_numLayers,
    ads_Section_ECooling_Blower_numRebarLayers,
    ads_Section_ECooling_Blower_direction,
    ads_Section_ECooling_Blower_fanHubRadius
};

enum ads_Section_ECooling_Blower_directionEnm
{
    ads_Section_ECooling_Blower_direction_X,
    ads_Section_ECooling_Blower_direction_Y,
    ads_Section_ECooling_Blower_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_Blower_ImpellerMembersEnm
{
    ads_Section_ECooling_Blower_Impeller_numLayers,
    ads_Section_ECooling_Blower_Impeller_numRebarLayers,
    ads_Section_ECooling_Blower_Impeller_direction,
    ads_Section_ECooling_Blower_Impeller_fanHubRadius,
    ads_Section_ECooling_Blower_Impeller_bladeAngle
};

enum ads_Section_ECooling_Blower_Impeller_directionEnm
{
    ads_Section_ECooling_Blower_Impeller_direction_X,
    ads_Section_ECooling_Blower_Impeller_direction_Y,
    ads_Section_ECooling_Blower_Impeller_direction_Z
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_Blower_inletRolesEnm
{
    ads_Section_ECooling_Blower_inlet_referent,
    ads_Section_ECooling_Blower_inlet_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_Blower_outletRolesEnm
{
    ads_Section_ECooling_Blower_outlet_referent,
    ads_Section_ECooling_Blower_outlet_referrer
};

/** 
Enum with record members. */
enum ads_Section_ECooling_CHeatSinkMembersEnm
{
    ads_Section_ECooling_CHeatSink_numLayers,
    ads_Section_ECooling_CHeatSink_numRebarLayers,
    ads_Section_ECooling_CHeatSink_direction
};

enum ads_Section_ECooling_CHeatSink_directionEnm
{
    ads_Section_ECooling_CHeatSink_direction_X,
    ads_Section_ECooling_CHeatSink_direction_Y,
    ads_Section_ECooling_CHeatSink_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_CHeatSink_ExtrudedMembersEnm
{
    ads_Section_ECooling_CHeatSink_Extruded_numLayers,
    ads_Section_ECooling_CHeatSink_Extruded_numRebarLayers,
    ads_Section_ECooling_CHeatSink_Extruded_direction,
    ads_Section_ECooling_CHeatSink_Extruded_baseLength,
    ads_Section_ECooling_CHeatSink_Extruded_blockage,
    ads_Section_ECooling_CHeatSink_Extruded_finBaseHeightRatio,
    ads_Section_ECooling_CHeatSink_Extruded_finHeightRatio,
    ads_Section_ECooling_CHeatSink_Extruded_numFins
};

enum ads_Section_ECooling_CHeatSink_Extruded_directionEnm
{
    ads_Section_ECooling_CHeatSink_Extruded_direction_X,
    ads_Section_ECooling_CHeatSink_Extruded_direction_Y,
    ads_Section_ECooling_CHeatSink_Extruded_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_CHeatSink_ManualMembersEnm
{
    ads_Section_ECooling_CHeatSink_Manual_numLayers,
    ads_Section_ECooling_CHeatSink_Manual_numRebarLayers,
    ads_Section_ECooling_CHeatSink_Manual_direction,
    ads_Section_ECooling_CHeatSink_Manual_dxx,
    ads_Section_ECooling_CHeatSink_Manual_dyy,
    ads_Section_ECooling_CHeatSink_Manual_dzz,
    ads_Section_ECooling_CHeatSink_Manual_kxx,
    ads_Section_ECooling_CHeatSink_Manual_kyy,
    ads_Section_ECooling_CHeatSink_Manual_kzz
};

enum ads_Section_ECooling_CHeatSink_Manual_directionEnm
{
    ads_Section_ECooling_CHeatSink_Manual_direction_X,
    ads_Section_ECooling_CHeatSink_Manual_direction_Y,
    ads_Section_ECooling_CHeatSink_Manual_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_CompactPCBMembersEnm
{
    ads_Section_ECooling_CompactPCB_numLayers,
    ads_Section_ECooling_CompactPCB_numRebarLayers,
    ads_Section_ECooling_CompactPCB_direction,
    ads_Section_ECooling_CompactPCB_nLayers
};

enum ads_Section_ECooling_CompactPCB_directionEnm
{
    ads_Section_ECooling_CompactPCB_direction_X,
    ads_Section_ECooling_CompactPCB_direction_Y,
    ads_Section_ECooling_CompactPCB_direction_Z
};

/** Enum with association roles. */
enum ads_Section_ECooling_CompactPCB_tableRolesEnm
{
    ads_Section_ECooling_CompactPCB_table_child,
    ads_Section_ECooling_CompactPCB_table_parent
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_CompactPCB_traceMaterialRolesEnm
{
    ads_Section_ECooling_CompactPCB_traceMaterial_referent,
    ads_Section_ECooling_CompactPCB_traceMaterial_referrer
};

/** 
Enum with record members. */
enum ads_Section_ECooling_HeatPipeMembersEnm
{
    ads_Section_ECooling_HeatPipe_numLayers,
    ads_Section_ECooling_HeatPipe_numRebarLayers,
    ads_Section_ECooling_HeatPipe_direction,
    ads_Section_ECooling_HeatPipe_effectiveConductivity,
    ads_Section_ECooling_HeatPipe_effectiveHeatCapacity
};

enum ads_Section_ECooling_HeatPipe_directionEnm
{
    ads_Section_ECooling_HeatPipe_direction_X,
    ads_Section_ECooling_HeatPipe_direction_Y,
    ads_Section_ECooling_HeatPipe_direction_Z
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_HeatPipe_sinkSurfaceRolesEnm
{
    ads_Section_ECooling_HeatPipe_sinkSurface_referent,
    ads_Section_ECooling_HeatPipe_sinkSurface_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_HeatPipe_sourceSurfaceRolesEnm
{
    ads_Section_ECooling_HeatPipe_sourceSurface_referent,
    ads_Section_ECooling_HeatPipe_sourceSurface_referrer
};

/** 
Enum with record members. */
enum ads_Section_ECooling_PerforatedPlateMembersEnm
{
    ads_Section_ECooling_PerforatedPlate_numLayers,
    ads_Section_ECooling_PerforatedPlate_numRebarLayers,
    ads_Section_ECooling_PerforatedPlate_direction
};

enum ads_Section_ECooling_PerforatedPlate_directionEnm
{
    ads_Section_ECooling_PerforatedPlate_direction_X,
    ads_Section_ECooling_PerforatedPlate_direction_Y,
    ads_Section_ECooling_PerforatedPlate_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_PerforatedPlate_GenericMembersEnm
{
    ads_Section_ECooling_PerforatedPlate_Generic_numLayers,
    ads_Section_ECooling_PerforatedPlate_Generic_numRebarLayers,
    ads_Section_ECooling_PerforatedPlate_Generic_direction,
    ads_Section_ECooling_PerforatedPlate_Generic_freeAreaRatio,
    ads_Section_ECooling_PerforatedPlate_Generic_holeSize,
    ads_Section_ECooling_PerforatedPlate_Generic_thickness
};

enum ads_Section_ECooling_PerforatedPlate_Generic_directionEnm
{
    ads_Section_ECooling_PerforatedPlate_Generic_direction_X,
    ads_Section_ECooling_PerforatedPlate_Generic_direction_Y,
    ads_Section_ECooling_PerforatedPlate_Generic_direction_Z
};

/** 
Enum with record members. */
enum ads_Section_ECooling_PerforatedPlate_PVCurveMembersEnm
{
    ads_Section_ECooling_PerforatedPlate_PVCurve_numLayers,
    ads_Section_ECooling_PerforatedPlate_PVCurve_numRebarLayers,
    ads_Section_ECooling_PerforatedPlate_PVCurve_direction,
    ads_Section_ECooling_PerforatedPlate_PVCurve_thickness
};

enum ads_Section_ECooling_PerforatedPlate_PVCurve_directionEnm
{
    ads_Section_ECooling_PerforatedPlate_PVCurve_direction_X,
    ads_Section_ECooling_PerforatedPlate_PVCurve_direction_Y,
    ads_Section_ECooling_PerforatedPlate_PVCurve_direction_Z
};

/** Enum with association roles. */
enum ads_Section_ECooling_PerforatedPlate_PVCurve_tableRolesEnm
{
    ads_Section_ECooling_PerforatedPlate_PVCurve_table_child,
    ads_Section_ECooling_PerforatedPlate_PVCurve_table_parent
};

/** 
Enum with record members. */
enum ads_Section_ECooling_ThermoelectricCoolerMembersEnm
{
    ads_Section_ECooling_ThermoelectricCooler_numLayers,
    ads_Section_ECooling_ThermoelectricCooler_numRebarLayers,
    ads_Section_ECooling_ThermoelectricCooler_direction,
    ads_Section_ECooling_ThermoelectricCooler_currentMax,
    ads_Section_ECooling_ThermoelectricCooler_dTempMax,
    ads_Section_ECooling_ThermoelectricCooler_geometryFactor,
    ads_Section_ECooling_ThermoelectricCooler_numberThermoCouples,
    ads_Section_ECooling_ThermoelectricCooler_voltageMax
};

enum ads_Section_ECooling_ThermoelectricCooler_directionEnm
{
    ads_Section_ECooling_ThermoelectricCooler_direction_X,
    ads_Section_ECooling_ThermoelectricCooler_direction_Y,
    ads_Section_ECooling_ThermoelectricCooler_direction_Z
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_ThermoelectricCooler_coldSideRolesEnm
{
    ads_Section_ECooling_ThermoelectricCooler_coldSide_referent,
    ads_Section_ECooling_ThermoelectricCooler_coldSide_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_ThermoelectricCooler_hotSideRolesEnm
{
    ads_Section_ECooling_ThermoelectricCooler_hotSide_referent,
    ads_Section_ECooling_ThermoelectricCooler_hotSide_referrer
};

/** 
Enum with record members. */
enum ads_Section_ECooling_TwoResistorMembersEnm
{
    ads_Section_ECooling_TwoResistor_numLayers,
    ads_Section_ECooling_TwoResistor_numRebarLayers,
    ads_Section_ECooling_TwoResistor_direction,
    ads_Section_ECooling_TwoResistor_emissivity,
    ads_Section_ECooling_TwoResistor_junctionPower,
    ads_Section_ECooling_TwoResistor_thetaJB,
    ads_Section_ECooling_TwoResistor_thetaJC
};

enum ads_Section_ECooling_TwoResistor_directionEnm
{
    ads_Section_ECooling_TwoResistor_direction_X,
    ads_Section_ECooling_TwoResistor_direction_Y,
    ads_Section_ECooling_TwoResistor_direction_Z
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_TwoResistor_boardSurfaceRolesEnm
{
    ads_Section_ECooling_TwoResistor_boardSurface_referent,
    ads_Section_ECooling_TwoResistor_boardSurface_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_TwoResistor_caseSurfaceRolesEnm
{
    ads_Section_ECooling_TwoResistor_caseSurface_referent,
    ads_Section_ECooling_TwoResistor_caseSurface_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_axisSysRolesEnm
{
    ads_Section_ECooling_axisSys_referent,
    ads_Section_ECooling_axisSys_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_materialRolesEnm
{
    ads_Section_ECooling_material_referent,
    ads_Section_ECooling_material_referrer
};

/** 
Enum with association roles. */
enum ads_Section_ECooling_supportsRolesEnm
{
    ads_Section_ECooling_supports_referent,
    ads_Section_ECooling_supports_referrer
};

/** 
Enum with record members. */
enum ads_Section_InterfaceMembersEnm
{
    ads_Section_Interface_numLayers,
    ads_Section_Interface_numRebarLayers,
    ads_Section_Interface_materialResponse,
    ads_Section_Interface_sectionType,
    ads_Section_Interface_stackDirection
};

enum ads_Section_Interface_materialResponseEnm
{
    ads_Section_Interface_materialResponse_TRACTION_SEPARATION,
    ads_Section_Interface_materialResponse_UNIAXIAL_STRAIN,
    ads_Section_Interface_materialResponse_UNIAXIAL_STRESS
};

enum ads_Section_Interface_sectionTypeEnm
{
    ads_Section_Interface_sectionType_COHESIVE,
    ads_Section_Interface_sectionType_GASKET,
    ads_Section_Interface_sectionType_INTERFACE
};

enum ads_Section_Interface_stackDirectionEnm
{
    ads_Section_Interface_stackDirection_STACK_1,
    ads_Section_Interface_stackDirection_STACK_2,
    ads_Section_Interface_stackDirection_STACK_3,
    ads_Section_Interface_stackDirection_STACK_ORIENTATION
};

/** 
Enum with association roles. */
enum ads_Section_Interface_areaRolesEnm
{
    ads_Section_Interface_area_child,
    ads_Section_Interface_area_parent
};

/** 
Enum with association roles. */
enum ads_Section_Interface_depthRolesEnm
{
    ads_Section_Interface_depth_child,
    ads_Section_Interface_depth_parent
};

/** 
Enum with association roles. */
enum ads_Section_Interface_localSysRolesEnm
{
    ads_Section_Interface_localSys_referent,
    ads_Section_Interface_localSys_referrer
};

/** 
Enum with association roles. */
enum ads_Section_Interface_normalRolesEnm
{
    ads_Section_Interface_normal_child,
    ads_Section_Interface_normal_parent
};

/** 
Enum with association roles. */
enum ads_Section_Interface_stabilizationStiffnessRolesEnm
{
    ads_Section_Interface_stabilizationStiffness_child,
    ads_Section_Interface_stabilizationStiffness_parent
};

/** 
Enum with record members. */
enum ads_Section_LumpedMembersEnm
{
    ads_Section_Lumped_numLayers,
    ads_Section_Lumped_numRebarLayers
};

/** 
Enum with association roles. */
enum ads_Section_Lumped_localSysRolesEnm
{
    ads_Section_Lumped_localSys_referent,
    ads_Section_Lumped_localSys_referrer
};

/** 
Enum with record members. */
enum ads_Section_ShellMembersEnm
{
    ads_Section_Shell_numLayers,
    ads_Section_Shell_numRebarLayers,
    ads_Section_Shell_composite,
    ads_Section_Shell_fieldVariableLocation,
    ads_Section_Shell_layupName,
    ads_Section_Shell_poissonOption,
    ads_Section_Shell_preIntegrate,
    ads_Section_Shell_response,
    ads_Section_Shell_sectionIntegration,
    ads_Section_Shell_sectionType,
    ads_Section_Shell_smearAllLayers,
    ads_Section_Shell_stackDirection,
    ads_Section_Shell_symmetricLayup,
    ads_Section_Shell_temperaturePoints
};

enum ads_Section_Shell_fieldVariableLocationEnm
{
    ads_Section_Shell_fieldVariableLocation_GRADIENT,
    ads_Section_Shell_fieldVariableLocation_POINTWISE
};

enum ads_Section_Shell_poissonOptionEnm
{
    ads_Section_Shell_poissonOption_DEFAULT,
    ads_Section_Shell_poissonOption_ELASTIC,
    ads_Section_Shell_poissonOption_MATERIAL,
    ads_Section_Shell_poissonOption_SPECIFIED
};

enum ads_Section_Shell_responseEnm
{
    ads_Section_Shell_response_BENDING_ONLY,
    ads_Section_Shell_response_COMPLETE,
    ads_Section_Shell_response_MEMBRANE_ONLY
};

enum ads_Section_Shell_sectionIntegrationEnm
{
    ads_Section_Shell_sectionIntegration_GAUSS,
    ads_Section_Shell_sectionIntegration_SIMPSON
};

enum ads_Section_Shell_sectionTypeEnm
{
    ads_Section_Shell_sectionType_MEMBRANE,
    ads_Section_Shell_sectionType_SHELL,
    ads_Section_Shell_sectionType_SURFACE
};

enum ads_Section_Shell_stackDirectionEnm
{
    ads_Section_Shell_stackDirection_LOCAL_SYS,
    ads_Section_Shell_stackDirection_X,
    ads_Section_Shell_stackDirection_Y,
    ads_Section_Shell_stackDirection_Z
};

/** Enum with association roles. */
enum ads_Section_Shell_addedMassDensityRolesEnm
{
    ads_Section_Shell_addedMassDensity_child,
    ads_Section_Shell_addedMassDensity_parent
};

/** Enum with association roles. */
enum ads_Section_Shell_offsetRolesEnm
{
    ads_Section_Shell_offset_child,
    ads_Section_Shell_offset_parent
};

/** 
Enum with association roles. */
enum ads_Section_Shell_poissonRolesEnm
{
    ads_Section_Shell_poisson_child,
    ads_Section_Shell_poisson_parent
};

/** Enum with association roles. */
enum ads_Section_Shell_thicknessRolesEnm
{
    ads_Section_Shell_thickness_child,
    ads_Section_Shell_thickness_parent
};

/** Enum with association roles. */
enum ads_Section_Shell_thicknessModulusRolesEnm
{
    ads_Section_Shell_thicknessModulus_child,
    ads_Section_Shell_thicknessModulus_parent
};

/** 
Enum with record members. */
enum ads_Section_SubstructureMembersEnm
{
    ads_Section_Substructure_numLayers,
    ads_Section_Substructure_numRebarLayers,
    ads_Section_Substructure_display,
    ads_Section_Substructure_positionTolerance
};

/** 
Enum with association roles. */
enum ads_Section_Substructure_centerOfRotationRolesEnm
{
    ads_Section_Substructure_centerOfRotation_child,
    ads_Section_Substructure_centerOfRotation_parent
};

/** 
Enum with association roles. */
enum ads_Section_Substructure_rotationRolesEnm
{
    ads_Section_Substructure_rotation_child,
    ads_Section_Substructure_rotation_parent
};

/** 
Enum with association roles. */
enum ads_Section_Substructure_translationRolesEnm
{
    ads_Section_Substructure_translation_child,
    ads_Section_Substructure_translation_parent
};

/** 
Enum with association roles. */
enum ads_Section_layersRolesEnm
{
    ads_Section_layers_child,
    ads_Section_layers_parent
};

/** 
Enum with association roles. */
enum ads_Section_propertiesRolesEnm
{
    ads_Section_properties_child,
    ads_Section_properties_parent
};

/** Enum with association roles. */
enum ads_Section_scaleMassRolesEnm
{
    ads_Section_scaleMass_child,
    ads_Section_scaleMass_parent
};

/** Enum with association roles. */
enum ads_Section_scaleStiffnessRolesEnm
{
    ads_Section_scaleStiffness_child,
    ads_Section_scaleStiffness_parent
};

/** Enum with association roles. */
enum ads_Section_scaleStressDesignRolesEnm
{
    ads_Section_scaleStressDesign_child,
    ads_Section_scaleStressDesign_parent
};

/** Enum with association roles. */
enum ads_Section_scaleThermalConductivityRolesEnm
{
    ads_Section_scaleThermalConductivity_child,
    ads_Section_scaleThermalConductivity_parent
};

/** 
Enum with association roles. */
enum ads_Section_sectionControlsRolesEnm
{
    ads_Section_sectionControls_referent,
    ads_Section_sectionControls_referrer
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerConstOrientationAngleRolesEnm
{
    ads_ShellSectionDetails_layerConstOrientationAngle_child,
    ads_ShellSectionDetails_layerConstOrientationAngle_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerConstThicknessRolesEnm
{
    ads_ShellSectionDetails_layerConstThickness_child,
    ads_ShellSectionDetails_layerConstThickness_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerDistribOrientationAngleRolesEnm
{
    ads_ShellSectionDetails_layerDistribOrientationAngle_child,
    ads_ShellSectionDetails_layerDistribOrientationAngle_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerDistribThicknessRolesEnm
{
    ads_ShellSectionDetails_layerDistribThickness_child,
    ads_ShellSectionDetails_layerDistribThickness_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerMaterialRolesEnm
{
    ads_ShellSectionDetails_layerMaterial_child,
    ads_ShellSectionDetails_layerMaterial_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerNumPointsRolesEnm
{
    ads_ShellSectionDetails_layerNumPoints_child,
    ads_ShellSectionDetails_layerNumPoints_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerOrientationRolesEnm
{
    ads_ShellSectionDetails_layerOrientation_child,
    ads_ShellSectionDetails_layerOrientation_parent
};

/** 
Enum with association roles. */
enum ads_ShellSectionDetails_layerPlyRolesEnm
{
    ads_ShellSectionDetails_layerPly_child,
    ads_ShellSectionDetails_layerPly_parent
};

#endif
