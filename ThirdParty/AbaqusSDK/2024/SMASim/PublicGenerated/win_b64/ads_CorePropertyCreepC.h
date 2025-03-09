//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyCreepC_h
#define ads_CorePropertyCreepC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyCreep of the latest level of form Core */

#define ads_MMecCreepFlowAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 0))

#define ads_MMecCreepOptionHyperbolicSineTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 1))

#define ads_MMecCreepOptionHysteresisTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 2))

#define ads_MMecCreepOptionSinghMitchellTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 3))

#define ads_MMecCreepOptionStrainHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 4))

#define ads_MMecCreepOptionTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 5))

#define ads_MMecCreepOptionTimeHardeningTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 6))

#define ads_MMecCreepSwellingAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 7))

#define ads_Prop_MMec_Creep (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 8))

#define ads_Prop_MMec_CreepOption (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 9))

/** An Anand law to define a creep model and material properties. */
#define ads_Prop_MMec_CreepOption_Anand (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 10))

/** creep calculations for type 304 and type 316 stainless steel according to the specification in Nuclear Standard NEF 9-5T. */
#define ads_Prop_MMec_CreepOption_Cycled (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 11))

/** A Darveaux law to define a creep model and material properties. */
#define ads_Prop_MMec_CreepOption_Darveaux (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 12))

/** A Double Power law to define a creep model and material properties. */
#define ads_Prop_MMec_CreepOption_DoublePower (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 13))

/** Metal creep behavior based on a hyperbolic-sine law. This option can also be used to define creep behavior in the thickness direction in a gasket. */
#define ads_Prop_MMec_CreepOption_HyperbolicSine (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 14))

#define ads_Prop_MMec_CreepOption_HyperbolicSine_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 15))

/** Specify a rate-dependent elastomer model. This option is used to specify the creep part of the material model for the hysteretic behavior of elastomers. */
#define ads_Prop_MMec_CreepOption_Hysteresis (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 16))

#define ads_Prop_MMec_CreepOption_Hysteresis_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 17))

/** A power law to define a creep model. */
#define ads_Prop_MMec_CreepOption_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 18))

/** A Singh-Mitchell type law to define a cap creep model and material properties. */
#define ads_Prop_MMec_CreepOption_SinghMitchell (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 19))

#define ads_Prop_MMec_CreepOption_SinghMitchell_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 20))

/** A strain hardening power law to define a cap creep model. */
#define ads_Prop_MMec_CreepOption_StrainHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 21))

#define ads_Prop_MMec_CreepOption_StrainHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 22))

/** Specify time-dependent metal swelling for a material. */
#define ads_Prop_MMec_CreepOption_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 23))

#define ads_Prop_MMec_CreepOption_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 24))

/** A time hardening power law to define a cap creep model. */
#define ads_Prop_MMec_CreepOption_TimeHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 25))

#define ads_Prop_MMec_CreepOption_TimeHardening_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 26))

/** A time power law to define a creep model. */
#define ads_Prop_MMec_CreepOption_TimePowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 27))

/** Specify a user-defined cap creep law. */
#define ads_Prop_MMec_CreepOption_User (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 28))

#define ads_Prop_MMec_CreepOption_User_parameterTables (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 29))

#define ads_Prop_MMec_CreepOption_User_propertyTables (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 30))

#define ads_Prop_MMec_Creep_Flow (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 31))

#define ads_Prop_MMec_Creep_Flow_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 32))

#define ads_Prop_MMec_Creep_Flow_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 33))

#define ads_Prop_MMec_Creep_Flow_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 34))

#define ads_Prop_MMec_Creep_Swelling (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 35))

#define ads_Prop_MMec_Creep_Swelling_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 36))

#define ads_Prop_MMec_Creep_Swelling_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 37))

#define ads_Prop_MMec_Creep_Swelling_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyCreepFragment, 38))

/** Enum with record members. */
enum ads_Prop_MMec_CreepOptionMembersEnm
{
    ads_Prop_MMec_CreepOption_mechanism,
    ads_Prop_MMec_CreepOption_time
};

enum ads_Prop_MMec_CreepOption_mechanismEnm
{
    ads_Prop_MMec_CreepOption_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_timeEnm
{
    ads_Prop_MMec_CreepOption_time_CREEP,
    ads_Prop_MMec_CreepOption_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_AnandMembersEnm
{
    ads_Prop_MMec_CreepOption_Anand_mechanism,
    ads_Prop_MMec_CreepOption_Anand_time
};

enum ads_Prop_MMec_CreepOption_Anand_mechanismEnm
{
    ads_Prop_MMec_CreepOption_Anand_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_Anand_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_Anand_timeEnm
{
    ads_Prop_MMec_CreepOption_Anand_time_CREEP,
    ads_Prop_MMec_CreepOption_Anand_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_CycledMembersEnm
{
    ads_Prop_MMec_CreepOption_Cycled_mechanism,
    ads_Prop_MMec_CreepOption_Cycled_time,
    ads_Prop_MMec_CreepOption_Cycled_a,
    ads_Prop_MMec_CreepOption_Cycled_h,
    ads_Prop_MMec_CreepOption_Cycled_hPresence,
    ads_Prop_MMec_CreepOption_Cycled_reset
};

enum ads_Prop_MMec_CreepOption_Cycled_mechanismEnm
{
    ads_Prop_MMec_CreepOption_Cycled_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_Cycled_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_Cycled_timeEnm
{
    ads_Prop_MMec_CreepOption_Cycled_time_CREEP,
    ads_Prop_MMec_CreepOption_Cycled_time_TOTAL
};

enum ads_Prop_MMec_CreepOption_Cycled_hPresenceEnm
{
    ads_Prop_MMec_CreepOption_Cycled_hPresence_ABSENT,
    ads_Prop_MMec_CreepOption_Cycled_hPresence_PRESENT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_DarveauxMembersEnm
{
    ads_Prop_MMec_CreepOption_Darveaux_mechanism,
    ads_Prop_MMec_CreepOption_Darveaux_time
};

enum ads_Prop_MMec_CreepOption_Darveaux_mechanismEnm
{
    ads_Prop_MMec_CreepOption_Darveaux_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_Darveaux_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_Darveaux_timeEnm
{
    ads_Prop_MMec_CreepOption_Darveaux_time_CREEP,
    ads_Prop_MMec_CreepOption_Darveaux_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_DoublePowerMembersEnm
{
    ads_Prop_MMec_CreepOption_DoublePower_mechanism,
    ads_Prop_MMec_CreepOption_DoublePower_time
};

enum ads_Prop_MMec_CreepOption_DoublePower_mechanismEnm
{
    ads_Prop_MMec_CreepOption_DoublePower_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_DoublePower_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_DoublePower_timeEnm
{
    ads_Prop_MMec_CreepOption_DoublePower_time_CREEP,
    ads_Prop_MMec_CreepOption_DoublePower_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_HyperbolicSineMembersEnm
{
    ads_Prop_MMec_CreepOption_HyperbolicSine_mechanism,
    ads_Prop_MMec_CreepOption_HyperbolicSine_time
};

enum ads_Prop_MMec_CreepOption_HyperbolicSine_mechanismEnm
{
    ads_Prop_MMec_CreepOption_HyperbolicSine_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_HyperbolicSine_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_HyperbolicSine_timeEnm
{
    ads_Prop_MMec_CreepOption_HyperbolicSine_time_CREEP,
    ads_Prop_MMec_CreepOption_HyperbolicSine_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_HyperbolicSine_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_HyperbolicSine_table_child,
    ads_Prop_MMec_CreepOption_HyperbolicSine_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_HysteresisMembersEnm
{
    ads_Prop_MMec_CreepOption_Hysteresis_mechanism,
    ads_Prop_MMec_CreepOption_Hysteresis_time
};

enum ads_Prop_MMec_CreepOption_Hysteresis_mechanismEnm
{
    ads_Prop_MMec_CreepOption_Hysteresis_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_Hysteresis_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_Hysteresis_timeEnm
{
    ads_Prop_MMec_CreepOption_Hysteresis_time_CREEP,
    ads_Prop_MMec_CreepOption_Hysteresis_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_Hysteresis_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_Hysteresis_table_child,
    ads_Prop_MMec_CreepOption_Hysteresis_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_PowerLawMembersEnm
{
    ads_Prop_MMec_CreepOption_PowerLaw_mechanism,
    ads_Prop_MMec_CreepOption_PowerLaw_time
};

enum ads_Prop_MMec_CreepOption_PowerLaw_mechanismEnm
{
    ads_Prop_MMec_CreepOption_PowerLaw_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_PowerLaw_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_PowerLaw_timeEnm
{
    ads_Prop_MMec_CreepOption_PowerLaw_time_CREEP,
    ads_Prop_MMec_CreepOption_PowerLaw_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_SinghMitchellMembersEnm
{
    ads_Prop_MMec_CreepOption_SinghMitchell_mechanism,
    ads_Prop_MMec_CreepOption_SinghMitchell_time
};

enum ads_Prop_MMec_CreepOption_SinghMitchell_mechanismEnm
{
    ads_Prop_MMec_CreepOption_SinghMitchell_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_SinghMitchell_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_SinghMitchell_timeEnm
{
    ads_Prop_MMec_CreepOption_SinghMitchell_time_CREEP,
    ads_Prop_MMec_CreepOption_SinghMitchell_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_SinghMitchell_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_SinghMitchell_table_child,
    ads_Prop_MMec_CreepOption_SinghMitchell_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_StrainHardeningMembersEnm
{
    ads_Prop_MMec_CreepOption_StrainHardening_mechanism,
    ads_Prop_MMec_CreepOption_StrainHardening_time
};

enum ads_Prop_MMec_CreepOption_StrainHardening_mechanismEnm
{
    ads_Prop_MMec_CreepOption_StrainHardening_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_StrainHardening_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_StrainHardening_timeEnm
{
    ads_Prop_MMec_CreepOption_StrainHardening_time_CREEP,
    ads_Prop_MMec_CreepOption_StrainHardening_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_StrainHardening_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_StrainHardening_table_child,
    ads_Prop_MMec_CreepOption_StrainHardening_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_TabularMembersEnm
{
    ads_Prop_MMec_CreepOption_Tabular_mechanism,
    ads_Prop_MMec_CreepOption_Tabular_time
};

enum ads_Prop_MMec_CreepOption_Tabular_mechanismEnm
{
    ads_Prop_MMec_CreepOption_Tabular_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_Tabular_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_Tabular_timeEnm
{
    ads_Prop_MMec_CreepOption_Tabular_time_CREEP,
    ads_Prop_MMec_CreepOption_Tabular_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_Tabular_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_Tabular_table_child,
    ads_Prop_MMec_CreepOption_Tabular_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_TimeHardeningMembersEnm
{
    ads_Prop_MMec_CreepOption_TimeHardening_mechanism,
    ads_Prop_MMec_CreepOption_TimeHardening_time
};

enum ads_Prop_MMec_CreepOption_TimeHardening_mechanismEnm
{
    ads_Prop_MMec_CreepOption_TimeHardening_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_TimeHardening_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_TimeHardening_timeEnm
{
    ads_Prop_MMec_CreepOption_TimeHardening_time_CREEP,
    ads_Prop_MMec_CreepOption_TimeHardening_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_TimeHardening_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_TimeHardening_table_child,
    ads_Prop_MMec_CreepOption_TimeHardening_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_TimePowerLawMembersEnm
{
    ads_Prop_MMec_CreepOption_TimePowerLaw_mechanism,
    ads_Prop_MMec_CreepOption_TimePowerLaw_time
};

enum ads_Prop_MMec_CreepOption_TimePowerLaw_mechanismEnm
{
    ads_Prop_MMec_CreepOption_TimePowerLaw_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_TimePowerLaw_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_TimePowerLaw_timeEnm
{
    ads_Prop_MMec_CreepOption_TimePowerLaw_time_CREEP,
    ads_Prop_MMec_CreepOption_TimePowerLaw_time_TOTAL
};

/** 
Enum with record members. */
enum ads_Prop_MMec_CreepOption_UserMembersEnm
{
    ads_Prop_MMec_CreepOption_User_mechanism,
    ads_Prop_MMec_CreepOption_User_time
};

enum ads_Prop_MMec_CreepOption_User_mechanismEnm
{
    ads_Prop_MMec_CreepOption_User_mechanism_COHESION,
    ads_Prop_MMec_CreepOption_User_mechanism_CONSOLIDATION
};

enum ads_Prop_MMec_CreepOption_User_timeEnm
{
    ads_Prop_MMec_CreepOption_User_time_CREEP,
    ads_Prop_MMec_CreepOption_User_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_User_parameterTablesRolesEnm
{
    ads_Prop_MMec_CreepOption_User_parameterTables_child,
    ads_Prop_MMec_CreepOption_User_parameterTables_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_User_propertyTablesRolesEnm
{
    ads_Prop_MMec_CreepOption_User_propertyTables_child,
    ads_Prop_MMec_CreepOption_User_propertyTables_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Creep_Flow_Anisotropic_tableRolesEnm
{
    ads_Prop_MMec_Creep_Flow_Anisotropic_table_child,
    ads_Prop_MMec_Creep_Flow_Anisotropic_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Creep_Swelling_Anisotropic_tableRolesEnm
{
    ads_Prop_MMec_Creep_Swelling_Anisotropic_table_child,
    ads_Prop_MMec_Creep_Swelling_Anisotropic_table_parent
};

#endif
