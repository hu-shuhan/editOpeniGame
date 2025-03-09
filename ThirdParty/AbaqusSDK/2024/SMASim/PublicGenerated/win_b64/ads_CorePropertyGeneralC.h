//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyGeneralC_h
#define ads_CorePropertyGeneralC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyGeneral of the latest level of form Core */

#define ads_BGeneralMassDensityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 0))

#define ads_BGeneralMassFluidTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 1))

#define ads_CGeneralConstitutiveReferenceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 2))

#define ads_GeneralUserPropertiesTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 3))

#define ads_IGeneralGeometryAreaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 4))

#define ads_IGeneralGeometryWidthTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 5))

#define ads_MGeneralMassDensityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 6))

#define ads_MGeneralMolarMassTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 7))

#define ads_MGeneralSaturationPressureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 8))

#define ads_MGeneralSurfaceTensionCoefficientTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 9))

#define ads_MMecClusterMassInertiaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 10))

#define ads_MMecElongationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 11))

#define ads_MMecProofYieldStressTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 12))

#define ads_MMecUltimateCompressiveStrengthTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 13))

#define ads_MMecUltimateStrengthTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 14))

#define ads_MMecUltimateTensileStrengthTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 15))

#define ads_Prop_BGeneral_Geometry (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 16))

#define ads_Prop_BGeneral_Mass (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 17))

#define ads_Prop_BGeneral_Mass_Density (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 18))

#define ads_Prop_BGeneral_Mass_Density_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 19))

#define ads_Prop_BGeneral_Mass_Fluid (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 20))

#define ads_Prop_BGeneral_Mass_Fluid_centerOfMassOffset (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 21))

#define ads_Prop_BGeneral_Mass_Fluid_cylindricalRadius (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 22))

#define ads_Prop_BGeneral_Mass_Fluid_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 23))

#define ads_Prop_CGeneral_ConstitutiveReference (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 24))

#define ads_Prop_CGeneral_ConstitutiveReference_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 25))

#define ads_Prop_General_UserDefinedField (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 26))

/** Record to capture user defined property tables and parameter tables. */
#define ads_Prop_General_UserMatProperties (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 27))

#define ads_Prop_General_UserMatProperties_parameterTables (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 28))

#define ads_Prop_General_UserMatProperties_propertyTables (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 29))

/** Define material constants for use in user-defined mechanical model or thermal model. */
#define ads_Prop_General_UserProperties (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 30))

#define ads_Prop_General_UserProperties_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 31))

#define ads_Prop_IGeneral_ButlerVolmer (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 32))

/** This is a temporary property to hold undesigned contact schema. */
#define ads_Prop_IGeneral_Contact (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 33))

#define ads_Prop_IGeneral_Contact_parameterTables (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 34))

#define ads_Prop_IGeneral_Contact_propertyTables (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 35))

#define ads_Prop_IGeneral_Geometry (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 36))

/** Specify a gasket contact area for average pressure output. */
#define ads_Prop_IGeneral_Geometry_Area (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 37))

#define ads_Prop_IGeneral_Geometry_Area_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 38))

/** Specify a gasket contact width for average pressure output. */
#define ads_Prop_IGeneral_Geometry_Width (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 39))

#define ads_Prop_IGeneral_Geometry_Width_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 40))

#define ads_Prop_MGeneral_FiberDispersion (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 41))

/** A reference to the orientation tensor field */
#define ads_Prop_MGeneral_FiberDispersion_oritensField (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 42))

#define ads_Prop_MGeneral_Mass (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 43))

/** Mass density record */
#define ads_Prop_MGeneral_Mass_Density (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 44))

#define ads_Prop_MGeneral_Mass_Density_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 45))

#define ads_Prop_MGeneral_MolarMass (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 46))

#define ads_Prop_MGeneral_MolarMass_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 47))

#define ads_Prop_MGeneral_SaturationPressure (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 48))

#define ads_Prop_MGeneral_SaturationPressure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 49))

#define ads_Prop_MGeneral_SurfaceTensionCoefficient (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 50))

#define ads_Prop_MGeneral_SurfaceTensionCoefficient_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 51))

#define ads_Prop_MMec_ClusterMassInertia (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 52))

#define ads_Prop_MMec_ClusterMassInertia_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 53))

/** Elongation % at fracture. % gauge length change */
#define ads_Prop_MMec_Elongation (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 54))

#define ads_Prop_MMec_Elongation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 55))

/** Only to be used for 6th Edition of the FKM material type designation. A new datatype Prop_MMec_Fatigue_FKMMaterialType for 7th Edition is in the PropertyFatigue fragment. */
#define ads_Prop_MMec_MaterialType (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 56))

/** 0.2% proof (yield) stress */
#define ads_Prop_MMec_ProofYieldStress (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 57))

#define ads_Prop_MMec_ProofYieldStress_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 58))

/** Ultimate Tensile Stress (UTS) and optional Ultimate Compressive Stress (UCS). Defined in terms of engineering/nominal stresses. */
#define ads_Prop_MMec_UltimateStrength (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 59))

#define ads_Prop_MMec_UltimateStrength_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 60))

#define ads_Prop_MMec_UltimateStrength_uCsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 61))

#define ads_Prop_MMec_UltimateStrength_uTsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 62))

#define ads_Prop_MMec_User (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 63))

#define ads_Prop_SGeneral_Mass (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 64))

#define ads_Prop_SGeneral_Mass_Density (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 65))

#define ads_Prop_SGeneral_Mass_Density_table (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 66))

#define ads_Prop_SMec_User (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 67))

#define ads_SGeneralMassDensityTable (ads_CoreFragmentTypeIndex(ads_CorePropertyGeneralFragment, 68))

/** Enum with record members. */
enum ads_Prop_BGeneral_GeometryMembersEnm
{
    ads_Prop_BGeneral_Geometry_nonlinearGeneral
};

/** Enum with association roles. */
enum ads_Prop_BGeneral_Mass_Density_tableRolesEnm
{
    ads_Prop_BGeneral_Mass_Density_table_child,
    ads_Prop_BGeneral_Mass_Density_table_parent
};

/** Enum with record members. */
enum ads_Prop_BGeneral_Mass_FluidMembersEnm
{
    ads_Prop_BGeneral_Mass_Fluid_submerged
};

enum ads_Prop_BGeneral_Mass_Fluid_submergedEnm
{
    ads_Prop_BGeneral_Mass_Fluid_submerged_FULL,
    ads_Prop_BGeneral_Mass_Fluid_submerged_HALF
};

/** Enum with association roles. */
enum ads_Prop_BGeneral_Mass_Fluid_centerOfMassOffsetRolesEnm
{
    ads_Prop_BGeneral_Mass_Fluid_centerOfMassOffset_child,
    ads_Prop_BGeneral_Mass_Fluid_centerOfMassOffset_parent
};

/** Enum with association roles. */
enum ads_Prop_BGeneral_Mass_Fluid_cylindricalRadiusRolesEnm
{
    ads_Prop_BGeneral_Mass_Fluid_cylindricalRadius_child,
    ads_Prop_BGeneral_Mass_Fluid_cylindricalRadius_parent
};

/** Enum with association roles. */
enum ads_Prop_BGeneral_Mass_Fluid_tableRolesEnm
{
    ads_Prop_BGeneral_Mass_Fluid_table_child,
    ads_Prop_BGeneral_Mass_Fluid_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CGeneral_ConstitutiveReference_tableRolesEnm
{
    ads_Prop_CGeneral_ConstitutiveReference_table_child,
    ads_Prop_CGeneral_ConstitutiveReference_table_parent
};

/** Enum with association roles. */
enum ads_Prop_General_UserMatProperties_parameterTablesRolesEnm
{
    ads_Prop_General_UserMatProperties_parameterTables_child,
    ads_Prop_General_UserMatProperties_parameterTables_parent
};

/** Enum with association roles. */
enum ads_Prop_General_UserMatProperties_propertyTablesRolesEnm
{
    ads_Prop_General_UserMatProperties_propertyTables_child,
    ads_Prop_General_UserMatProperties_propertyTables_parent
};

/** Enum with association roles. */
enum ads_Prop_General_UserProperties_tableRolesEnm
{
    ads_Prop_General_UserProperties_table_child,
    ads_Prop_General_UserProperties_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IGeneral_ContactMembersEnm
{
    ads_Prop_IGeneral_Contact_outOfPlaneThickness
};

/** Enum with association roles. */
enum ads_Prop_IGeneral_Contact_parameterTablesRolesEnm
{
    ads_Prop_IGeneral_Contact_parameterTables_child,
    ads_Prop_IGeneral_Contact_parameterTables_parent
};

/** Enum with association roles. */
enum ads_Prop_IGeneral_Contact_propertyTablesRolesEnm
{
    ads_Prop_IGeneral_Contact_propertyTables_child,
    ads_Prop_IGeneral_Contact_propertyTables_parent
};

/** Enum with association roles. */
enum ads_Prop_IGeneral_Geometry_Area_tableRolesEnm
{
    ads_Prop_IGeneral_Geometry_Area_table_child,
    ads_Prop_IGeneral_Geometry_Area_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IGeneral_Geometry_Width_tableRolesEnm
{
    ads_Prop_IGeneral_Geometry_Width_table_child,
    ads_Prop_IGeneral_Geometry_Width_table_parent
};

/** Enum with record members. */
enum ads_Prop_MGeneral_FiberDispersionMembersEnm
{
    ads_Prop_MGeneral_FiberDispersion_closure
};

enum ads_Prop_MGeneral_FiberDispersion_closureEnm
{
    ads_Prop_MGeneral_FiberDispersion_closure_FITTED,
    ads_Prop_MGeneral_FiberDispersion_closure_HYBRID,
    ads_Prop_MGeneral_FiberDispersion_closure_QUADRATIC,
    ads_Prop_MGeneral_FiberDispersion_closure_SMOOTH
};

/** 
Enum with association roles. */
enum ads_Prop_MGeneral_FiberDispersion_oritensFieldRolesEnm
{
    ads_Prop_MGeneral_FiberDispersion_oritensField_referent,
    ads_Prop_MGeneral_FiberDispersion_oritensField_referrer
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_Mass_Density_tableRolesEnm
{
    ads_Prop_MGeneral_Mass_Density_table_child,
    ads_Prop_MGeneral_Mass_Density_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_MolarMass_tableRolesEnm
{
    ads_Prop_MGeneral_MolarMass_table_child,
    ads_Prop_MGeneral_MolarMass_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_SaturationPressure_tableRolesEnm
{
    ads_Prop_MGeneral_SaturationPressure_table_child,
    ads_Prop_MGeneral_SaturationPressure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MGeneral_SurfaceTensionCoefficient_tableRolesEnm
{
    ads_Prop_MGeneral_SurfaceTensionCoefficient_table_child,
    ads_Prop_MGeneral_SurfaceTensionCoefficient_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ClusterMassInertia_tableRolesEnm
{
    ads_Prop_MMec_ClusterMassInertia_table_child,
    ads_Prop_MMec_ClusterMassInertia_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elongation_tableRolesEnm
{
    ads_Prop_MMec_Elongation_table_child,
    ads_Prop_MMec_Elongation_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_MaterialTypeMembersEnm
{
    ads_Prop_MMec_MaterialType_fkmGJLType,
    ads_Prop_MMec_MaterialType_fkmMatType,
    ads_Prop_MMec_MaterialType_fkmWeldSofteningFactor
};

enum ads_Prop_MMec_MaterialType_fkmGJLTypeEnm
{
    ads_Prop_MMec_MaterialType_fkmGJLType_100,
    ads_Prop_MMec_MaterialType_fkmGJLType_150,
    ads_Prop_MMec_MaterialType_fkmGJLType_200,
    ads_Prop_MMec_MaterialType_fkmGJLType_250,
    ads_Prop_MMec_MaterialType_fkmGJLType_300,
    ads_Prop_MMec_MaterialType_fkmGJLType_350
};

enum ads_Prop_MMec_MaterialType_fkmMatTypeEnm
{
    ads_Prop_MMec_MaterialType_fkmMatType_CASEHARDENINGSTEELBH_DINEN10084,
    ads_Prop_MMec_MaterialType_fkmMatType_CAST_ALUMINUM_ALLOY,
    ads_Prop_MMec_MaterialType_fkmMatType_FINEGRAINSTRUCTSTEEL_DIN10113,
    ads_Prop_MMec_MaterialType_fkmMatType_FINEGRAINSTRUCTSTEEL_DIN17102,
    ads_Prop_MMec_MaterialType_fkmMatType_GJL_LAMELLARGRAPHITECASTIRON_DINEN1561,
    ads_Prop_MMec_MaterialType_fkmMatType_GJM_MALLEABLECASTIRON_DINEN1562,
    ads_Prop_MMec_MaterialType_fkmMatType_GJS_NODULARCASTIRON_DINEN1563,
    ads_Prop_MMec_MaterialType_fkmMatType_GS_CASTSTEEL_DINEN10293,
    ads_Prop_MMec_MaterialType_fkmMatType_GS_HEATTREATABLECASTSTEEL_DINEN10293,
    ads_Prop_MMec_MaterialType_fkmMatType_HEATTREATABLESTEELN_DINEN10083_1,
    ads_Prop_MMec_MaterialType_fkmMatType_HEATTREATABLESTEELQT_DINEN10083_1,
    ads_Prop_MMec_MaterialType_fkmMatType_LARGERFORGINGSSTEELN_SEW550,
    ads_Prop_MMec_MaterialType_fkmMatType_LARGERFORGINGSSTEELQT_SEW550,
    ads_Prop_MMec_MaterialType_fkmMatType_NITRIDINGSTEELQT_DINEN10085,
    ads_Prop_MMec_MaterialType_fkmMatType_NONALLOYEDSS_DINEN10025,
    ads_Prop_MMec_MaterialType_fkmMatType_NONE,
    ads_Prop_MMec_MaterialType_fkmMatType_STAINLESSSTEEL_DINEN10088_2,
    ads_Prop_MMec_MaterialType_fkmMatType_WROUGHT_ALUMINUM_ALLOY
};

/** Enum with association roles. */
enum ads_Prop_MMec_ProofYieldStress_tableRolesEnm
{
    ads_Prop_MMec_ProofYieldStress_table_child,
    ads_Prop_MMec_ProofYieldStress_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_UltimateStrength_tableRolesEnm
{
    ads_Prop_MMec_UltimateStrength_table_child,
    ads_Prop_MMec_UltimateStrength_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_UltimateStrength_uCsTableRolesEnm
{
    ads_Prop_MMec_UltimateStrength_uCsTable_child,
    ads_Prop_MMec_UltimateStrength_uCsTable_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_UltimateStrength_uTsTableRolesEnm
{
    ads_Prop_MMec_UltimateStrength_uTsTable_child,
    ads_Prop_MMec_UltimateStrength_uTsTable_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_UserMembersEnm
{
    ads_Prop_MMec_User_hybridFormulation,
    ads_Prop_MMec_User_unsymm
};

enum ads_Prop_MMec_User_hybridFormulationEnm
{
    ads_Prop_MMec_User_hybridFormulation_INCOMPRESSIBLE,
    ads_Prop_MMec_User_hybridFormulation_INCREMENTAL,
    ads_Prop_MMec_User_hybridFormulation_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_SGeneral_Mass_Density_tableRolesEnm
{
    ads_Prop_SGeneral_Mass_Density_table_child,
    ads_Prop_SGeneral_Mass_Density_table_parent
};

/** Enum with record members. */
enum ads_Prop_SMec_UserMembersEnm
{
    ads_Prop_SMec_User_numIProperties,
    ads_Prop_SMec_User_numProperties,
    ads_Prop_SMec_User_numVariables,
    ads_Prop_SMec_User_unsymm
};

#endif
