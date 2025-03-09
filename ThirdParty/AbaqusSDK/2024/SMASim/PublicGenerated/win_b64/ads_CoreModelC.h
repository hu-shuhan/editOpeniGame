//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreModelC_h
#define ads_CoreModelC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Model of the latest level of form Core */

/** This option is used to capture active elements */
#define ads_ActivateElements (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 0))

#define ads_ActivateElements_elementProgressiveActivation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 1))

#define ads_ActivateElements_tableContainer (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 2))

/** This option is used to adjust user-specified nodal coordinates */
#define ads_AdjustNodeCoordinates (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 3))

/** Distribution that specifies nodes and adjustments (displacements) to the position of these nodes */
#define ads_AdjustNodeCoordinates_distribution (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 4))

/** Node set containing the nodes to be adjusted */
#define ads_AdjustNodeCoordinates_nodeSet (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 5))

/** The direction for adjusting nodes. If this parameter is omitted, the nodes are adjusted normal to the specified surface */
#define ads_AdjustNodeCoordinates_orientation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 6))

/** Surface to which the nodes are to be adjusted */
#define ads_AdjustNodeCoordinates_surface (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 7))

/** This option is used to capture the design variables in *DISTRIBUTION */
#define ads_DesignVariable (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 8))

/** Distribution that specifies the nodes adjustments */
#define ads_DesignVariable_distribution (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 9))

#define ads_ElementOccurrenceGrid (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 10))

/** This option is used to capture element progressive activations */
#define ads_ElementProgressiveActivation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 11))

#define ads_ElementProgressiveActivation_region (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 12))

/** *Equation */
#define ads_Equation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 13))

/** *Equation */
#define ads_Equation6Terms (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 14))

#define ads_Equation_coefficients (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 15))

#define ads_Equation_nodalDofSets (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 16))

#define ads_Equation_nodalDofTable (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 17))

/** This option is used to capture event series. The columns are captured through the GenericFTableType. The columns in this FTable must be same as the columns in the EventSeriesType FTable. */
#define ads_EventSeries (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 18))

/** This option is used to capture event series types. The column definitions of the EventSeriesType are captured through the GenericFTableType. The first four columns are always "TIME", "X", "Y", "Z". */
#define ads_EventSeriesType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 19))

#define ads_EventSeriesType_table (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 20))

/** This association represents the point on rotational axis. This is stored as a Vector. */
#define ads_EventSeries_centerOfRotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 21))

/** Reference to EventSeriesType */
#define ads_EventSeries_eventSeriesType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 22))

/** Links EventSeries to an ExternalSource. */
#define ads_EventSeries_externalSource (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 23))

/** Reference to the PhysicalDimensions of the columns in the FTable. */
#define ads_EventSeries_physicalDimensions (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 24))

/** This association represents the rotational positioning. This is stored as a Quaternion. */
#define ads_EventSeries_rotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 25))

#define ads_EventSeries_table (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 26))

/** This association represents the translational positioning. This is stored as a Vector. */
#define ads_EventSeries_translation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 27))

#define ads_EventSeries_unitsRecords (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 28))

/** The simulated model. */
#define ads_Focus_model (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 29))

/** Models for occurences */
#define ads_Focus_occurrenceModels (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 30))

/** No columns are mandatory when defining a GenericFTableType. The column names must have corresponding field types for the units conversion to work. The client dynamically adds the dependant optional and independent optional columns. The client also creates the required field types at run time that are not already there in the SIMLib. */
#define ads_GenericFTableType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 31))

#define ads_ImportInstance (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 32))

/** Links ImportInstance to an ExternalSource. */
#define ads_ImportInstance_externalSource (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 33))

/** Positioning of the ImportInstance. */
#define ads_ImportInstance_positioning (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 34))

/** Reference coordinates for the ImportInstance. */
#define ads_ImportInstance_referenceCoordinates (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 35))

/** Regions referred by this import instance. */
#define ads_ImportInstance_regions (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 36))

#define ads_ImportInstance_stepInc (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 37))

#define ads_LinearizedMaterialProperties (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 38))

/** An anchor to hold all the links from GShape to the auxids of the CGM files and the translations and rotations of the CGM bodies. */
#define ads_LinkedShapes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 39))

/** Distribution to capture the rotations of the GShapes. */
#define ads_LinkedShapes_rotations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 40))

/** Distribution to capture the map from GShapes to CGM auxids. */
#define ads_LinkedShapes_shape2auxid (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 41))

/** Distribution to capture the translations of the GShapes. */
#define ads_LinkedShapes_translations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 42))

/** This option is used to define mass matrix. */
#define ads_MassMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 43))

/** This option is used to identify a stiffness, mass, or damping matrix that will be assembled into the corresponding global finite element matrix. */
#define ads_MatrixAssemble (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 44))

/** This association represents the point on rotational axis. This is stored as a Vector. */
#define ads_MatrixAssemble_centerOfRotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 45))

#define ads_MatrixAssemble_massMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 46))

/** This association represents the rotational positionning. This is stored as a Quaternion. */
#define ads_MatrixAssemble_rotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 47))

#define ads_MatrixAssemble_stiffnessMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 48))

#define ads_MatrixAssemble_structuralDampingMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 49))

/** This association represents the translational positionning. This is stored as a Vector. */
#define ads_MatrixAssemble_translation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 50))

#define ads_MatrixAssemble_viscousDampingMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 51))

/** This option is used to define stiffness, mass, viscous damping, or structural damping matrix for a part of the model or for the entire model. It is defined by giving it a unique name and by specifying matrix data, which may be scaled. */
#define ads_MatrixInput (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 52))

/** Container data record which serves as an anchor for required matrix distributions. */
#define ads_MatrixInputData (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 53))

/** Matrix dimension for DOF. */
#define ads_MatrixInputData_dimDOF1 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 54))

/** Matrix dimension for DOF. */
#define ads_MatrixInputData_dimDOF2 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 55))

/** Matrix dimension for Node Indices. */
#define ads_MatrixInputData_dimNode1 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 56))

/** Matrix dimension for Node Indices. */
#define ads_MatrixInputData_dimNode2 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 57))

/** To store mass matrix Values. */
#define ads_MatrixInputData_matrixValues (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 58))

/** Stores each matrix grid dimension data in saperate distribution as they may contain repeating values. */
#define ads_MatrixInput_data (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 59))

/** A d-set representing nested susbtructures within the current mesh and their link with the mesh elements. Any nested susbtructure results in one or more occurrences, each of which must be linked with one unique element - 1-to-1 relation. */
#define ads_Mesh_substructureOccurrences (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 60))

/** The anchor record for the part of the SIM data model that describes the representation of the physical objects whose behavior we wish to simulate: The physical objects include the objects being designed as well as surrounding objects, like attached structures, fluids, gases, foundations. In a SIM document, there is a single top Model. Other Models may be present in the SIM document if they have a relationship to the top model. For example: Parts in an Assembly, or Subtructures, etc. */
#define ads_Model (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 61))

#define ads_Model_adjusts (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 62))

#define ads_Model_designVariables (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 63))

#define ads_Model_dofsAnalysisMeshToFlatMeshMap (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 64))

#define ads_Model_elementProgressiveActivations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 65))

#define ads_Model_elementsAnalysisMeshToFlatMeshMap (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 66))

#define ads_Model_equations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 67))

#define ads_Model_eventSeries (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 68))

#define ads_Model_eventSeriesTypes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 69))

/** This is the composition of all fields that are "global" to the model. Typical uses of such fields are to capture: non-uniform distributions of things like thicknesses that may be referred to by multiple section assignments via FieldBindings fields that are mapped from previous SIM-based analysis, e.g. temperature field mapped from a heat transfer analysis to the structural model/mesh fields mapped from spreadsheets, text files or other non-SIM formats The fields aggregated here can be analytic or discrete. : Discrete fields can have any grid type, e.g. one might be a nodal field, another might be surface-facet-based, and yet another might be material point based. Consistency conditions between the grid type of a discrete model field and the grid type that a referrer expects will be captured elsewhere. Having a field in this map, by itself, has no impact on the analysis or simulation. The field has to be refered by something else to actually have an impact on the analysis. *DISTRIBUTION, *NODAL THICKNESS, *TEMPERATURE */
#define ads_Model_fields (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 70))

#define ads_Model_gBodyCollection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 71))

#define ads_Model_gCellCollection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 72))

#define ads_Model_gShapeCollection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 73))

/** Set of ImportInstances for this model. */
#define ads_Model_importInstances (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 74))

#define ads_Model_injectionLocations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 75))

#define ads_Model_linearizedMaterialProperties (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 76))

/** This is used to capture the CGM based contact corrections. */
#define ads_Model_linkedShapes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 77))

/** This is the composition of all the load cases to the model. */
#define ads_Model_loadCases (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 78))

#define ads_Model_matrixAssembles (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 79))

#define ads_Model_matrixInputs (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 80))

/** The same model may be meshed in several different ways. */
#define ads_Model_meshes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 81))

#define ads_Model_nodesAnalysisMeshToFlatMeshMap (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 82))

/** Model to nonstructural mass container. */
#define ads_Model_nonstructuralMasses (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 83))

#define ads_Model_occurrenceCollection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 84))

/** This composition represents the occurrences of a sub-component Model which are referenced by the top-level Model. In an assembly SIM document, it represents all the sub-components that explicitly impact the top level Model of the assembled Product,which means that Occurrences will be instantiated by connections or discrete regions on demand. In a flattened SIM document, it represents all the sub-components which defined the the assembled Product. */
#define ads_Model_occurrences (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 85))

#define ads_Model_orientations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 86))

#define ads_Model_parameterTableTypes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 87))

/** The same model may be meshed in several different ways. */
#define ads_Model_physicalConstants (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 88))

/** This is the composition of all the plies to the model. */
#define ads_Model_plies (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 89))

/** Model to pre-tension sections container. */
#define ads_Model_pretensionSections (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 90))

#define ads_Model_propertyTableTypes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 91))

#define ads_Model_regionActivations (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 92))

#define ads_Model_secondaryBases (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 93))

/** Section assignmnents in the model. */
#define ads_Model_sectionAssignments (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 94))

/** This is the composition of all the sections to the model. */
#define ads_Model_sections (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 95))

/** Link to the ShellSectionDetails anchor from the Model. */
#define ads_Model_shellSectionDetails (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 96))

#define ads_Model_submodels (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 97))

#define ads_Model_surfaceFinishes (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 98))

#define ads_Model_tableContainers (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 99))

/** Allows node sets to be assigned to each other */
#define ads_NodeNodeGrid (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 100))

/** This option is used to include the mass contribution from nonstructural features in the model. The nonstructural mass can be applied over an element set that contains solid, shell, membrane, surface, beam, or truss elements. */
#define ads_NonstructuralMass (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 101))

/** Element set containing the elements over which a given nonstructural mass is to be distributed. */
#define ads_NonstructuralMass_region (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 102))

/** It can be one of the following 4 values with appropriate FieldTypes. Mass magnitude of the nonstructural feature for distribution over the element set region. Mass, not weight, should be given. The FieldType for this is MASS Mass per unit volume of the nonstructural feature for application over the element set region. The FieldType for this is MASS_PER_VOL Mass per unit area of the nonstructural feature for application over the element set region. The FieldType for this is MASS_PER_AREA Mass per unit length of the nonstructural feature for application over the element set region. The FieldType for this is MASS_PER_LEN */
#define ads_NonstructuralMass_value (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 103))

/** The Occurrence Type represents occurrences of Meshed Parts or Product sub-components (aka FEMRep in V6 world) which are referenced by the top level Model exclusively. In an assembly SIM document, it represents all the sub-components that explicitly impact the top level Model of the assembled Product, which means that Occurrences will be instantiated by connections or discrete regions on demand. In a flattened SIM document, it represents all the sub-components which defined the the assembled Product. For multiply-nested assemblies, the "instantiation" of Part at the top-level assembly is represented by the occurrence. For example: if a certain meshed part is instantiated twice at the level of the subassembly, and the subassembly is itself instantiated twice at the level of the top assembly, then we say the part occurs four times in the top assembly. If theses four occurrences of the meshed part are connected together at level of the top assembly, four instances of the Occurrence record will be created in top level assembly SIM document. Besides keeping track of parts at the top-assembly level, occurrences are also be used to keep track of when any other intermediate component occurs at any other level higher than that component, under the condition that top-level document needs to refer to these sub-components to define connections or regions. */
#define ads_Occurrence (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 104))

/** A collection of occurrences. */
#define ads_OccurrenceCollection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 105))

/** This composition represents represent Elements index space mapping from the top level Model mesh to the occurrence mesh. - The fromCSet of the CMemberMap defines the CSet which members represents Elements indices from the top level Model mesh. - The toCSet of the CMemberMap defines the CSet which members represents Elements Tags (unique and stable ID) from the occurrences mesh. In a flattened assembly SIM Document this relationship is created in order to store disassembly information. In a non-flattened assembly SIM Document the "Occurrence-elementMapping" composition is not showing up. */
#define ads_Occurrence_elementMapping (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 106))

/** Links Occurences to global/flat Mesh's MeshParts. */
#define ads_Occurrence_meshParts (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 107))

/** This reference represents the occurrences of a particular sub-component Model. The same model may occur several times in an assembled Model. From the end-user point of view, there are three "types" of "entities" that can have a Mesh: the "Part", the Assembly, and the Occurrence of Assembly sub-components. The "Part" and the Assembly map to a "Model" SIM record, and the "Occurrence" maps to an Occurrence record which is refers to a Model record. Doing so, we can make sure that anything that can be done at the assembly or part level (such as aggregating Meshes and Regions) can be done at the level of the Occurrence too. In other words, "model" relationship between Occurrence and Model provides overwriting capabilities for Mesh, Properties and Regions at the occurrences level. */
#define ads_Occurrence_model (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 108))

/** This composition represents nodes index space mapping from the top level Model mesh to the occurrence mesh. - The fromCSet of the CMemberMap defines the CSet which members represents nodes indices from the top level Model mesh. - The toCSet of the CMemberMap defines the CSet which members represents nodes Tags (unique and stable ID) from the occurrences mesh. In a non-flattened assembly SIM Document the "Occurrence-nodeMapping" composition is used to describe condensed nodes generated by meshed connections. In a flattened assembly SIM Document this relationship is created in order to store disassembly information. */
#define ads_Occurrence_nodeMapping (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 109))

/** A relational distribution used to capture assembly to occurrence node equivalency mapping. The direction of the mapping is from the assembly node collection to occurrence node collection since there may be multiple assembly nodes that are equivalenced to the same occurrence node. This relationship is not used within the global flat mesh that is read by elaborators/pre - it is used as the input to mesh flattening. */
#define ads_Occurrence_nodeMappingAlt (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 110))

/** This association represents the rotational positionning of referenced sub-component occurrence within the assembled model. This is stored as a Quaternion. */
#define ads_Occurrence_rotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 111))

/** Captures the mapping between local GShapes and global GShapes for a given Occurrence. */
#define ads_Occurrence_shapeMapping (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 112))

/** This association represents the translational positionning of referenced sub-component occurrence within the assembled model. This is stored as a Vector. */
#define ads_Occurrence_translation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 113))

/** Abstract base type used to derive the Double, String, Int, Bool and Float derived types for handling parameters. */
#define ads_ParameterItem (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 114))

/** Parameter item options to capture bool data */
#define ads_ParameterItem_Bool (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 115))

/** Parameter item options to capture double data */
#define ads_ParameterItem_Double (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 116))

/** Parameter item options to capture float data */
#define ads_ParameterItem_Float (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 117))

/** Parameter item options to capture int data */
#define ads_ParameterItem_Int (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 118))

/** Parameter item options to capture string data */
#define ads_ParameterItem_String (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 119))

/** Reference to PhysicalDimension to capture the units for the parameter. This is only applicable to the ParameterItem_Double and ParameterItem_Float types. */
#define ads_ParameterItem_physicalDimension (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 120))

/** This option is used to capture parameter tables. */
#define ads_ParameterTable (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 121))

/** This option is used to capture parameter table types. It captures the individual parameters using the ParameterItems. The value of the ParameterItems can be used as the default value for the ParameterTable parameters. */
#define ads_ParameterTableType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 122))

#define ads_ParameterTableType_parameterItems (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 123))

#define ads_ParameterTable_parameterItems (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 124))

/** Reference to the ParameterTableType. The ParameterItems in the table and the table type must match. */
#define ads_ParameterTable_parameterTableType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 125))

#define ads_PeriodicSegment (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 126))

#define ads_PeriodicSurfaceDataItem (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 127))

/** The surface region on one side of the original sector. */
#define ads_PeriodicSurfaceDataItem_surface1 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 128))

/** The corresponding surface region on the other side of the original sector. */
#define ads_PeriodicSurfaceDataItem_surface2 (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 129))

/** Describes various Physical constants. */
#define ads_PhysicalConstants (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 130))

#define ads_Positioning (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 131))

#define ads_PositioningByThreePointPlane (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 132))

/** Point C coordinates. */
#define ads_PositioningByThreePointPlane_pointC (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 133))

#define ads_PositioningByTwoPointAxis (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 134))

/** Second point on the rotation exis. This is stored as a Vector. */
#define ads_PositioningByTwoPointAxis_pointA (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 135))

/** Third point. This is stored as a Vector. */
#define ads_PositioningByTwoPointAxis_pointB (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 136))

/** Origin of the rotation exis. This is stored as a Vector. */
#define ads_Positioning_origin (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 137))

/** Rotational positioning. This is stored as a Quaternion. */
#define ads_Positioning_rotation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 138))

/** Translational positioning. This is stored as a Vector. */
#define ads_Positioning_translation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 139))

/** Datatype to capture *pre-tension section. Through this user can assign a pretension node, beam or truss element used to defined the pretension section or surface used to define the pretension section in case of continuum elements. */
#define ads_PretensionSection (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 140))

/** An optional normal to the pretension section. If the normal is not specified, Abaqus/Standard will compute an average normal to the pre-tension section for continuum elements. For truss or beam elements the default normal points from the first to the last node in the element connectivity. */
#define ads_PretensionSection_direction (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 141))

/** regions[0] represents the pre-tension node and regions[1] represents the elements defining the pre-tension in case of truss or beams OR the internal surface in case of continuum elements. */
#define ads_PretensionSection_regions (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 142))

/** This option is used to capture property tables. The columns of the property table are captured through the GenericFTableType composition. The data is captured through the FTable data. */
#define ads_PropertyTable (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 143))

/** This option is used to capture property table types. The columns of the property table are captured through the GenericFTableType composition. There is no data associated with the FTable. It is only required to capture the column names. Optionally the first and only row in the FTable can be used to capture the default values. */
#define ads_PropertyTableType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 144))

#define ads_PropertyTableType_table (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 145))

/** Reference to the PhysicalDimensions of the columns in the FTable. */
#define ads_PropertyTable_physicalDimensions (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 146))

/** Reference to the PropertyTabletype. The FTable columns in the PropertyTable and PropertyTableType must match. */
#define ads_PropertyTable_propertyTableType (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 147))

#define ads_PropertyTable_table (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 148))

#define ads_RegionActivation (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 149))

#define ads_RegionActivation_region (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 150))

#define ads_RevolvingSegment (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 151))

/** This option is used to define stiffness matrix. */
#define ads_StiffnessMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 152))

/** This option is used to define structural damping matrix. */
#define ads_StructuralDampingMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 153))

/** This option is used to specify that this a submodel that will be driven by a global model. */
#define ads_Submodel (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 154))

/** Region that defines the driven boundary of the submodel. */
#define ads_Submodel_drivenRegion (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 155))

/** Data type to capture the surface finish. */
#define ads_SurfaceFinish (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 156))

/** Reference to one or more regions where the surface finish will be assigned. */
#define ads_SurfaceFinish_region (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 157))

/** The surface finish roughness value. Deprecated by new SurfaceFinishProp schema. */
#define ads_SurfaceFinish_roughness (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 158))

/** Reference to the surface for which the surface finish is assigned. Null link to Surface captures the default surface finish for the entire model. */
#define ads_SurfaceFinish_surface (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 159))

/** *Symmetric Model Generation */
#define ads_SymmetricModelGenerationInstance (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 160))

#define ads_SymmetricModelGenerationInstance_Periodic (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 161))

#define ads_SymmetricModelGenerationInstance_PeriodicVariable (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 162))

/** Sectors of the model in the circumferential direction. */
#define ads_SymmetricModelGenerationInstance_PeriodicVariable_segments (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 163))

/** Data for pairs of corresponding surfaces on each side of the original repetitive sector. */
#define ads_SymmetricModelGenerationInstance_Periodic_surfaces (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 164))

#define ads_SymmetricModelGenerationInstance_ReflectLine (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 165))

#define ads_SymmetricModelGenerationInstance_ReflectPlane (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 166))

#define ads_SymmetricModelGenerationInstance_Revolve (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 167))

/** Discretization of the model in the circumferential direction. */
#define ads_SymmetricModelGenerationInstance_Revolve_segments (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 168))

/** This option is used to capture table collections. This is the parent type that contains the property tables and parameter tables. */
#define ads_TableContainer (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 169))

#define ads_TableContainer_parameterTables (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 170))

#define ads_TableContainer_propertyTables (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 171))

#define ads_Task_activateElements (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 172))

/** This datatype captures the units of an entity along with its physical dimension. It does it by capturing the dsMagnitude for the entity. */
#define ads_UnitsRecord (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 173))

/** Reference to PhysicalDimension. */
#define ads_UnitsRecord_physicalDimension (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 174))

/** This option is used to define viscuous damping matrix. */
#define ads_ViscousDampingMatrix (ads_CoreFragmentTypeIndex(ads_CoreModelFragment, 175))

/** 
Enum with record members. */
enum ads_ActivateElementsMembersEnm
{
    ads_ActivateElements_eigenStrainTimeConst,
    ads_ActivateElements_expansionTimeConst
};

/** Enum with association roles. */
enum ads_ActivateElements_elementProgressiveActivationRolesEnm
{
    ads_ActivateElements_elementProgressiveActivation_referent,
    ads_ActivateElements_elementProgressiveActivation_referrer
};

/** Enum with association roles. */
enum ads_ActivateElements_tableContainerRolesEnm
{
    ads_ActivateElements_tableContainer_referent,
    ads_ActivateElements_tableContainer_referrer
};

/** 
Enum with association roles. */
enum ads_AdjustNodeCoordinates_distributionRolesEnm
{
    ads_AdjustNodeCoordinates_distribution_referent,
    ads_AdjustNodeCoordinates_distribution_referrer
};

/** 
Enum with association roles. */
enum ads_AdjustNodeCoordinates_nodeSetRolesEnm
{
    ads_AdjustNodeCoordinates_nodeSet_referent,
    ads_AdjustNodeCoordinates_nodeSet_referrer
};

/** 
Enum with association roles. */
enum ads_AdjustNodeCoordinates_orientationRolesEnm
{
    ads_AdjustNodeCoordinates_orientation_referent,
    ads_AdjustNodeCoordinates_orientation_referrer
};

/** 
Enum with association roles. */
enum ads_AdjustNodeCoordinates_surfaceRolesEnm
{
    ads_AdjustNodeCoordinates_surface_referent,
    ads_AdjustNodeCoordinates_surface_referrer
};

/** 
Enum with association roles. */
enum ads_DesignVariable_distributionRolesEnm
{
    ads_DesignVariable_distribution_referent,
    ads_DesignVariable_distribution_referrer
};

/** Enum with grid dimensions. */
enum ads_ElementOccurrenceGridDimensionsEnm
{
    ads_ElementOccurrenceGrid_element,
    ads_ElementOccurrenceGrid_occurrence
};

/** 
Enum with record members. */
enum ads_ElementProgressiveActivationMembersEnm
{
    ads_ElementProgressiveActivation_allowOverlap,
    ads_ElementProgressiveActivation_followDeformation,
    ads_ElementProgressiveActivation_freeSurface
};

enum ads_ElementProgressiveActivation_freeSurfaceEnm
{
    ads_ElementProgressiveActivation_freeSurface_FACET,
    ads_ElementProgressiveActivation_freeSurface_NONE,
    ads_ElementProgressiveActivation_freeSurface_NORMAL
};

/** Enum with association roles. */
enum ads_ElementProgressiveActivation_regionRolesEnm
{
    ads_ElementProgressiveActivation_region_referent,
    ads_ElementProgressiveActivation_region_referrer
};

/** 
Enum with record members. */
enum ads_Equation6TermsMembersEnm
{
    ads_Equation6Terms_coefficients
};

/** Enum with association roles. */
enum ads_Equation_coefficientsRolesEnm
{
    ads_Equation_coefficients_child,
    ads_Equation_coefficients_parent
};

/** Enum with association roles. */
enum ads_Equation_nodalDofSetsRolesEnm
{
    ads_Equation_nodalDofSets_child,
    ads_Equation_nodalDofSets_parent
};

/** Enum with association roles. */
enum ads_Equation_nodalDofTableRolesEnm
{
    ads_Equation_nodalDofTable_child,
    ads_Equation_nodalDofTable_parent
};

/** 
Enum with record members. */
enum ads_EventSeriesMembersEnm
{
    ads_EventSeries_sourceName,
    ads_EventSeries_time,
    ads_EventSeries_timeShift
};

enum ads_EventSeries_timeEnm
{
    ads_EventSeries_time_STEP,
    ads_EventSeries_time_TOTAL
};

/** 
Enum with record members. */
enum ads_EventSeriesTypeMembersEnm
{
    ads_EventSeriesType_location
};

enum ads_EventSeriesType_locationEnm
{
    ads_EventSeriesType_location_GLOBAL,
    ads_EventSeriesType_location_LOCAL
};

/** Enum with association roles. */
enum ads_EventSeriesType_tableRolesEnm
{
    ads_EventSeriesType_table_child,
    ads_EventSeriesType_table_parent
};

/** 
Enum with association roles. */
enum ads_EventSeries_centerOfRotationRolesEnm
{
    ads_EventSeries_centerOfRotation_child,
    ads_EventSeries_centerOfRotation_parent
};

/** 
Enum with association roles. */
enum ads_EventSeries_eventSeriesTypeRolesEnm
{
    ads_EventSeries_eventSeriesType_referent,
    ads_EventSeries_eventSeriesType_referrer
};

/** 
Enum with association roles. */
enum ads_EventSeries_externalSourceRolesEnm
{
    ads_EventSeries_externalSource_referent,
    ads_EventSeries_externalSource_referrer
};

/** 
Enum with association roles. */
enum ads_EventSeries_physicalDimensionsRolesEnm
{
    ads_EventSeries_physicalDimensions_referent,
    ads_EventSeries_physicalDimensions_referrer
};

/** 
Enum with association roles. */
enum ads_EventSeries_rotationRolesEnm
{
    ads_EventSeries_rotation_child,
    ads_EventSeries_rotation_parent
};

/** Enum with association roles. */
enum ads_EventSeries_tableRolesEnm
{
    ads_EventSeries_table_child,
    ads_EventSeries_table_parent
};

/** 
Enum with association roles. */
enum ads_EventSeries_translationRolesEnm
{
    ads_EventSeries_translation_child,
    ads_EventSeries_translation_parent
};

/** Enum with association roles. */
enum ads_EventSeries_unitsRecordsRolesEnm
{
    ads_EventSeries_unitsRecords_child,
    ads_EventSeries_unitsRecords_parent
};

/** 
Enum with association roles. */
enum ads_Focus_modelRolesEnm
{
    ads_Focus_model_child,
    ads_Focus_model_parent
};

/** 
Enum with association roles. */
enum ads_Focus_occurrenceModelsRolesEnm
{
    ads_Focus_occurrenceModels_child,
    ads_Focus_occurrenceModels_parent
};

/** Enum with record members. */
enum ads_ImportInstanceMembersEnm
{
    ads_ImportInstance_elementLabelOffset,
    ads_ImportInstance_nodeLabelOffset,
    ads_ImportInstance_resetConfiguration,
    ads_ImportInstance_state
};

/** 
Enum with association roles. */
enum ads_ImportInstance_externalSourceRolesEnm
{
    ads_ImportInstance_externalSource_referent,
    ads_ImportInstance_externalSource_referrer
};

/** 
Enum with association roles. */
enum ads_ImportInstance_positioningRolesEnm
{
    ads_ImportInstance_positioning_child,
    ads_ImportInstance_positioning_parent
};

/** 
Enum with association roles. */
enum ads_ImportInstance_referenceCoordinatesRolesEnm
{
    ads_ImportInstance_referenceCoordinates_child,
    ads_ImportInstance_referenceCoordinates_parent
};

/** 
Enum with association roles. */
enum ads_ImportInstance_regionsRolesEnm
{
    ads_ImportInstance_regions_referent,
    ads_ImportInstance_regions_referrer
};

/** Enum with association roles. */
enum ads_ImportInstance_stepIncRolesEnm
{
    ads_ImportInstance_stepInc_child,
    ads_ImportInstance_stepInc_parent
};

/** 
Enum with association roles. */
enum ads_LinkedShapes_rotationsRolesEnm
{
    ads_LinkedShapes_rotations_child,
    ads_LinkedShapes_rotations_parent
};

/** 
Enum with association roles. */
enum ads_LinkedShapes_shape2auxidRolesEnm
{
    ads_LinkedShapes_shape2auxid_child,
    ads_LinkedShapes_shape2auxid_parent
};

/** 
Enum with association roles. */
enum ads_LinkedShapes_translationsRolesEnm
{
    ads_LinkedShapes_translations_child,
    ads_LinkedShapes_translations_parent
};

/** 
Enum with record members. */
enum ads_MassMatrixMembersEnm
{
    ads_MassMatrix_scaleFactor
};

/** 
Enum with association roles. */
enum ads_MatrixAssemble_centerOfRotationRolesEnm
{
    ads_MatrixAssemble_centerOfRotation_child,
    ads_MatrixAssemble_centerOfRotation_parent
};

/** Enum with association roles. */
enum ads_MatrixAssemble_massMatrixRolesEnm
{
    ads_MatrixAssemble_massMatrix_referent,
    ads_MatrixAssemble_massMatrix_referrer
};

/** 
Enum with association roles. */
enum ads_MatrixAssemble_rotationRolesEnm
{
    ads_MatrixAssemble_rotation_child,
    ads_MatrixAssemble_rotation_parent
};

/** Enum with association roles. */
enum ads_MatrixAssemble_stiffnessMatrixRolesEnm
{
    ads_MatrixAssemble_stiffnessMatrix_referent,
    ads_MatrixAssemble_stiffnessMatrix_referrer
};

/** Enum with association roles. */
enum ads_MatrixAssemble_structuralDampingMatrixRolesEnm
{
    ads_MatrixAssemble_structuralDampingMatrix_referent,
    ads_MatrixAssemble_structuralDampingMatrix_referrer
};

/** 
Enum with association roles. */
enum ads_MatrixAssemble_translationRolesEnm
{
    ads_MatrixAssemble_translation_child,
    ads_MatrixAssemble_translation_parent
};

/** Enum with association roles. */
enum ads_MatrixAssemble_viscousDampingMatrixRolesEnm
{
    ads_MatrixAssemble_viscousDampingMatrix_referent,
    ads_MatrixAssemble_viscousDampingMatrix_referrer
};

/** 
Enum with record members. */
enum ads_MatrixInputMembersEnm
{
    ads_MatrixInput_scaleFactor
};

/** 
Enum with record members. */
enum ads_MatrixInputDataMembersEnm
{
    ads_MatrixInputData_type
};

enum ads_MatrixInputData_typeEnm
{
    ads_MatrixInputData_type_SYMMETRIC,
    ads_MatrixInputData_type_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_MatrixInputData_dimDOF1RolesEnm
{
    ads_MatrixInputData_dimDOF1_child,
    ads_MatrixInputData_dimDOF1_parent
};

/** 
Enum with association roles. */
enum ads_MatrixInputData_dimDOF2RolesEnm
{
    ads_MatrixInputData_dimDOF2_child,
    ads_MatrixInputData_dimDOF2_parent
};

/** 
Enum with association roles. */
enum ads_MatrixInputData_dimNode1RolesEnm
{
    ads_MatrixInputData_dimNode1_child,
    ads_MatrixInputData_dimNode1_parent
};

/** 
Enum with association roles. */
enum ads_MatrixInputData_dimNode2RolesEnm
{
    ads_MatrixInputData_dimNode2_child,
    ads_MatrixInputData_dimNode2_parent
};

/** 
Enum with association roles. */
enum ads_MatrixInputData_matrixValuesRolesEnm
{
    ads_MatrixInputData_matrixValues_child,
    ads_MatrixInputData_matrixValues_parent
};

/** 
Enum with association roles. */
enum ads_MatrixInput_dataRolesEnm
{
    ads_MatrixInput_data_child,
    ads_MatrixInput_data_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_substructureOccurrencesRolesEnm
{
    ads_Mesh_substructureOccurrences_child,
    ads_Mesh_substructureOccurrences_parent
};

/** 
Enum with record members. */
enum ads_ModelMembersEnm
{
    ads_Model_alias,
    ads_Model_externalID
};

/** Enum with association roles. */
enum ads_Model_adjustsRolesEnm
{
    ads_Model_adjusts_child,
    ads_Model_adjusts_parent
};

/** Enum with association roles. */
enum ads_Model_designVariablesRolesEnm
{
    ads_Model_designVariables_child,
    ads_Model_designVariables_parent
};

/** Enum with association roles. */
enum ads_Model_dofsAnalysisMeshToFlatMeshMapRolesEnm
{
    ads_Model_dofsAnalysisMeshToFlatMeshMap_child,
    ads_Model_dofsAnalysisMeshToFlatMeshMap_parent
};

/** Enum with association roles. */
enum ads_Model_elementProgressiveActivationsRolesEnm
{
    ads_Model_elementProgressiveActivations_child,
    ads_Model_elementProgressiveActivations_parent
};

/** Enum with association roles. */
enum ads_Model_elementsAnalysisMeshToFlatMeshMapRolesEnm
{
    ads_Model_elementsAnalysisMeshToFlatMeshMap_child,
    ads_Model_elementsAnalysisMeshToFlatMeshMap_parent
};

/** Enum with association roles. */
enum ads_Model_equationsRolesEnm
{
    ads_Model_equations_child,
    ads_Model_equations_parent
};

/** Enum with association roles. */
enum ads_Model_eventSeriesRolesEnm
{
    ads_Model_eventSeries_child,
    ads_Model_eventSeries_parent
};

/** Enum with association roles. */
enum ads_Model_eventSeriesTypesRolesEnm
{
    ads_Model_eventSeriesTypes_child,
    ads_Model_eventSeriesTypes_parent
};

/** 
Enum with association roles. */
enum ads_Model_fieldsRolesEnm
{
    ads_Model_fields_child,
    ads_Model_fields_parent
};

/** Enum with association roles. */
enum ads_Model_gBodyCollectionRolesEnm
{
    ads_Model_gBodyCollection_child,
    ads_Model_gBodyCollection_parent
};

/** Enum with association roles. */
enum ads_Model_gCellCollectionRolesEnm
{
    ads_Model_gCellCollection_child,
    ads_Model_gCellCollection_parent
};

/** Enum with association roles. */
enum ads_Model_gShapeCollectionRolesEnm
{
    ads_Model_gShapeCollection_child,
    ads_Model_gShapeCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_importInstancesRolesEnm
{
    ads_Model_importInstances_child,
    ads_Model_importInstances_parent
};

/** Enum with association roles. */
enum ads_Model_injectionLocationsRolesEnm
{
    ads_Model_injectionLocations_child,
    ads_Model_injectionLocations_parent
};

/** Enum with association roles. */
enum ads_Model_linearizedMaterialPropertiesRolesEnm
{
    ads_Model_linearizedMaterialProperties_child,
    ads_Model_linearizedMaterialProperties_parent
};

/** 
Enum with association roles. */
enum ads_Model_linkedShapesRolesEnm
{
    ads_Model_linkedShapes_child,
    ads_Model_linkedShapes_parent
};

/** 
Enum with association roles. */
enum ads_Model_loadCasesRolesEnm
{
    ads_Model_loadCases_child,
    ads_Model_loadCases_parent
};

/** Enum with association roles. */
enum ads_Model_matrixAssemblesRolesEnm
{
    ads_Model_matrixAssembles_child,
    ads_Model_matrixAssembles_parent
};

/** Enum with association roles. */
enum ads_Model_matrixInputsRolesEnm
{
    ads_Model_matrixInputs_child,
    ads_Model_matrixInputs_parent
};

/** 
Enum with association roles. */
enum ads_Model_meshesRolesEnm
{
    ads_Model_meshes_child,
    ads_Model_meshes_parent
};

/** Enum with association roles. */
enum ads_Model_nodesAnalysisMeshToFlatMeshMapRolesEnm
{
    ads_Model_nodesAnalysisMeshToFlatMeshMap_child,
    ads_Model_nodesAnalysisMeshToFlatMeshMap_parent
};

/** 
Enum with association roles. */
enum ads_Model_nonstructuralMassesRolesEnm
{
    ads_Model_nonstructuralMasses_child,
    ads_Model_nonstructuralMasses_parent
};

/** Enum with association roles. */
enum ads_Model_occurrenceCollectionRolesEnm
{
    ads_Model_occurrenceCollection_child,
    ads_Model_occurrenceCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_occurrencesRolesEnm
{
    ads_Model_occurrences_child,
    ads_Model_occurrences_parent
};

/** Enum with association roles. */
enum ads_Model_orientationsRolesEnm
{
    ads_Model_orientations_child,
    ads_Model_orientations_parent
};

/** Enum with association roles. */
enum ads_Model_parameterTableTypesRolesEnm
{
    ads_Model_parameterTableTypes_child,
    ads_Model_parameterTableTypes_parent
};

/** 
Enum with association roles. */
enum ads_Model_physicalConstantsRolesEnm
{
    ads_Model_physicalConstants_child,
    ads_Model_physicalConstants_parent
};

/** 
Enum with association roles. */
enum ads_Model_pliesRolesEnm
{
    ads_Model_plies_child,
    ads_Model_plies_parent
};

/** 
Enum with association roles. */
enum ads_Model_pretensionSectionsRolesEnm
{
    ads_Model_pretensionSections_child,
    ads_Model_pretensionSections_parent
};

/** Enum with association roles. */
enum ads_Model_propertyTableTypesRolesEnm
{
    ads_Model_propertyTableTypes_child,
    ads_Model_propertyTableTypes_parent
};

/** Enum with association roles. */
enum ads_Model_regionActivationsRolesEnm
{
    ads_Model_regionActivations_child,
    ads_Model_regionActivations_parent
};

/** Enum with association roles. */
enum ads_Model_secondaryBasesRolesEnm
{
    ads_Model_secondaryBases_child,
    ads_Model_secondaryBases_parent
};

/** 
Enum with association roles. */
enum ads_Model_sectionAssignmentsRolesEnm
{
    ads_Model_sectionAssignments_child,
    ads_Model_sectionAssignments_parent
};

/** 
Enum with association roles. */
enum ads_Model_sectionsRolesEnm
{
    ads_Model_sections_child,
    ads_Model_sections_parent
};

/** 
Enum with association roles. */
enum ads_Model_shellSectionDetailsRolesEnm
{
    ads_Model_shellSectionDetails_child,
    ads_Model_shellSectionDetails_parent
};

/** Enum with association roles. */
enum ads_Model_submodelsRolesEnm
{
    ads_Model_submodels_child,
    ads_Model_submodels_parent
};

/** Enum with association roles. */
enum ads_Model_surfaceFinishesRolesEnm
{
    ads_Model_surfaceFinishes_child,
    ads_Model_surfaceFinishes_parent
};

/** Enum with association roles. */
enum ads_Model_tableContainersRolesEnm
{
    ads_Model_tableContainers_child,
    ads_Model_tableContainers_parent
};

/** 
Enum with grid dimensions. */
enum ads_NodeNodeGridDimensionsEnm
{
    ads_NodeNodeGrid_column,
    ads_NodeNodeGrid_row
};

/** 
Enum with record members. */
enum ads_NonstructuralMassMembersEnm
{
    ads_NonstructuralMass_distributionProportionality
};

enum ads_NonstructuralMass_distributionProportionalityEnm
{
    ads_NonstructuralMass_distributionProportionality_MASS_PROPORTIONAL,
    ads_NonstructuralMass_distributionProportionality_NULL,
    ads_NonstructuralMass_distributionProportionality_VOLUME_PROPORTIONAL
};

/** 
Enum with association roles. */
enum ads_NonstructuralMass_regionRolesEnm
{
    ads_NonstructuralMass_region_referent,
    ads_NonstructuralMass_region_referrer
};

/** 
Enum with association roles. */
enum ads_NonstructuralMass_valueRolesEnm
{
    ads_NonstructuralMass_value_child,
    ads_NonstructuralMass_value_parent
};

/** 
Enum with record members. */
enum ads_OccurrenceMembersEnm
{
    ads_Occurrence_elementLabelOffset,
    ads_Occurrence_nodeLabelOffset
};

/** 
Enum with association roles. */
enum ads_Occurrence_elementMappingRolesEnm
{
    ads_Occurrence_elementMapping_child,
    ads_Occurrence_elementMapping_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_meshPartsRolesEnm
{
    ads_Occurrence_meshParts_referent,
    ads_Occurrence_meshParts_referrer
};

/** 
Enum with association roles. */
enum ads_Occurrence_modelRolesEnm
{
    ads_Occurrence_model_referent,
    ads_Occurrence_model_referrer
};

/** 
Enum with association roles. */
enum ads_Occurrence_nodeMappingRolesEnm
{
    ads_Occurrence_nodeMapping_child,
    ads_Occurrence_nodeMapping_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_nodeMappingAltRolesEnm
{
    ads_Occurrence_nodeMappingAlt_child,
    ads_Occurrence_nodeMappingAlt_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_rotationRolesEnm
{
    ads_Occurrence_rotation_child,
    ads_Occurrence_rotation_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_shapeMappingRolesEnm
{
    ads_Occurrence_shapeMapping_child,
    ads_Occurrence_shapeMapping_parent
};

/** 
Enum with association roles. */
enum ads_Occurrence_translationRolesEnm
{
    ads_Occurrence_translation_child,
    ads_Occurrence_translation_parent
};

/** 
Enum with record members. */
enum ads_ParameterItemMembersEnm
{
    ads_ParameterItem_description,
    ads_ParameterItem_name
};

/** 
Enum with record members. */
enum ads_ParameterItem_BoolMembersEnm
{
    ads_ParameterItem_Bool_description,
    ads_ParameterItem_Bool_name,
    ads_ParameterItem_Bool_value
};

/** 
Enum with record members. */
enum ads_ParameterItem_DoubleMembersEnm
{
    ads_ParameterItem_Double_description,
    ads_ParameterItem_Double_name,
    ads_ParameterItem_Double_value
};

/** 
Enum with record members. */
enum ads_ParameterItem_FloatMembersEnm
{
    ads_ParameterItem_Float_description,
    ads_ParameterItem_Float_name,
    ads_ParameterItem_Float_value
};

/** 
Enum with record members. */
enum ads_ParameterItem_IntMembersEnm
{
    ads_ParameterItem_Int_description,
    ads_ParameterItem_Int_name,
    ads_ParameterItem_Int_value
};

/** 
Enum with record members. */
enum ads_ParameterItem_StringMembersEnm
{
    ads_ParameterItem_String_description,
    ads_ParameterItem_String_name,
    ads_ParameterItem_String_value
};

/** 
Enum with association roles. */
enum ads_ParameterItem_physicalDimensionRolesEnm
{
    ads_ParameterItem_physicalDimension_referent,
    ads_ParameterItem_physicalDimension_referrer
};

/** 
Enum with record members. */
enum ads_ParameterTableMembersEnm
{
    ads_ParameterTable_name
};

/** Enum with association roles. */
enum ads_ParameterTableType_parameterItemsRolesEnm
{
    ads_ParameterTableType_parameterItems_child,
    ads_ParameterTableType_parameterItems_parent
};

/** Enum with association roles. */
enum ads_ParameterTable_parameterItemsRolesEnm
{
    ads_ParameterTable_parameterItems_child,
    ads_ParameterTable_parameterItems_parent
};

/** 
Enum with association roles. */
enum ads_ParameterTable_parameterTableTypeRolesEnm
{
    ads_ParameterTable_parameterTableType_referent,
    ads_ParameterTable_parameterTableType_referrer
};

/** Enum with record members. */
enum ads_PeriodicSegmentMembersEnm
{
    ads_PeriodicSegment_numSectors,
    ads_PeriodicSegment_scaling
};

/** Enum with record members. */
enum ads_PeriodicSurfaceDataItemMembersEnm
{
    ads_PeriodicSurfaceDataItem_node2Surface,
    ads_PeriodicSurfaceDataItem_surface1Name,
    ads_PeriodicSurfaceDataItem_surface2Name,
    ads_PeriodicSurfaceDataItem_tolerance
};

/** 
Enum with association roles. */
enum ads_PeriodicSurfaceDataItem_surface1RolesEnm
{
    ads_PeriodicSurfaceDataItem_surface1_referent,
    ads_PeriodicSurfaceDataItem_surface1_referrer
};

/** 
Enum with association roles. */
enum ads_PeriodicSurfaceDataItem_surface2RolesEnm
{
    ads_PeriodicSurfaceDataItem_surface2_referent,
    ads_PeriodicSurfaceDataItem_surface2_referrer
};

/** 
Enum with record members. */
enum ads_PhysicalConstantsMembersEnm
{
    ads_PhysicalConstants_absoluteZero,
    ads_PhysicalConstants_avogadroNumber,
    ads_PhysicalConstants_boltzmannConstant,
    ads_PhysicalConstants_elementaryCharge,
    ads_PhysicalConstants_faradayConstant,
    ads_PhysicalConstants_splReferencePressure,
    ads_PhysicalConstants_stefanBoltzmann,
    ads_PhysicalConstants_universalGasConstant
};

/** 
Enum with association roles. */
enum ads_PositioningByThreePointPlane_pointCRolesEnm
{
    ads_PositioningByThreePointPlane_pointC_child,
    ads_PositioningByThreePointPlane_pointC_parent
};

/** 
Enum with association roles. */
enum ads_PositioningByTwoPointAxis_pointARolesEnm
{
    ads_PositioningByTwoPointAxis_pointA_child,
    ads_PositioningByTwoPointAxis_pointA_parent
};

/** 
Enum with association roles. */
enum ads_PositioningByTwoPointAxis_pointBRolesEnm
{
    ads_PositioningByTwoPointAxis_pointB_child,
    ads_PositioningByTwoPointAxis_pointB_parent
};

/** 
Enum with association roles. */
enum ads_Positioning_originRolesEnm
{
    ads_Positioning_origin_child,
    ads_Positioning_origin_parent
};

/** 
Enum with association roles. */
enum ads_Positioning_rotationRolesEnm
{
    ads_Positioning_rotation_child,
    ads_Positioning_rotation_parent
};

/** 
Enum with association roles. */
enum ads_Positioning_translationRolesEnm
{
    ads_Positioning_translation_child,
    ads_Positioning_translation_parent
};

/** 
Enum with association roles. */
enum ads_PretensionSection_directionRolesEnm
{
    ads_PretensionSection_direction_child,
    ads_PretensionSection_direction_parent
};

/** 
Enum with association roles. */
enum ads_PretensionSection_regionsRolesEnm
{
    ads_PretensionSection_regions_referent,
    ads_PretensionSection_regions_referrer
};

/** 
Enum with record members. */
enum ads_PropertyTableMembersEnm
{
    ads_PropertyTable_extrapolation,
    ads_PropertyTable_name,
    ads_PropertyTable_regularize,
    ads_PropertyTable_rtol
};

enum ads_PropertyTable_extrapolationEnm
{
    ads_PropertyTable_extrapolation_CONSTANT,
    ads_PropertyTable_extrapolation_LINEAR
};

enum ads_PropertyTable_regularizeEnm
{
    ads_PropertyTable_regularize_OFF,
    ads_PropertyTable_regularize_ON,
    ads_PropertyTable_regularize_ORIGIN
};

/** 
Enum with record members. */
enum ads_PropertyTableTypeMembersEnm
{
    ads_PropertyTableType_numberIndependentVariables
};

/** Enum with association roles. */
enum ads_PropertyTableType_tableRolesEnm
{
    ads_PropertyTableType_table_child,
    ads_PropertyTableType_table_parent
};

/** 
Enum with association roles. */
enum ads_PropertyTable_physicalDimensionsRolesEnm
{
    ads_PropertyTable_physicalDimensions_referent,
    ads_PropertyTable_physicalDimensions_referrer
};

/** 
Enum with association roles. */
enum ads_PropertyTable_propertyTableTypeRolesEnm
{
    ads_PropertyTable_propertyTableType_referent,
    ads_PropertyTable_propertyTableType_referrer
};

/** Enum with association roles. */
enum ads_PropertyTable_tableRolesEnm
{
    ads_PropertyTable_table_child,
    ads_PropertyTable_table_parent
};

/** Enum with association roles. */
enum ads_RegionActivation_regionRolesEnm
{
    ads_RegionActivation_region_referent,
    ads_RegionActivation_region_referrer
};

/** Enum with record members. */
enum ads_RevolvingSegmentMembersEnm
{
    ads_RevolvingSegment_angle,
    ads_RevolvingSegment_biasRatio,
    ads_RevolvingSegment_cylindricalElements,
    ads_RevolvingSegment_numSubdivisions
};

/** 
Enum with record members. */
enum ads_StiffnessMatrixMembersEnm
{
    ads_StiffnessMatrix_scaleFactor
};

/** 
Enum with record members. */
enum ads_StructuralDampingMatrixMembersEnm
{
    ads_StructuralDampingMatrix_scaleFactor
};

/** 
Enum with record members. */
enum ads_SubmodelMembersEnm
{
    ads_Submodel_globalElset,
    ads_Submodel_type
};

enum ads_Submodel_typeEnm
{
    ads_Submodel_type_NODE,
    ads_Submodel_type_SURFACE
};

/** 
Enum with association roles. */
enum ads_Submodel_drivenRegionRolesEnm
{
    ads_Submodel_drivenRegion_referent,
    ads_Submodel_drivenRegion_referrer
};

/** 
Enum with association roles. */
enum ads_SurfaceFinish_regionRolesEnm
{
    ads_SurfaceFinish_region_referent,
    ads_SurfaceFinish_region_referrer
};

/** 
Enum with association roles. */
enum ads_SurfaceFinish_roughnessRolesEnm
{
    ads_SurfaceFinish_roughness_child,
    ads_SurfaceFinish_roughness_parent
};

/** 
Enum with association roles. */
enum ads_SurfaceFinish_surfaceRolesEnm
{
    ads_SurfaceFinish_surface_referent,
    ads_SurfaceFinish_surface_referrer
};

/** 
Enum with record members. */
enum ads_SymmetricModelGenerationInstanceMembersEnm
{
    ads_SymmetricModelGenerationInstance_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_resetConfiguration,
    ads_SymmetricModelGenerationInstance_state,
    ads_SymmetricModelGenerationInstance_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_tolerance
};

/** Enum with record members. */
enum ads_SymmetricModelGenerationInstance_PeriodicMembersEnm
{
    ads_SymmetricModelGenerationInstance_Periodic_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_Periodic_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_Periodic_resetConfiguration,
    ads_SymmetricModelGenerationInstance_Periodic_state,
    ads_SymmetricModelGenerationInstance_Periodic_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_Periodic_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_Periodic_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_Periodic_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_Periodic_tolerance,
    ads_SymmetricModelGenerationInstance_Periodic_angle,
    ads_SymmetricModelGenerationInstance_Periodic_numSectors
};

/** Enum with record members. */
enum ads_SymmetricModelGenerationInstance_PeriodicVariableMembersEnm
{
    ads_SymmetricModelGenerationInstance_PeriodicVariable_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_resetConfiguration,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_state,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_tolerance,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_angle,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_numSectors
};

/** 
Enum with association roles. */
enum ads_SymmetricModelGenerationInstance_PeriodicVariable_segmentsRolesEnm
{
    ads_SymmetricModelGenerationInstance_PeriodicVariable_segments_child,
    ads_SymmetricModelGenerationInstance_PeriodicVariable_segments_parent
};

/** 
Enum with association roles. */
enum ads_SymmetricModelGenerationInstance_Periodic_surfacesRolesEnm
{
    ads_SymmetricModelGenerationInstance_Periodic_surfaces_child,
    ads_SymmetricModelGenerationInstance_Periodic_surfaces_parent
};

/** Enum with record members. */
enum ads_SymmetricModelGenerationInstance_ReflectLineMembersEnm
{
    ads_SymmetricModelGenerationInstance_ReflectLine_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_ReflectLine_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_ReflectLine_resetConfiguration,
    ads_SymmetricModelGenerationInstance_ReflectLine_state,
    ads_SymmetricModelGenerationInstance_ReflectLine_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_ReflectLine_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_ReflectLine_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_ReflectLine_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_ReflectLine_tolerance
};

/** Enum with record members. */
enum ads_SymmetricModelGenerationInstance_ReflectPlaneMembersEnm
{
    ads_SymmetricModelGenerationInstance_ReflectPlane_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_ReflectPlane_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_ReflectPlane_resetConfiguration,
    ads_SymmetricModelGenerationInstance_ReflectPlane_state,
    ads_SymmetricModelGenerationInstance_ReflectPlane_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_ReflectPlane_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_ReflectPlane_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_ReflectPlane_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_ReflectPlane_tolerance
};

/** Enum with record members. */
enum ads_SymmetricModelGenerationInstance_RevolveMembersEnm
{
    ads_SymmetricModelGenerationInstance_Revolve_elementLabelOffset,
    ads_SymmetricModelGenerationInstance_Revolve_nodeLabelOffset,
    ads_SymmetricModelGenerationInstance_Revolve_resetConfiguration,
    ads_SymmetricModelGenerationInstance_Revolve_state,
    ads_SymmetricModelGenerationInstance_Revolve_numOfStreamlineElements,
    ads_SymmetricModelGenerationInstance_Revolve_numOfStreamlineNodes,
    ads_SymmetricModelGenerationInstance_Revolve_streamlineElementOffset,
    ads_SymmetricModelGenerationInstance_Revolve_streamlineNodeOffset,
    ads_SymmetricModelGenerationInstance_Revolve_tolerance
};

/** 
Enum with association roles. */
enum ads_SymmetricModelGenerationInstance_Revolve_segmentsRolesEnm
{
    ads_SymmetricModelGenerationInstance_Revolve_segments_child,
    ads_SymmetricModelGenerationInstance_Revolve_segments_parent
};

/** Enum with association roles. */
enum ads_TableContainer_parameterTablesRolesEnm
{
    ads_TableContainer_parameterTables_child,
    ads_TableContainer_parameterTables_parent
};

/** Enum with association roles. */
enum ads_TableContainer_propertyTablesRolesEnm
{
    ads_TableContainer_propertyTables_child,
    ads_TableContainer_propertyTables_parent
};

/** Enum with association roles. */
enum ads_Task_activateElementsRolesEnm
{
    ads_Task_activateElements_child,
    ads_Task_activateElements_parent
};

/** 
Enum with record members. */
enum ads_UnitsRecordMembersEnm
{
    ads_UnitsRecord_dsMagnitude
};

/** 
Enum with association roles. */
enum ads_UnitsRecord_physicalDimensionRolesEnm
{
    ads_UnitsRecord_physicalDimension_referent,
    ads_UnitsRecord_physicalDimension_referrer
};

/** 
Enum with record members. */
enum ads_ViscousDampingMatrixMembersEnm
{
    ads_ViscousDampingMatrix_scaleFactor
};

#endif
