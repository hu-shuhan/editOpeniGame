//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreMaterialLibraryC_h
#define ads_CoreMaterialLibraryC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment MaterialLibrary of the latest level of form Core */

/** Define the constituent. */
#define ads_Constituent (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 0))

/** A reference to the aspect ratio field */
#define ads_Constituent_arField (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 1))

#define ads_Constituent_isoparamtable (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 2))

/** Material that defines the constituent material. */
#define ads_Constituent_material (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 3))

/** A reference to the orientation tensor field */
#define ads_Constituent_oritensField (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 4))

#define ads_Constituent_properties (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 5))

/** A reference to the volume fraction field */
#define ads_Constituent_volfracField (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 6))

/** MaterialMechanism collection. */
#define ads_GlobalCollections_materialMechanismCollection (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 7))

#define ads_MGeneralConcentrationTensorTable (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 8))

#define ads_MGeneralConcentrationTensorThermTable (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 9))

#define ads_MGeneralIsotropizationParamsTable (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 10))

/** A central record for associating material properites. */
#define ads_Material (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 11))

/** A record for identifying material 'mechanisms', such as prony-series mechanisms of kinematic hardening mechanisms. These will be essentially only c-members (the data-type itself may not be used). Material mechanisms are a counting-collection. Their namespace is the material to which they apply. For example, let m1, m2, m3 be the first three c-members of the MaterialMechanisms collection. MaterialA, say a material with prony-series viscosity, could have two prony-series mechanisms; these will be m1 and m2 for this material. MaterialB may have 3 kinematic hardening mechanisms, which would be identified as m1, m2, m3. So a (material, mechanism c-member) pair identifies a mechanism; a mechanism c-member alone does not. If a MaterialMechanism collection is part of a grid, the Material collection should also be part of the grid. */
#define ads_MaterialMechanism (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 12))

/** A collection of materials mechanisms. */
#define ads_MaterialMechanismCollection (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 13))

/** A central record for associating gasket properites. */
#define ads_Material_GasketBehavior (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 14))

#define ads_Material_constituents (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 15))

#define ads_Material_parameterTables (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 16))

#define ads_Material_properties (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 17))

#define ads_Material_propertyTables (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 18))

#define ads_Material_regularization (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 19))

#define ads_Material_sdv (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 20))

/** Reference to Depvar. */
#define ads_Material_sdvUsedForActivate (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 21))

/** Reference to Depvar. */
#define ads_Material_sdvUsedForDelete (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 22))

#define ads_Model_materials (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 23))

/** Specify concentration tensor for strain partitioning. */
#define ads_Prop_MGeneral_Constituent_ConcentrationTensor (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 24))

/** Specify concentration tensor for conductivity. */
#define ads_Prop_MGeneral_Constituent_ConcentrationTensorTherm (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 25))

#define ads_Prop_MGeneral_Constituent_ConcentrationTensorTherm_table (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 26))

#define ads_Prop_MGeneral_Constituent_ConcentrationTensor_table (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 27))

/** Abstraction of the mean field damage model. */
#define ads_Prop_MGeneral_MeanFieldDamage (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 28))

/** Abstraction of the mean field homogenization model. */
#define ads_Prop_MGeneral_MeanFieldHomogenization (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 29))

/** Adds regularization information to the material */
#define ads_Regularization (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 30))

#define ads_Section_sdv (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 31))

/** Reference to Element solution dependent state variable . */
#define ads_Section_sdvUsedForActivate (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 32))

/** Reference to Element solution dependent state variable . */
#define ads_Section_sdvUsedForDelete (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 33))

/** Specify solution-dependent state variables. */
#define ads_SolutionDependentStateVariable (ads_CoreFragmentTypeIndex(ads_CoreMaterialLibraryFragment, 34))

/** 
Enum with record members. */
enum ads_ConstituentMembersEnm
{
    ads_Constituent_a11,
    ads_Constituent_a12,
    ads_Constituent_a13,
    ads_Constituent_a22,
    ads_Constituent_a23,
    ads_Constituent_a33,
    ads_Constituent_ar,
    ads_Constituent_directionEnm,
    ads_Constituent_isotropizationCoefficient,
    ads_Constituent_responseEnm,
    ads_Constituent_shapeEnm,
    ads_Constituent_typeEnm,
    ads_Constituent_v1,
    ads_Constituent_v2,
    ads_Constituent_v3,
    ads_Constituent_volfrac
};

enum ads_Constituent_directionEnmEnm
{
    ads_Constituent_directionEnm_FIXED,
    ads_Constituent_directionEnm_ORIENTATIONTENSOR,
    ads_Constituent_directionEnm_RANDOM3D
};

enum ads_Constituent_responseEnmEnm
{
    ads_Constituent_responseEnm_AVERAGE,
    ads_Constituent_responseEnm_GRANULAR
};

enum ads_Constituent_shapeEnmEnm
{
    ads_Constituent_shapeEnm_CYLINDER,
    ads_Constituent_shapeEnm_ELLIPTICCYLINDER,
    ads_Constituent_shapeEnm_OBLATE,
    ads_Constituent_shapeEnm_PENNY,
    ads_Constituent_shapeEnm_PROLATE,
    ads_Constituent_shapeEnm_SPHERE
};

enum ads_Constituent_typeEnmEnm
{
    ads_Constituent_typeEnm_INCLUSION,
    ads_Constituent_typeEnm_MATRIX,
    ads_Constituent_typeEnm_VOID
};

/** 
Enum with association roles. */
enum ads_Constituent_arFieldRolesEnm
{
    ads_Constituent_arField_referent,
    ads_Constituent_arField_referrer
};

/** Enum with association roles. */
enum ads_Constituent_isoparamtableRolesEnm
{
    ads_Constituent_isoparamtable_child,
    ads_Constituent_isoparamtable_parent
};

/** 
Enum with association roles. */
enum ads_Constituent_materialRolesEnm
{
    ads_Constituent_material_referent,
    ads_Constituent_material_referrer
};

/** 
Enum with association roles. */
enum ads_Constituent_oritensFieldRolesEnm
{
    ads_Constituent_oritensField_referent,
    ads_Constituent_oritensField_referrer
};

/** Enum with association roles. */
enum ads_Constituent_propertiesRolesEnm
{
    ads_Constituent_properties_child,
    ads_Constituent_properties_parent
};

/** 
Enum with association roles. */
enum ads_Constituent_volfracFieldRolesEnm
{
    ads_Constituent_volfracField_referent,
    ads_Constituent_volfracField_referrer
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_materialMechanismCollectionRolesEnm
{
    ads_GlobalCollections_materialMechanismCollection_child,
    ads_GlobalCollections_materialMechanismCollection_parent
};

/** 
Enum with record members. */
enum ads_MaterialMembersEnm
{
    ads_Material_externalMaterialID,
    ads_Material_numLocalDirections,
    ads_Material_numUserOutputVariables
};

/** 
Enum with record members. */
enum ads_Material_GasketBehaviorMembersEnm
{
    ads_Material_GasketBehavior_externalMaterialID,
    ads_Material_GasketBehavior_numLocalDirections,
    ads_Material_GasketBehavior_numUserOutputVariables
};

/** Enum with association roles. */
enum ads_Material_constituentsRolesEnm
{
    ads_Material_constituents_child,
    ads_Material_constituents_parent
};

/** Enum with association roles. */
enum ads_Material_parameterTablesRolesEnm
{
    ads_Material_parameterTables_child,
    ads_Material_parameterTables_parent
};

/** Enum with association roles. */
enum ads_Material_propertiesRolesEnm
{
    ads_Material_properties_child,
    ads_Material_properties_parent
};

/** Enum with association roles. */
enum ads_Material_propertyTablesRolesEnm
{
    ads_Material_propertyTables_child,
    ads_Material_propertyTables_parent
};

/** Enum with association roles. */
enum ads_Material_regularizationRolesEnm
{
    ads_Material_regularization_child,
    ads_Material_regularization_parent
};

/** Enum with association roles. */
enum ads_Material_sdvRolesEnm
{
    ads_Material_sdv_child,
    ads_Material_sdv_parent
};

/** 
Enum with association roles. */
enum ads_Material_sdvUsedForActivateRolesEnm
{
    ads_Material_sdvUsedForActivate_referent,
    ads_Material_sdvUsedForActivate_referrer
};

/** 
Enum with association roles. */
enum ads_Material_sdvUsedForDeleteRolesEnm
{
    ads_Material_sdvUsedForDelete_referent,
    ads_Material_sdvUsedForDelete_referrer
};

/** Enum with association roles. */
enum ads_Model_materialsRolesEnm
{
    ads_Model_materials_child,
    ads_Model_materials_parent
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_Constituent_ConcentrationTensorTherm_tableRolesEnm
{
    ads_Prop_MGeneral_Constituent_ConcentrationTensorTherm_table_child,
    ads_Prop_MGeneral_Constituent_ConcentrationTensorTherm_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_Constituent_ConcentrationTensor_tableRolesEnm
{
    ads_Prop_MGeneral_Constituent_ConcentrationTensor_table_child,
    ads_Prop_MGeneral_Constituent_ConcentrationTensor_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MGeneral_MeanFieldDamageMembersEnm
{
    ads_Prop_MGeneral_MeanFieldDamage_degradationEnm
};

enum ads_Prop_MGeneral_MeanFieldDamage_degradationEnmEnm
{
    ads_Prop_MGeneral_MeanFieldDamage_degradationEnm_MAXIMUM,
    ads_Prop_MGeneral_MeanFieldDamage_degradationEnm_MULTIPLICATIVE,
    ads_Prop_MGeneral_MeanFieldDamage_degradationEnm_USER
};

/** 
Enum with record members. */
enum ads_Prop_MGeneral_MeanFieldHomogenizationMembersEnm
{
    ads_Prop_MGeneral_MeanFieldHomogenization_angleSubdivision,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm,
    ads_Prop_MGeneral_MeanFieldHomogenization_isotropizationEnm,
    ads_Prop_MGeneral_MeanFieldHomogenization_uniformmatrixstrain
};

enum ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnmEnm
{
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_BALANCED,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_CCA,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_CHAMIS,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_INVERSEDMT,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_MT,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_REUSS,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_UNSPECIFIED,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_USER,
    ads_Prop_MGeneral_MeanFieldHomogenization_formulationEnm_VOIGT
};

enum ads_Prop_MGeneral_MeanFieldHomogenization_isotropizationEnmEnm
{
    ads_Prop_MGeneral_MeanFieldHomogenization_isotropizationEnm_ALLISO,
    ads_Prop_MGeneral_MeanFieldHomogenization_isotropizationEnm_EISO,
    ads_Prop_MGeneral_MeanFieldHomogenization_isotropizationEnm_PISO
};

/** 
Enum with record members. */
enum ads_RegularizationMembersEnm
{
    ads_Regularization_rtol,
    ads_Regularization_sRateFactor,
    ads_Regularization_strainRateRegularization
};

enum ads_Regularization_strainRateRegularizationEnm
{
    ads_Regularization_strainRateRegularization_LINEAR,
    ads_Regularization_strainRateRegularization_LOGARITHMIC
};

/** Enum with association roles. */
enum ads_Section_sdvRolesEnm
{
    ads_Section_sdv_child,
    ads_Section_sdv_parent
};

/** 
Enum with association roles. */
enum ads_Section_sdvUsedForActivateRolesEnm
{
    ads_Section_sdvUsedForActivate_referent,
    ads_Section_sdvUsedForActivate_referrer
};

/** 
Enum with association roles. */
enum ads_Section_sdvUsedForDeleteRolesEnm
{
    ads_Section_sdvUsedForDelete_referent,
    ads_Section_sdvUsedForDelete_referrer
};

/** 
Enum with record members. */
enum ads_SolutionDependentStateVariableMembersEnm
{
    ads_SolutionDependentStateVariable_variableDescription,
    ads_SolutionDependentStateVariable_variableLabel
};

#endif
