//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyTestDataC_h
#define ads_CorePropertyTestDataC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyTestData of the latest level of form Core */

#define ads_MMecTestDataBiaxialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 0))

#define ads_MMecTestDataBulkComplianceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 1))

#define ads_MMecTestDataBulkModulusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 2))

#define ads_MMecTestDataKinematicHardeningHalfCycleTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 3))

#define ads_MMecTestDataKinematicHardeningStabilizedTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 4))

#define ads_MMecTestDataPlanarTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 5))

#define ads_MMecTestDataShearBulkComplianceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 6))

#define ads_MMecTestDataShearBulkModulusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 7))

#define ads_MMecTestDataShearComplianceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 8))

#define ads_MMecTestDataShearModulusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 9))

#define ads_MMecTestDataSimpleShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 10))

#define ads_MMecTestDataTriaxialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 11))

#define ads_MMecTestDataUniaxialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 12))

#define ads_MMecTestDataVolumetricTable (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 13))

#define ads_Prop_MMec_TestData (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 14))

#define ads_Prop_MMec_TestDataCalibration (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 15))

#define ads_Prop_MMec_TestDataCalibration_ViscoelasticProny (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 16))

#define ads_Prop_MMec_TestData_Biaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 17))

#define ads_Prop_MMec_TestData_Biaxial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 18))

#define ads_Prop_MMec_TestData_BulkCompliance (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 19))

#define ads_Prop_MMec_TestData_BulkCompliance_longTermNormalizedBulkCompliance (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 20))

#define ads_Prop_MMec_TestData_BulkCompliance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 21))

#define ads_Prop_MMec_TestData_BulkModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 22))

#define ads_Prop_MMec_TestData_BulkModulus_longTermNormalizedBulkModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 23))

#define ads_Prop_MMec_TestData_BulkModulus_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 24))

/** Linear Kinematic hardening plasticity record */
#define ads_Prop_MMec_TestData_KinematicHardening (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 25))

#define ads_Prop_MMec_TestData_KinematicHardening_HalfCycle (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 26))

#define ads_Prop_MMec_TestData_KinematicHardening_HalfCycle_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 27))

#define ads_Prop_MMec_TestData_KinematicHardening_Stabilized (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 28))

#define ads_Prop_MMec_TestData_KinematicHardening_Stabilized_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 29))

#define ads_Prop_MMec_TestData_Planar (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 30))

#define ads_Prop_MMec_TestData_Planar_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 31))

#define ads_Prop_MMec_TestData_ShearBulkCompliance (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 32))

#define ads_Prop_MMec_TestData_ShearBulkCompliance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 33))

#define ads_Prop_MMec_TestData_ShearBulkModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 34))

#define ads_Prop_MMec_TestData_ShearBulkModulus_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 35))

#define ads_Prop_MMec_TestData_ShearCompliance (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 36))

#define ads_Prop_MMec_TestData_ShearCompliance_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 37))

#define ads_Prop_MMec_TestData_ShearModulus (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 38))

#define ads_Prop_MMec_TestData_ShearModulus_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 39))

/** SimpleShearTestData for hypoelastic materials. */
#define ads_Prop_MMec_TestData_SimpleShear (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 40))

#define ads_Prop_MMec_TestData_SimpleShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 41))

/** Provide triaxial test data. This option is required if some or all of the material parameters that define the exponent form of the Drucker Prager option are to be calibrated from triaxial test data. */
#define ads_Prop_MMec_TestData_Triaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 42))

#define ads_Prop_MMec_TestData_Triaxial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 43))

#define ads_Prop_MMec_TestData_Uniaxial (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 44))

#define ads_Prop_MMec_TestData_Uniaxial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 45))

#define ads_Prop_MMec_TestData_Volumetric (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 46))

#define ads_Prop_MMec_TestData_Volumetric_table (ads_CoreFragmentTypeIndex(ads_CorePropertyTestDataFragment, 47))

/** Enum with record members. */
enum ads_Prop_MMec_TestDataCalibration_ViscoelasticPronyMembersEnm
{
    ads_Prop_MMec_TestDataCalibration_ViscoelasticProny_errtol,
    ads_Prop_MMec_TestDataCalibration_ViscoelasticProny_nmax,
    ads_Prop_MMec_TestDataCalibration_ViscoelasticProny_relax,
    ads_Prop_MMec_TestDataCalibration_ViscoelasticProny_time
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_BiaxialMembersEnm
{
    ads_Prop_MMec_TestData_Biaxial_smooth
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_Biaxial_tableRolesEnm
{
    ads_Prop_MMec_TestData_Biaxial_table_child,
    ads_Prop_MMec_TestData_Biaxial_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_BulkComplianceMembersEnm
{
    ads_Prop_MMec_TestData_BulkCompliance_volinf,
    ads_Prop_MMec_TestData_BulkCompliance_volinfType
};

enum ads_Prop_MMec_TestData_BulkCompliance_volinfTypeEnm
{
    ads_Prop_MMec_TestData_BulkCompliance_volinfType_ABSENT,
    ads_Prop_MMec_TestData_BulkCompliance_volinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_BulkCompliance_longTermNormalizedBulkComplianceRolesEnm
{
    ads_Prop_MMec_TestData_BulkCompliance_longTermNormalizedBulkCompliance_child,
    ads_Prop_MMec_TestData_BulkCompliance_longTermNormalizedBulkCompliance_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_BulkCompliance_tableRolesEnm
{
    ads_Prop_MMec_TestData_BulkCompliance_table_child,
    ads_Prop_MMec_TestData_BulkCompliance_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_BulkModulusMembersEnm
{
    ads_Prop_MMec_TestData_BulkModulus_volinf,
    ads_Prop_MMec_TestData_BulkModulus_volinfType
};

enum ads_Prop_MMec_TestData_BulkModulus_volinfTypeEnm
{
    ads_Prop_MMec_TestData_BulkModulus_volinfType_ABSENT,
    ads_Prop_MMec_TestData_BulkModulus_volinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_BulkModulus_longTermNormalizedBulkModulusRolesEnm
{
    ads_Prop_MMec_TestData_BulkModulus_longTermNormalizedBulkModulus_child,
    ads_Prop_MMec_TestData_BulkModulus_longTermNormalizedBulkModulus_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_BulkModulus_tableRolesEnm
{
    ads_Prop_MMec_TestData_BulkModulus_table_child,
    ads_Prop_MMec_TestData_BulkModulus_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_KinematicHardening_HalfCycleMembersEnm
{
    ads_Prop_MMec_TestData_KinematicHardening_HalfCycle_numBackStresses
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_KinematicHardening_HalfCycle_tableRolesEnm
{
    ads_Prop_MMec_TestData_KinematicHardening_HalfCycle_table_child,
    ads_Prop_MMec_TestData_KinematicHardening_HalfCycle_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_KinematicHardening_StabilizedMembersEnm
{
    ads_Prop_MMec_TestData_KinematicHardening_Stabilized_numBackStresses
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_KinematicHardening_Stabilized_tableRolesEnm
{
    ads_Prop_MMec_TestData_KinematicHardening_Stabilized_table_child,
    ads_Prop_MMec_TestData_KinematicHardening_Stabilized_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_PlanarMembersEnm
{
    ads_Prop_MMec_TestData_Planar_smooth
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_Planar_tableRolesEnm
{
    ads_Prop_MMec_TestData_Planar_table_child,
    ads_Prop_MMec_TestData_Planar_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_ShearBulkComplianceMembersEnm
{
    ads_Prop_MMec_TestData_ShearBulkCompliance_shrinf,
    ads_Prop_MMec_TestData_ShearBulkCompliance_shrinfType,
    ads_Prop_MMec_TestData_ShearBulkCompliance_volinf,
    ads_Prop_MMec_TestData_ShearBulkCompliance_volinfType
};

enum ads_Prop_MMec_TestData_ShearBulkCompliance_shrinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearBulkCompliance_shrinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearBulkCompliance_shrinfType_PRESENT
};

enum ads_Prop_MMec_TestData_ShearBulkCompliance_volinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearBulkCompliance_volinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearBulkCompliance_volinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_ShearBulkCompliance_tableRolesEnm
{
    ads_Prop_MMec_TestData_ShearBulkCompliance_table_child,
    ads_Prop_MMec_TestData_ShearBulkCompliance_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_ShearBulkModulusMembersEnm
{
    ads_Prop_MMec_TestData_ShearBulkModulus_shrinf,
    ads_Prop_MMec_TestData_ShearBulkModulus_shrinfType,
    ads_Prop_MMec_TestData_ShearBulkModulus_volinf,
    ads_Prop_MMec_TestData_ShearBulkModulus_volinfType
};

enum ads_Prop_MMec_TestData_ShearBulkModulus_shrinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearBulkModulus_shrinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearBulkModulus_shrinfType_PRESENT
};

enum ads_Prop_MMec_TestData_ShearBulkModulus_volinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearBulkModulus_volinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearBulkModulus_volinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_ShearBulkModulus_tableRolesEnm
{
    ads_Prop_MMec_TestData_ShearBulkModulus_table_child,
    ads_Prop_MMec_TestData_ShearBulkModulus_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_ShearComplianceMembersEnm
{
    ads_Prop_MMec_TestData_ShearCompliance_shrinf,
    ads_Prop_MMec_TestData_ShearCompliance_shrinfType
};

enum ads_Prop_MMec_TestData_ShearCompliance_shrinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearCompliance_shrinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearCompliance_shrinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_ShearCompliance_tableRolesEnm
{
    ads_Prop_MMec_TestData_ShearCompliance_table_child,
    ads_Prop_MMec_TestData_ShearCompliance_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_ShearModulusMembersEnm
{
    ads_Prop_MMec_TestData_ShearModulus_shrinf,
    ads_Prop_MMec_TestData_ShearModulus_shrinfType
};

enum ads_Prop_MMec_TestData_ShearModulus_shrinfTypeEnm
{
    ads_Prop_MMec_TestData_ShearModulus_shrinfType_ABSENT,
    ads_Prop_MMec_TestData_ShearModulus_shrinfType_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_ShearModulus_tableRolesEnm
{
    ads_Prop_MMec_TestData_ShearModulus_table_child,
    ads_Prop_MMec_TestData_ShearModulus_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_SimpleShear_tableRolesEnm
{
    ads_Prop_MMec_TestData_SimpleShear_table_child,
    ads_Prop_MMec_TestData_SimpleShear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_TestData_TriaxialMembersEnm
{
    ads_Prop_MMec_TestData_Triaxial_a,
    ads_Prop_MMec_TestData_Triaxial_aEnm,
    ads_Prop_MMec_TestData_Triaxial_b,
    ads_Prop_MMec_TestData_Triaxial_bEnm,
    ads_Prop_MMec_TestData_Triaxial_pt,
    ads_Prop_MMec_TestData_Triaxial_ptEnm
};

enum ads_Prop_MMec_TestData_Triaxial_aEnmEnm
{
    ads_Prop_MMec_TestData_Triaxial_aEnm_COMPUTE,
    ads_Prop_MMec_TestData_Triaxial_aEnm_USER_INPUT
};

enum ads_Prop_MMec_TestData_Triaxial_bEnmEnm
{
    ads_Prop_MMec_TestData_Triaxial_bEnm_COMPUTE,
    ads_Prop_MMec_TestData_Triaxial_bEnm_USER_INPUT
};

enum ads_Prop_MMec_TestData_Triaxial_ptEnmEnm
{
    ads_Prop_MMec_TestData_Triaxial_ptEnm_COMPUTE,
    ads_Prop_MMec_TestData_Triaxial_ptEnm_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_Triaxial_tableRolesEnm
{
    ads_Prop_MMec_TestData_Triaxial_table_child,
    ads_Prop_MMec_TestData_Triaxial_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_UniaxialMembersEnm
{
    ads_Prop_MMec_TestData_Uniaxial_smooth
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_Uniaxial_tableRolesEnm
{
    ads_Prop_MMec_TestData_Uniaxial_table_child,
    ads_Prop_MMec_TestData_Uniaxial_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_TestData_VolumetricMembersEnm
{
    ads_Prop_MMec_TestData_Volumetric_smooth
};

/** Enum with association roles. */
enum ads_Prop_MMec_TestData_Volumetric_tableRolesEnm
{
    ads_Prop_MMec_TestData_Volumetric_table_child,
    ads_Prop_MMec_TestData_Volumetric_table_parent
};

#endif
