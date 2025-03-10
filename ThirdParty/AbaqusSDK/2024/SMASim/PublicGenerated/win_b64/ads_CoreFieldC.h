//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFieldC_h
#define ads_CoreFieldC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Field of the latest level of form Core */

/** Gives values of arcLength c-members */
#define ads_ArcLengthDescription (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 0))

/** Link from ArcLengthDescription to the arcLength collection being described. */
#define ads_ArcLengthDescription_arcLengthCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 1))

/** Link from ArcLengthDescription to distribution giving values. The grid of this distribution is the arcLength collection being described. */
#define ads_ArcLengthDescription_data (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 2))

/** The instances of the ComplexNumberPart type indicate the real and imaginary parts of a Complex number or, alternatively, represent a complex number in magnitude-phase form. */
#define ads_ComplexNumberPart (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 3))

#define ads_ComplexNumberPartCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 4))

/** Instances of the component type represent scalar/tensor/vector components. */
#define ads_Component (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 5))

#define ads_ComponentCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 6))

/** For distributions that are basically a single vector or a tensor, for example. */
#define ads_ComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 7))

/** Grid representing section variables which do not vary through the thickness */
#define ads_ElementIntegrationStationGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 8))

/** Grid representing variables at integration stations that can have different values from point to point through the thickness of the element */
#define ads_ElementIntegrationStationLayerLayerPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 9))

/** Grid representing variables at integration stations that can have different values from point to point through the thickness of the element */
#define ads_ElementIntegrationStationLayerSectionPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 10))

/** Grid representing variables at integration stations that can have different values from point to point through the thickness of the element */
#define ads_ElementIntegrationStationSectionPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 11))

/** Elements of this grid are layers on finite elements. All major geometrical components of elements must share the same layup. D-sets defined on this grid will specify mesh regions. */
#define ads_ElementLayerGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 12))

#define ads_ElementPointSectionPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 13))

/** Element x SectionPoint x Component */
#define ads_ElementSectionPointComponentGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 14))

#define ads_ElementSectionPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 15))

/** In Mathematics, a Field is any set of elements that satisfies the field axioms for both addition and multiplication, and is a commutative-division algebra. For example: the Reals Set is a Field. In Physics, a Field is generally thought of as a quantity that varies in space and time. The Field function may not vary in space (uniform) or in time (constant), but space and time is still part of its domain. The values of a physical field are elements in one of the following mathematical fields: Real, Complex, Vector, or Tensor. In SIM, the Field concept is the generalization of the two concepts above: a specific SIM Field may be a function of space, or it may be simply a real, complex, vector, or tensor value with no function behind it. The SIM Field further extends the concept by allowing the values to be prescribed as a function of other fields (table look-up). SIM Fields can also be built as operations on other fields. Only certain pre-defined operations are supported so far (a generic expression is not supported.) The various types of Fields described above (with no function, with a discrete function of space, with a function of other fields, etc.) are represented by different record types derived from this base record type. */
#define ads_Field (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 16))

/** A field expression. */
#define ads_FieldExpression (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 17))

/** No columns are mandatory when defining a Field_FTable. */
#define ads_FieldFTableType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 18))

/** Field mapper control definition. */
#define ads_FieldMapperControls (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 19))

/** Values assigned to the tensor at orphan locations. */
#define ads_FieldMapperControls_defaultValues (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 20))

/** Search tolerance radius for the nearest neighbor algorithm. */
#define ads_FieldMapperControls_nearestNeighborRadius (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 21))

/** Field operations definition. */
#define ads_FieldOperations (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 22))

/** Component of a vector or tensor field value to import. */
#define ads_FieldOperations_components (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 23))

/** Repositioning of the source region and the field. */
#define ads_FieldOperations_positioning (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 24))

/** Represents a field that references another field. */
#define ads_FieldReference (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 25))

#define ads_FieldReference_field (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 26))

/** This field is a time-shift of another field. The FieldShifter object has two members, one the time-shift (a direct member) and another member-by-association, shiftedField, which is another field. */
#define ads_FieldShifter (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 27))

#define ads_FieldShifter_field (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 28))

/** Precisely identifies the nature of a field/quantity; MISES and STRESS are two different field types even though they are both stresses. Built-in FieldTypes can be looked up using the symbols used here -- user-constructed FieldTypes can not. Field also holds physical dimension. Physical dimension is a formula of a product of powers of base physical dimension, such as Length^2.Mass^1. etc. */
#define ads_FieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 29))

/** A field type (D) for a field which represents an underlying field type (F) differentiatied with respect to P. D = del F / del P. This is an abstract base class. Like all field types, this must be correctly associated with an AlgebraicType. */
#define ads_FieldTypeDerivative (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 30))

/** For a FieldTypeDerivative in which P is a Parameter. */
#define ads_FieldTypeDerivativeParameter (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 31))

/** Link between a FieldTypeParameter (D) and the parameter, P, which is the differntiator in the expression D = del F/del P. */
#define ads_FieldTypeDerivativeParameter_parameter (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 32))

/** Link between a derivative field type (D) and the underlying field type, F, of which it is the derivative. */
#define ads_FieldTypeDerivative_underlyingFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 33))

/** For a field type which is an expression of other field types. An example expression (not implemented yet) is one field type divided by another. */
#define ads_FieldTypeExpression (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 34))

/** Focus based library of string keyed field type records */
#define ads_FieldTypeLib (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 35))

/** String-keyed field types. */
#define ads_FieldTypeLib_fieldTypes (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 36))

/** Class for a field-type which identifies only two things: the algebraic type, and the physical dimension, no more. This class is being used to prevent over-pollution of the field type dictionary because the needs of the input data model. Such fields are not in the data dictionary; instead, create them using the service SMASrvFieldServices::CreateFieldTypeNonspecific. */
#define ads_FieldTypeNonspecific (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 37))

/** FieldTypes are either FieldTypePrimary or FieldTypeExpression. A FieldTypePrimary is a 'primitive' FieldType which stands on its own and does not refer, for its meaning, to another FieldType; this includes all the familiar field types such as stress, strain etc. A FieldTypeExpression is some sort of specific known expression of one or more other FieldType(s). A FieldTypeExpression ALWAYS refers to other FieldType(s), from which it gets its meaning; and therefore it ultimately refers to one or more FieldTypePrimarys. An example of a FieldTypeExpression is FieldTypeDerivativeParameter; this refers to an 'underlying' FieldType from which it gets its meaning (if this FieldType represented derivative of stress with respect to a design parameter, it will refer to the Stress field type, and also to the design parameter.) All of the 'built-in' field types (that is, field types which appear in the dictionary, meaning, which are accessible through the symbols) are FieldTypePrimarys. None of the FieldTypeExpression(s) are built-in; they are made on the fly as needed. However, a FieldTypePrimary does not have to be built-in; user-field types are primary field types, but are not in the dictionary either and are only made as needed. So 'Primary' in a FieldType means 'not-an-expression-FieldType', but does not necessary mean 'is a built-in field type'. To find out if a FieldType is primary, use fieldType.IsA( fieldTypePrimaryTypeID ); you almost always want to use IsA(..) to find something's type, not equality of typeIDs. The field type dictionary, indicated by the symbols below, is larger than it needs to be; some of the field types here can be reduced to expressions of other field types (an example is TRESCA and MISES, which are a known function of stress). Note about mass concentration related field types: We need to distinguish the MFL for fluid links from MFL for mass diffusion. Let's keep MFL for fluid links as MFL with dimensions Mass/Time. For mass diffusion, we should make up new variables. The key to understanding what to do with ISOL, ESOL, MFL, MFLM is to deal with CONC properly. We need several variables instead of just the one for CONC: MASS_FRAC - dimensionless VOL_FRAC - dimensionless (but must distinguish it from MASS_FRAC case) MASS_PER_VOL AOS_PER_VOL - AOS=Amount of Substance AOS_PER_MASS The following links are useful the understand the topic: http://en.wikipedia.org/wiki/Fick's_laws_of_diffusion http://www.engineeringtoolbox.com/ppm-d_1039.html ISOL and ESOL have same units, and ISOL = IVOL*CONC(where IVOL is volume). So we need: ISOL_MASS_FRAC_VOL = VOL * MASS_FRAC ISOL_VOL = VOL * VOL_FRAC ISOL_MASS = VOL * MASS_PER_VOL ISOL_AOS = VOL * AOS_PER_VOL ISOL_AOS_PER_MASS_VOL = VOL * AOS_PER_MASS The ones for ESOL are directly parallel. For mass diffusion MFL, we need CONC/TIME, so the following field quantities replace MFL: MASS_FRAC_FLUX (1/Time) VOL_FRAC_FLUX (1/Time) MASS_PER_VOL_FLUX (M,L,T = (1,-3,-1)) AOS_PER_VOL_FLUX (AOS,L,T = (1,-3,-1)) AOS_PER_MASS_FLUX (AOS,M,T = (1,-1,-1)) MFLM variables has been changed to new variables made by concatenating "_MAG" to MFL's new variables. The same parallel replacements for the mass diffusion variables NFLUX, NFLn, RFL, RFLn, CFL, CFLn, RFLE, RFLEn, SOD, and SOL (parallels ISOL and ESOL directly) has been made too. This has generated 5 field types for each of these! */
#define ads_FieldTypePrimary (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 38))

/** The physical dimension of the FieldType. */
#define ads_FieldType_physicalDimension (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 39))

/** Link from FieldType to a related rotation FieldType. This is not just displacement-rotation, but also things like nodal-force to nodal-moments. */
#define ads_FieldType_rotationFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 40))

/** Represents a field (or part of a field) whose domain is of uniform type. For example its domain could be a grid of definite type, say Elements x LocalNodes; or, alternatively, a grid of type Elements x IntStations x MatPoints; but its domain could not be both, since these are different domain types. This is an abstract base class; the most important subclass is MeshedField. In time, analytic fields will be supported in derived classes. */
#define ads_FieldUniformDomain (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 41))

/** This field is a union of other fields. The FieldUnion object has any number of fields as members. Using the FieldUnion, it becomes possible to put MeshedFields of different grid types together. The FieldUnion should have the same FieldType as all of its member fields. */
#define ads_FieldUnion (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 42))

/** Provides the field member of FieldUnion. */
#define ads_FieldUnion_componentFields (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 43))

/** Represents a Field captures in an external SIMDoc along with parameters for mapping the external field data onto the mesh of the current SIMDoc. */
#define ads_Field_ExternalSourceAbq (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 44))

/** Links Field_ExternalSourceAbq to the ExternalRegion. */
#define ads_Field_ExternalSourceAbq_externalRegion (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 45))

/** Links ExternalSource records to the Focus. */
#define ads_Field_ExternalSourceAbq_externalSource (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 46))

/** Links Field_ExternalSourceAbq records to FieldMapperControls. */
#define ads_Field_ExternalSourceAbq_fieldMapperControls (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 47))

/** Links Field_ExternalSourceAbq to FieldOperations. */
#define ads_Field_ExternalSourceAbq_fieldOperations (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 48))

/** Link to attach IntervalStepInc to a Field_ExternalSourceAbq. Effectively defines a "time slice" of the Field_ExternalSourceAbq */
#define ads_Field_ExternalSourceAbq_intervalStepInc (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 49))

/** Link to stepInc record. */
#define ads_Field_ExternalSourceAbq_stepInc (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 50))

/** Field_FTable enables the definition of the values of one or more fields (the dependent variables) as a tabular function of the values of one or more other fields (the independent variables). Clients must define the names of the dependent and independent variables to match the expected usage of the Field_FTable. For example, a Field_FTable for use with a fluid boundary condition of type PV might expect "PRESSURE" as the sole dependent variable and "VOLUME" as the sole independent variable. As with FTables, Field_FTables cannot be mesh dependent, i.e. one cannot use nodes, elements, element "locations" as independent variables. In the future we will allow for combining a mesh-based field representation to be attached to a Field to allow an FTable to be distributed over a mesh, but this schema has not yet been implemented. However, a particular feature that refers to an Field_FTable might allow for independent variables like "X", "Y", and "Z". The implementation of that feature would implicitly bind those independent variables to the coordinates of the nodes or element/element face locations. */
#define ads_Field_FTable (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 51))

#define ads_Field_FTable_externalField (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 52))

/** The actual distribution to capture the spatial distribution of the data. */
#define ads_Field_FTable_ftableDistribution (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 53))

/** The actual table carrying the tabular function that relates the dependent variables to the independent ones. */
#define ads_Field_FTable_table (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 54))

/** Quaternion Field to provide ease of storage as opposed to storing it as a distribution. */
#define ads_Field_Quaternion (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 55))

/** Scalar Field to provide ease of storage as opposed to storing it as a distribution.. */
#define ads_Field_Scalar (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 56))

/** Tensor Field to provide ease of storage as opposed to storing it as a distribution. */
#define ads_Field_Tensor (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 57))

/** Vector Field to provide ease of storage as opposed to storing it as a distribution.. */
#define ads_Field_Vector (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 58))

/** Link from Field to FieldType; every field must have exactly one field type. So, for example, a stress field would be linked to a field type indication precisely "STRESS"; a mises field would be linked to a field type indicating precisely "MISES"; these latter two field types are different, even though they are both 'stresses'. */
#define ads_Field_fieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 59))

/** Reference to a Material. This is useful to capture the boundary conditions or initial conditions per species in an analysis involving multi-species. This can also be used to capture output per material instance. */
#define ads_Field_material (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 60))

/** Link from Field to an orientation field. Orientation field is not statically distinct from a field, but it has an orientation field type. A field is not required to have a link to an orientation field, but if an orientation applies, it should. */
#define ads_Field_orientationField (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 61))

/** A region linked to a field indicates the spatial domain of that field. A field may be defined over a Lagrangian region (moves with the material) or an eulerian region (relative to space), for either a lagragian, eulerian or combined analysis. The Region is optional because for certain fields it may be implicit in other data. For example, in a MeshedField the region is represented by the domain of the drawers. */
#define ads_Field_region (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 62))

/** Library for string-keyed field types. */
#define ads_Focus_fieldTypeLib (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 63))

/** Abstract base class for FramesDescriptions */
#define ads_FramesDescription (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 64))

/** Gives values of frequency c-members */
#define ads_FrequencyDescription (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 65))

/** Link from FrequencyDescription to distribution giving values. The grid of this distribution is the frequency collection being described. */
#define ads_FrequencyDescription_data (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 66))

/** Link from FrequencyDescription to the frequency collection being described. */
#define ads_FrequencyDescription_frequencyCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 67))

/** The "ComplexNumberPart" collection is associated with distributions that have Complex numbers. */
#define ads_GlobalCollections_complexNumberPartCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 68))

/** The "components" collection is used in one dimension of a certin distribution if that distribution's range should be (conceptually) a vector, tensor, etc. */
#define ads_GlobalCollections_componentCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 69))

/** This collection groups all Invariants. */
#define ads_GlobalCollections_invariantCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 70))

/** The "LimitType" collection is used to indicate discontinuities. */
#define ads_GlobalCollections_limitTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 71))

/** A collection of user subroutines. */
#define ads_GlobalCollections_userSubroutineCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 72))

/** A collection of user subroutine parameters. */
#define ads_GlobalCollections_userSubroutineParameterCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 73))

/** Represents an interval of Abaqus steps and increments */
#define ads_IntervalStepInc (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 74))

/** Identifies an invariant-function on a tensor. An invariant function does not change in value with change of basis. These Invariants are **functions** of tensor components. */
#define ads_Invariant (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 75))

/** Collection of invariants. */
#define ads_InvariantCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 76))

/** The LimitType collection, like Components, is a singleton. The instances of the LimitType type indicate whether a limit applies from the left or from the right. A frame c-member associated with LEFT is a left-limit frame and a frame c-member associated with RIGHT is a right-limit frame. This system allows discontinuties in time to be expressed unambiguously while using frames. A discontinuty is represented when two frames have identical associated time t but one is associated with the left limit, t-, and the other with the right limit, t+. If a frame c-member is neither associated with a LEFT or RIGHT limit, both limits apply; that is, the frame c-member is not indicated to be at a discontinuity. Limits can also potentially be used to indicate that a function is undefined in part of a region; but it is unknown if this potential feature will be used. For example, consider four frames f1, f2, f3, f4 with distinct ascending time values t1, t2, t3, t4. A left-limit applied at frame f2 and a right-limit applied at frame f3 could indicate that the function is undefined in the range t2..t3. For now, limits will just be used to indicate discontinuties. */
#define ads_LimitType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 77))

#define ads_LimitTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 78))

/** A grid Time. */
#define ads_LimitTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 79))

/** A meshed field defines a field at certain locations in a mesh. A mesh for this purpose can be more general than a finite-element mesh; for example, a set of nodes alone could be supported as a basis for a meshed field. GridType is a member of MeshedField; all SerializedFields stored under this MeshedField must have exactly the same GridType; for example, the MeshedField could incorporate SerializedFields with grids of type Elements x LocalNodes; or Elements x IntStations x MatPoints; but it is illegal for it to use both grid types. If it did use both, it would be violating the assumtions of a UniformDomainField, which is the base class of MeshedField. */
#define ads_MeshedField (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 80))

/** The default value for the meshed field. For the domain outside of MeshedField the values from the default field are assigned. */
#define ads_MeshedField_default (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 81))

/** The distribution that actually stores the field values. */
#define ads_MeshedField_distribution (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 82))

/** TOOD-by Hwala, Modi and Shravan. */
#define ads_MeshedField_occurrence (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 83))

/** Link from MeshedField to Serialized field. */
#define ads_MeshedField_serializedField (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 84))

#define ads_Model_fieldMapperControls (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 85))

#define ads_Model_fieldOperations (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 86))

/** Link from model to all FramesDescriptions, which includes both Time and Frequency descriptions. */
#define ads_Model_framesDescriptions (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 87))

#define ads_NodeSectionPointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 88))

/** Grid spanning mesh nodes and temperature points. */
#define ads_NodeTemperaturePointGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 89))

/** This grid is empty. There are no collections in it. Annull grid type can be used in a meshed field to indicate that the field has the same value everywhere. */
#define ads_NullGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 90))

/** This grid represents whole model or whole region output */
#define ads_OutputRequestGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 91))

/** Associates all XYDataSet objects with the results anchor in a string-keyed association. */
#define ads_Results_xyDataSet (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 92))

/** A Serialized Field is a part of a meshed field for which the same algebraic serialization assumption applies to the field range; this serialization is described in the components c-set. For example, stress for plane stress can be put together in a single SerializedField, which would have an according components c-set; similarly for continuum elements; but stress for plane stress and continuum elements could not be put together in the same SerializedField, because the serialization is different. */
#define ads_SerializedField (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 93))

/** Describes the algebraic type of the data stored in this field. It must be consistent with the AlgebraciType associated to the FieldType. */
#define ads_SerializedField_components (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 94))

/** SerializedField references to drawers. */
#define ads_SerializedField_drawers (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 95))

/** Link from SerializedField to any number of Invariants which apply to the serialized field. This would not be necessary or useful if the field type itself implied it was an invariant (such as pressure, an invariant of stress). But it allows an invariant of a tensor field to be stored with that field. For example, let?s consider a field type T, which is a tensor, it is possible to present, in a field of field type T, in some regions, just the invariant of T. This is done by attaching a serialized field to the field, this serialized field being attached to the relevant Invariant record. The serialized field in turn will need a different SerializedAlgebraicType to the rest of the field; it will in fact be scalar. (This is not greatly different from the practice of storing principal values of a field.) */
#define ads_SerializedField_invariant (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 96))

/** A field type for a user solution-dependent variable (SDV). Any number of these can be created, each with a different ordinal. Each should be assocated with the SCALAR algebraic type. */
#define ads_SolutionDependentVariableFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 97))

/** Represents Abaqus step, increments, interval, and iteration. */
#define ads_StepInc (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 98))

/** Gives values of time c-members */
#define ads_TimeDescription (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 99))

/** Link from TimeDescription to distribution giving values. The grid of this distribution is the time collection being described. */
#define ads_TimeDescription_data (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 100))

/** Link from TimeDescription to the time collection being described. */
#define ads_TimeDescription_timeCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 101))

/** A user field expression. */
#define ads_UserFieldExpression (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 102))

#define ads_UserFieldExpression_orientation (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 103))

/** Abstract base class for a field type for a user-defined variable. This base class is just used for grouping, so an expression like fieldTypeRecord.IsA( userFieldTypeTypeID) can be used. There are a few distinct concrete derived classes. */
#define ads_UserFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 104))

/** A field type with a user string label, which will be used, for example, for XY data, not generally used in the analysis codes currently. Potentially, a user-labelled field type could be associated (by the 'user') with any appropriate alegebraic type, but if made in the context of XY data, it should be associated with SCALAR algebraic type. */
#define ads_UserLabelledFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 105))

/** A field type for a user output variable (UVARM). Any number of these can be created, each with a different ordinal. Each should be assocated with the SCALAR algebraic type. */
#define ads_UserOutputVariableFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 106))

/** A field type for a user predefined variable (FV). Any number of these can be created, each with a different ordinal. Each should be assocated with the SCALAR algebraic type. */
#define ads_UserPredefinedVariableFieldType (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 107))

/** This record represents a base user subroutine. */
#define ads_UserSubroutine (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 108))

/** A collection of user subroutines. */
#define ads_UserSubroutineCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 109))

/** This record represents a parameter to a user subroutine. Parameters are never instantiated - they only serve as c-members in distributions that store the parameter values. */
#define ads_UserSubroutineParameter (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 110))

/** A counting collection of user subroutine parameters. */
#define ads_UserSubroutineParameterCollection (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 111))

/** A grid which elements are parameters of a user subroutine. */
#define ads_UserSubroutineParameterGrid (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 112))

/** Link from UserSubroutine to its parameters. */
#define ads_UserSubroutine_arguments (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 113))

/** Represents set of either abscissa or ordinate values for XY-data. Has, as members-by-assocation, a distribution representing the values, and a FieldType and a SerializedAlgebraicType. That SerializedAlgebraicType indicates what component that particular XData represents. Often it is jsut scalar. */
#define ads_XData (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 114))

/** Link XData to distribution which is Frame collection --> XData values (doubles or floats). Implementation note: add more grid types, if needed. */
#define ads_XData_values (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 115))

/** In XY-data set; for XY-data capabilities; basically, the data of an XY-plot. The XYDataSet is associated with 1 or 2 XData records. Each XData represents either X or Y data in a way which will be explained below. Each XData record refers to a distribution which define (either X- or Y-) values corresponding to respective c-members of a Frames CSet, which is of course in a Frames collection. The distribution will contain one drawer and the Frames CSet in the drawer description will be ascending. There are three different cases: (i) The abscissa, X, is time, and the ordinate, Y is another value, (ii) X is a non-time value, Y is another non-time value and time is a third member of the triple (iii) X is non-time, Y is non-time, but there are no third (time-like) member of the triple. For case (i), there is only one XData, representing Y, the ordinate. The time members (X) are available from the FramesDescription of the frames collection, which is globally registered. For case (ii), the X values are represented by the first XData, and the Y values by the second. The frames CSet of the drawer of the distribution of of each of the two XData's is identical. The time values are available from the FramesDescription of the frames collection, which is globally registered. Case (iii) is like case (ii) except the frames collection in question is not globally registered so no time values are available. */
#define ads_XYDataSet (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 116))

/** Link each XDataSet to 1 or 2 XData. */
#define ads_XYDataSet_xData (ads_CoreFragmentTypeIndex(ads_CoreFieldFragment, 117))

/** 
Enum with association roles. */
enum ads_ArcLengthDescription_arcLengthCollectionRolesEnm
{
    ads_ArcLengthDescription_arcLengthCollection_child,
    ads_ArcLengthDescription_arcLengthCollection_parent
};

/** 
Enum with association roles. */
enum ads_ArcLengthDescription_dataRolesEnm
{
    ads_ArcLengthDescription_data_child,
    ads_ArcLengthDescription_data_parent
};

/** Enum with the symbols of data type ComplexNumberPart*/
enum ads_ComplexNumberPartSymbolsEnm
{
    ads_ComplexNumberPart_IMAGINARY,
    ads_ComplexNumberPart_MAGNITUDE,
    ads_ComplexNumberPart_PHASE,
    ads_ComplexNumberPart_PHASE_DEGREES,
    ads_ComplexNumberPart_REAL
};

/** Enum with the symbols of data type Component*/
enum ads_ComponentSymbolsEnm
{
    ads_Component_C_1,
    ads_Component_C_11,
    ads_Component_C_1111,
    ads_Component_C_1112,
    ads_Component_C_1122,
    ads_Component_C_12,
    ads_Component_C_1211,
    ads_Component_C_1212,
    ads_Component_C_1222,
    ads_Component_C_12PLUS,
    ads_Component_C_13,
    ads_Component_C_13PLUS,
    ads_Component_C_2,
    ads_Component_C_21,
    ads_Component_C_22,
    ads_Component_C_2211,
    ads_Component_C_2212,
    ads_Component_C_2222,
    ads_Component_C_23,
    ads_Component_C_23PLUS,
    ads_Component_C_3,
    ads_Component_C_31,
    ads_Component_C_32,
    ads_Component_C_33,
    ads_Component_C_PRIN1,
    ads_Component_C_PRIN2,
    ads_Component_C_PRIN3,
    ads_Component_C_QUAT0,
    ads_Component_C_QUAT1,
    ads_Component_C_QUAT2,
    ads_Component_C_QUAT3,
    ads_Component_C_ROT1,
    ads_Component_C_ROT2,
    ads_Component_C_ROT3,
    ads_Component_C_SCALAR,
    ads_Component_C_SORT_PRIN1,
    ads_Component_C_SORT_PRIN2,
    ads_Component_C_SORT_PRIN3,
    ads_Component_C_UNDEFINED
};

/** 
Enum with grid dimensions. */
enum ads_ComponentGridDimensionsEnm
{
    ads_ComponentGrid_component
};

/** 
Enum with grid dimensions. */
enum ads_ElementIntegrationStationGridDimensionsEnm
{
    ads_ElementIntegrationStationGrid_element,
    ads_ElementIntegrationStationGrid_integrationStation
};

/** 
Enum with grid dimensions. */
enum ads_ElementIntegrationStationLayerLayerPointGridDimensionsEnm
{
    ads_ElementIntegrationStationLayerLayerPointGrid_element,
    ads_ElementIntegrationStationLayerLayerPointGrid_integrationStation,
    ads_ElementIntegrationStationLayerLayerPointGrid_layer,
    ads_ElementIntegrationStationLayerLayerPointGrid_layerPoint
};

/** 
Enum with grid dimensions. */
enum ads_ElementIntegrationStationLayerSectionPointGridDimensionsEnm
{
    ads_ElementIntegrationStationLayerSectionPointGrid_element,
    ads_ElementIntegrationStationLayerSectionPointGrid_integrationStation,
    ads_ElementIntegrationStationLayerSectionPointGrid_layer,
    ads_ElementIntegrationStationLayerSectionPointGrid_sectionPoint
};

/** 
Enum with grid dimensions. */
enum ads_ElementIntegrationStationSectionPointGridDimensionsEnm
{
    ads_ElementIntegrationStationSectionPointGrid_element,
    ads_ElementIntegrationStationSectionPointGrid_integrationStation,
    ads_ElementIntegrationStationSectionPointGrid_sectionPoint
};

/** 
Enum with grid dimensions. */
enum ads_ElementLayerGridDimensionsEnm
{
    ads_ElementLayerGrid_element,
    ads_ElementLayerGrid_layer
};

/** Enum with grid dimensions. */
enum ads_ElementPointSectionPointGridDimensionsEnm
{
    ads_ElementPointSectionPointGrid_element,
    ads_ElementPointSectionPointGrid_point,
    ads_ElementPointSectionPointGrid_sectionPoint
};

/** 
Enum with grid dimensions. */
enum ads_ElementSectionPointComponentGridDimensionsEnm
{
    ads_ElementSectionPointComponentGrid_component,
    ads_ElementSectionPointComponentGrid_element,
    ads_ElementSectionPointComponentGrid_sectionPoint
};

/** Enum with grid dimensions. */
enum ads_ElementSectionPointGridDimensionsEnm
{
    ads_ElementSectionPointGrid_element,
    ads_ElementSectionPointGrid_sectionPoint
};

/** 
Enum with record members. */
enum ads_FieldMapperControlsMembersEnm
{
    ads_FieldMapperControls_algorithm,
    ads_FieldMapperControls_configuration,
    ads_FieldMapperControls_elementDistance,
    ads_FieldMapperControls_facetInwardNormalDistance,
    ads_FieldMapperControls_facetOutwardNormalDistance,
    ads_FieldMapperControls_tensorAveraging,
    ads_FieldMapperControls_unmappedMembers
};

enum ads_FieldMapperControls_algorithmEnm
{
    ads_FieldMapperControls_algorithm_GLOBAL_CONSERVATIVE,
    ads_FieldMapperControls_algorithm_SUBELEMENT
};

enum ads_FieldMapperControls_configurationEnm
{
    ads_FieldMapperControls_configuration_DEFORMED,
    ads_FieldMapperControls_configuration_REFERENCE
};

enum ads_FieldMapperControls_tensorAveragingEnm
{
    ads_FieldMapperControls_tensorAveraging_COMPONENT,
    ads_FieldMapperControls_tensorAveraging_EIGENVALUE,
    ads_FieldMapperControls_tensorAveraging_INVARIANT
};

enum ads_FieldMapperControls_unmappedMembersEnm
{
    ads_FieldMapperControls_unmappedMembers_CONSTANT_VALUE,
    ads_FieldMapperControls_unmappedMembers_ERROR,
    ads_FieldMapperControls_unmappedMembers_NEAREST_NEIGHBOR
};

/** 
Enum with association roles. */
enum ads_FieldMapperControls_defaultValuesRolesEnm
{
    ads_FieldMapperControls_defaultValues_child,
    ads_FieldMapperControls_defaultValues_parent
};

/** 
Enum with association roles. */
enum ads_FieldMapperControls_nearestNeighborRadiusRolesEnm
{
    ads_FieldMapperControls_nearestNeighborRadius_child,
    ads_FieldMapperControls_nearestNeighborRadius_parent
};

/** 
Enum with record members. */
enum ads_FieldOperationsMembersEnm
{
    ads_FieldOperations_scaleFactor
};

/** 
Enum with association roles. */
enum ads_FieldOperations_componentsRolesEnm
{
    ads_FieldOperations_components_referent,
    ads_FieldOperations_components_referrer
};

/** 
Enum with association roles. */
enum ads_FieldOperations_positioningRolesEnm
{
    ads_FieldOperations_positioning_child,
    ads_FieldOperations_positioning_parent
};

/** Enum with association roles. */
enum ads_FieldReference_fieldRolesEnm
{
    ads_FieldReference_field_referent,
    ads_FieldReference_field_referrer
};

/** 
Enum with record members. */
enum ads_FieldShifterMembersEnm
{
    ads_FieldShifter_timeShift
};

/** Enum with association roles. */
enum ads_FieldShifter_fieldRolesEnm
{
    ads_FieldShifter_field_referent,
    ads_FieldShifter_field_referrer
};

/** 
Enum with record members. */
enum ads_FieldTypeMembersEnm
{
    ads_FieldType_algebraicType,
    ads_FieldType_description,
    ads_FieldType_dsMagnitude,
    ads_FieldType_isExtensive,
    ads_FieldType_nonNegative
};

enum ads_FieldType_algebraicTypeEnm
{
    ads_FieldType_algebraicType_POSITION,
    ads_FieldType_algebraicType_SCALAR,
    ads_FieldType_algebraicType_TENSOR2_GENERAL,
    ads_FieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldType_algebraicType_TENSOR3,
    ads_FieldType_algebraicType_TENSOR4,
    ads_FieldType_algebraicType_TUPLE,
    ads_FieldType_algebraicType_UNDEFINED,
    ads_FieldType_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_FieldTypeDerivativeMembersEnm
{
    ads_FieldTypeDerivative_algebraicType,
    ads_FieldTypeDerivative_description,
    ads_FieldTypeDerivative_dsMagnitude,
    ads_FieldTypeDerivative_isExtensive,
    ads_FieldTypeDerivative_nonNegative
};

enum ads_FieldTypeDerivative_algebraicTypeEnm
{
    ads_FieldTypeDerivative_algebraicType_POSITION,
    ads_FieldTypeDerivative_algebraicType_SCALAR,
    ads_FieldTypeDerivative_algebraicType_TENSOR2_GENERAL,
    ads_FieldTypeDerivative_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldTypeDerivative_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldTypeDerivative_algebraicType_TENSOR3,
    ads_FieldTypeDerivative_algebraicType_TENSOR4,
    ads_FieldTypeDerivative_algebraicType_TUPLE,
    ads_FieldTypeDerivative_algebraicType_UNDEFINED,
    ads_FieldTypeDerivative_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_FieldTypeDerivativeParameterMembersEnm
{
    ads_FieldTypeDerivativeParameter_algebraicType,
    ads_FieldTypeDerivativeParameter_description,
    ads_FieldTypeDerivativeParameter_dsMagnitude,
    ads_FieldTypeDerivativeParameter_isExtensive,
    ads_FieldTypeDerivativeParameter_nonNegative
};

enum ads_FieldTypeDerivativeParameter_algebraicTypeEnm
{
    ads_FieldTypeDerivativeParameter_algebraicType_POSITION,
    ads_FieldTypeDerivativeParameter_algebraicType_SCALAR,
    ads_FieldTypeDerivativeParameter_algebraicType_TENSOR2_GENERAL,
    ads_FieldTypeDerivativeParameter_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldTypeDerivativeParameter_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldTypeDerivativeParameter_algebraicType_TENSOR3,
    ads_FieldTypeDerivativeParameter_algebraicType_TENSOR4,
    ads_FieldTypeDerivativeParameter_algebraicType_TUPLE,
    ads_FieldTypeDerivativeParameter_algebraicType_UNDEFINED,
    ads_FieldTypeDerivativeParameter_algebraicType_VECTOR
};

/** 
Enum with association roles. */
enum ads_FieldTypeDerivativeParameter_parameterRolesEnm
{
    ads_FieldTypeDerivativeParameter_parameter_referent,
    ads_FieldTypeDerivativeParameter_parameter_referrer
};

/** 
Enum with association roles. */
enum ads_FieldTypeDerivative_underlyingFieldTypeRolesEnm
{
    ads_FieldTypeDerivative_underlyingFieldType_referent,
    ads_FieldTypeDerivative_underlyingFieldType_referrer
};

/** 
Enum with record members. */
enum ads_FieldTypeExpressionMembersEnm
{
    ads_FieldTypeExpression_algebraicType,
    ads_FieldTypeExpression_description,
    ads_FieldTypeExpression_dsMagnitude,
    ads_FieldTypeExpression_isExtensive,
    ads_FieldTypeExpression_nonNegative
};

enum ads_FieldTypeExpression_algebraicTypeEnm
{
    ads_FieldTypeExpression_algebraicType_POSITION,
    ads_FieldTypeExpression_algebraicType_SCALAR,
    ads_FieldTypeExpression_algebraicType_TENSOR2_GENERAL,
    ads_FieldTypeExpression_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldTypeExpression_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldTypeExpression_algebraicType_TENSOR3,
    ads_FieldTypeExpression_algebraicType_TENSOR4,
    ads_FieldTypeExpression_algebraicType_TUPLE,
    ads_FieldTypeExpression_algebraicType_UNDEFINED,
    ads_FieldTypeExpression_algebraicType_VECTOR
};

/** 
Enum with association roles. */
enum ads_FieldTypeLib_fieldTypesRolesEnm
{
    ads_FieldTypeLib_fieldTypes_child,
    ads_FieldTypeLib_fieldTypes_parent
};

/** 
Enum with record members. */
enum ads_FieldTypeNonspecificMembersEnm
{
    ads_FieldTypeNonspecific_algebraicType,
    ads_FieldTypeNonspecific_description,
    ads_FieldTypeNonspecific_dsMagnitude,
    ads_FieldTypeNonspecific_isExtensive,
    ads_FieldTypeNonspecific_nonNegative
};

enum ads_FieldTypeNonspecific_algebraicTypeEnm
{
    ads_FieldTypeNonspecific_algebraicType_POSITION,
    ads_FieldTypeNonspecific_algebraicType_SCALAR,
    ads_FieldTypeNonspecific_algebraicType_TENSOR2_GENERAL,
    ads_FieldTypeNonspecific_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldTypeNonspecific_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldTypeNonspecific_algebraicType_TENSOR3,
    ads_FieldTypeNonspecific_algebraicType_TENSOR4,
    ads_FieldTypeNonspecific_algebraicType_TUPLE,
    ads_FieldTypeNonspecific_algebraicType_UNDEFINED,
    ads_FieldTypeNonspecific_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_FieldTypePrimaryMembersEnm
{
    ads_FieldTypePrimary_algebraicType,
    ads_FieldTypePrimary_description,
    ads_FieldTypePrimary_dsMagnitude,
    ads_FieldTypePrimary_isExtensive,
    ads_FieldTypePrimary_nonNegative
};

enum ads_FieldTypePrimary_algebraicTypeEnm
{
    ads_FieldTypePrimary_algebraicType_POSITION,
    ads_FieldTypePrimary_algebraicType_SCALAR,
    ads_FieldTypePrimary_algebraicType_TENSOR2_GENERAL,
    ads_FieldTypePrimary_algebraicType_TENSOR2_ORTHOGONAL,
    ads_FieldTypePrimary_algebraicType_TENSOR2_SYMMETRIC,
    ads_FieldTypePrimary_algebraicType_TENSOR3,
    ads_FieldTypePrimary_algebraicType_TENSOR4,
    ads_FieldTypePrimary_algebraicType_TUPLE,
    ads_FieldTypePrimary_algebraicType_UNDEFINED,
    ads_FieldTypePrimary_algebraicType_VECTOR
};

/** 
Enum with association roles. */
enum ads_FieldType_physicalDimensionRolesEnm
{
    ads_FieldType_physicalDimension_referent,
    ads_FieldType_physicalDimension_referrer
};

/** 
Enum with association roles. */
enum ads_FieldType_rotationFieldTypeRolesEnm
{
    ads_FieldType_rotationFieldType_referent,
    ads_FieldType_rotationFieldType_referrer
};

/** 
Enum with association roles. */
enum ads_FieldUnion_componentFieldsRolesEnm
{
    ads_FieldUnion_componentFields_child,
    ads_FieldUnion_componentFields_parent
};

/** 
Enum with record members. */
enum ads_Field_ExternalSourceAbqMembersEnm
{
    ads_Field_ExternalSourceAbq_absExteriorTolerance,
    ads_Field_ExternalSourceAbq_exteriorFieldType,
    ads_Field_ExternalSourceAbq_exteriorRegionType,
    ads_Field_ExternalSourceAbq_exteriorTolerance,
    ads_Field_ExternalSourceAbq_interpolate,
    ads_Field_ExternalSourceAbq_midside,
    ads_Field_ExternalSourceAbq_reader,
    ads_Field_ExternalSourceAbq_regionType,
    ads_Field_ExternalSourceAbq_submodel,
    ads_Field_ExternalSourceAbq_timeRange,
    ads_Field_ExternalSourceAbq_timeScaling
};

enum ads_Field_ExternalSourceAbq_exteriorRegionTypeEnm
{
    ads_Field_ExternalSourceAbq_exteriorRegionType_ELEMENT,
    ads_Field_ExternalSourceAbq_exteriorRegionType_MONITOR,
    ads_Field_ExternalSourceAbq_exteriorRegionType_NODE,
    ads_Field_ExternalSourceAbq_exteriorRegionType_SURFACE
};

enum ads_Field_ExternalSourceAbq_regionTypeEnm
{
    ads_Field_ExternalSourceAbq_regionType_ELEMENT,
    ads_Field_ExternalSourceAbq_regionType_NODE,
    ads_Field_ExternalSourceAbq_regionType_SURFACE
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_externalRegionRolesEnm
{
    ads_Field_ExternalSourceAbq_externalRegion_referent,
    ads_Field_ExternalSourceAbq_externalRegion_referrer
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_externalSourceRolesEnm
{
    ads_Field_ExternalSourceAbq_externalSource_referent,
    ads_Field_ExternalSourceAbq_externalSource_referrer
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_fieldMapperControlsRolesEnm
{
    ads_Field_ExternalSourceAbq_fieldMapperControls_referent,
    ads_Field_ExternalSourceAbq_fieldMapperControls_referrer
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_fieldOperationsRolesEnm
{
    ads_Field_ExternalSourceAbq_fieldOperations_referent,
    ads_Field_ExternalSourceAbq_fieldOperations_referrer
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_intervalStepIncRolesEnm
{
    ads_Field_ExternalSourceAbq_intervalStepInc_child,
    ads_Field_ExternalSourceAbq_intervalStepInc_parent
};

/** 
Enum with association roles. */
enum ads_Field_ExternalSourceAbq_stepIncRolesEnm
{
    ads_Field_ExternalSourceAbq_stepInc_child,
    ads_Field_ExternalSourceAbq_stepInc_parent
};

/** Enum with association roles. */
enum ads_Field_FTable_externalFieldRolesEnm
{
    ads_Field_FTable_externalField_child,
    ads_Field_FTable_externalField_parent
};

/** 
Enum with association roles. */
enum ads_Field_FTable_ftableDistributionRolesEnm
{
    ads_Field_FTable_ftableDistribution_child,
    ads_Field_FTable_ftableDistribution_parent
};

/** 
Enum with association roles. */
enum ads_Field_FTable_tableRolesEnm
{
    ads_Field_FTable_table_child,
    ads_Field_FTable_table_parent
};

/** 
Enum with record members. */
enum ads_Field_QuaternionMembersEnm
{
    ads_Field_Quaternion_value
};

/** 
Enum with record members. */
enum ads_Field_ScalarMembersEnm
{
    ads_Field_Scalar_value
};

/** 
Enum with record members. */
enum ads_Field_TensorMembersEnm
{
    ads_Field_Tensor_value
};

/** 
Enum with record members. */
enum ads_Field_VectorMembersEnm
{
    ads_Field_Vector_value
};

/** 
Enum with association roles. */
enum ads_Field_fieldTypeRolesEnm
{
    ads_Field_fieldType_referent,
    ads_Field_fieldType_referrer
};

/** 
Enum with association roles. */
enum ads_Field_materialRolesEnm
{
    ads_Field_material_referent,
    ads_Field_material_referrer
};

/** 
Enum with association roles. */
enum ads_Field_orientationFieldRolesEnm
{
    ads_Field_orientationField_referent,
    ads_Field_orientationField_referrer
};

/** 
Enum with association roles. */
enum ads_Field_regionRolesEnm
{
    ads_Field_region_referent,
    ads_Field_region_referrer
};

/** 
Enum with association roles. */
enum ads_Focus_fieldTypeLibRolesEnm
{
    ads_Focus_fieldTypeLib_child,
    ads_Focus_fieldTypeLib_parent
};

/** 
Enum with association roles. */
enum ads_FrequencyDescription_dataRolesEnm
{
    ads_FrequencyDescription_data_child,
    ads_FrequencyDescription_data_parent
};

/** 
Enum with association roles. */
enum ads_FrequencyDescription_frequencyCollectionRolesEnm
{
    ads_FrequencyDescription_frequencyCollection_child,
    ads_FrequencyDescription_frequencyCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_complexNumberPartCollectionRolesEnm
{
    ads_GlobalCollections_complexNumberPartCollection_child,
    ads_GlobalCollections_complexNumberPartCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_componentCollectionRolesEnm
{
    ads_GlobalCollections_componentCollection_child,
    ads_GlobalCollections_componentCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_invariantCollectionRolesEnm
{
    ads_GlobalCollections_invariantCollection_child,
    ads_GlobalCollections_invariantCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_limitTypeCollectionRolesEnm
{
    ads_GlobalCollections_limitTypeCollection_child,
    ads_GlobalCollections_limitTypeCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_userSubroutineCollectionRolesEnm
{
    ads_GlobalCollections_userSubroutineCollection_child,
    ads_GlobalCollections_userSubroutineCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_userSubroutineParameterCollectionRolesEnm
{
    ads_GlobalCollections_userSubroutineParameterCollection_child,
    ads_GlobalCollections_userSubroutineParameterCollection_parent
};

/** 
Enum with record members. */
enum ads_IntervalStepIncMembersEnm
{
    ads_IntervalStepInc_binc,
    ads_IntervalStepInc_bstep,
    ads_IntervalStepInc_einc,
    ads_IntervalStepInc_estep
};

/** Enum with the symbols of data type Invariant*/
enum ads_InvariantSymbolsEnm
{
    ads_Invariant_INV1,
    ads_Invariant_INV3,
    ads_Invariant_MAGNITUDE,
    ads_Invariant_MAX_INPLANE_PRINCIPAL,
    ads_Invariant_MAX_PRINCIPAL,
    ads_Invariant_MID_PRINCIPAL,
    ads_Invariant_MIN_INPLANE_PRINCIPAL,
    ads_Invariant_MIN_PRINCIPAL,
    ads_Invariant_MISES_LIKE,
    ads_Invariant_OUTOFPLANE_PRINCIPAL,
    ads_Invariant_PRESSURE_LIKE,
    ads_Invariant_TRESCA_LIKE,
    ads_Invariant_UNDEFINED
};

/** Enum with the symbols of data type LimitType*/
enum ads_LimitTypeSymbolsEnm
{
    ads_LimitType_LEFT,
    ads_LimitType_RIGHT
};

/** 
Enum with grid dimensions. */
enum ads_LimitTypeGridDimensionsEnm
{
    ads_LimitTypeGrid_component
};

/** 
Enum with record members. */
enum ads_MeshedFieldMembersEnm
{
    ads_MeshedField_gridType,
    ads_MeshedField_qualifiedFieldQuantity,
    ads_MeshedField_qualifyingGridType
};

/** 
Enum with association roles. */
enum ads_MeshedField_defaultRolesEnm
{
    ads_MeshedField_default_child,
    ads_MeshedField_default_parent
};

/** 
Enum with association roles. */
enum ads_MeshedField_distributionRolesEnm
{
    ads_MeshedField_distribution_child,
    ads_MeshedField_distribution_parent
};

/** 
Enum with association roles. */
enum ads_MeshedField_occurrenceRolesEnm
{
    ads_MeshedField_occurrence_referent,
    ads_MeshedField_occurrence_referrer
};

/** 
Enum with association roles. */
enum ads_MeshedField_serializedFieldRolesEnm
{
    ads_MeshedField_serializedField_child,
    ads_MeshedField_serializedField_parent
};

/** Enum with association roles. */
enum ads_Model_fieldMapperControlsRolesEnm
{
    ads_Model_fieldMapperControls_child,
    ads_Model_fieldMapperControls_parent
};

/** Enum with association roles. */
enum ads_Model_fieldOperationsRolesEnm
{
    ads_Model_fieldOperations_child,
    ads_Model_fieldOperations_parent
};

/** 
Enum with association roles. */
enum ads_Model_framesDescriptionsRolesEnm
{
    ads_Model_framesDescriptions_child,
    ads_Model_framesDescriptions_parent
};

/** Enum with grid dimensions. */
enum ads_NodeSectionPointGridDimensionsEnm
{
    ads_NodeSectionPointGrid_node,
    ads_NodeSectionPointGrid_sectionPoint
};

/** 
Enum with grid dimensions. */
enum ads_NodeTemperaturePointGridDimensionsEnm
{
    ads_NodeTemperaturePointGrid_node,
    ads_NodeTemperaturePointGrid_temperaturePoint
};

/** 
Enum with grid dimensions. */
enum ads_OutputRequestGridDimensionsEnm
{
    ads_OutputRequestGrid_outputRequest
};

/** 
Enum with association roles. */
enum ads_Results_xyDataSetRolesEnm
{
    ads_Results_xyDataSet_child,
    ads_Results_xyDataSet_parent
};

/** 
Enum with association roles. */
enum ads_SerializedField_componentsRolesEnm
{
    ads_SerializedField_components_referent,
    ads_SerializedField_components_referrer
};

/** 
Enum with association roles. */
enum ads_SerializedField_drawersRolesEnm
{
    ads_SerializedField_drawers_referent,
    ads_SerializedField_drawers_referrer
};

/** 
Enum with association roles. */
enum ads_SerializedField_invariantRolesEnm
{
    ads_SerializedField_invariant_referent,
    ads_SerializedField_invariant_referrer
};

/** 
Enum with record members. */
enum ads_SolutionDependentVariableFieldTypeMembersEnm
{
    ads_SolutionDependentVariableFieldType_algebraicType,
    ads_SolutionDependentVariableFieldType_description,
    ads_SolutionDependentVariableFieldType_dsMagnitude,
    ads_SolutionDependentVariableFieldType_isExtensive,
    ads_SolutionDependentVariableFieldType_nonNegative,
    ads_SolutionDependentVariableFieldType_label,
    ads_SolutionDependentVariableFieldType_ordinal
};

enum ads_SolutionDependentVariableFieldType_algebraicTypeEnm
{
    ads_SolutionDependentVariableFieldType_algebraicType_POSITION,
    ads_SolutionDependentVariableFieldType_algebraicType_SCALAR,
    ads_SolutionDependentVariableFieldType_algebraicType_TENSOR2_GENERAL,
    ads_SolutionDependentVariableFieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_SolutionDependentVariableFieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_SolutionDependentVariableFieldType_algebraicType_TENSOR3,
    ads_SolutionDependentVariableFieldType_algebraicType_TENSOR4,
    ads_SolutionDependentVariableFieldType_algebraicType_TUPLE,
    ads_SolutionDependentVariableFieldType_algebraicType_UNDEFINED,
    ads_SolutionDependentVariableFieldType_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_StepIncMembersEnm
{
    ads_StepInc_cycle,
    ads_StepInc_endTime,
    ads_StepInc_increment,
    ads_StepInc_interval,
    ads_StepInc_iteration,
    ads_StepInc_mode,
    ads_StepInc_step,
    ads_StepInc_time
};

/** 
Enum with association roles. */
enum ads_TimeDescription_dataRolesEnm
{
    ads_TimeDescription_data_child,
    ads_TimeDescription_data_parent
};

/** 
Enum with association roles. */
enum ads_TimeDescription_timeCollectionRolesEnm
{
    ads_TimeDescription_timeCollection_child,
    ads_TimeDescription_timeCollection_parent
};

/** 
Enum with record members. */
enum ads_UserFieldExpressionMembersEnm
{
    ads_UserFieldExpression_script
};

/** Enum with association roles. */
enum ads_UserFieldExpression_orientationRolesEnm
{
    ads_UserFieldExpression_orientation_referent,
    ads_UserFieldExpression_orientation_referrer
};

/** 
Enum with record members. */
enum ads_UserFieldTypeMembersEnm
{
    ads_UserFieldType_algebraicType,
    ads_UserFieldType_description,
    ads_UserFieldType_dsMagnitude,
    ads_UserFieldType_isExtensive,
    ads_UserFieldType_nonNegative
};

enum ads_UserFieldType_algebraicTypeEnm
{
    ads_UserFieldType_algebraicType_POSITION,
    ads_UserFieldType_algebraicType_SCALAR,
    ads_UserFieldType_algebraicType_TENSOR2_GENERAL,
    ads_UserFieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_UserFieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_UserFieldType_algebraicType_TENSOR3,
    ads_UserFieldType_algebraicType_TENSOR4,
    ads_UserFieldType_algebraicType_TUPLE,
    ads_UserFieldType_algebraicType_UNDEFINED,
    ads_UserFieldType_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_UserLabelledFieldTypeMembersEnm
{
    ads_UserLabelledFieldType_algebraicType,
    ads_UserLabelledFieldType_description,
    ads_UserLabelledFieldType_dsMagnitude,
    ads_UserLabelledFieldType_isExtensive,
    ads_UserLabelledFieldType_nonNegative,
    ads_UserLabelledFieldType_label
};

enum ads_UserLabelledFieldType_algebraicTypeEnm
{
    ads_UserLabelledFieldType_algebraicType_POSITION,
    ads_UserLabelledFieldType_algebraicType_SCALAR,
    ads_UserLabelledFieldType_algebraicType_TENSOR2_GENERAL,
    ads_UserLabelledFieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_UserLabelledFieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_UserLabelledFieldType_algebraicType_TENSOR3,
    ads_UserLabelledFieldType_algebraicType_TENSOR4,
    ads_UserLabelledFieldType_algebraicType_TUPLE,
    ads_UserLabelledFieldType_algebraicType_UNDEFINED,
    ads_UserLabelledFieldType_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_UserOutputVariableFieldTypeMembersEnm
{
    ads_UserOutputVariableFieldType_algebraicType,
    ads_UserOutputVariableFieldType_description,
    ads_UserOutputVariableFieldType_dsMagnitude,
    ads_UserOutputVariableFieldType_isExtensive,
    ads_UserOutputVariableFieldType_nonNegative,
    ads_UserOutputVariableFieldType_ordinal
};

enum ads_UserOutputVariableFieldType_algebraicTypeEnm
{
    ads_UserOutputVariableFieldType_algebraicType_POSITION,
    ads_UserOutputVariableFieldType_algebraicType_SCALAR,
    ads_UserOutputVariableFieldType_algebraicType_TENSOR2_GENERAL,
    ads_UserOutputVariableFieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_UserOutputVariableFieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_UserOutputVariableFieldType_algebraicType_TENSOR3,
    ads_UserOutputVariableFieldType_algebraicType_TENSOR4,
    ads_UserOutputVariableFieldType_algebraicType_TUPLE,
    ads_UserOutputVariableFieldType_algebraicType_UNDEFINED,
    ads_UserOutputVariableFieldType_algebraicType_VECTOR
};

/** 
Enum with record members. */
enum ads_UserPredefinedVariableFieldTypeMembersEnm
{
    ads_UserPredefinedVariableFieldType_algebraicType,
    ads_UserPredefinedVariableFieldType_description,
    ads_UserPredefinedVariableFieldType_dsMagnitude,
    ads_UserPredefinedVariableFieldType_isExtensive,
    ads_UserPredefinedVariableFieldType_nonNegative,
    ads_UserPredefinedVariableFieldType_ordinal
};

enum ads_UserPredefinedVariableFieldType_algebraicTypeEnm
{
    ads_UserPredefinedVariableFieldType_algebraicType_POSITION,
    ads_UserPredefinedVariableFieldType_algebraicType_SCALAR,
    ads_UserPredefinedVariableFieldType_algebraicType_TENSOR2_GENERAL,
    ads_UserPredefinedVariableFieldType_algebraicType_TENSOR2_ORTHOGONAL,
    ads_UserPredefinedVariableFieldType_algebraicType_TENSOR2_SYMMETRIC,
    ads_UserPredefinedVariableFieldType_algebraicType_TENSOR3,
    ads_UserPredefinedVariableFieldType_algebraicType_TENSOR4,
    ads_UserPredefinedVariableFieldType_algebraicType_TUPLE,
    ads_UserPredefinedVariableFieldType_algebraicType_UNDEFINED,
    ads_UserPredefinedVariableFieldType_algebraicType_VECTOR
};

/** 
Enum with grid dimensions. */
enum ads_UserSubroutineParameterGridDimensionsEnm
{
    ads_UserSubroutineParameterGrid_parameter
};

/** 
Enum with association roles. */
enum ads_UserSubroutine_argumentsRolesEnm
{
    ads_UserSubroutine_arguments_child,
    ads_UserSubroutine_arguments_parent
};

/** 
Enum with association roles. */
enum ads_XData_valuesRolesEnm
{
    ads_XData_values_child,
    ads_XData_values_parent
};

/** 
Enum with record members. */
enum ads_XYDataSetMembersEnm
{
    ads_XYDataSet_contentDescription,
    ads_XYDataSet_legendLabel,
    ads_XYDataSet_positionDescription,
    ads_XYDataSet_sourceDescription,
    ads_XYDataSet_sourceFileName,
    ads_XYDataSet_sourceType,
    ads_XYDataSet_xAxisLabel,
    ads_XYDataSet_yAxisLabel
};

enum ads_XYDataSet_sourceTypeEnm
{
    ads_XYDataSet_sourceType_FROM_ASCII_FILE,
    ads_XYDataSet_sourceType_FROM_KEYBOARD,
    ads_XYDataSet_sourceType_FROM_ODB,
    ads_XYDataSet_sourceType_FROM_OPERATION,
    ads_XYDataSet_sourceType_FROM_USER_DEFINED,
    ads_XYDataSet_sourceType_UNDEFINED
};

/** 
Enum with association roles. */
enum ads_XYDataSet_xDataRolesEnm
{
    ads_XYDataSet_xData_child,
    ads_XYDataSet_xData_parent
};

#endif
