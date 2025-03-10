//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyUniaxialC_h
#define ads_CorePropertyUniaxialC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyUniaxial of the latest level of form Core */

#define ads_BMecUniaxialAxialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 0))

#define ads_BMecUniaxialRotationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 1))

#define ads_CMecUniaxialOptionRotationalUnloadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 2))

#define ads_CMecUniaxialOptionTranslationalUnloadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 3))

#define ads_CMecUniaxialRotationalLoadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 4))

#define ads_CMecUniaxialTranslationalLoadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 5))

#define ads_EMecUniaxialLoadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 6))

#define ads_EMecUniaxialOptionUnloadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 7))

#define ads_IMecUniaxialLoadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 8))

#define ads_IMecUniaxialOptionUnloadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 9))

#define ads_MMecUniaxialOptionDissipationTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 10))

#define ads_MMecUniaxialOptionUnloadingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 11))

#define ads_Prop_BMec_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 12))

#define ads_Prop_BMec_Uniaxial_Axial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 13))

#define ads_Prop_BMec_Uniaxial_Axial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 14))

#define ads_Prop_BMec_Uniaxial_Rotational (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 15))

#define ads_Prop_BMec_Uniaxial_Rotational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 16))

#define ads_Prop_CMec_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 17))

#define ads_Prop_CMec_UniaxialOption (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 18))

#define ads_Prop_CMec_UniaxialOption_Dissipation (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 19))

#define ads_Prop_CMec_UniaxialOption_RotationalUnloading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 20))

#define ads_Prop_CMec_UniaxialOption_RotationalUnloading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 21))

/** Specify a gasket thickness-direction behavior. */
#define ads_Prop_CMec_UniaxialOption_TranslationalUnloading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 22))

#define ads_Prop_CMec_UniaxialOption_TranslationalUnloading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 23))

#define ads_Prop_CMec_Uniaxial_RotationalLoading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 24))

#define ads_Prop_CMec_Uniaxial_RotationalLoading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 25))

/** Specify a gasket thickness-direction behavior. */
#define ads_Prop_CMec_Uniaxial_TranslationalLoading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 26))

#define ads_Prop_CMec_Uniaxial_TranslationalLoading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 27))

#define ads_Prop_EMec_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 28))

#define ads_Prop_EMec_UniaxialOption (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 29))

#define ads_Prop_EMec_UniaxialOption_Unloading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 30))

#define ads_Prop_EMec_UniaxialOption_Unloading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 31))

#define ads_Prop_EMec_Uniaxial_Loading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 32))

#define ads_Prop_EMec_Uniaxial_Loading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 33))

#define ads_Prop_IMec_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 34))

#define ads_Prop_IMec_UniaxialOption (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 35))

/** Specify a gasket thickness-direction behavior. */
#define ads_Prop_IMec_UniaxialOption_Unloading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 36))

#define ads_Prop_IMec_UniaxialOption_Unloading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 37))

/** Specify a gasket thickness-direction behavior. */
#define ads_Prop_IMec_Uniaxial_Loading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 38))

#define ads_Prop_IMec_Uniaxial_Loading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 39))

#define ads_Prop_MMec_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 40))

#define ads_Prop_MMec_UniaxialOption (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 41))

#define ads_Prop_MMec_UniaxialOption_Dissipation (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 42))

#define ads_Prop_MMec_UniaxialOption_Dissipation_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 43))

#define ads_Prop_MMec_UniaxialOption_Unloading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 44))

#define ads_Prop_MMec_UniaxialOption_Unloading_table (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 45))

#define ads_Prop_MMec_Uniaxial_Loading (ads_CoreFragmentTypeIndex(ads_CorePropertyUniaxialFragment, 46))

/** Enum with record members. */
enum ads_Prop_BMec_Uniaxial_AxialMembersEnm
{
    ads_Prop_BMec_Uniaxial_Axial_type
};

enum ads_Prop_BMec_Uniaxial_Axial_typeEnm
{
    ads_Prop_BMec_Uniaxial_Axial_type_ELASTIC,
    ads_Prop_BMec_Uniaxial_Axial_type_PERMANANT_DEFORMATION
};

/** Enum with association roles. */
enum ads_Prop_BMec_Uniaxial_Axial_tableRolesEnm
{
    ads_Prop_BMec_Uniaxial_Axial_table_child,
    ads_Prop_BMec_Uniaxial_Axial_table_parent
};

/** Enum with record members. */
enum ads_Prop_BMec_Uniaxial_RotationalMembersEnm
{
    ads_Prop_BMec_Uniaxial_Rotational_component,
    ads_Prop_BMec_Uniaxial_Rotational_type
};

enum ads_Prop_BMec_Uniaxial_Rotational_componentEnm
{
    ads_Prop_BMec_Uniaxial_Rotational_component_M1,
    ads_Prop_BMec_Uniaxial_Rotational_component_M2,
    ads_Prop_BMec_Uniaxial_Rotational_component_TORQUE
};

enum ads_Prop_BMec_Uniaxial_Rotational_typeEnm
{
    ads_Prop_BMec_Uniaxial_Rotational_type_ELASTIC,
    ads_Prop_BMec_Uniaxial_Rotational_type_PERMANANT_DEFORMATION
};

/** Enum with association roles. */
enum ads_Prop_BMec_Uniaxial_Rotational_tableRolesEnm
{
    ads_Prop_BMec_Uniaxial_Rotational_table_child,
    ads_Prop_BMec_Uniaxial_Rotational_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_UniaxialOption_RotationalUnloading_tableRolesEnm
{
    ads_Prop_CMec_UniaxialOption_RotationalUnloading_table_child,
    ads_Prop_CMec_UniaxialOption_RotationalUnloading_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_UniaxialOption_TranslationalUnloading_tableRolesEnm
{
    ads_Prop_CMec_UniaxialOption_TranslationalUnloading_table_child,
    ads_Prop_CMec_UniaxialOption_TranslationalUnloading_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Uniaxial_RotationalLoading_tableRolesEnm
{
    ads_Prop_CMec_Uniaxial_RotationalLoading_table_child,
    ads_Prop_CMec_Uniaxial_RotationalLoading_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_Uniaxial_TranslationalLoadingMembersEnm
{
    ads_Prop_CMec_Uniaxial_TranslationalLoading_slopeDrop,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_tensileStiffnessFactor,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_type,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnset,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnsetMethod
};

enum ads_Prop_CMec_Uniaxial_TranslationalLoading_typeEnm
{
    ads_Prop_CMec_Uniaxial_TranslationalLoading_type_DAMAGE,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_type_ELASTIC_PLASTIC
};

enum ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnsetMethodEnm
{
    ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnsetMethod_CLOSURE_VALUE,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnsetMethod_DEFAULT,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_yieldOnsetMethod_SLOPE_DROP
};

/** Enum with association roles. */
enum ads_Prop_CMec_Uniaxial_TranslationalLoading_tableRolesEnm
{
    ads_Prop_CMec_Uniaxial_TranslationalLoading_table_child,
    ads_Prop_CMec_Uniaxial_TranslationalLoading_table_parent
};

/** Enum with association roles. */
enum ads_Prop_EMec_UniaxialOption_Unloading_tableRolesEnm
{
    ads_Prop_EMec_UniaxialOption_Unloading_table_child,
    ads_Prop_EMec_UniaxialOption_Unloading_table_parent
};

/** Enum with record members. */
enum ads_Prop_EMec_Uniaxial_LoadingMembersEnm
{
    ads_Prop_EMec_Uniaxial_Loading_slopeDrop,
    ads_Prop_EMec_Uniaxial_Loading_tensileStiffnessFactor,
    ads_Prop_EMec_Uniaxial_Loading_type,
    ads_Prop_EMec_Uniaxial_Loading_yieldOnset,
    ads_Prop_EMec_Uniaxial_Loading_yieldOnsetMethod
};

enum ads_Prop_EMec_Uniaxial_Loading_typeEnm
{
    ads_Prop_EMec_Uniaxial_Loading_type_DAMAGE,
    ads_Prop_EMec_Uniaxial_Loading_type_ELASTIC_PLASTIC
};

enum ads_Prop_EMec_Uniaxial_Loading_yieldOnsetMethodEnm
{
    ads_Prop_EMec_Uniaxial_Loading_yieldOnsetMethod_CLOSURE_VALUE,
    ads_Prop_EMec_Uniaxial_Loading_yieldOnsetMethod_DEFAULT,
    ads_Prop_EMec_Uniaxial_Loading_yieldOnsetMethod_SLOPE_DROP
};

/** Enum with association roles. */
enum ads_Prop_EMec_Uniaxial_Loading_tableRolesEnm
{
    ads_Prop_EMec_Uniaxial_Loading_table_child,
    ads_Prop_EMec_Uniaxial_Loading_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_UniaxialOption_Unloading_tableRolesEnm
{
    ads_Prop_IMec_UniaxialOption_Unloading_table_child,
    ads_Prop_IMec_UniaxialOption_Unloading_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IMec_Uniaxial_LoadingMembersEnm
{
    ads_Prop_IMec_Uniaxial_Loading_slopeDrop,
    ads_Prop_IMec_Uniaxial_Loading_tensileStiffnessFactor,
    ads_Prop_IMec_Uniaxial_Loading_type,
    ads_Prop_IMec_Uniaxial_Loading_yieldOnset,
    ads_Prop_IMec_Uniaxial_Loading_yieldOnsetMethod
};

enum ads_Prop_IMec_Uniaxial_Loading_typeEnm
{
    ads_Prop_IMec_Uniaxial_Loading_type_DAMAGE,
    ads_Prop_IMec_Uniaxial_Loading_type_ELASTIC_PLASTIC
};

enum ads_Prop_IMec_Uniaxial_Loading_yieldOnsetMethodEnm
{
    ads_Prop_IMec_Uniaxial_Loading_yieldOnsetMethod_CLOSURE_VALUE,
    ads_Prop_IMec_Uniaxial_Loading_yieldOnsetMethod_DEFAULT,
    ads_Prop_IMec_Uniaxial_Loading_yieldOnsetMethod_SLOPE_DROP
};

/** Enum with association roles. */
enum ads_Prop_IMec_Uniaxial_Loading_tableRolesEnm
{
    ads_Prop_IMec_Uniaxial_Loading_table_child,
    ads_Prop_IMec_Uniaxial_Loading_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_UniaxialOption_Dissipation_tableRolesEnm
{
    ads_Prop_MMec_UniaxialOption_Dissipation_table_child,
    ads_Prop_MMec_UniaxialOption_Dissipation_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_UniaxialOption_Unloading_tableRolesEnm
{
    ads_Prop_MMec_UniaxialOption_Unloading_table_child,
    ads_Prop_MMec_UniaxialOption_Unloading_table_parent
};

#endif
